#include <iostream>
#include <vector>
#include <string>
#include "include/midi/MidiInterface.h"

using namespace AIMusicHardware;

int main() {
    std::cout << "=== MIDI Device Detection Test ===" << std::endl;
    
    // Create MIDI input
    MidiInput midiInput;
    
    // Get list of devices
    auto devices = midiInput.getDevices();
    
    std::cout << "\nFound " << devices.size() << " MIDI input device(s):" << std::endl;
    
    if (devices.empty()) {
        std::cout << "No MIDI input devices detected!" << std::endl;
        std::cout << "\nTroubleshooting tips:" << std::endl;
        std::cout << "1. Make sure the device is connected via USB" << std::endl;
        std::cout << "2. Check if the device appears in Audio MIDI Setup" << std::endl;
        std::cout << "3. Try disconnecting and reconnecting the device" << std::endl;
        std::cout << "4. Some devices need to be in a specific mode" << std::endl;
    } else {
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << "  [" << i << "] " << devices[i] << std::endl;
        }
        
        // Try to open each device
        std::cout << "\nTesting device connections:" << std::endl;
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << "Opening device " << i << ": ";
            if (midiInput.openDevice(i)) {
                std::cout << "SUCCESS" << std::endl;
                midiInput.closeDevice();
            } else {
                std::cout << "FAILED" << std::endl;
            }
        }
    }
    
    // Also check MIDI output devices
    std::cout << "\n=== MIDI Output Devices ===" << std::endl;
    MidiOutput midiOutput;
    auto outputDevices = midiOutput.getDevices();
    
    std::cout << "Found " << outputDevices.size() << " MIDI output device(s):" << std::endl;
    for (size_t i = 0; i < outputDevices.size(); ++i) {
        std::cout << "  [" << i << "] " << outputDevices[i] << std::endl;
    }
    
    return 0;
}