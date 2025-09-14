#ifndef FOURIER_H
#define FOURIER_H

#include <vector>
#include <complex>

using cd = std::complex<double>;

void fft_inplace(std::vector<cd>& a);

#endif