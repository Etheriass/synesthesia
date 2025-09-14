#include <complex>
#include <vector>
#include <cmath>
#include "fourier.h"

// In-place iterative radix-2 FFT (a.size() MUST be power of 2)
void fft_inplace(std::vector<cd>& a) {
    const size_t n = a.size();
    // bit-reverse permutation
    size_t j = 0;
    for (size_t i = 1; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    // butterflies
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = -2.0 * std::numbers::pi / double(len);
        cd wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            cd w(1.0, 0.0);
            for (size_t k = 0; k < len/2; ++k) {
                cd u = a[i + k];
                cd v = a[i + k + len/2] * w;
                a[i + k]             = u + v;
                a[i + k + len/2]     = u - v;
                w *= wlen;
            }
        }
    }
}