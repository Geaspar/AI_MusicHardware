#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <cmath>
#include "synthesis/FrequencyDomainWavetable.h"
#include "audio/FourierTransform.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <output_directory>" << std::endl;
        return 1;
    }

    std::string output_dir = argv[1];

    std::vector<float> sine_wavetable(2048);
    for (int i = 0; i < 2048; ++i) {
        sine_wavetable[i] = sin(2.0 * M_PI * static_cast<double>(i) / 2048.0);
    }

    FrequencyDomainWavetable fd_wavetable;
    fd_wavetable.setName("sine");
    fd_wavetable.setNumFrames(1);

    std::vector<std::complex<float>> harmonic_data(sine_wavetable.size());
    for (size_t i = 0; i < sine_wavetable.size(); ++i) {
        harmonic_data[i] = {sine_wavetable[i], 0.0f};
    }

    FourierTransform fft;
    fft.performFFT(harmonic_data);

    fd_wavetable.setHarmonicData(0, harmonic_data);

    std::string output_path = output_dir + "/sine.json";
    if (fd_wavetable.saveToFile(output_path)) {
        std::cout << "Successfully converted sine wavetable to " << output_path << std::endl;
    } else {
        std::cerr << "Failed to convert sine wavetable." << std::endl;
        return 1;
    }

    return 0;
}