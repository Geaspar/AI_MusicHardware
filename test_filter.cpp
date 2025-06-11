#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include "include/audio/AudioEngine.h"
#include "include/audio/Synthesizer.h"
#include "include/effects/EffectProcessor.h"
#include "include/effects/Filter.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "Filter Test Program" << std::endl;
    
    // Create audio components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto effectProcessor = std::make_unique<EffectProcessor>();
    
    // Initialize
    if (!audioEngine->initialize() || !synthesizer->initialize() || !effectProcessor->initialize()) {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }
    
    // Add filter to effect processor
    auto filter = std::make_unique<Filter>(audioEngine->getSampleRate(), Filter::Type::LowPass);
    filter->setParameter("mix", 1.0f);
    filter->setParameter("frequency", 20000.0f); // Start wide open
    filter->setParameter("resonance", 0.7f);
    effectProcessor->addEffect(std::move(filter));
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* buffer, int numFrames) {
        // Process synthesizer
        synthesizer->process(buffer, numFrames);
        
        // Process effects
        effectProcessor->process(buffer, numFrames);
    });
    
    // Play test sequence
    std::cout << "\nPlaying note with filter sweep..." << std::endl;
    
    // Play a note
    synthesizer->noteOn(60, 0.7f); // Middle C
    
    // Sweep filter from high to low
    for (int i = 0; i < 50; i++) {
        float freq = 20000.0f * std::pow(0.9f, i); // Exponential sweep
        if (effectProcessor->getNumEffects() > 0) {
            effectProcessor->getEffect(0)->setParameter("frequency", freq);
            std::cout << "Filter frequency: " << freq << " Hz" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Note off
    synthesizer->noteOff(60);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test resonance
    std::cout << "\nTesting resonance..." << std::endl;
    effectProcessor->getEffect(0)->setParameter("frequency", 1000.0f);
    
    synthesizer->noteOn(60, 0.7f);
    for (float res = 0.7f; res <= 10.0f; res += 1.0f) {
        if (effectProcessor->getNumEffects() > 0) {
            effectProcessor->getEffect(0)->setParameter("resonance", res);
            std::cout << "Resonance: " << res << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    synthesizer->noteOff(60);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cleanup
    audioEngine->shutdown();
    
    std::cout << "\nTest complete!" << std::endl;
    return 0;
}