#include <iostream>
#include <vector>
#include <string>
#include <RtMidi.h>

int main() {
    std::cout << "=== Direct RtMidi Device Detection Test ===" << std::endl;
    
    try {
        // Test MIDI Input
        RtMidiIn midiIn;
        unsigned int nPorts = midiIn.getPortCount();
        
        std::cout << "\nMIDI Input Ports: " << nPorts << std::endl;
        
        if (nPorts == 0) {
            std::cout << "No MIDI input ports available!" << std::endl;
        } else {
            for (unsigned int i = 0; i < nPorts; i++) {
                std::string portName = midiIn.getPortName(i);
                std::cout << "  Input Port #" << i << ": " << portName << std::endl;
            }
        }
        
        // Test MIDI Output
        RtMidiOut midiOut;
        nPorts = midiOut.getPortCount();
        
        std::cout << "\nMIDI Output Ports: " << nPorts << std::endl;
        
        if (nPorts == 0) {
            std::cout << "No MIDI output ports available!" << std::endl;
        } else {
            for (unsigned int i = 0; i < nPorts; i++) {
                std::string portName = midiOut.getPortName(i);
                std::cout << "  Output Port #" << i << ": " << portName << std::endl;
            }
        }
        
    } catch (RtMidiError &error) {
        error.printMessage();
        return 1;
    }
    
    std::cout << "\nNote: If Oxi One is not showing up, try:" << std::endl;
    std::cout << "1. Open Audio MIDI Setup on macOS" << std::endl;
    std::cout << "2. Click 'Window' -> 'Show MIDI Studio'" << std::endl;
    std::cout << "3. Check if Oxi One appears there" << std::endl;
    std::cout << "4. If it's grayed out, double-click to configure it" << std::endl;
    std::cout << "5. Make sure the device is in the correct mode (not just USB power mode)" << std::endl;
    
    return 0;
}