#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <vector>
#include <string>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"
#include "../include/ui/presets/PresetManager.h"

using namespace AIMusicHardware;

// Signal handler for clean shutdown
bool running = true;
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

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
    std::cout << "\nPreset Manager Demo Controls:" << std::endl;
    std::cout << "  1-5: Change oscillator type (Sine, Square, Saw, Triangle, Noise)" << std::endl;
    std::cout << "  l: List all available presets" << std::endl;
    std::cout << "  n: Load next preset" << std::endl;
    std::cout << "  p: Load previous preset" << std::endl;
    std::cout << "  s: Save current preset" << std::endl;
    std::cout << "  i: Show current preset info" << std::endl;
    std::cout << "  h: Show this help message" << std::endl;
    std::cout << "  q: Quit application" << std::endl;
}

// Helper function to create a simple preset
void createDefaultPresets(PresetManager* presetManager) {
    std::cout << "Creating default presets..." << std::endl;
    
    // Create directories
    std::filesystem::create_directories(PresetManager::getFactoryPresetsDirectory() + "/Bass");
    std::filesystem::create_directories(PresetManager::getFactoryPresetsDirectory() + "/Lead");
    std::filesystem::create_directories(PresetManager::getFactoryPresetsDirectory() + "/Pad");
    std::filesystem::create_directories(PresetManager::getUserPresetsDirectory());
    
    // Create a basic sine preset
    presetManager->savePreset(
        PresetManager::getFactoryPresetsDirectory() + "/Bass/basic_sine.preset",
        "Basic Sine",
        "AIMusicHardware",
        "Bass",
        "A simple sine wave bass preset"
    );
    
    // Create a square lead preset
    presetManager->savePreset(
        PresetManager::getFactoryPresetsDirectory() + "/Lead/square_lead.preset",
        "Square Lead",
        "AIMusicHardware",
        "Lead",
        "A classic square wave lead sound"
    );
    
    // Create a pad preset
    presetManager->savePreset(
        PresetManager::getFactoryPresetsDirectory() + "/Pad/soft_pad.preset",
        "Soft Pad",
        "AIMusicHardware",
        "Pad",
        "A smooth atmospheric pad"
    );
    
    std::cout << "Default presets created." << std::endl;
}

// Helper function to save a preset with user input
void savePresetWithInput(PresetManager* presetManager) {
    std::string name, author, category, description;
    
    std::cout << "Enter preset name: ";
    std::getline(std::cin, name);
    
    std::cout << "Enter author name: ";
    std::getline(std::cin, author);
    
    std::cout << "Enter category: ";
    std::getline(std::cin, category);
    
    std::cout << "Enter description: ";
    std::getline(std::cin, description);
    
    // Create the preset path
    std::string directory = PresetManager::getUserPresetsDirectory();
    if (!category.empty()) {
        directory += "/" + category;
        std::filesystem::create_directories(directory);
    }
    
    std::string filename = name;
    // Replace spaces with underscores for the filename
    std::replace(filename.begin(), filename.end(), ' ', '_');
    std::string path = directory + "/" + filename + ".preset";
    
    if (presetManager->savePreset(path, name, author, category, description)) {
        std::cout << "Preset saved to: " << path << std::endl;
    } else {
        std::cout << "Failed to save preset!" << std::endl;
    }
}

int main() {
    // Set up signal handling for clean shutdown
    std::signal(SIGINT, signalHandler);
    
    std::cout << "AI Music Hardware - Simplified Preset Manager Demo" << std::endl;
    std::cout << "===============================================\n" << std::endl;
    
    // Create all the necessary components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto midiListener = std::make_unique<MidiListener>();
    auto midiManager = std::make_unique<MidiManager>(synthesizer.get(), midiListener.get());
    
    // Create preset manager
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    
    // Initialize the audio engine
    std::cout << "Initializing audio engine..." << std::endl;
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process synthesizer
        synthesizer->process(outputBuffer, numFrames);
    });
    
    // Create default presets if none exist
    if (presetManager->getAllPresets().empty()) {
        createDefaultPresets(presetManager.get());
    }
    
    // Display main help
    printHelp();
    
    // Clean up std::cin buffer
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    // Main loop
    while (running) {
        // Handle keyboard input
        if (std::cin.peek() != EOF) {
            char key = std::cin.get();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            
            // Handle common commands
            switch (key) {
                case 'q':
                    std::cout << "Exiting..." << std::endl;
                    running = false;
                    break;
                    
                case 'h':
                    printHelp();
                    break;
                    
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
                    
                case 'l': {
                    std::cout << "Available Presets:" << std::endl;
                    auto presets = presetManager->getAllPresets();
                    if (presets.empty()) {
                        std::cout << "  No presets found" << std::endl;
                    } else {
                        for (const auto& preset : presets) {
                            std::cout << "  " << preset << std::endl;
                        }
                    }
                    break;
                }
                
                case 'n':
                    if (presetManager->loadNextPreset()) {
                        std::cout << "Loaded preset: " << presetManager->getCurrentPresetName() << std::endl;
                    } else {
                        std::cout << "No next preset available" << std::endl;
                    }
                    break;
                    
                case 'p':
                    if (presetManager->loadPreviousPreset()) {
                        std::cout << "Loaded preset: " << presetManager->getCurrentPresetName() << std::endl;
                    } else {
                        std::cout << "No previous preset available" << std::endl;
                    }
                    break;
                    
                case 's':
                    savePresetWithInput(presetManager.get());
                    break;
                    
                case 'i': {
                    std::cout << "Current Preset Info:" << std::endl;
                    std::cout << "  Name: " << presetManager->getCurrentPresetName() << std::endl;
                    std::cout << "  Author: " << presetManager->getCurrentPresetAuthor() << std::endl;
                    std::cout << "  Category: " << presetManager->getCurrentPresetCategory() << std::endl;
                    std::cout << "  Description: " << presetManager->getCurrentPresetDescription() << std::endl;
                    std::cout << "  Path: " << presetManager->getCurrentPresetPath() << std::endl;
                    
                    // Show synthesizer parameters
                    std::cout << "Parameter values:" << std::endl;
                    auto params = synthesizer->getAllParameters();
                    for (const auto& [paramId, value] : params) {
                        std::cout << "  " << paramId << ": " << value << std::endl;
                    }
                    break;
                }
                
                default:
                    break;
            }
        }
        
        // Small delay to prevent high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Clean up resources
    std::cout << "Shutting down..." << std::endl;
    audioEngine->shutdown();
    
    return 0;
}