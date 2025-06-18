#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <fstream>
#include <RtAudio.h>

#include "../include/synthesis/oscillators/band_limited_oscillator.h"

using namespace AIMusicHardware;

// Audio callback data
struct AudioData {
    std::unique_ptr<BandLimitedOscillator> oscillator;
    float frequency = 440.0f;
    float targetFrequency = 440.0f;
    float sweepRate = 0.001f;
    bool sweeping = false;
};

// RtAudio callback
int audioCallback(void* outputBuffer, void* /*inputBuffer*/, unsigned int nBufferFrames,
                  double /*streamTime*/, RtAudioStreamStatus status, void* userData) {
    
    if (status) {
        std::cout << "Stream underflow detected!" << std::endl;
    }
    
    float* buffer = static_cast<float*>(outputBuffer);
    AudioData* data = static_cast<AudioData*>(userData);
    
    // Generate audio
    for (unsigned int i = 0; i < nBufferFrames; ++i) {
        // Smooth frequency sweep for testing
        if (data->sweeping) {
            data->frequency += (data->targetFrequency - data->frequency) * data->sweepRate;
            data->oscillator->setFrequency(data->frequency);
        }
        
        // Generate stereo output
        float sample = data->oscillator->generateSample() * 0.3f;  // Reduce volume
        buffer[i * 2] = sample;      // Left channel
        buffer[i * 2 + 1] = sample;  // Right channel
    }
    
    return 0;
}

void printSpectralContent(BandLimitedOscillator& osc, float frequency, int sampleRate) {
    // Generate a short buffer to analyze
    const int bufferSize = 4096;
    std::vector<float> buffer(bufferSize);
    
    osc.setFrequency(frequency);
    osc.resetPhase();
    osc.generateBlock(buffer.data(), bufferSize);
    
    // Simple peak detection to find harmonics
    std::cout << "\nFrequency: " << frequency << " Hz" << std::endl;
    std::cout << "Expected harmonics without aliasing: " << (sampleRate / 2.0f) / frequency << std::endl;
    
    // Save to file for analysis
    std::string filename = "wavetable_" + std::to_string(static_cast<int>(frequency)) + "Hz.txt";
    std::ofstream file(filename);
    for (float sample : buffer) {
        file << sample << "\n";
    }
    file.close();
    std::cout << "Saved waveform to " << filename << std::endl;
}

int main() {
    std::cout << "=== Band-Limited Oscillator Test ===" << std::endl;
    
    // Audio setup
    RtAudio audio;
    if (audio.getDeviceCount() < 1) {
        std::cout << "No audio devices found!" << std::endl;
        return 1;
    }
    
    RtAudio::StreamParameters parameters;
    parameters.deviceId = audio.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 256;
    
    // Create oscillator
    AudioData data;
    data.oscillator = OscillatorFactory::createBandLimitedOscillator(
        sampleRate,
        BandLimitedWavetable::WaveType::Saw,
        true  // Enable oversampling
    );
    
    // Test different waveforms and frequencies
    std::cout << "\n1. Testing band-limiting at different frequencies..." << std::endl;
    
    // Analyze spectral content at different frequencies
    data.oscillator->setWaveform(BandLimitedWavetable::WaveType::Saw);
    printSpectralContent(*data.oscillator, 100.0f, sampleRate);   // Low frequency - many harmonics
    printSpectralContent(*data.oscillator, 1000.0f, sampleRate);  // Mid frequency
    printSpectralContent(*data.oscillator, 5000.0f, sampleRate);  // High frequency - few harmonics
    
    // Open audio stream
    try {
        audio.openStream(&parameters, nullptr, RTAUDIO_FLOAT32,
                        sampleRate, &bufferFrames, &audioCallback, &data);
        audio.startStream();
    } catch (std::exception& e) {
        std::cerr << "Error opening audio stream: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n2. Interactive test - press keys to control oscillator:" << std::endl;
    std::cout << "  1-4: Change waveform (Sine, Saw, Square, Triangle)" << std::endl;
    std::cout << "  o: Toggle oversampling" << std::endl;
    std::cout << "  s: Start/stop frequency sweep" << std::endl;
    std::cout << "  +/-: Increase/decrease frequency" << std::endl;
    std::cout << "  q: Quit" << std::endl;
    
    bool running = true;
    bool oversamplingEnabled = true;
    
    while (running) {
        char key;
        std::cin >> key;
        
        switch (key) {
            case '1':
                data.oscillator->setWaveform(BandLimitedWavetable::WaveType::Sine);
                std::cout << "Waveform: Sine" << std::endl;
                break;
                
            case '2':
                data.oscillator->setWaveform(BandLimitedWavetable::WaveType::Saw);
                std::cout << "Waveform: Saw" << std::endl;
                break;
                
            case '3':
                data.oscillator->setWaveform(BandLimitedWavetable::WaveType::Square);
                std::cout << "Waveform: Square" << std::endl;
                break;
                
            case '4':
                data.oscillator->setWaveform(BandLimitedWavetable::WaveType::Triangle);
                std::cout << "Waveform: Triangle" << std::endl;
                break;
                
            case 'o':
                oversamplingEnabled = !oversamplingEnabled;
                data.oscillator->setOversamplingEnabled(oversamplingEnabled);
                std::cout << "Oversampling: " << (oversamplingEnabled ? "ON" : "OFF") << std::endl;
                break;
                
            case 's':
                data.sweeping = !data.sweeping;
                if (data.sweeping) {
                    data.targetFrequency = (data.frequency < 1000) ? 8000.0f : 100.0f;
                    std::cout << "Frequency sweep started" << std::endl;
                } else {
                    std::cout << "Frequency sweep stopped" << std::endl;
                }
                break;
                
            case '+':
                data.frequency *= 1.1f;
                data.oscillator->setFrequency(data.frequency);
                std::cout << "Frequency: " << data.frequency << " Hz" << std::endl;
                break;
                
            case '-':
                data.frequency /= 1.1f;
                data.oscillator->setFrequency(data.frequency);
                std::cout << "Frequency: " << data.frequency << " Hz" << std::endl;
                break;
                
            case 'q':
                running = false;
                break;
        }
    }
    
    // Cleanup
    try {
        audio.stopStream();
        if (audio.isStreamOpen()) {
            audio.closeStream();
        }
    } catch (std::exception& e) {
        std::cerr << "Error closing audio stream: " << e.what() << std::endl;
    }
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}