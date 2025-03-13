#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <limits>

#include "../include/sequencer/Sequencer.h"
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"

using namespace AIMusicHardware;

// Helper function to create a simple pattern with custom envelopes
std::unique_ptr<Pattern> createSimplePattern() {
    auto pattern = std::make_unique<Pattern>("Simple Pattern");
    
    // Add a simple C major scale
    // C4, D4, E4, F4, G4, A4, B4, C5
    int notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
    
    for (int i = 0; i < 8; ++i) {
        // Each note is placed at a specific beat position
        // with a duration of 0.5 beats
        
        // Short attack, medium decay, high sustain, medium release
        Note note(notes[i], 0.8f, i * 0.5, 0.4, 0, 
                 0.02f,  // attack (20ms)
                 0.1f,   // decay (100ms)
                 0.8f,   // sustain (80%)
                 0.3f);  // release (300ms)
        pattern->addNote(note);
    }
    
    return pattern;
}

// Helper function to create a chord pattern
std::unique_ptr<Pattern> createChordPattern() {
    auto pattern = std::make_unique<Pattern>("Chord Pattern");
    
    // Envelope for pads - slow attack, long release
    float padAttack = 0.3f;    // 300ms attack
    float padDecay = 0.2f;     // 200ms decay
    float padSustain = 0.7f;   // 70% sustain level
    float padRelease = 0.8f;   // 800ms release
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 0.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // C4
    pattern->addNote(Note(64, 0.7f, 0.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // E4
    pattern->addNote(Note(67, 0.7f, 0.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // G4
    
    // F Major chord (F, A, C)
    pattern->addNote(Note(65, 0.7f, 1.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // F4
    pattern->addNote(Note(69, 0.7f, 1.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // A4
    pattern->addNote(Note(72, 0.7f, 1.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // C5
    
    // G Major chord (G, B, D)
    pattern->addNote(Note(67, 0.7f, 2.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // G4
    pattern->addNote(Note(71, 0.7f, 2.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // B4
    pattern->addNote(Note(74, 0.7f, 2.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // D5
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 3.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // C4
    pattern->addNote(Note(64, 0.7f, 3.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // E4
    pattern->addNote(Note(67, 0.7f, 3.0, 1.0, 0, padAttack, padDecay, padSustain, padRelease));  // G4
    
    return pattern;
}

// Helper function to create an arpeggio pattern
std::unique_ptr<Pattern> createArpeggioPattern() {
    auto pattern = std::make_unique<Pattern>("Arpeggio Pattern");
    
    // Envelope for arpeggios - very fast attack, short decay, quick release
    float arpAttack = 0.005f;  // 5ms attack - very fast
    float arpDecay = 0.1f;     // 100ms decay
    float arpSustain = 0.6f;   // 60% sustain level
    float arpRelease = 0.1f;   // 100ms release - quick
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 0.0, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // C4
    pattern->addNote(Note(64, 0.7f, 0.25, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // E4
    pattern->addNote(Note(67, 0.7f, 0.5, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // G4
    pattern->addNote(Note(72, 0.7f, 0.75, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // C5
    
    // F Major arpeggio (F, A, C, F)
    pattern->addNote(Note(65, 0.7f, 1.0, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // F4
    pattern->addNote(Note(69, 0.7f, 1.25, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // A4
    pattern->addNote(Note(72, 0.7f, 1.5, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // C5
    pattern->addNote(Note(77, 0.7f, 1.75, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // F5
    
    // G Major arpeggio (G, B, D, G)
    pattern->addNote(Note(67, 0.7f, 2.0, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // G4
    pattern->addNote(Note(71, 0.7f, 2.25, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // B4
    pattern->addNote(Note(74, 0.7f, 2.5, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // D5
    pattern->addNote(Note(79, 0.7f, 2.75, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // G5
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 3.0, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // C4
    pattern->addNote(Note(64, 0.7f, 3.25, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // E4
    pattern->addNote(Note(67, 0.7f, 3.5, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease));  // G4
    pattern->addNote(Note(72, 0.7f, 3.75, 0.25, 0, arpAttack, arpDecay, arpSustain, arpRelease)); // C5
    
    return pattern;
}

// Print a simple transport display showing current position and beat
void printTransport(double positionInBeats, double tempo) {
    int bar = static_cast<int>(positionInBeats / 4) + 1;
    double beatInBar = fmod(positionInBeats, 4.0);
    int beat = static_cast<int>(beatInBar) + 1;
    
    // Clear the line and print the new position
    std::cout << "\r";
    std::cout << "Bar: " << std::setw(2) << bar << " | Beat: " << beat << "." 
              << static_cast<int>((beatInBar - static_cast<int>(beatInBar)) * 10) 
              << " | Tempo: " << static_cast<int>(tempo) << " BPM";
    std::cout << std::flush;
}

int main() {
    std::cout << "===== Sequencer Test =====" << std::endl;
    
    // Create components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto sequencer = std::make_unique<Sequencer>(120.0, 4); // 120 BPM, 4/4 time
    
    // Create patterns
    auto simplePattern = createSimplePattern();
    auto chordPattern = createChordPattern();
    auto arpeggioPattern = createArpeggioPattern();
    
    // Add patterns to sequencer
    sequencer->addPattern(std::move(simplePattern));
    sequencer->addPattern(std::move(chordPattern));
    sequencer->addPattern(std::move(arpeggioPattern));
    
    // Set up note callbacks from sequencer to synthesizer
    sequencer->setNoteCallbacks(
        // Note on callback with envelope
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            std::cout << "\nNote On: " << pitch << " Velocity: " << velocity 
                      << " Env: [A:" << env.attack << " D:" << env.decay 
                      << " S:" << env.sustain << " R:" << env.release << "]" << std::endl;
            synthesizer->noteOn(pitch, velocity, env);
        },
        // Note off callback
        [&](int pitch, int channel) {
            std::cout << "\nNote Off: " << pitch << std::endl;
            synthesizer->noteOff(pitch);
        }
    );
    
    // Initialize audio engine
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process sequencer
        sequencer->process(1.0 / audioEngine->getSampleRate() * numFrames);
        
        // Process synthesizer
        synthesizer->process(outputBuffer, numFrames);
    });
    
    // Interactive test menu
    bool running = true;
    while (running) {
        std::cout << "\n\nSequencer Test Menu:" << std::endl;
        std::cout << "1. Play Simple Pattern (Scale)" << std::endl;
        std::cout << "2. Play Chord Pattern" << std::endl;
        std::cout << "3. Play Arpeggio Pattern" << std::endl;
        std::cout << "4. Change Tempo" << std::endl;
        std::cout << "5. Toggle Looping" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Enter choice: ";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 0:
                running = false;
                break;
                
            case 1: {
                std::cout << "Playing Simple Pattern (Scale)..." << std::endl;
                std::cout << "Press Enter to stop playback..." << std::endl;
                
                // Make sure input buffer is clear
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                
                // Start sequencer
                sequencer->setCurrentPattern(0);
                sequencer->start();
                
                // Set up async input thread
                bool stopRequested = false;
                std::thread inputThread([&stopRequested]() {
                    std::cin.get();
                    stopRequested = true;
                });
                
                // Play until stopped or pattern ends naturally
                while (sequencer->isPlaying() && !stopRequested) {
                    printTransport(sequencer->getPositionInBeats(), sequencer->getTempo());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                // Clean up
                sequencer->stop();
                
                if (inputThread.joinable()) {
                    inputThread.detach();
                }
                
                std::cout << "\nStopped." << std::endl;
                break;
            }
                
            case 2: {
                std::cout << "Playing Chord Pattern..." << std::endl;
                std::cout << "Press Enter to stop playback..." << std::endl;
                
                // Make sure input buffer is clear
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                
                // Start sequencer
                sequencer->setCurrentPattern(1);
                sequencer->start();
                
                // Set up async input thread
                bool stopRequested = false;
                std::thread inputThread([&stopRequested]() {
                    std::cin.get();
                    stopRequested = true;
                });
                
                // Play until stopped or pattern ends naturally
                while (sequencer->isPlaying() && !stopRequested) {
                    printTransport(sequencer->getPositionInBeats(), sequencer->getTempo());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                // Clean up
                sequencer->stop();
                
                if (inputThread.joinable()) {
                    inputThread.detach();
                }
                
                std::cout << "\nStopped." << std::endl;
                break;
            }
                
            case 3: {
                std::cout << "Playing Arpeggio Pattern..." << std::endl;
                std::cout << "Press Enter to stop playback..." << std::endl;
                
                // Make sure input buffer is clear
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                
                // Start sequencer
                sequencer->setCurrentPattern(2);
                sequencer->start();
                
                // Set up async input thread
                bool stopRequested = false;
                std::thread inputThread([&stopRequested]() {
                    std::cin.get();
                    stopRequested = true;
                });
                
                // Play until stopped or pattern ends naturally
                while (sequencer->isPlaying() && !stopRequested) {
                    printTransport(sequencer->getPositionInBeats(), sequencer->getTempo());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                // Clean up
                sequencer->stop();
                
                if (inputThread.joinable()) {
                    inputThread.detach();
                }
                
                std::cout << "\nStopped." << std::endl;
                break;
            }
                
            case 4: {
                double tempo;
                std::cout << "Enter new tempo (BPM): ";
                std::cin >> tempo;
                if (tempo > 0 && tempo < 300) {
                    sequencer->setTempo(tempo);
                    std::cout << "Tempo set to " << tempo << " BPM" << std::endl;
                } else {
                    std::cout << "Invalid tempo. Must be between 1 and 300 BPM." << std::endl;
                }
                break;
            }
                
            case 5: {
                bool looping = sequencer->isLooping();
                sequencer->setLooping(!looping);
                std::cout << "Looping " << (sequencer->isLooping() ? "enabled" : "disabled") << std::endl;
                break;
            }
                
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    // Cleanup
    audioEngine->shutdown();
    
    std::cout << "Sequencer test completed!" << std::endl;
    return 0;
}