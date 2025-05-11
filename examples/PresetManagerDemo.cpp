#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetSelector.h"
#include "../include/ui/presets/PresetSaveDialog.h"
#include "../include/ui/DisplayManager.h"
#include "../include/ui/UIContext.h"

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

// Custom DisplayManager implementation for console-based testing
class TestDisplayManager : public DisplayManager {
public:
    TestDisplayManager() {
        std::cout << "Test Display Manager initialized" << std::endl;
        // Create a basic buffer for display operations
        width_ = 800;
        height_ = 600;
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    
    void beginFrame() override {
        // std::cout << "Begin frame" << std::endl;
    }
    
    void endFrame() override {
        // std::cout << "End frame" << std::endl;
    }
    
    void clear(const Color& color) override {
        // std::cout << "Clear screen with color: " << color.toString() << std::endl;
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        // std::cout << "Draw line from (" << x1 << "," << y1 << ") to (" << x2 << "," << y2 << ")" << std::endl;
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        // std::cout << "Draw rect at (" << x << "," << y << ") with size (" << width << "," << height << ")" << std::endl;
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        // std::cout << "Fill rect at (" << x << "," << y << ") with size (" << width << "," << height << ")" << std::endl;
    }
    
    void drawEllipse(int x, int y, int radiusX, int radiusY, const Color& color) override {
        // std::cout << "Draw ellipse at (" << x << "," << y << ") with radius (" << radiusX << "," << radiusY << ")" << std::endl;
    }
    
    void fillEllipse(int x, int y, int radiusX, int radiusY, const Color& color) override {
        // std::cout << "Fill ellipse at (" << x << "," << y << ") with radius (" << radiusX << "," << radiusY << ")" << std::endl;
    }
    
    void drawText(int x, int y, const std::string& text, const Color& color) override {
        // std::cout << "Draw text '" << text << "' at (" << x << "," << y << ")" << std::endl;
    }
    
private:
    int width_;
    int height_;
};

// Helper function to display help information
void printHelp() {
    std::cout << "\nPreset Manager Demo Controls:" << std::endl;
    std::cout << "  1-5: Change oscillator type (Sine, Square, Saw, Triangle, Noise)" << std::endl;
    std::cout << "  l: List all available presets" << std::endl;
    std::cout << "  n: Load next preset" << std::endl;
    std::cout << "  p: Load previous preset" << std::endl;
    std::cout << "  s: Save current preset" << std::endl;
    std::cout << "  c: Cancel save dialog (when shown)" << std::endl;
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

int main() {
    // Set up signal handling for clean shutdown
    std::signal(SIGINT, signalHandler);
    
    std::cout << "AI Music Hardware - Preset Manager Demo" << std::endl;
    std::cout << "=======================================\n" << std::endl;
    
    // Create all the necessary components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto midiListener = std::make_unique<MidiListener>();
    auto midiManager = std::make_unique<MidiManager>(synthesizer.get(), midiListener.get());
    
    // Create preset manager
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    
    // Create UI context and components
    auto displayManager = std::make_unique<TestDisplayManager>();
    auto uiContext = std::make_unique<UIContext>(displayManager.get());
    
    auto presetSelector = std::make_unique<PresetSelector>(presetManager.get());
    auto presetSaveDialog = std::make_unique<PresetSaveDialog>(presetManager.get());
    
    // Set up the preset selector
    presetSelector->setOnSaveRequested([&]() {
        std::cout << "Save requested" << std::endl;
        presetSaveDialog->show();
    });
    
    presetSelector->setOnPresetChanged([](const std::string& path) {
        std::cout << "Preset changed: " << path << std::endl;
    });
    
    // Set up the save dialog
    presetSaveDialog->setOnSaveComplete([]() {
        std::cout << "Save completed" << std::endl;
    });
    
    presetSaveDialog->setOnCancel([]() {
        std::cout << "Save canceled" << std::endl;
    });
    
    // Add UI components to context
    uiContext->addComponent(presetSelector.get());
    uiContext->addComponent(presetSaveDialog.get());
    
    // Initially hide the save dialog
    presetSaveDialog->hide();
    
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
    
    // Main loop
    while (running) {
        // Simulate UI rendering (in a real app, this would be graphical)
        displayManager->beginFrame();
        uiContext->render();
        displayManager->endFrame();
        
        // Handle keyboard input
        if (std::cin.peek() != EOF) {
            char key = std::cin.get();
            
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
                    std::cout << "Save dialog shown" << std::endl;
                    presetSaveDialog->show();
                    break;
                    
                case 'c':
                    if (presetSaveDialog->isVisible()) {
                        presetSaveDialog->hide();
                        std::cout << "Save dialog hidden" << std::endl;
                    }
                    break;
                    
                case 'i': {
                    std::cout << "Current Preset Info:" << std::endl;
                    std::cout << "  Name: " << presetManager->getCurrentPresetName() << std::endl;
                    std::cout << "  Author: " << presetManager->getCurrentPresetAuthor() << std::endl;
                    std::cout << "  Category: " << presetManager->getCurrentPresetCategory() << std::endl;
                    std::cout << "  Description: " << presetManager->getCurrentPresetDescription() << std::endl;
                    std::cout << "  Path: " << presetManager->getCurrentPresetPath() << std::endl;
                    break;
                }
                
                default:
                    // Send key to UI context for text input
                    InputEvent event;
                    event.type = InputEvent::Type::KeyPress;
                    event.character = key;
                    uiContext->handleInput(event);
                    break;
            }
        }
        
        // Simulate 60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Clean up resources
    std::cout << "Shutting down..." << std::endl;
    audioEngine->shutdown();
    
    return 0;
}