#include "../../include/midi/MidiInterface.h"
#include <stdexcept>
#include <cassert>
#include <mutex>
#include <iostream>

#ifdef HAVE_RTMIDI
#include "RtMidi.h"
#endif

namespace AIMusicHardware {

// Internal helper functions for thread-safe callbacks
namespace {
    std::mutex callbackMutex;
}

#ifdef HAVE_RTMIDI
// RtMidi callback function
void rtMidiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
    if (!message || !userData)
        return;
    
    auto* callback = static_cast<MidiInputCallback*>(userData);
    
    // Create a MidiMessage from the RtMidi data
    MidiMessage midiMessage;
    midiMessage.timestamp = timeStamp;
    
    if (message->size() >= 1) {
        unsigned char status = message->at(0);
        unsigned char type = status & 0xF0;
        unsigned char channel = status & 0x0F;
        
        // Convert to 1-based channel numbers for consistency with MIDI standard
        midiMessage.channel = channel + 1;
        
        // Parse message type
        switch (type) {
            case 0x80: // Note Off
                midiMessage.type = MidiMessage::Type::NoteOff;
                break;
            case 0x90: // Note On
                // If velocity is 0, treat as note off
                if (message->size() >= 3 && message->at(2) == 0)
                    midiMessage.type = MidiMessage::Type::NoteOff;
                else
                    midiMessage.type = MidiMessage::Type::NoteOn;
                break;
            case 0xA0: // Aftertouch
                midiMessage.type = MidiMessage::Type::AfterTouch;
                break;
            case 0xB0: // Control Change
                midiMessage.type = MidiMessage::Type::ControlChange;
                break;
            case 0xC0: // Program Change
                midiMessage.type = MidiMessage::Type::ProgramChange;
                break;
            case 0xD0: // Channel Pressure
                midiMessage.type = MidiMessage::Type::ChannelPressure;
                break;
            case 0xE0: // Pitch Bend
                midiMessage.type = MidiMessage::Type::PitchBend;
                break;
            default:
                midiMessage.type = MidiMessage::Type::SystemMessage;
                break;
        }
        
        // Parse data bytes
        if (message->size() >= 2)
            midiMessage.data1 = message->at(1);
        
        if (message->size() >= 3)
            midiMessage.data2 = message->at(2);
    }
    
    // Thread-safe callback handling
    std::lock_guard<std::mutex> lock(callbackMutex);
    callback->handleIncomingMidiMessage(midiMessage);
}
#endif

// MidiInput implementation with RtMidi
class MidiInput::Impl {
public:
    Impl() : rtMidiIn_(nullptr), callback_(nullptr), isOpen_(false) {
#ifdef HAVE_RTMIDI
        try {
            rtMidiIn_ = std::make_unique<RtMidiIn>();
        } catch (RtMidiError& error) {
            std::cerr << "RtMidiIn initialization error: " << error.getMessage() << std::endl;
        }
#endif
    }
    
    ~Impl() {
        closeDevice();
    }
    
    std::vector<std::string> getDevices() {
        std::vector<std::string> devices;
#ifdef HAVE_RTMIDI
        if (rtMidiIn_) {
            try {
                unsigned int portCount = rtMidiIn_->getPortCount();
                for (unsigned int i = 0; i < portCount; ++i) {
                    devices.push_back(rtMidiIn_->getPortName(i));
                }
            } catch (RtMidiError& error) {
                std::cerr << "Error getting MIDI input devices: " << error.getMessage() << std::endl;
            }
        }
#endif
        return devices;
    }
    
    bool openDevice(int deviceIndex) {
        closeDevice(); // Close any existing connection first
        
#ifdef HAVE_RTMIDI
        if (rtMidiIn_) {
            try {
                unsigned int portCount = rtMidiIn_->getPortCount();
                if (deviceIndex >= 0 && deviceIndex < static_cast<int>(portCount)) {
                    rtMidiIn_->openPort(deviceIndex);
                    isOpen_ = true;
                    
                    // Set callback if we have one
                    if (callback_) {
                        rtMidiIn_->setCallback(rtMidiCallback, callback_);
                        rtMidiIn_->ignoreTypes(false, false, false); // Accept all message types
                    }
                    
                    return true;
                }
            } catch (RtMidiError& error) {
                std::cerr << "Error opening MIDI input device: " << error.getMessage() << std::endl;
            }
        }
#endif
        return false;
    }
    
    void closeDevice() {
#ifdef HAVE_RTMIDI
        if (rtMidiIn_ && isOpen_) {
            try {
                rtMidiIn_->closePort();
                isOpen_ = false;
            } catch (RtMidiError& error) {
                std::cerr << "Error closing MIDI input device: " << error.getMessage() << std::endl;
            }
        }
#endif
    }
    
    bool isDeviceOpen() const {
        return isOpen_;
    }
    
    void setCallback(MidiInputCallback* callback) {
        callback_ = callback;
        
#ifdef HAVE_RTMIDI
        if (rtMidiIn_ && isOpen_ && callback_) {
            try {
                rtMidiIn_->setCallback(rtMidiCallback, callback_);
                rtMidiIn_->ignoreTypes(false, false, false); // Accept all message types
            } catch (RtMidiError& error) {
                std::cerr << "Error setting MIDI callback: " << error.getMessage() << std::endl;
            }
        }
#endif
    }
    
private:
#ifdef HAVE_RTMIDI
    std::unique_ptr<RtMidiIn> rtMidiIn_;
#endif
    MidiInputCallback* callback_;
    bool isOpen_;
};

MidiInput::MidiInput() : pimpl_(std::make_unique<Impl>()) {
}

MidiInput::~MidiInput() {
    // Unique_ptr will clean up the pimpl
}

std::vector<std::string> MidiInput::getDevices() {
#ifdef HAVE_RTMIDI
    try {
        RtMidiIn rtMidiIn;
        std::vector<std::string> devices;
        unsigned int portCount = rtMidiIn.getPortCount();
        for (unsigned int i = 0; i < portCount; ++i) {
            devices.push_back(rtMidiIn.getPortName(i));
        }
        return devices;
    } catch (RtMidiError& error) {
        std::cerr << "Error getting MIDI input devices: " << error.getMessage() << std::endl;
    }
#endif
    return {};
}

bool MidiInput::openDevice(int deviceIndex) {
    return pimpl_->openDevice(deviceIndex);
}

void MidiInput::closeDevice() {
    pimpl_->closeDevice();
}

bool MidiInput::isDeviceOpen() const {
    return pimpl_->isDeviceOpen();
}

void MidiInput::setCallback(MidiInputCallback* callback) {
    pimpl_->setCallback(callback);
}

// MidiOutput implementation with RtMidi
class MidiOutput::Impl {
public:
    Impl() : rtMidiOut_(nullptr), isOpen_(false) {
#ifdef HAVE_RTMIDI
        try {
            rtMidiOut_ = std::make_unique<RtMidiOut>();
        } catch (RtMidiError& error) {
            std::cerr << "RtMidiOut initialization error: " << error.getMessage() << std::endl;
        }
#endif
    }
    
    ~Impl() {
        closeDevice();
    }
    
    std::vector<std::string> getDevices() {
        std::vector<std::string> devices;
#ifdef HAVE_RTMIDI
        if (rtMidiOut_) {
            try {
                unsigned int portCount = rtMidiOut_->getPortCount();
                for (unsigned int i = 0; i < portCount; ++i) {
                    devices.push_back(rtMidiOut_->getPortName(i));
                }
            } catch (RtMidiError& error) {
                std::cerr << "Error getting MIDI output devices: " << error.getMessage() << std::endl;
            }
        }
#endif
        return devices;
    }
    
    bool openDevice(int deviceIndex) {
        closeDevice(); // Close any existing connection first
        
#ifdef HAVE_RTMIDI
        if (rtMidiOut_) {
            try {
                unsigned int portCount = rtMidiOut_->getPortCount();
                if (deviceIndex >= 0 && deviceIndex < static_cast<int>(portCount)) {
                    rtMidiOut_->openPort(deviceIndex);
                    isOpen_ = true;
                    return true;
                }
            } catch (RtMidiError& error) {
                std::cerr << "Error opening MIDI output device: " << error.getMessage() << std::endl;
            }
        }
#endif
        return false;
    }
    
    void closeDevice() {
#ifdef HAVE_RTMIDI
        if (rtMidiOut_ && isOpen_) {
            try {
                rtMidiOut_->closePort();
                isOpen_ = false;
            } catch (RtMidiError& error) {
                std::cerr << "Error closing MIDI output device: " << error.getMessage() << std::endl;
            }
        }
#endif
    }
    
    bool isDeviceOpen() const {
        return isOpen_;
    }
    
    void sendMessage(const std::vector<unsigned char>& message) {
#ifdef HAVE_RTMIDI
        if (rtMidiOut_ && isOpen_) {
            try {
                rtMidiOut_->sendMessage(&message);
            } catch (RtMidiError& error) {
                std::cerr << "Error sending MIDI message: " << error.getMessage() << std::endl;
            }
        }
#endif
    }
    
private:
#ifdef HAVE_RTMIDI
    std::unique_ptr<RtMidiOut> rtMidiOut_;
#endif
    bool isOpen_;
};

MidiOutput::MidiOutput() : pimpl_(std::make_unique<Impl>()) {
}

MidiOutput::~MidiOutput() {
    // Unique_ptr will clean up the pimpl
}

std::vector<std::string> MidiOutput::getDevices() {
#ifdef HAVE_RTMIDI
    try {
        RtMidiOut rtMidiOut;
        std::vector<std::string> devices;
        unsigned int portCount = rtMidiOut.getPortCount();
        for (unsigned int i = 0; i < portCount; ++i) {
            devices.push_back(rtMidiOut.getPortName(i));
        }
        return devices;
    } catch (RtMidiError& error) {
        std::cerr << "Error getting MIDI output devices: " << error.getMessage() << std::endl;
    }
#endif
    return {};
}

bool MidiOutput::openDevice(int deviceIndex) {
    return pimpl_->openDevice(deviceIndex);
}

void MidiOutput::closeDevice() {
    pimpl_->closeDevice();
}

bool MidiOutput::isDeviceOpen() const {
    return pimpl_->isDeviceOpen();
}

void MidiOutput::sendNoteOn(int channel, int noteNumber, int velocity) {
    // Range validation
    if (channel < 1 || channel > 16 || noteNumber < 0 || noteNumber > 127 || velocity < 0 || velocity > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendNoteOn");
    }
    
    // Create a MIDI message
    std::vector<unsigned char> message;
    message.push_back(0x90 | (channel - 1)); // Note On status byte (0x90) + channel
    message.push_back(static_cast<unsigned char>(noteNumber));
    message.push_back(static_cast<unsigned char>(velocity));
    
    // Send the message
    pimpl_->sendMessage(message);
}

void MidiOutput::sendNoteOff(int channel, int noteNumber) {
    // Range validation
    if (channel < 1 || channel > 16 || noteNumber < 0 || noteNumber > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendNoteOff");
    }
    
    // Create a MIDI message
    std::vector<unsigned char> message;
    message.push_back(0x80 | (channel - 1)); // Note Off status byte (0x80) + channel
    message.push_back(static_cast<unsigned char>(noteNumber));
    message.push_back(0); // Zero velocity for note off
    
    // Send the message
    pimpl_->sendMessage(message);
}

void MidiOutput::sendControlChange(int channel, int controller, int value) {
    // Range validation
    if (channel < 1 || channel > 16 || controller < 0 || controller > 127 || value < 0 || value > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendControlChange");
    }
    
    // Create a MIDI message
    std::vector<unsigned char> message;
    message.push_back(0xB0 | (channel - 1)); // CC status byte (0xB0) + channel
    message.push_back(static_cast<unsigned char>(controller));
    message.push_back(static_cast<unsigned char>(value));
    
    // Send the message
    pimpl_->sendMessage(message);
}

void MidiOutput::sendProgramChange(int channel, int programNumber) {
    // Range validation
    if (channel < 1 || channel > 16 || programNumber < 0 || programNumber > 127) {
        throw std::out_of_range("Invalid MIDI parameters in sendProgramChange");
    }
    
    // Create a MIDI message
    std::vector<unsigned char> message;
    message.push_back(0xC0 | (channel - 1)); // Program Change status byte (0xC0) + channel
    message.push_back(static_cast<unsigned char>(programNumber));
    
    // Send the message
    pimpl_->sendMessage(message);
}

void MidiOutput::sendPitchBend(int channel, int value) {
    // Range validation
    if (channel < 1 || channel > 16 || value < 0 || value > 16383) {
        throw std::out_of_range("Invalid MIDI parameters in sendPitchBend");
    }
    
    // Create a MIDI message
    std::vector<unsigned char> message;
    message.push_back(0xE0 | (channel - 1)); // Pitch Bend status byte (0xE0) + channel
    message.push_back(value & 0x7F); // LSB (7 bits)
    message.push_back((value >> 7) & 0x7F); // MSB (7 bits)
    
    // Send the message
    pimpl_->sendMessage(message);
}

void MidiOutput::sendMessage(const MidiMessage& message) {
    std::vector<unsigned char> rtMessage;
    
    // Determine the status byte based on message type and channel
    unsigned char statusByte = 0;
    
    switch (message.type) {
        case MidiMessage::Type::NoteOn:
            statusByte = 0x90;
            break;
        case MidiMessage::Type::NoteOff:
            statusByte = 0x80;
            break;
        case MidiMessage::Type::ControlChange:
            statusByte = 0xB0;
            break;
        case MidiMessage::Type::ProgramChange:
            statusByte = 0xC0;
            break;
        case MidiMessage::Type::PitchBend:
            statusByte = 0xE0;
            break;
        case MidiMessage::Type::AfterTouch:
            statusByte = 0xA0;
            break;
        case MidiMessage::Type::ChannelPressure:
            statusByte = 0xD0;
            break;
        case MidiMessage::Type::SystemMessage:
            statusByte = 0xF0;
            break;
        default:
            return; // Unsupported message type
    }
    
    statusByte |= (message.channel & 0x0F);
    rtMessage.push_back(statusByte);
    
    // Add data bytes
    rtMessage.push_back(static_cast<unsigned char>(message.data1 & 0x7F));
    
    // Program Change and Channel Pressure only have one data byte
    if (message.type != MidiMessage::Type::ProgramChange && 
        message.type != MidiMessage::Type::ChannelPressure) {
        rtMessage.push_back(static_cast<unsigned char>(message.data2 & 0x7F));
    }
    
    // Send the message
    pimpl_->sendMessage(rtMessage);
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