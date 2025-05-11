#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"

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

int main() {
    // Set up signal handling for clean shutdown
    std::signal(SIGINT, signalHandler);
    
    std::cout << "AI Music Hardware - MIDI Keyboard Demo" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Create all the necessary components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    auto midiListener = std::make_unique<MidiListener>();
    auto midiManager = std::make_unique<MidiManager>(synthesizer.get(), midiListener.get());
    
    // Initialize the audio engine
    std::cout << "Initializing audio engine..." << std::endl;
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    std::cout << "Audio engine initialized successfully!" << std::endl;
    
    // Set up audio callback
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        // Process synthesizer directly
        synthesizer->process(outputBuffer, numFrames);
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
    
    // Main loop
    std::cout << "\nReady! Play your MIDI keyboard... (Press Ctrl+C to exit)" << std::endl;
    std::cout << "Keyboard controls:" << std::endl;
    std::cout << "  1-5: Change oscillator type (Sine, Square, Saw, Triangle, Noise)" << std::endl;
    
    while (running) {
        // Handle keyboard input for changing synth parameters
        if (std::cin.peek() != EOF) {
            char key = std::cin.get();
            
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