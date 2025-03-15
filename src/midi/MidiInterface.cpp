#include "../../include/midi/MidiInterface.h"
#include <stdexcept>
#include <cassert>
#include <mutex>
#include <iostream>

namespace AIMusicHardware {

// MidiInput implementation
MidiInput::MidiInput() : pimpl_(nullptr) {
}

MidiInput::~MidiInput() {
    // Virtual destructor now matches the header
}

std::vector<std::string> MidiInput::getDevices() {
    // Stub implementation
    return {};
}

bool MidiInput::openDevice(int deviceIndex) {
    // Stub implementation
    return false;
}

void MidiInput::closeDevice() {
    // Stub implementation
}

bool MidiInput::isDeviceOpen() const {
    // Stub implementation
    return false;
}

void MidiInput::setCallback(MidiInputCallback* callback) {
    // Stub implementation
}

// MidiOutput implementation
MidiOutput::MidiOutput() : pimpl_(nullptr) {
}

MidiOutput::~MidiOutput() {
    // Virtual destructor implied by design
}

std::vector<std::string> MidiOutput::getDevices() {
    // Stub implementation
    return {};
}

bool MidiOutput::openDevice(int deviceIndex) {
    // Stub implementation
    return false;
}

void MidiOutput::closeDevice() {
    // Stub implementation
}

bool MidiOutput::isDeviceOpen() const {
    // Stub implementation
    return false;
}

void MidiOutput::sendNoteOn(int channel, int noteNumber, int velocity) {
    // Range validation
    if (channel < 1 || channel > 16 || noteNumber < 0 || noteNumber > 127 || velocity < 0 || velocity > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendNoteOn");
    }
    
    // Stub implementation - will be expanded with actual MIDI output
}

void MidiOutput::sendNoteOff(int channel, int noteNumber) {
    // Range validation
    if (channel < 1 || channel > 16 || noteNumber < 0 || noteNumber > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendNoteOff");
    }
    
    // Stub implementation - will be expanded with actual MIDI output
}

void MidiOutput::sendControlChange(int channel, int controller, int value) {
    // Range validation
    if (channel < 1 || channel > 16 || controller < 0 || controller > 127 || value < 0 || value > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendControlChange");
    }
    
    // Stub implementation
}

void MidiOutput::sendProgramChange(int channel, int programNumber) {
    // Range validation
    if (channel < 1 || channel > 16 || programNumber < 0 || programNumber > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendProgramChange");
    }
    
    // Stub implementation
}

void MidiOutput::sendPitchBend(int channel, int value) {
    // Range validation
    if (channel < 1 || channel > 16 || value < 0 || value > 16383) {
        throw std::out_of_range("Invalid MIDI parameters in sendPitchBend");
    }
    
    // Stub implementation
}

void MidiOutput::sendMessage(const MidiMessage& message) {
    // Stub implementation
}

// MidiHandler implementation
MidiHandler::MidiHandler() 
    : noteOnCallback_(nullptr), 
      noteOffCallback_(nullptr),
      ccCallback_(nullptr),
      genericCallback_(nullptr) {
}

MidiHandler::~MidiHandler() {
    // Virtual destructor implementation
}

void MidiHandler::handleIncomingMidiMessage(const MidiMessage& message) {
    static std::mutex callbackMutex;
    
    // Thread-safe callback handling with mutex
    std::lock_guard<std::mutex> lock(callbackMutex);
    
    // First check generic callback
    if (genericCallback_) {
        try {
            genericCallback_(message);
        } catch (const std::exception& e) {
            std::cerr << "Error in genericCallback_: " << e.what() << std::endl;
        }
    }
    
    // Then check specific callbacks based on message type
    try {
        switch (message.type) {
            case MidiMessage::Type::NoteOn:
                if (noteOnCallback_) {
                    noteOnCallback_(message.channel, message.data1, message.data2);
                }
                break;
                
            case MidiMessage::Type::NoteOff:
                if (noteOffCallback_) {
                    noteOffCallback_(message.channel, message.data1);
                }
                break;
                
            case MidiMessage::Type::ControlChange:
                if (ccCallback_) {
                    ccCallback_(message.channel, message.data1, message.data2);
                }
                break;
                
            default:
                // Other message types handled by the generic callback
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing MIDI message: " << e.what() << std::endl;
    }
}

void MidiHandler::setNoteOnCallback(NoteOnCallback callback) {
    static std::mutex callbackMutex;
    std::lock_guard<std::mutex> lock(callbackMutex);
    noteOnCallback_ = callback;
}

void MidiHandler::setNoteOffCallback(NoteOffCallback callback) {
    static std::mutex callbackMutex;
    std::lock_guard<std::mutex> lock(callbackMutex);
    noteOffCallback_ = callback;
}

void MidiHandler::setControlChangeCallback(ControlChangeCallback callback) {
    static std::mutex callbackMutex;
    std::lock_guard<std::mutex> lock(callbackMutex);
    ccCallback_ = callback;
}

void MidiHandler::setGenericCallback(GenericMidiCallback callback) {
    static std::mutex callbackMutex;
    std::lock_guard<std::mutex> lock(callbackMutex);
    genericCallback_ = callback;
}

} // namespace AIMusicHardware