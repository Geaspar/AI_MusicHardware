#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cmath>
#include "synthesis/modulators/LFO.h"

using namespace AIMusicHardware;

void printWaveform(const std::string& name, LFO& lfo, int samples = 100) {
    std::cout << "\n" << name << " Waveform:" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (int i = 0; i < samples; ++i) {
        float value = lfo.process();
        
        // Convert to 0-40 range for display
        int pos = static_cast<int>((value + 1.0f) * 20.0f);
        pos = std::max(0, std::min(40, pos));
        
        // Print the waveform
        std::cout << "|";
        for (int j = 0; j < 40; ++j) {
            if (j == pos) {
                std::cout << "*";
            } else if (j == 20) {
                std::cout << "|";  // Center line
            } else {
                std::cout << " ";
            }
        }
        std::cout << "| " << std::fixed << std::setprecision(3) << value << std::endl;
    }
}

void testLFOShapes() {
    const float sampleRate = 1000.0f;  // Low sample rate for visualization
    LFO lfo(sampleRate);
    
    // Test each waveform
    std::vector<std::pair<LFO::WaveShape, std::string>> shapes = {
        {LFO::WaveShape::Sine, "Sine"},
        {LFO::WaveShape::Triangle, "Triangle"},
        {LFO::WaveShape::Saw, "Saw"},
        {LFO::WaveShape::Square, "Square"},
        {LFO::WaveShape::Random, "Random (S&H)"},
        {LFO::WaveShape::Smooth, "Smooth Random"}
    };
    
    lfo.setRate(10.0f);  // 10 Hz
    lfo.setDepth(1.0f);
    
    for (const auto& [shape, name] : shapes) {
        lfo.setShape(shape);
        lfo.reset();
        printWaveform(name, lfo, 50);  // Reduced samples for brevity
    }
}

void testModulationSpeed() {
    const float sampleRate = 44100.0f;
    LFO lfo(sampleRate);
    
    std::cout << "\n\nTesting LFO Speed and Performance" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    lfo.setShape(LFO::WaveShape::Sine);
    lfo.setDepth(1.0f);
    
    std::vector<float> rates = {0.1f, 1.0f, 5.0f, 20.0f};
    
    for (float rate : rates) {
        lfo.setRate(rate);
        lfo.reset();
        
        std::cout << "\nRate: " << rate << " Hz" << std::endl;
        
        // Measure performance
        auto start = std::chrono::high_resolution_clock::now();
        
        const int numSamples = 441000;  // 10 seconds at 44.1kHz
        for (int i = 0; i < numSamples; ++i) {
            lfo.process();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Processed " << numSamples << " samples in " 
                  << duration.count() << " microseconds" << std::endl;
        std::cout << "Average time per sample: " 
                  << (duration.count() / static_cast<double>(numSamples)) 
                  << " microseconds" << std::endl;
    }
}

void testUnipolarBipolar() {
    const float sampleRate = 1000.0f;
    LFO lfo(sampleRate);
    
    std::cout << "\n\nTesting Unipolar vs Bipolar Output" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    lfo.setShape(LFO::WaveShape::Sine);
    lfo.setRate(5.0f);
    lfo.setDepth(1.0f);
    
    // Test bipolar
    lfo.setBipolar(true);
    lfo.reset();
    
    std::cout << "\nBipolar output (range -1 to 1):" << std::endl;
    float minBi = 1.0f, maxBi = -1.0f;
    for (int i = 0; i < 200; ++i) {
        float val = lfo.process();
        minBi = std::min(minBi, val);
        maxBi = std::max(maxBi, val);
    }
    std::cout << "Min: " << minBi << ", Max: " << maxBi << std::endl;
    
    // Test unipolar
    lfo.setBipolar(false);
    lfo.reset();
    
    std::cout << "\nUnipolar output (range 0 to 1):" << std::endl;
    float minUni = 1.0f, maxUni = 0.0f;
    for (int i = 0; i < 200; ++i) {
        float val = lfo.process();
        minUni = std::min(minUni, val);
        maxUni = std::max(maxUni, val);
    }
    std::cout << "Min: " << minUni << ", Max: " << maxUni << std::endl;
}

void testPhaseOffset() {
    const float sampleRate = 1000.0f;
    
    std::cout << "\n\nTesting Phase Offset" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Create two LFOs with different phase offsets
    LFO lfo1(sampleRate);
    LFO lfo2(sampleRate);
    
    lfo1.setShape(LFO::WaveShape::Sine);
    lfo1.setRate(5.0f);
    lfo1.setDepth(1.0f);
    lfo1.setPhase(0.0f);
    
    lfo2.setShape(LFO::WaveShape::Sine);
    lfo2.setRate(5.0f);
    lfo2.setDepth(1.0f);
    lfo2.setPhase(0.25f);  // 90 degrees phase offset
    
    lfo1.reset();
    lfo2.reset();
    
    std::cout << "LFO1 (0°) vs LFO2 (90°):" << std::endl;
    std::cout << "Sample\tLFO1\tLFO2\tDifference" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    for (int i = 0; i < 20; ++i) {
        float val1 = lfo1.process();
        float val2 = lfo2.process();
        std::cout << i << "\t" 
                  << std::fixed << std::setprecision(3) << val1 << "\t"
                  << val2 << "\t"
                  << (val2 - val1) << std::endl;
    }
}

int main() {
    std::cout << "=== LFO Test Program ===" << std::endl;
    std::cout << "Testing the LFO implementation without UI" << std::endl;
    
    // Test different waveform shapes
    testLFOShapes();
    
    // Test modulation speed and performance
    testModulationSpeed();
    
    // Test unipolar/bipolar modes
    testUnipolarBipolar();
    
    // Test phase offset
    testPhaseOffset();
    
    std::cout << "\n\nAll tests completed!" << std::endl;
    
    return 0;
}