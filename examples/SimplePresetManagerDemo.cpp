#include <iostream>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>
#include <map>

#include "../include/audio/Synthesizer.h"
#include "../include/ui/presets/PresetManager.h"

using namespace AIMusicHardware;

// Helper function to display help information
void printHelp() {
    std::cout << "\nSimple Preset Manager Demo Controls:" << std::endl;
    std::cout << "  l: List all available presets" << std::endl;
    std::cout << "  c: Create test presets" << std::endl;
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
    std::cout << "AI Music Hardware - Simple Preset Manager Demo" << std::endl;
    std::cout << "============================================\n" << std::endl;
    
    // Create a synthesizer and preset manager
    auto synthesizer = std::make_unique<Synthesizer>();
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    
    // Display help
    printHelp();
    
    bool running = true;
    while (running) {
        // Get user input
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        char command = input[0];
        switch (command) {
            case 'q':
                std::cout << "Exiting..." << std::endl;
                running = false;
                break;
                
            case 'h':
                printHelp();
                break;
                
            case 'c':
                createDefaultPresets(presetManager.get());
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
            
            case 's':
                savePresetWithInput(presetManager.get());
                break;
                
            case 'i': {
                // Show current preset information
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
                std::cout << "Unknown command. Type 'h' for help." << std::endl;
                break;
        }
    }
    
    return 0;
}