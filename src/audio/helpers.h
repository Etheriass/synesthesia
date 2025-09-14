#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

#define INT16_MAX_FLOAT 32768.0f  // 2^15

std::vector<uint8_t> read_binary(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot open: " + path);
    f.seekg(0, std::ios::end);
    size_t len = (size_t)f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(len);
    if (len && !f.read((char *)data.data(), (std::streamsize)len))
        throw std::runtime_error("Read failed");
    return data;
}

inline double db10(double x) {
    return 10.0 * std::log10(std::max(x, 1e-20));
}

inline double snr_db(double signal_energy) {
    return 10.0 * std::log10(std::max(signal_energy, 1e-20));
}

#endif