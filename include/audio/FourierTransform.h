#pragma once

#include <vector>
#include <complex>

class FourierTransform {
public:
    FourierTransform() = default;
    ~FourierTransform() = default;

    void performFFT(std::vector<std::complex<float>>& data);
    void performIFFT(std::vector<std::complex<float>>& data);
};