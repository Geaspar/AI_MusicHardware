#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"

using namespace AIMusicHardware;

// Color output for the terminal
void printColorText(const std::string& text, int colorCode) {
    std::cout << "\033[1;" << colorCode << "m" << text << "\033[0m" << std::endl;
}

int main(int argc, char* argv[]) {
    printColorText("=== AI Music Hardware - Audio Test ===", 34); // Blue
    
    // Create components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    
    // Initialize audio engine
    printColorText("Initializing audio engine...", 33); // Yellow
    if (!audioEngine->initialize()) {
        printColorText("Failed to initialize audio engine!", 31); // Red
        return 1;
    }
    printColorText("Audio engine initialized successfully!", 32); // Green
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process synthesizer directly
        synthesizer->process(outputBuffer, numFrames);
    });
    
    // Interactive audio test menu
    bool running = true;
    while (running) {
        std::cout << "\n";
        printColorText("Audio Test Menu:", 36); // Cyan
        std::cout << "1. Play C4 note (Sine wave)" << std::endl;
        std::cout << "2. Play C major scale" << std::endl;
        std::cout << "3. Test different waveforms" << std::endl;
        std::cout << "4. Play chord (C major)" << std::endl;
        std::cout << "5. Play arpeggio pattern" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Enter choice: ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 0:
                running = false;
                break;
                
            case 1: {
                // Play C4 note (MIDI note 60)
                printColorText("Playing C4 note for 2 seconds...", 33); // Yellow
                synthesizer->setOscillatorType(OscillatorType::Sine);
                synthesizer->noteOn(60, 0.7f);
                std::this_thread::sleep_for(std::chrono::seconds(2));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for release
                break;
            }
                
            case 2: {
                // C major scale: C4, D4, E4, F4, G4, A4, B4, C5
                printColorText("Playing C major scale...", 33); // Yellow
                synthesizer->setOscillatorType(OscillatorType::Sine);
                
                int scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
                for (int note : scale) {
                    synthesizer->noteOn(note, 0.7f);
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    synthesizer->noteOff(note);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                break;
            }
                
            case 3: {
                // Test different waveforms
                printColorText("Testing different waveforms...", 33); // Yellow
                
                // Square wave
                printColorText("Square wave:", 36); // Cyan
                synthesizer->setOscillatorType(OscillatorType::Square);
                synthesizer->noteOn(60, 0.5f);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                // Saw wave
                printColorText("Saw wave:", 36); // Cyan
                synthesizer->setOscillatorType(OscillatorType::Saw);
                synthesizer->noteOn(60, 0.5f);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                // Triangle wave
                printColorText("Triangle wave:", 36); // Cyan
                synthesizer->setOscillatorType(OscillatorType::Triangle);
                synthesizer->noteOn(60, 0.5f);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                // Sine wave
                printColorText("Sine wave:", 36); // Cyan
                synthesizer->setOscillatorType(OscillatorType::Sine);
                synthesizer->noteOn(60, 0.5f);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                // Noise
                printColorText("Noise:", 36); // Cyan
                synthesizer->setOscillatorType(OscillatorType::Noise);
                synthesizer->noteOn(60, 0.5f);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                synthesizer->noteOff(60);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                break;
            }
                
            case 4: {
                // Play C major chord (C4, E4, G4)
                printColorText("Playing C major chord...", 33); // Yellow
                synthesizer->setOscillatorType(OscillatorType::Sine);
                
                synthesizer->noteOn(60, 0.5f); // C4
                synthesizer->noteOn(64, 0.5f); // E4
                synthesizer->noteOn(67, 0.5f); // G4
                
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                synthesizer->noteOff(60);
                synthesizer->noteOff(64);
                synthesizer->noteOff(67);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            }
                
            case 5: {
                // Play arpeggio pattern
                printColorText("Playing arpeggio pattern...", 33); // Yellow
                synthesizer->setOscillatorType(OscillatorType::Sine);
                
                // C major chord notes
                int notes[] = {60, 64, 67, 72, 67, 64}; // C4, E4, G4, C5, G4, E4
                
                // Play the pattern twice
                for (int i = 0; i < 2; i++) {
                    for (int note : notes) {
                        synthesizer->noteOn(note, 0.6f);
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        synthesizer->noteOff(note);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
                break;
            }
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    // Cleanup
    printColorText("Shutting down audio engine...", 33); // Yellow
    audioEngine->shutdown();
    
    printColorText("Audio test completed!", 32); // Green
    return 0;
}