#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "../include/sequencer/AdaptiveSequencer.h"
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/hardware/HardwareInterface.h"

using namespace AIMusicHardware;
using namespace std::chrono_literals;

// Helper function to create a note pattern
std::vector<MidiEvent> createPattern(int baseNote, int octave, int numNotes, float noteDuration) {
    std::vector<MidiEvent> pattern;
    float timeOffset = 0.0f;
    
    for (int i = 0; i < numNotes; ++i) {
        // Note on event
        MidiEvent noteOn;
        noteOn.type = MidiEventType::NoteOn;
        noteOn.note = baseNote + i + (octave * 12);
        noteOn.velocity = 100;
        noteOn.time = timeOffset;
        pattern.push_back(noteOn);
        
        // Note off event
        MidiEvent noteOff;
        noteOff.type = MidiEventType::NoteOff;
        noteOff.note = baseNote + i + (octave * 12);
        noteOff.velocity = 0;
        noteOff.time = timeOffset + noteDuration;
        pattern.push_back(noteOff);
        
        // Increment time for next note
        timeOffset += noteDuration;
    }
    
    return pattern;
}

int main() {
    // Print welcome message
    std::cout << "=== Adaptive Sequencer Test ===" << std::endl;
    std::cout << "This example demonstrates the adaptive sequencer capabilities" << std::endl;
    std::cout << "including state transitions, parameter control, and layered mixing." << std::endl;
    std::cout << std::endl;
    
    // Create dependencies
    auto audioEngine = std::make_shared<AudioEngine>();
    auto synthesizer = std::make_shared<Synthesizer>();
    auto hardwareInterface = std::make_shared<HardwareInterface>();
    
    // Initialize dependencies
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        return -1;
    }
    
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer." << std::endl;
        return -1;
    }
    
    if (!hardwareInterface->initialize()) {
        std::cerr << "Failed to initialize hardware interface." << std::endl;
        // Continue anyway, hardware interface is optional
    }
    
    // Create the adaptive sequencer
    auto adaptiveSequencer = std::make_shared<AdaptiveSequencer>();
    
    if (!adaptiveSequencer->initialize(audioEngine, synthesizer, hardwareInterface)) {
        std::cerr << "Failed to initialize adaptive sequencer." << std::endl;
        return -1;
    }
    
    // Set up global parameters
    auto intensityParam = std::make_shared<Parameter>("intensity", 0.0f, 0.0f, 1.0f);
    adaptiveSequencer->addParameter(intensityParam);
    
    auto moodParam = std::make_shared<Parameter>("mood", 0.5f, 0.0f, 1.0f);
    adaptiveSequencer->addParameter(moodParam);
    
    // Create musical states
    
    // 1. Ambient state (low intensity)
    auto ambientState = std::make_shared<MusicState>("ambient");
    ambientState->setTempo(80.0f);
    ambientState->setTimeSignature(4, 4);
    ambientState->setLoopLength(4);  // 4 bars
    
    // Create layers for the ambient state
    auto ambientBass = std::make_shared<TrackLayer>("bass");
    ambientBass->setPattern(createPattern(36, 2, 4, 1.0f)); // Simple bass pattern
    ambientBass->setVolume(0.6f);
    
    auto ambientPad = std::make_shared<TrackLayer>("pad");
    ambientPad->setPattern(createPattern(48, 3, 2, 2.0f)); // Slow pad chords
    ambientPad->setVolume(0.8f);
    
    auto ambientMelody = std::make_shared<TrackLayer>("melody");
    ambientMelody->setPattern(createPattern(60, 4, 8, 0.5f)); // Sparse melody
    ambientMelody->setVolume(0.4f);
    
    // Add layers to the ambient state
    ambientState->addLayer(ambientBass);
    ambientState->addLayer(ambientPad);
    ambientState->addLayer(ambientMelody);
    
    // Create mix snapshots for the ambient state
    auto ambientFullMix = std::make_shared<MixSnapshot>("full");
    ambientFullMix->setLayerVolume("bass", 0.6f);
    ambientFullMix->setLayerVolume("pad", 0.8f);
    ambientFullMix->setLayerVolume("melody", 0.4f);
    
    auto ambientMinimalMix = std::make_shared<MixSnapshot>("minimal");
    ambientMinimalMix->setLayerVolume("bass", 0.4f);
    ambientMinimalMix->setLayerVolume("pad", 0.6f);
    ambientMinimalMix->setLayerMuted("melody", true);
    
    // Add mix snapshots to the ambient state
    ambientState->addSnapshot(ambientFullMix);
    ambientState->addSnapshot(ambientMinimalMix);
    ambientState->setActiveSnapshot("minimal");
    
    // 2. Energetic state (high intensity)
    auto energeticState = std::make_shared<MusicState>("energetic");
    energeticState->setTempo(120.0f);
    energeticState->setTimeSignature(4, 4);
    energeticState->setLoopLength(4);  // 4 bars
    
    // Create layers for the energetic state
    auto energeticBass = std::make_shared<TrackLayer>("bass");
    energeticBass->setPattern(createPattern(36, 2, 16, 0.25f)); // Fast bass pattern
    energeticBass->setVolume(0.8f);
    
    auto energeticLead = std::make_shared<TrackLayer>("lead");
    energeticLead->setPattern(createPattern(72, 4, 8, 0.5f)); // Lead synth
    energeticLead->setVolume(0.7f);
    
    auto energeticDrums = std::make_shared<TrackLayer>("drums");
    energeticDrums->setPattern(createPattern(48, 3, 16, 0.25f)); // Drum pattern
    energeticDrums->setVolume(0.9f);
    
    // Add layers to the energetic state
    energeticState->addLayer(energeticBass);
    energeticState->addLayer(energeticLead);
    energeticState->addLayer(energeticDrums);
    
    // Create mix snapshots for the energetic state
    auto energeticFullMix = std::make_shared<MixSnapshot>("full");
    energeticFullMix->setLayerVolume("bass", 0.8f);
    energeticFullMix->setLayerVolume("lead", 0.7f);
    energeticFullMix->setLayerVolume("drums", 0.9f);
    
    auto energeticDrumsMix = std::make_shared<MixSnapshot>("drums_only");
    energeticDrumsMix->setLayerMuted("bass", true);
    energeticDrumsMix->setLayerMuted("lead", true);
    energeticDrumsMix->setLayerVolume("drums", 1.0f);
    
    // Add mix snapshots to the energetic state
    energeticState->addSnapshot(energeticFullMix);
    energeticState->addSnapshot(energeticDrumsMix);
    energeticState->setActiveSnapshot("full");
    
    // Add states to the adaptive sequencer
    adaptiveSequencer->addState(ambientState);
    adaptiveSequencer->addState(energeticState);
    
    // Create transitions between states
    auto ambientToEnergeticTransition = std::make_shared<StateTransition>(
        "ambient_to_energetic", ambientState, energeticState);
    ambientToEnergeticTransition->setTransitionType(StateTransition::TransitionType::Crossfade);
    ambientToEnergeticTransition->setDuration(8.0f); // 8 beats (2 bars at 4/4)
    ambientToEnergeticTransition->setCondition("intensity", 0.7f, true); // When intensity > 0.7
    
    auto energeticToAmbientTransition = std::make_shared<StateTransition>(
        "energetic_to_ambient", energeticState, ambientState);
    energeticToAmbientTransition->setTransitionType(StateTransition::TransitionType::Crossfade);
    energeticToAmbientTransition->setDuration(8.0f); // 8 beats (2 bars at 4/4)
    energeticToAmbientTransition->setCondition("intensity", 0.3f, false); // When intensity < 0.3
    
    // Add transitions to the adaptive sequencer
    adaptiveSequencer->addTransition(ambientToEnergeticTransition);
    adaptiveSequencer->addTransition(energeticToAmbientTransition);
    
    // Set the initial state
    adaptiveSequencer->setActiveState("ambient");
    
    // Set up a callback for the state change event
    adaptiveSequencer->addEventListener("stateChanged", 
        [](const std::string& eventName, const std::map<std::string, float>& eventData) {
            std::cout << "State changed event: " << eventName << std::endl;
        }
    );
    
    // Start playback
    adaptiveSequencer->play();
    
    // Main control loop
    bool running = true;
    std::cout << "Adaptive Sequencer is running. Press keys to control:" << std::endl;
    std::cout << "1-5: Set intensity (1=0.0, 5=1.0)" << std::endl;
    std::cout << "a/e: Force state (a=ambient, e=energetic)" << std::endl;
    std::cout << "m/M: Decrease/increase mood parameter" << std::endl;
    std::cout << "p: Pause/resume" << std::endl;
    std::cout << "q: Quit" << std::endl;
    
    // Simulation of parameter changes (in a real application, this would come from the hardware interface)
    while (running) {
        // Process keyboard input (simulate hardware control)
        if (std::cin.rdbuf()->in_avail()) {
            char input = std::cin.get();
            switch (input) {
                case '1':
                    intensityParam->setValue(0.0f);
                    std::cout << "Intensity set to 0.0" << std::endl;
                    break;
                case '2':
                    intensityParam->setValue(0.25f);
                    std::cout << "Intensity set to 0.25" << std::endl;
                    break;
                case '3':
                    intensityParam->setValue(0.5f);
                    std::cout << "Intensity set to 0.5" << std::endl;
                    break;
                case '4':
                    intensityParam->setValue(0.75f);
                    std::cout << "Intensity set to 0.75" << std::endl;
                    break;
                case '5':
                    intensityParam->setValue(1.0f);
                    std::cout << "Intensity set to 1.0" << std::endl;
                    break;
                case 'a':
                    adaptiveSequencer->setActiveState("ambient");
                    std::cout << "Forced state: ambient" << std::endl;
                    break;
                case 'e':
                    adaptiveSequencer->setActiveState("energetic");
                    std::cout << "Forced state: energetic" << std::endl;
                    break;
                case 'm':
                    moodParam->setValue(std::max(0.0f, moodParam->getValue() - 0.1f));
                    std::cout << "Mood decreased to " << moodParam->getValue() << std::endl;
                    break;
                case 'M':
                    moodParam->setValue(std::min(1.0f, moodParam->getValue() + 0.1f));
                    std::cout << "Mood increased to " << moodParam->getValue() << std::endl;
                    break;
                case 'p':
                    if (adaptiveSequencer->isPlaying()) {
                        adaptiveSequencer->pause();
                        std::cout << "Playback paused" << std::endl;
                    } else {
                        adaptiveSequencer->play();
                        std::cout << "Playback resumed" << std::endl;
                    }
                    break;
                case 'q':
                    running = false;
                    break;
                default:
                    break;
            }
        }
        
        // Update the adaptive sequencer
        adaptiveSequencer->update(0.016f); // ~60 fps
        
        // Sleep to avoid maxing out CPU
        std::this_thread::sleep_for(16ms);
    }
    
    // Cleanup
    adaptiveSequencer->stop();
    adaptiveSequencer->shutdown();
    synthesizer->shutdown();
    audioEngine->shutdown();
    
    std::cout << "Adaptive Sequencer test completed." << std::endl;
    
    return 0;
}