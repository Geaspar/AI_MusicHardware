#include "../include/midi/MidiInterface.h"
#include "../include/midi/MidiManager.h"
#include "../include/audio/Synthesizer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <csignal>

using namespace AIMusicHardware;

bool running = true;

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Custom MIDI input callback to display incoming messages
class MidiMonitor : public MidiInputCallback {
public:
    void handleIncomingMidiMessage(const MidiMessage& message) override {
        std::cout << "MIDI Message: ";
        
        switch (message.type) {
            case MidiMessage::Type::NoteOn:
                std::cout << "Note On, Channel: " << message.channel 
                          << ", Note: " << message.data1 
                          << ", Velocity: " << message.data2;
                break;
                
            case MidiMessage::Type::NoteOff:
                std::cout << "Note Off, Channel: " << message.channel 
                          << ", Note: " << message.data1;
                break;
                
            case MidiMessage::Type::ControlChange:
                std::cout << "Control Change, Channel: " << message.channel 
                          << ", Controller: " << message.data1 
                          << ", Value: " << message.data2;
                break;
                
            case MidiMessage::Type::PitchBend:
                std::cout << "Pitch Bend, Channel: " << message.channel 
                          << ", Value: " << ((message.data2 << 7) | message.data1);
                break;
                
            case MidiMessage::Type::AfterTouch:
                std::cout << "Aftertouch, Channel: " << message.channel 
                          << ", Note: " << message.data1 
                          << ", Pressure: " << message.data2;
                break;
                
            case MidiMessage::Type::ChannelPressure:
                std::cout << "Channel Pressure, Channel: " << message.channel 
                          << ", Pressure: " << message.data1;
                break;
                
            case MidiMessage::Type::ProgramChange:
                std::cout << "Program Change, Channel: " << message.channel 
                          << ", Program: " << message.data1;
                break;
                
            default:
                std::cout << "Other Message Type";
                break;
        }
        
        std::cout << ", Time: " << std::fixed << std::setprecision(2) << message.timestamp << "s" << std::endl;
    }
};

// Listener for MIDI manager events
class MidiListener : public MidiManager::Listener {
public:
    void parameterChangedViaMidi(const std::string& paramId, float value) override {
        std::cout << "Parameter changed via MIDI: " << paramId << " = " << value << std::endl;
    }
    
    void pitchBendChanged(int channel, float value) override {
        std::cout << "Pitch bend changed: Channel " << channel << ", Value: " << value << std::endl;
    }
    
    void modWheelChanged(int channel, float value) override {
        std::cout << "Mod wheel changed: Channel " << channel << ", Value: " << value << std::endl;
    }
    
    void afterTouchChanged(int channel, float value) override {
        std::cout << "Aftertouch changed: Channel " << channel << ", Value: " << value << std::endl;
    }
};

int main() {
    std::cout << "MIDI Test Application" << std::endl;
    std::cout << "====================" << std::endl;
    
    // Set up signal handling for clean shutdown
    std::signal(SIGINT, signalHandler);
    
    // Create a MIDI input to monitor all incoming messages
    MidiMonitor monitor;
    MidiInput midiInput;
    
    // List available MIDI input devices
    std::vector<std::string> inputDevices = midiInput.getDevices();
    std::cout << "Available MIDI input devices:" << std::endl;
    
    if (inputDevices.empty()) {
        std::cout << "  No MIDI input devices found" << std::endl;
    } else {
        for (size_t i = 0; i < inputDevices.size(); ++i) {
            std::cout << "  " << i << ": " << inputDevices[i] << std::endl;
        }
        
        // Prompt user to select a device
        std::cout << "Select an input device (0-" << inputDevices.size() - 1 << "): ";
        int deviceIndex;
        std::cin >> deviceIndex;
        
        if (deviceIndex >= 0 && deviceIndex < static_cast<int>(inputDevices.size())) {
            if (midiInput.openDevice(deviceIndex)) {
                midiInput.setCallback(&monitor);
                std::cout << "MIDI input device opened: " << inputDevices[deviceIndex] << std::endl;
            } else {
                std::cout << "Failed to open MIDI input device" << std::endl;
                return 1;
            }
        } else {
            std::cout << "Invalid device index" << std::endl;
            return 1;
        }
    }
    
    // Create a synthesizer and MIDI manager for more advanced testing
    Synthesizer synth;
    MidiListener listener;
    MidiManager midiManager(&synth, &listener);
    
    // List available output devices
    std::vector<std::string> outputDevices = midiManager.getMidiOutputDevices();
    std::cout << "Available MIDI output devices:" << std::endl;
    
    if (outputDevices.empty()) {
        std::cout << "  No MIDI output devices found" << std::endl;
    } else {
        for (size_t i = 0; i < outputDevices.size(); ++i) {
            std::cout << "  " << i << ": " << outputDevices[i] << std::endl;
        }
    }
    
    // Test MIDI learn functionality
    std::cout << "\nMIDI Learn Test" << std::endl;
    std::cout << "Move a controller on your MIDI device to assign it to 'test_param'..." << std::endl;
    midiManager.armMidiLearn("test_param");
    
    // Main loop
    std::cout << "\nMIDI Monitor Active (Press Ctrl+C to exit)" << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Clean up
    midiInput.closeDevice();
    std::cout << "MIDI monitor closed" << std::endl;
    
    return 0;
}