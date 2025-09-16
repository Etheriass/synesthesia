#ifndef DECODER_H
#define DECODER_H

#include <vector>

void make_hann(std::vector<double>& w);

std::vector<std::pair<int,double>> peaks_selector(const std::vector<double>& mag, int thresh, int max_peaks);

std::vector<double> timbre_harmonics(const std::vector<double>& mag_db, double fundamental_freq, int sample_rate, int N);

float fullness_timbre(const std::vector<double>& timbre);

// Optional quadratic interpolation around peaks for sub-bin freq estimate
double interp_quadratic_bin(const std::vector<double>& mag, int k);

#endif