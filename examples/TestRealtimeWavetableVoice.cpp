#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "synthesis/RealtimeWavetableVoice.h"
#include "synthesis/FrequencyDomainWavetable.h"

using namespace AIMusicHardware;

void write_wav_header(std::ofstream& file, int sample_rate, int num_samples) {
    file.write("RIFF", 4);
    int chunk_size = 36 + num_samples * 2;
    file.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    int subchunk1_size = 16;
    file.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    short audio_format = 1;
    file.write(reinterpret_cast<const char*>(&audio_format), 2);
    short num_channels = 1;
    file.write(reinterpret_cast<const char*>(&num_channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    int byte_rate = sample_rate * 2;
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    short block_align = 2;
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    short bits_per_sample = 16;
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    file.write("data", 4);
    int subchunk2_size = num_samples * 2;
    file.write(reinterpret_cast<const char*>(&subchunk2_size), 4);
}

int main() {
    const int sample_rate = 44100;
    const int duration_seconds = 2;
    const int num_samples = sample_rate * duration_seconds;

    auto wavetable = std::make_shared<FrequencyDomainWavetable>();
    if (!wavetable->loadFromFile("generated_presets/sine.json")) {
        std::cerr << "Failed to load wavetable file." << std::endl;
        return 1;
    }

    RealtimeWavetableVoice voice(wavetable, sample_rate);
    voice.noteOn(69, 1.0f); // A4

    std::vector<float> audio_buffer(num_samples);
    voice.process(audio_buffer.data(), num_samples);

    std::ofstream output_file("test_realtime_voice.wav", std::ios::binary);
    if (!output_file.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
        return 1;
    }

    write_wav_header(output_file, sample_rate, num_samples);

    for (int i = 0; i < num_samples; ++i) {
        short sample = static_cast<short>(audio_buffer[i] * 32767.0f);
        output_file.write(reinterpret_cast<const char*>(&sample), 2);
    }

    std::cout << "Successfully wrote output to test_realtime_voice.wav" << std::endl;

    return 0;
}