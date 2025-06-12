#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <vector>
#include "synthesis/modulators/LFO.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== Simple LFO Test ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    LFO lfo(sampleRate);
    
    // Test 1: Basic sine wave
    std::cout << "\nTest 1: Sine wave at 1Hz" << std::endl;
    lfo.setShape(LFO::WaveShape::Sine);
    lfo.setRate(1.0f);
    lfo.setDepth(1.0f);
    lfo.setBipolar(true);  // Explicitly set bipolar mode
    lfo.reset();
    
    // Sample for 1 second (should complete 1 cycle)
    float minVal = 1.0f, maxVal = -1.0f;
    for (int i = 0; i < sampleRate; ++i) {
        float val = lfo.process();
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    std::cout << "Range: [" << minVal << ", " << maxVal << "]" << std::endl;
    std::cout << "Expected: [-1, 1]" << std::endl;
    
    // Test 2: Unipolar mode
    std::cout << "\nTest 2: Unipolar sine wave" << std::endl;
    lfo.setBipolar(false);
    lfo.reset();
    
    minVal = 1.0f;
    maxVal = 0.0f;
    for (int i = 0; i < sampleRate; ++i) {
        float val = lfo.process();
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
    }
    std::cout << "Range: [" << minVal << ", " << maxVal << "]" << std::endl;
    std::cout << "Expected: [0, 1]" << std::endl;
    
    // Test 3: Different waveforms
    std::cout << "\nTest 3: Waveform shapes (first 10 samples)" << std::endl;
    std::vector<std::pair<LFO::WaveShape, std::string>> shapes = {
        {LFO::WaveShape::Sine, "Sine"},
        {LFO::WaveShape::Triangle, "Triangle"},
        {LFO::WaveShape::Saw, "Saw"},
        {LFO::WaveShape::Square, "Square"}
    };
    
    lfo.setBipolar(true);
    lfo.setRate(100.0f);  // Higher rate to see shape changes
    
    for (const auto& [shape, name] : shapes) {
        std::cout << "\n" << name << ": ";
        lfo.setShape(shape);
        lfo.reset();
        
        // Debug: Check parameters after reset
        auto params = lfo.getParameters();
        std::cout << "(bipolar=" << params.bipolar << ", phase=" << params.phase << ") ";
        
        for (int i = 0; i < 10; ++i) {
            float val = lfo.process();
            std::cout << std::fixed << std::setprecision(3) << val << " ";
        }
        std::cout << std::endl;
    }
    
    // Test 4: Performance
    std::cout << "\nTest 4: Performance (1 million samples)" << std::endl;
    lfo.setShape(LFO::WaveShape::Sine);
    lfo.setRate(440.0f);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        lfo.process();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Per sample: " << (duration.count() / 1000000.0) << " microseconds" << std::endl;
    
    std::cout << "\nAll tests completed!" << std::endl;
    return 0;
}