#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/wait.h>
#include <chrono>
#include <thread>

#include "audio.h"
#include "decoder.h"
#include "player.h"
#include "helpers.h"
#include "spatial.h"
#include "fourier.h"

#include "../shared_state.h"

void audio_thread(mp3dec_t *dec, std::vector<uint8_t> audio_binary, int rate, SharedState *shared)
{
    size_t pos = 0;
    int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME]; // interleaved
    mp3dec_frame_info_t info{};

    std::vector<float> left_ring, right_ring;              // rolling stereo
    std::vector<float> mid_ring;                           // Mid (used for STFT) rolling buffer
    std::vector<float> left_frame, right_frame, mid_frame; // per decoded frame

    int pipefd[2] = {-1, -1}; // pipefd[0] for reading in child, pipefd[1] for writing PCM to child
    pid_t child_pid = -1;

    size_t frame_idx = 0;

    // STFT parameters
    const int N = 2 << 12; // window size (power of 2)
    const int HOP = N / 2; // hop size (% overlap)
    std::vector<double> hann(N);
    make_hann(hann);
    long long total_samples_mid = 0; // running index for timestamps

    int samples = mp3dec_decode_frame(dec, audio_binary.data() + pos, (int)(audio_binary.size() - pos), pcm, &info);

    start_aplay(rate, pipefd, child_pid);

    left_frame.resize(samples);
    right_frame.resize(samples);
    mid_frame.resize(samples);

    int Nh = N / 2;
    std::vector<cd> X(N);
    std::vector<double> mag(Nh + 1);
    std::vector<double> mag_db(Nh + 1);
    auto t_start = std::chrono::steady_clock::now();

    while (pos < audio_binary.size())
    {
        samples = mp3dec_decode_frame(dec, audio_binary.data() + pos, (int)(audio_binary.size() - pos), pcm, &info);

        // if (info.frame_bytes <= 0)
        // { // Not a valid frame here; advance minimally to resync
        //     std::cerr << "Warning: mp3dec_decode_frame() returned frame_bytes=" << info.frame_bytes << " at pos=" << pos << "\n";
        //     pos++;
        //     continue;
        // }
        pos += info.frame_bytes;

        // // Print frame info + a few samples
        // left_frame.resize(samples);
        // right_frame.resize(samples);
        // mid_frame.resize(samples);

        for (int i = 0; i < samples; ++i)
        {
            int16_t Ls = pcm[2 * i + 0];
            int16_t Rs = pcm[2 * i + 1];
            float L = float(Ls) / INT16_MAX_FLOAT;
            float R = float(Rs) / INT16_MAX_FLOAT;

            left_frame[i] = L;
            right_frame[i] = R;
            mid_frame[i] = 0.5f * (L + R); // Mid
        }

        // Append to rolling rings
        left_ring.insert(left_ring.end(), left_frame.begin(), left_frame.end());
        right_ring.insert(right_ring.end(), right_frame.begin(), right_frame.end());
        mid_ring.insert(mid_ring.end(), mid_frame.begin(), mid_frame.end());

        // Process as many STFT frames as we have (N every HOP)
        int processed = 0;
        while ((int)mid_ring.size() - processed >= N)
        {
            const float *Lw = left_ring.data() + processed;
            const float *Rw = right_ring.data() + processed;

            // Energies
            double L2, R2;
            energy(Lw, Rw, N, &L2, &R2);

            // ILD
            double ILD_dB = db10((R2) / (L2)); // +Right, −Left

            // ITD via cross-correlation
            int maxLag = (int)std::round(0.001 * rate); // ~1 ms
            int lag = xcorr_argmax_lag(Lw, Rw, N, maxLag);
            double ITD_sec = (double)lag / (double)rate;

            // Azimuth estimate via simple model (ILD+ITD)
            double azimuth_deg = azimuth_from_ild_itd(ILD_dB, ITD_sec);

            // Width via Mid/Side and correlation
            double width_db = width_from_mid_side(Lw, Rw, N);
            // double rho = pearson_corr(Lw, Rw, N);

            double t0 = double(total_samples_mid) / double(rate);
            // std::printf("[t=%.3fs] Azimuth≈%6.1f°  ITD=%6.2f ms  ILD=%6.2f dB  width=%5.1f dB  ρ=%+.2f\n",
            //             t0, azimuth_deg, ITD_sec * 1000.0, ILD_dB, width_db, rho);

            // Window Mid
            for (int n = 0; n < N; ++n)
            {
                double xw = double(mid_ring[processed + n]) * hann[n];
                X[n] = cd(xw, 0.0);
            }

            // FFT
            fft_inplace(X);

            // Magnitude spectrum (only 0..N/2 are unique for real input)

            for (int k = 0; k < Nh; ++k)
            {
                mag[k] = std::abs(X[k]) / (N * 0.5); // simple scale (approx)
            }

            // Convert to dBFS (reference 1.0 full-scale)
            for (int k = 0; k < Nh; ++k)
            {
                // double v = std::max(mag[k], 1e-12);
                mag_db[k] = 20.0 * std::log10(mag[k]);
            }

            // Peak pick: top 10 peaks above -50 dB
            auto peaks = peaks_selector(mag_db, -35, 1);
            // std::cout << "Number of peaks found: " << peaks.size() << "\n";

            // Print the peaks with sub-bin interpolation for better frequency est.
            // std::printf("[t=%.3fs] STFT N=%d hop=%d  Top peaks:\n", t0, N, HOP);
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - t_start);
            printf("[t=%.3fs], t_s= %ldms\n", t0, duration.count());
            double fullness = 0.0;
            for (auto &[bin, db] : peaks)
            {
                double k_hat = interp_quadratic_bin(mag_db, bin);
                double freq = (k_hat * rate) / double(N);

                std::vector<double> timbre = timbre_harmonics(mag_db, freq, rate, N);

                fullness = fullness_timbre(timbre);

                // std::printf("   bin≈%.2f  freq≈%8.2f Hz   mag≈%6.1f dBFS", k_hat, freq, db);
                // std::printf("  Fullness: %.2f\n", fullness);

                // Add circle to shared state (if available)
                if (shared)
                {
                    // Map azimuth -90..+90 to x=0.1..0.9
                    float ux = 0.5f + 0.4f * float(azimuth_deg / 90.0);
                    // Map width 0..20 dB to y=0.1..0.9
                    float uy = 0.1f + 0.8f * freq / 1000.0f; // freq/1000Hz
                    if (uy < 0.1f)
                        uy = 0.1f;
                    if (uy > 0.9f)
                        uy = 0.9f;
                    // Radius from fullness (0.0..0.5)
                    float radius = 0.02f + 0.06f * timbre.size() / 10.0f;
                    // Falloff from width (0.8..2.0)
                    float falloff = 0.8f + 1.2f * fullness;
                    // Intensity from overall level (0.5..1.5)
                    double level_db = db;
                    float intensity = 0.5f + 1.0f * float(std::min(std::max(level_db + 40.0, 0.0), 40.0) / 40.0);
                    // std::cout << "  Circle: (" << ux << "," << uy << ") r=" << radius << " f=" << falloff << " I=" << intensity << "\n";
                    add_circle_shared(shared, ux, uy, radius, falloff, intensity);
                }
            }

            // Advance
            processed += HOP;
            total_samples_mid += HOP;

            // Drop consumed samples from rings (keep tail for overlap)
            mid_ring.erase(mid_ring.begin(), mid_ring.begin() + processed);
            left_ring.erase(left_ring.begin(), left_ring.begin() + processed);
            right_ring.erase(right_ring.begin(), right_ring.begin() + processed);
        }

        // Drop consumed samples from ring buffer (keep tail for overlap)
        if (processed > 0)
        {
            mid_ring.erase(mid_ring.begin(), mid_ring.begin() + processed);
        }

        // Write PCM to child (if launched)
        write_pcm_to_aplay(pipefd, pcm, samples);

        frame_idx++;
    }

    // Close writer to signal EOF to aplay
    if (pipefd[1] != -1)
        close(pipefd[1]);

    // Reap child
    if (child_pid > 0)
    {
        int status = 0;
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::fprintf(stderr, "aplay exited with status %d\n", WEXITSTATUS(status));
        }
    }
    // aplay_thread_handle.join();
}