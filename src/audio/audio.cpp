#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <sys/wait.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "../../external/minimp3/minimp3.h"
#include "../../external/minimp3/minimp3_ex.h"

#include "audio.h"
#include "decoder.h"
#include "player.h"
#include "helpers.h"
#include "spatial.h"
#include "fourier.h"

void audio_thread(const std::string &path)
{
    std::vector<uint8_t> audio_binary;
    try
    {
        audio_binary = read_binary(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    mp3dec_t dec;
    mp3dec_init(&dec);

    size_t pos = 0;
    mp3dec_frame_info_t info{};
    int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME]; // interleaved

    std::vector<float> left_ring, right_ring;              // rolling stereo
    std::vector<float> mid_ring;                           // Mid (used for STFT) rolling buffer
    std::vector<float> left_frame, right_frame, mid_frame; // per decoded frame

    bool launched_child = false;
    int pipefd[2] = {-1, -1}; // pipefd[0] for reading in child, pipefd[1] for writing PCM to child
    pid_t child_pid = -1;

    size_t frame_idx = 0;

    // STFT parameters
    const int N = 1024;  // window size (power of 2)
    const int HOP = 512; // hop size (% overlap)
    std::vector<double> hann(N);
    make_hann(hann);
    long long total_samples_mid = 0; // running index for timestamps

    while (pos < audio_binary.size())
    {
        int samples = mp3dec_decode_frame(&dec, audio_binary.data() + pos, (int)(audio_binary.size() - pos), pcm, &info);

        if (info.channels < 2){
            std::cerr << "Only stereo files are supported\n";
            break;
        }

        if (info.frame_bytes <= 0)
        { // Not a valid frame here; advance minimally to resync
            pos++;
            continue;
        }
        pos += info.frame_bytes;

        // Upon first decoded frame with samples, capture format and launch child
        if (samples > 0 && !launched_child)
        {
            try
            {
                start_aplay(info, pipefd, child_pid);
                launched_child = true;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }

        // Print frame info + a few samples
        if (samples > 0)
        {
            left_frame.resize(samples);
            right_frame.resize(samples);
            mid_frame.resize(samples);

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
                std::tie(L2, R2) = energy(Lw, Rw, N);

                // ILD
                double eps = 1e-20;
                double ILD_dB = db10((R2 + eps) / (L2 + eps)); // +Right, −Left

                // ITD via cross-correlation
                int maxLag = (int)std::round(0.001 * info.hz); // ~1 ms
                int lag = xcorr_argmax_lag(Lw, Rw, N, maxLag);
                double ITD_sec = (double)lag / (double)info.hz;

                // Azimuth estimate via simple model (ILD+ITD)
                double azimuth_deg = azimuth_from_ild_itd(ILD_dB, ITD_sec);

                // Width via Mid/Side and correlation
                double width_db = width_from_mid_side(Lw, Rw, N);
                double rho = pearson_corr(Lw, Rw, N);

                double t0 = double(total_samples_mid) / double(info.hz);
                std::printf("[t=%.3fs] Azimuth≈%6.1f°  ITD=%6.2f ms  ILD=%6.2f dB  width=%5.1f dB  ρ=%+.2f\n",
                            t0, azimuth_deg, ITD_sec * 1000.0, ILD_dB, width_db, rho);

                // Window Mid 
                std::vector<cd> X(N); // complex spectrum
                for (int n = 0; n < N; ++n)
                {
                    double xw = double(mid_ring[processed + n]) * hann[n];
                    X[n] = cd(xw, 0.0);
                }

                // FFT
                fft_inplace(X);

                // Magnitude spectrum (only 0..N/2 are unique for real input)
                int Nh = N / 2 + 1;
                std::vector<double> mag(Nh);
                for (int k = 0; k < Nh; ++k)
                {
                    mag[k] = std::abs(X[k]) / (N * 0.5); // simple scale (approx)
                }

                // Convert to dBFS (reference 1.0 full-scale)
                std::vector<double> mag_db(Nh);
                for (int k = 0; k < Nh; ++k)
                {
                    double v = std::max(mag[k], 1e-12);
                    mag_db[k] = 20.0 * std::log10(v);
                }

                // Peak pick: top 10 peaks above -50 dB
                auto peaks = peaks_selector(mag_db, -50, 10);
                std::cout << "Number of peaks found: " << peaks.size() << "\n";

                // Print the peaks with sub-bin interpolation for better frequency est.
                std::printf("[t=%.3fs] STFT N=%d hop=%d  Top peaks:\n", t0, N, HOP);
                for (auto &[bin, db] : peaks)
                {
                    double k_hat = interp_quadratic_bin(mag_db, bin);
                    double freq = (k_hat * info.hz) / double(N);

                    std::vector<double> timbre = timbre_harmonics(mag_db, freq, info.hz, N);

                    double fullness = fullness_timbre(timbre);

                    std::printf("   bin≈%.2f  freq≈%8.2f Hz   mag≈%6.1f dBFS", k_hat, freq, db);
                    std::printf("  Fullness: %.2f\n", fullness);
                }

                // Advance
                processed += HOP;
                total_samples_mid += HOP;

                // Drop consumed samples from rings (keep tail for overlap)
                if (processed > 0)
                {
                    mid_ring.erase(mid_ring.begin(), mid_ring.begin() + processed);
                    left_ring.erase(left_ring.begin(), left_ring.begin() + processed);
                    right_ring.erase(right_ring.begin(), right_ring.begin() + processed);
                }
            }

            // Drop consumed samples from ring buffer (keep tail for overlap)
            if (processed > 0)
            {
                mid_ring.erase(mid_ring.begin(), mid_ring.begin() + processed);
            }

            // Write PCM to child (if launched)
            if (launched_child)
            {
                write_pcm_to_aplay(pipefd, child_pid, pcm, samples, info.channels);
            }
            frame_idx++;
        }
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
}