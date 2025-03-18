#include "../../include/midi/MidiManager.h"
#include "../../include/audio/Synthesizer.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

MidiManager::MidiManager(Synthesizer* synthesizer, Listener* listener)
    : synthesizer_(synthesizer),
      listener_(listener),
      midiInput_(std::make_unique<MidiInput>()),
      midiOutput_(std::make_unique<MidiOutput>()) {
}

MidiManager::~MidiManager() {
    // Ensure MIDI devices are closed properly
    closeMidiInput();
    closeMidiOutput();
}

void MidiManager::handleIncomingMidiMessage(const MidiMessage& message) {
    // Process the message with sample position 0 (immediate)
    processMidiMessage(message, 0);
}

void MidiManager::processMidiMessage(const MidiMessage& message, int samplePosition) {
    // Process different message types
    switch (message.type) {
        case MidiMessage::Type::NoteOn:
            processNoteOn(message, samplePosition);
            break;
            
        case MidiMessage::Type::NoteOff:
            processNoteOff(message, samplePosition);
            break;
            
        case MidiMessage::Type::ControlChange:
            processControlChange(message, samplePosition);
            break;
            
        case MidiMessage::Type::PitchBend:
            processPitchBend(message, samplePosition);
            break;
            
        case MidiMessage::Type::AfterTouch:
            processAfterTouch(message, samplePosition);
            break;
            
        case MidiMessage::Type::ChannelPressure:
            processChannelPressure(message, samplePosition);
            break;
            
        default:
            // Other message types not handled
            break;
    }
}

void MidiManager::armMidiLearn(const std::string& paramId) {
    std::lock_guard<std::mutex> lock(learnMutex_);
    learnParamId_ = paramId;
    std::cout << "MIDI Learn armed for parameter: " << paramId << std::endl;
}

void MidiManager::cancelMidiLearn() {
    std::lock_guard<std::mutex> lock(learnMutex_);
    learnParamId_.clear();
    std::cout << "MIDI Learn canceled" << std::endl;
}

void MidiManager::clearMidiLearn(const std::string& paramId) {
    std::lock_guard<std::mutex> lock(mappingMutex_);
    
    // Search and remove any mapping for this parameter
    for (auto& channelMap : midiMappings_) {
        for (auto it = channelMap.second.begin(); it != channelMap.second.end(); ) {
            if (it->second == paramId) {
                it = channelMap.second.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    std::cout << "MIDI mapping cleared for parameter: " << paramId << std::endl;
}

bool MidiManager::isMidiMapped(const std::string& paramId) const {
    std::lock_guard<std::mutex> lock(mappingMutex_);
    
    // Check if parameter is mapped to any controller
    for (const auto& channelMap : midiMappings_) {
        for (const auto& controllerMap : channelMap.second) {
            if (controllerMap.second == paramId) {
                return true;
            }
        }
    }
    
    return false;
}

bool MidiManager::openMidiInput(int deviceIndex) {
    if (midiInput_->openDevice(deviceIndex)) {
        midiInput_->setCallback(this);
        return true;
    }
    return false;
}

void MidiManager::closeMidiInput() {
    midiInput_->closeDevice();
}

std::vector<std::string> MidiManager::getMidiInputDevices() const {
    return midiInput_->getDevices();
}

bool MidiManager::openMidiOutput(int deviceIndex) {
    return midiOutput_->openDevice(deviceIndex);
}

void MidiManager::closeMidiOutput() {
    midiOutput_->closeDevice();
}

std::vector<std::string> MidiManager::getMidiOutputDevices() const {
    return midiOutput_->getDevices();
}

MidiManager::MidiParameterMap MidiManager::getMidiMappings() const {
    std::lock_guard<std::mutex> lock(mappingMutex_);
    return midiMappings_;
}

void MidiManager::setMidiMappings(const MidiParameterMap& mappings) {
    std::lock_guard<std::mutex> lock(mappingMutex_);
    midiMappings_ = mappings;
}

void MidiManager::processNoteOn(const MidiMessage& message, int samplePosition) {
    if (synthesizer_) {
        int noteNumber = message.data1;
        int velocity = message.data2;
        
        // Channel is 0-based in our internal representation
        synthesizer_->noteOn(noteNumber, velocity / 127.0f, message.channel);
    }
}

void MidiManager::processNoteOff(const MidiMessage& message, int samplePosition) {
    if (synthesizer_) {
        int noteNumber = message.data1;
        
        // Channel is 0-based in our internal representation
        synthesizer_->noteOff(noteNumber, message.channel);
    }
}

void MidiManager::processControlChange(const MidiMessage& message, int samplePosition) {
    int controller = message.data1;
    int value = message.data2;
    int channel = message.channel;
    
    // Check for MIDI learn
    {
        std::lock_guard<std::mutex> lock(learnMutex_);
        if (!learnParamId_.empty()) {
            // Map this controller to the parameter being learned
            std::lock_guard<std::mutex> mappingLock(mappingMutex_);
            midiMappings_[channel][controller] = learnParamId_;
            
            std::cout << "MIDI Learn: Channel " << channel << ", Controller " << controller 
                      << " mapped to parameter " << learnParamId_ << std::endl;
            
            // Clear learn state after successful mapping
            learnParamId_.clear();
            
            // Also update the parameter value
            updateMappedParameter(channel, controller, value);
            return;
        }
    }
    
    // Handle specific controllers
    switch (controller) {
        case kSustainPedal:
            processSustain(message, samplePosition);
            break;
            
        case kAllNotesOff:
            processAllNotesOff(message, samplePosition);
            break;
            
        case kResetAllControllers:
            // Reset all controllers to default values
            if (synthesizer_) {
                synthesizer_->resetAllControllers();
            }
            break;
            
        case kModWheel:
            // Handle mod wheel
            if (listener_) {
                listener_->modWheelChanged(channel, midiValueToParameter(value));
            }
            break;
            
        default:
            // Check if this controller is mapped to a parameter
            updateMappedParameter(channel, controller, value);
            break;
    }
}

void MidiManager::processPitchBend(const MidiMessage& message, int samplePosition) {
    int channel = message.channel;
    
    // Combine MSB and LSB for 14-bit pitch bend value
    int combined = message.data1 | (message.data2 << 7);
    
    // Convert to normalized value (-1 to +1)
    float normalizedValue = (combined / 8192.0f) - 1.0f;
    
    if (synthesizer_) {
        synthesizer_->setPitchBend(normalizedValue, channel);
    }
    
    if (listener_) {
        listener_->pitchBendChanged(channel, normalizedValue);
    }
}

void MidiManager::processAfterTouch(const MidiMessage& message, int samplePosition) {
    int channel = message.channel;
    int note = message.data1;
    int pressure = message.data2;
    
    // Polyphonic aftertouch - per-note pressure
    if (synthesizer_) {
        synthesizer_->setAftertouch(note, midiValueToParameter(pressure), channel);
    }
}

void MidiManager::processChannelPressure(const MidiMessage& message, int samplePosition) {
    int channel = message.channel;
    int pressure = message.data1;
    
    // Channel pressure - affects all notes on channel
    if (synthesizer_) {
        synthesizer_->setChannelPressure(midiValueToParameter(pressure), channel);
    }
    
    if (listener_) {
        listener_->afterTouchChanged(channel, midiValueToParameter(pressure));
    }
}

void MidiManager::processAllNotesOff(const MidiMessage& message, int samplePosition) {
    int channel = message.channel;
    
    if (synthesizer_) {
        synthesizer_->allNotesOff(channel);
    }
}

void MidiManager::processSustain(const MidiMessage& message, int samplePosition) {
    int channel = message.channel;
    int value = message.data2;
    
    // Sustain pedal on if value >= 64, off otherwise
    bool sustainOn = value >= 64;
    
    if (synthesizer_) {
        if (sustainOn) {
            synthesizer_->sustainOn(channel);
        } else {
            synthesizer_->sustainOff(channel);
        }
    }
}

float MidiManager::midiValueToParameter(int value) const {
    // Convert MIDI value (0-127) to parameter value (0.0-1.0)
    return value / 127.0f;
}

int MidiManager::parameterToMidiValue(float value) const {
    // Convert parameter value (0.0-1.0) to MIDI value (0-127)
    return static_cast<int>(std::clamp(value * 127.0f, 0.0f, 127.0f));
}

void MidiManager::updateMappedParameter(int channel, int controller, int value) {
    std::lock_guard<std::mutex> lock(mappingMutex_);
    
    // Check if this controller is mapped to a parameter
    auto channelIt = midiMappings_.find(channel);
    if (channelIt != midiMappings_.end()) {
        auto controllerIt = channelIt->second.find(controller);
        if (controllerIt != channelIt->second.end()) {
            // Found a mapping
            const std::string& paramId = controllerIt->second;
            float normalizedValue = midiValueToParameter(value);
            
            // Update the parameter in the synthesizer
            if (synthesizer_) {
                synthesizer_->setParameter(paramId, normalizedValue);
            }
            
            // Notify the listener
            if (listener_) {
                listener_->parameterChangedViaMidi(paramId, normalizedValue);
            }
        }
    }
}

} // namespace AIMusicHardware