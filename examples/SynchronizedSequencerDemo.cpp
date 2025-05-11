#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;
using namespace std::chrono_literals;

// Global flag for signal handling
std::atomic<bool> keepRunning{true};

// Signal handler to gracefully terminate
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    keepRunning.store(false);
}

// Helper function to create a simple pattern
std::unique_ptr<Pattern> createSimplePattern(const std::string& name) {
    auto pattern = std::make_unique<Pattern>(name);
    
    // Create a simple C major scale
    int notes[] = {60, 62, 64, 65, 67, 69, 71, 72}; // C4 to C5
    
    for (int i = 0; i < 8; ++i) {
        float velocity = 0.7f + (i % 2) * 0.2f; // Alternate between 0.7 and 0.9 velocity
        float duration = 0.24f; // Slightly less than a quarter note for distinction
        Note note(notes[i], velocity, i * 0.25, duration);
        pattern->addNote(note);
    }
    
    return pattern;
}

int main() {
    std::cout << "=== Synchronized Sequencer Demo ===" << std::endl;
    std::cout << "This example demonstrates precise timing synchronization" << std::endl;
    std::cout << "between the audio engine and sequencer." << std::endl;
    
    // Set up signal handler for graceful termination
    std::signal(SIGINT, signalHandler);
    
    // Create components
    auto audioEngine = std::make_shared<AudioEngine>(44100, 256); // Smaller buffer for lower latency
    auto synthesizer = std::make_shared<Synthesizer>();
    auto sequencer = std::make_shared<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    
    // Initialize the audio engine
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Initialize the synthesizer
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer!" << std::endl;
        return 1;
    }
    
    // Initialize the sequencer
    if (!sequencer->initialize()) {
        std::cerr << "Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    
    // Add a pattern to the sequencer
    sequencer->addPattern(createSimplePattern("Synchronized Pattern"));
    
    // Set up synchronization on a periodic basis
    std::thread syncThread([&]() {
        while (keepRunning.load()) {
            // Synchronize every 500ms to maintain tight synchronization
            audioEngine->synchronizeSequencer(sequencer);
            std::this_thread::sleep_for(500ms);
        }
    });
    
    // Set up note callbacks from sequencer to synthesizer
    sequencer->setNoteCallbacks(
        // Note on callback
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            std::cout << "Note On: " << pitch << " Velocity: " << velocity << std::endl;
            synthesizer->noteOn(pitch, velocity, env);
        },
        // Note off callback
        [&](int pitch, int channel) {
            std::cout << "Note Off: " << pitch << std::endl;
            synthesizer->noteOff(pitch);
        }
    );
    
    // Set up transport callback for precise timing information
    sequencer->setTransportCallback([](double positionInBeats, int bar, int beat) {
        // Only print occasionally to avoid console spam
        static int printCounter = 0;
        if (++printCounter % 8 == 0) {
            std::cout << "\rPosition: " << std::fixed << std::setprecision(4) 
                      << positionInBeats << " | Bar: " << bar 
                      << " Beat: " << beat << std::flush;
        }
    });
    
    // Set up audio callback that processes both sequencer and synthesizer
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Calculate deltaTime in seconds based on audio buffer size and sample rate
        double deltaTime = static_cast<double>(numFrames) / audioEngine->getSampleRate();
        
        // Process sequencer with precise timing
        sequencer->process(deltaTime);
        
        // Process synthesizer to generate audio
        synthesizer->process(outputBuffer, numFrames);
    });
    
    // Start the sequencer
    sequencer->start();
    
    std::cout << "Synchronized sequencer playback started. Press Ctrl+C to stop." << std::endl;
    
    // Enable looping
    sequencer->setLooping(true);
    
    // Run until interrupted
    while (keepRunning.load()) {
        std::this_thread::sleep_for(100ms);
    }
    
    // Clean up
    sequencer->stop();
    
    // Wait for sync thread to finish
    syncThread.join();
    
    // Shut down components
    synthesizer->allNotesOff();
    audioEngine->shutdown();
    
    std::cout << "\nSynchronized sequencer demo completed." << std::endl;
    return 0;
}