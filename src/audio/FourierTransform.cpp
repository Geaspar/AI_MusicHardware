#include "audio/FourierTransform.h"
#include "pocketfft_hdronly.h"

void FourierTransform::performFFT(std::vector<std::complex<float>>& data) {
    pocketfft::shape_t shape = {data.size()};
    pocketfft::stride_t stride_in = {sizeof(std::complex<float>)};
    pocketfft::stride_t stride_out = {sizeof(std::complex<float>)};
    pocketfft::shape_t axes = {0};
    pocketfft::c2c(shape, stride_in, stride_out, axes, true, data.data(), data.data(), 1.0f);
}

void FourierTransform::performIFFT(std::vector<std::complex<float>>& data) {
    pocketfft::shape_t shape = {data.size()};
    pocketfft::stride_t stride_in = {sizeof(std::complex<float>)};
    pocketfft::stride_t stride_out = {sizeof(std::complex<float>)};
    pocketfft::shape_t axes = {0};
    float fct = 1.0f / data.size();
    pocketfft::c2c(shape, stride_in, stride_out, axes, false, data.data(), data.data(), fct);
}