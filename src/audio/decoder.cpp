#include <vector>
#include <cmath>
#include <algorithm>
#include "fourier.h"

// Hann window
void make_hann(std::vector<double>& w) {
    const size_t N = w.size();
    for (size_t n = 0; n < N; ++n) {
        w[n] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * n / (N - 1)));
    }
}

std::vector<std::pair<int,double>> peaks_selector(const std::vector<double>& mag, int thresh, int max_peaks){
    std::vector<std::pair<int,double>> peaks;
    const int N = (int)mag.size();
    for (int i = 1; i < N - 1; ++i) {
        if (mag[i] > thresh) {
            if (mag[i] > mag[i-1] && mag[i] >= mag[i+1]) {
                peaks.emplace_back(i, mag[i]);
            }
        }
    }
    std::partial_sort(peaks.begin(), peaks.begin() + std::min(max_peaks, (int)peaks.size()), peaks.end(),
                      [](auto& a, auto& b){ return a.second > b.second; });
    if ((int)peaks.size() > max_peaks) peaks.resize(max_peaks);
    return peaks;
}

std::vector<double> timbre_harmonics(const std::vector<double>& mag_db, double fundamental_freq, int sample_rate, int N) {
    std::vector<double> timbre;
    int h = 1;
    while (timbre.empty() || (h <= 100 && timbre[h - 1] > -120.0)) {
        double target = h * fundamental_freq;
        int harmonic_bin = int(target / (sample_rate / N) + 0.5); // nearest bin
        if (harmonic_bin < (int)mag_db.size()) {
            double amp = mag_db[harmonic_bin];
            timbre.push_back(amp);
        } else {
            timbre.push_back(-120.0); // treat as silence if out of range
        }
        h++;
    }
    return timbre;
}

float fullness_timbre(const std::vector<double>& timbre) {
    double fullness = 0.0;
    for (size_t i = 1; i < timbre.size(); ++i) {
        fullness += timbre[i] + 120.0;
    }
    fullness /= double(timbre.size() * 120.0);
    return float(fullness);
}

// Optional quadratic interpolation around peaks for sub-bin freq estimate
double interp_quadratic_bin(const std::vector<double>& mag, int k) {
    if (k <= 0 || k >= (int)mag.size()-1) return (double)k;
    double m1 = mag[k-1], m0 = mag[k], p1 = mag[k+1];
    double denom = (m1 - 2*m0 + p1);
    if (std::abs(denom) < 1e-12) return (double)k;
    double delta = 0.5 * (m1 - p1) / denom; // shift in bins, (-0.5..0.5)
    return (double)k + delta;
}
