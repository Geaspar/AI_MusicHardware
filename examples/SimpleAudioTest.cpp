#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== Simple Audio Test ===\n";
    
    // Create components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    
    // Initialize audio engine
    std::cout << "Initializing audio engine...\n";
    if (!audioEngine->initialize()) {
        std::cout << "Failed to initialize audio engine!\n";
        return 1;
    }
    std::cout << "Audio engine initialized successfully!\n";
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        synthesizer->process(outputBuffer, numFrames);
    });
    
    // Play a simple C major scale
    std::cout << "Playing C major scale...\n";
    synthesizer->setOscillatorType(OscillatorType::Sine);
    
    // C major scale: C4, D4, E4, F4, G4, A4, B4, C5
    int scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
    for (int note : scale) {
        std::cout << "Playing note: " << note << std::endl;
        synthesizer->noteOn(note, 0.7f);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        synthesizer->noteOff(note);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << "Shutting down audio engine...\n";
    audioEngine->shutdown();
    
    std::cout << "Audio test completed!\n";
    return 0;
}