#ifndef SPATIAL_H
#define SPATIAL_H

#include <cmath>
#include <algorithm>

// Clamp helper
template <typename T>
inline T clamp(T v, T lo, T hi) { return std::max(lo, std::min(hi, v)); }

// Compute energies of two signals: sum of squares
inline std::pair<double, double> energy(const float* L, const float* R, int N) {
    double L2 = 0.0, R2 = 0.0;
    for (int n = 0; n < N; ++n) {
        double l = L[n], r = R[n];
        L2 += l * l;
        R2 += r * r;
    }
    return {L2, R2};
}

inline double azimuth_from_ild_itd(double ILD_dB, double ITD_sec) {
    // Simple model: azimuth from ILD and ITD combined
    // ILD_dB: +Right, -Left
    // ITD_sec: +Right, -Left
    const double ILD_scale = 30.0;   // ~30 dB from 90° Right to 90° Left
    const double ITD_max = 0.0007;   // ~0.7 ms from 90° Right to 90° Left
    double az_ild = clamp(-ILD_dB / ILD_scale, -1.0, 1.0); // +Left, -Right
    double az_itd = clamp(-ITD_sec / ITD_max, -1.0, 1.0); // +Left, -Right
    double az = 0.5 * (az_ild + az_itd);                  // average
    az = clamp(az, -1.0, 1.0);
    return az * 90.0; // degrees: -90° Right to +90° Left
}

inline double width_from_mid_side(const float* L, const float* R, int N) {
    double M2 = 0.0, S2 = 0.0;
    for (int n = 0; n < N; ++n) {
        double M = 0.5 * (L[n] + R[n]);
        double S = 0.5 * (L[n] - R[n]);
        M2 += M * M;
        S2 += S * S;
    }
    double eps = 1e-20;
    double width = 10.0 * std::log10((S2 + eps) / (M2 + eps)); // in dB
    return width;
}

// Pearson correlation between two equal-length arrays (no mean removal -> quick, robust-ish)
inline double pearson_corr(const float* a, const float* b, int N) {
    double sumx = 0, sumy = 0, sumxx = 0, sumyy = 0, sumxy = 0;
    for (int i = 0; i < N; ++i) {
        double x = a[i], y = b[i];
        sumx += x; sumy += y;
        sumxx += x * x; sumyy += y * y;
        sumxy += x * y;
    }
    double num = N * sumxy - sumx * sumy;
    double den = std::sqrt(std::max(1e-20, (N * sumxx - sumx * sumx) * (N * sumyy - sumy * sumy)));
    return num / den;
}

// Cross-correlation (coarse) to get ITD lag within +/- maxLag samples.
// Returns lag where R matches L best: positive lag means R is delayed w.r.t. L.
inline int xcorr_argmax_lag(const float* L, const float* R, int N, int maxLag) {
    maxLag = std::min(maxLag, N - 1);
    double best = -1e300;
    int bestLag = 0;
    // Normalize by energies for rough NCC
    double L2 = 0, R2 = 0;
    for (int i = 0; i < N; ++i) { L2 += (double)L[i] * L[i]; R2 += (double)R[i] * R[i]; }
    L2 = std::max(L2, 1e-20); R2 = std::max(R2, 1e-20);
    double norm = std::sqrt(L2 * R2);

    for (int lag = -maxLag; lag <= maxLag; ++lag) {
        double acc = 0.0;
        if (lag >= 0) {
            for (int n = 0; n < N - lag; ++n) acc += (double)L[n] * R[n + lag];
        } else {
            int k = -lag;
            for (int n = 0; n < N - k; ++n) acc += (double)L[n + k] * R[n];
        }
        double val = acc / norm;
        if (val > best) { best = val; bestLag = lag; }
    }
    return bestLag;
}

#endif