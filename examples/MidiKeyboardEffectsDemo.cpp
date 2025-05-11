#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <cctype> // for tolower

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"
#include "../include/effects/AllEffects.h"
#include "../include/effects/ReorderableEffectsChain.h"
#include "../include/effects/MidiEffectControl.h"

using namespace AIMusicHardware;

// Signal handler for clean shutdown
bool running = true;
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// MIDI manager listener to handle parameter changes
class MidiListener : public MidiManager::Listener {
public:
    void parameterChangedViaMidi(const std::string& paramId, float value) override {
        std::cout << "Parameter changed: " << paramId << " = " << value << std::endl;
    }
    
    void pitchBendChanged(int channel, float value) override {
        std::cout << "Pitch bend: " << value << " on channel " << channel << std::endl;
    }
    
    void modWheelChanged(int channel, float value) override {
        std::cout << "Mod wheel: " << value << " on channel " << channel << std::endl;
    }
    
    void afterTouchChanged(int channel, float value) override {
        std::cout << "Aftertouch: " << value << " on channel " << channel << std::endl;
    }
};

// Helper function to display help information
void printHelp() {
    std::cout << "\nKeyboard controls:" << std::endl;
    std::cout << "  1-5: Change oscillator type (Sine, Square, Saw, Triangle, Noise)" << std::endl;
    std::cout << "  e: Show effects menu" << std::endl;
    std::cout << "  h: Show this help message" << std::endl;
    std::cout << "  q: Quit application" << std::endl;
}

// Helper function to display effects menu
void printEffectsMenu() {
    std::cout << "\nEffects Menu:" << std::endl;
    std::cout << "  a: Add effect" << std::endl;
    std::cout << "  r: Remove effect" << std::endl;
    std::cout << "  c: Clear all effects" << std::endl;
    std::cout << "  p: Print current effects chain" << std::endl;
    std::cout << "  m: Modify effect parameter" << std::endl;
    std::cout << "  o: Reorder effects" << std::endl;
    std::cout << "  t: Toggle effect on/off" << std::endl;
    std::cout << "  l: MIDI learn for effect parameter" << std::endl; // New option
    std::cout << "  u: Unmap MIDI for effect parameter" << std::endl; // New option
    std::cout << "  v: View MIDI mappings" << std::endl; // New option
    std::cout << "  b: Back to main controls" << std::endl;
}

// Helper function to print effect parameter values
void printEffectParameters(Effect* effect) {
    if (!effect)
        return;
        
    std::string type = effect->getName();
    std::cout << "Parameters for " << type << ":" << std::endl;
    
    // Define known parameters for each effect type
    std::map<std::string, std::vector<std::string>> effectParams = {
        {"Delay", {"delayTime", "feedback", "mix"}},
        {"Reverb", {"roomSize", "damping", "wetLevel", "dryLevel", "width"}},
        {"LowPassFilter", {"frequency", "resonance", "mix"}},
        {"HighPassFilter", {"frequency", "resonance", "mix"}},
        {"BandPassFilter", {"frequency", "resonance", "mix"}},
        {"NotchFilter", {"frequency", "resonance", "mix"}},
        {"Distortion", {"drive", "tone", "mix"}},
        {"Compressor", {"threshold", "ratio", "attack", "release", "makeup"}},
        {"Phaser", {"rate", "depth", "feedback", "mix"}},
        {"BitCrusher", {"bitDepth", "sampleRateReduction", "mix"}},
        {"EQ", {"lowGain", "midGain", "highGain", "lowFreq", "highFreq", "q"}}
    };
    
    // Display current values
    if (effectParams.find(type) != effectParams.end()) {
        for (const auto& param : effectParams[type]) {
            float value = effect->getParameter(param);
            std::cout << "  " << param << ": " << value << std::endl;
        }
    } else {
        std::cout << "  (No known parameters for this effect type)" << std::endl;
    }
}

int main() {
    // Set up signal handling for clean shutdown
    std::signal(SIGINT, signalHandler);
    
    std::cout << "AI Music Hardware - MIDI Keyboard with Effects Demo" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    // Create all the necessary components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto midiListener = std::make_unique<MidiListener>();
    auto midiManager = std::make_unique<MidiManager>(synthesizer.get(), midiListener.get());
    auto effectsChain = std::make_unique<ReorderableEffectsChain>(audioEngine->getSampleRate());
    auto midiEffectControl = std::make_unique<MidiEffectControl>(effectsChain.get(), midiManager.get());
    
    // Initialize the audio engine
    std::cout << "Initializing audio engine..." << std::endl;
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    std::cout << "Audio engine initialized successfully!" << std::endl;
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process synthesizer
        synthesizer->process(outputBuffer, numFrames);
        
        // Process effects chain
        effectsChain->process(outputBuffer, numFrames);
    });
    
    // List available MIDI input devices
    std::vector<std::string> midiDevices = midiManager->getMidiInputDevices();
    std::cout << "\nAvailable MIDI input devices:" << std::endl;
    
    if (midiDevices.empty()) {
        std::cout << "  No MIDI input devices found!" << std::endl;
        return 1;
    }
    
    for (size_t i = 0; i < midiDevices.size(); ++i) {
        std::cout << "  " << i << ": " << midiDevices[i] << std::endl;
    }
    
    // Select a MIDI device
    std::cout << "Select a MIDI input device (0-" << midiDevices.size() - 1 << "): ";
    int deviceIndex;
    std::cin >> deviceIndex;
    
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(midiDevices.size())) {
        std::cerr << "Invalid device index" << std::endl;
        return 1;
    }
    
    // Open the selected MIDI device
    if (!midiManager->openMidiInput(deviceIndex)) {
        std::cerr << "Failed to open MIDI input device" << std::endl;
        return 1;
    }
    
    std::cout << "MIDI input device opened: " << midiDevices[deviceIndex] << std::endl;
    
    // Set up the synthesizer
    synthesizer->setOscillatorType(OscillatorType::Sine);
    synthesizer->setParameter("volume", 0.8f);
    
    // Main menu state
    enum class MenuState {
        Main,
        Effects
    };
    
    MenuState currentMenu = MenuState::Main;
    
    // Display initial help
    printHelp();
    
    // Main loop
    std::cout << "\nReady! Play your MIDI keyboard... (Press 'q' to exit)" << std::endl;
    
    while (running) {
        // Handle keyboard input
        if (std::cin.peek() != EOF) {
            char key = std::cin.get();
            
            // Handle common commands in both menus
            if (key == 'q') {
                std::cout << "Exiting..." << std::endl;
                running = false;
                continue;
            }
            
            // Process key based on current menu
            if (currentMenu == MenuState::Main) {
                switch (key) {
                    case '1':
                        synthesizer->setOscillatorType(OscillatorType::Sine);
                        std::cout << "Oscillator: Sine" << std::endl;
                        break;
                    case '2':
                        synthesizer->setOscillatorType(OscillatorType::Square);
                        std::cout << "Oscillator: Square" << std::endl;
                        break;
                    case '3':
                        synthesizer->setOscillatorType(OscillatorType::Saw);
                        std::cout << "Oscillator: Saw" << std::endl;
                        break;
                    case '4':
                        synthesizer->setOscillatorType(OscillatorType::Triangle);
                        std::cout << "Oscillator: Triangle" << std::endl;
                        break;
                    case '5':
                        synthesizer->setOscillatorType(OscillatorType::Noise);
                        std::cout << "Oscillator: Noise" << std::endl;
                        break;
                    case 'e':
                        currentMenu = MenuState::Effects;
                        printEffectsMenu();
                        break;
                    case 'h':
                        printHelp();
                        break;
                }
            } else if (currentMenu == MenuState::Effects) {
                switch (key) {
                    case 'a': { // Add effect
                        // List available effects
                        std::cout << "Available effects:" << std::endl;
                        for (const auto& effectType : getAvailableEffects()) {
                            std::cout << "  " << effectType << std::endl;
                        }
                        
                        std::cout << "Enter effect type to add: ";
                        std::string effectType;
                        std::cin >> effectType;
                        
                        auto effect = effectsChain->createEffect(effectType);
                        if (effect) {
                            int index = effectsChain->addEffect(std::move(effect));
                            std::cout << "Added " << effectType << " effect at position " << index << std::endl;
                        } else {
                            std::cout << "Unknown effect type: " << effectType << std::endl;
                        }
                        break;
                    }
                    case 'r': { // Remove effect
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                            }
                            
                            std::cout << "Enter effect index to remove: ";
                            size_t index;
                            std::cin >> index;
                            
                            if (index < effectsChain->getNumEffects()) {
                                std::string effectType = effectsChain->getEffectType(index);
                                effectsChain->removeEffect(index);
                                std::cout << "Removed " << effectType << " effect" << std::endl;
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'c': // Clear all effects
                        effectsChain->clearEffects();
                        std::cout << "Cleared all effects" << std::endl;
                        break;
                    case 'p': { // Print effects chain
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects chain:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                                
                                // Print parameters
                                Effect* effect = effectsChain->getEffect(i);
                                if (effect) {
                                    printEffectParameters(effect);
                                }
                                std::cout << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'm': { // Modify effect parameter
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                            }
                            
                            std::cout << "Enter effect index: ";
                            size_t index;
                            std::cin >> index;
                            
                            if (index < effectsChain->getNumEffects()) {
                                Effect* effect = effectsChain->getEffect(index);
                                if (effect) {
                                    printEffectParameters(effect);
                                    
                                    std::cout << "Enter parameter name: ";
                                    std::string paramName;
                                    std::cin >> paramName;
                                    
                                    std::cout << "Enter value: ";
                                    float value;
                                    std::cin >> value;
                                    
                                    effect->setParameter(paramName, value);
                                    std::cout << "Set " << paramName << " to " << value << std::endl;
                                }
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'o': { // Reorder effects
                        if (effectsChain->getNumEffects() > 1) {
                            std::cout << "Current effects order:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                std::cout << "  " << i << ": " << effectType << std::endl;
                            }
                            
                            std::cout << "Enter effect index to move: ";
                            size_t fromIndex;
                            std::cin >> fromIndex;
                            
                            if (fromIndex < effectsChain->getNumEffects()) {
                                std::cout << "Enter new position: ";
                                size_t toIndex;
                                std::cin >> toIndex;
                                
                                if (toIndex < effectsChain->getNumEffects()) {
                                    effectsChain->moveEffect(fromIndex, toIndex);
                                    std::cout << "Moved effect from position " << fromIndex << " to " << toIndex << std::endl;
                                } else {
                                    std::cout << "Invalid target position" << std::endl;
                                }
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "Need at least two effects to reorder" << std::endl;
                        }
                        break;
                    }
                    case 't': { // Toggle effect on/off
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                            }
                            
                            std::cout << "Enter effect index to toggle: ";
                            size_t index;
                            std::cin >> index;
                            
                            if (index < effectsChain->getNumEffects()) {
                                bool currentState = effectsChain->isEffectEnabled(index);
                                effectsChain->setEffectEnabled(index, !currentState);
                                std::cout << effectsChain->getEffectType(index) << " effect " 
                                          << (!currentState ? "enabled" : "disabled") << std::endl;
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'l': { // MIDI learn for effect parameter
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                            }
                            
                            std::cout << "Enter effect index for MIDI learn: ";
                            size_t index;
                            std::cin >> index;
                            
                            if (index < effectsChain->getNumEffects()) {
                                Effect* effect = effectsChain->getEffect(index);
                                if (effect) {
                                    printEffectParameters(effect);
                                    
                                    std::cout << "Enter parameter name to assign to MIDI controller: ";
                                    std::string paramName;
                                    std::cin >> paramName;
                                    
                                    std::cout << "MIDI learn mode activated for " 
                                              << effectsChain->getEffectType(index) 
                                              << " parameter '" << paramName << "'" << std::endl;
                                    std::cout << "Move a MIDI controller knob/slider to assign..." << std::endl;
                                    
                                    // Start MIDI learn
                                    midiEffectControl->startMidiLearn(index, paramName);
                                    
                                    // Continue with main loop - learning will happen in the background
                                    // when a MIDI CC message is received
                                }
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'u': { // Unmap MIDI for effect parameter
                        if (effectsChain->getNumEffects() > 0) {
                            std::cout << "Current effects:" << std::endl;
                            for (size_t i = 0; i < effectsChain->getNumEffects(); ++i) {
                                std::string effectType = effectsChain->getEffectType(i);
                                bool enabled = effectsChain->isEffectEnabled(i);
                                std::cout << "  " << i << ": " << effectType << (enabled ? " (enabled)" : " (disabled)") << std::endl;
                            }
                            
                            std::cout << "Enter effect index: ";
                            size_t index;
                            std::cin >> index;
                            
                            if (index < effectsChain->getNumEffects()) {
                                Effect* effect = effectsChain->getEffect(index);
                                if (effect) {
                                    printEffectParameters(effect);
                                    
                                    std::cout << "Enter parameter name to unmap from MIDI: ";
                                    std::string paramName;
                                    std::cin >> paramName;
                                    
                                    if (midiEffectControl->unmapEffectParameter(index, paramName)) {
                                        std::cout << "Parameter unmapped from MIDI control" << std::endl;
                                    } else {
                                        std::cout << "Parameter was not mapped to MIDI" << std::endl;
                                    }
                                }
                            } else {
                                std::cout << "Invalid effect index" << std::endl;
                            }
                        } else {
                            std::cout << "No effects in the chain" << std::endl;
                        }
                        break;
                    }
                    case 'v': { // View MIDI mappings
                        auto mappings = midiEffectControl->getMidiMappings();
                        
                        if (mappings.empty()) {
                            std::cout << "No MIDI mappings exist" << std::endl;
                        } else {
                            std::cout << "Current MIDI mappings:" << std::endl;
                            
                            for (const auto& mapping : mappings) {
                                // Parse the parameter ID to get effect index and parameter name
                                auto result = MidiEffectControl::parseParameterId(mapping.first);
                                if (result.first >= 0 && static_cast<size_t>(result.first) < effectsChain->getNumEffects()) {
                                    std::string effectType = effectsChain->getEffectType(result.first);
                                    std::cout << "  " << effectType << " #" << result.first 
                                              << " parameter '" << result.second 
                                              << "' mapped to Channel " << mapping.second.first 
                                              << ", CC " << mapping.second.second << std::endl;
                                }
                            }
                        }
                        break;
                    }
                    case 'b': // Back to main menu
                        currentMenu = MenuState::Main;
                        printHelp();
                        break;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean up resources
    std::cout << "Shutting down..." << std::endl;
    midiManager->closeMidiInput();
    audioEngine->shutdown();
    
    return 0;
}