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
            
            // First check if this parameter is already mapped elsewhere and clear that mapping
            for (auto& channelMap : midiMappings_) {
                for (auto it = channelMap.second.begin(); it != channelMap.second.end(); ) {
                    if (it->second == learnParamId_) {
                        std::cout << "Removing existing mapping for " << learnParamId_ 
                                  << " from channel " << channelMap.first 
                                  << ", controller " << it->first << std::endl;
                        it = channelMap.second.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            // Create new mapping
            midiMappings_[channel][controller] = learnParamId_;
            
            std::cout << "MIDI Learn: Channel " << channel << ", Controller " << controller 
                      << " mapped to parameter " << learnParamId_ << std::endl;
            
            // Clear learn state after successful mapping
            learnParamId_.clear();
            
            // Also update the parameter value
            updateMappedParameter(channel, controller, value);
            
            // Notify listener if available
            if (listener_) {
                listener_->parameterChangedViaMidi("midi_learn_complete", 1.0f);
            }
            
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
            // Also update any mapped parameter
            updateMappedParameter(channel, controller, value);
            break;
            
        case kExpression:
            // Handle expression pedal (can be useful for volume/filter sweeps)
            updateMappedParameter(channel, controller, value);
            break;
            
        case kBreathController:
            // Handle breath controller (useful for wind instrument modeling)
            updateMappedParameter(channel, controller, value);
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

// Scaling type implementation moved to header file

float MidiManager::midiValueToParameter(int value, ParameterScaling scaling, 
                                        float min, float max, int steps) const {
    // Normalize to 0.0-1.0 range
    float normalized = value / 127.0f;
    
    // Apply scaling
    float scaled = 0.0f;
    switch (scaling) {
        case ParameterScaling::Linear:
            scaled = normalized;
            break;
            
        case ParameterScaling::Logarithmic:
            // Log scaling for frequencies, etc.
            // Avoid log(0) by adding a small value
            scaled = std::log10(normalized * 0.9f + 0.1f) / std::log10(1.1f);
            break;
            
        case ParameterScaling::Exponential:
            // Exponential scaling for times, etc.
            scaled = std::pow(normalized, 3.0f);  // Cubic for more sensitivity at low values
            break;
            
        case ParameterScaling::Stepped:
            if (steps > 1) {
                // Convert to discrete steps
                int step = static_cast<int>(normalized * steps);
                scaled = static_cast<float>(step) / static_cast<float>(steps - 1);
            } else {
                scaled = normalized;
            }
            break;
    }
    
    // Map to parameter range
    return min + scaled * (max - min);
}

int MidiManager::parameterToMidiValue(float value, ParameterScaling scaling, 
                                      float min, float max, int steps) const {
    // Normalize to 0.0-1.0 range
    float normalized = (value - min) / (max - min);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    // Apply inverse scaling
    float scaled = 0.0f;
    switch (scaling) {
        case ParameterScaling::Linear:
            scaled = normalized;
            break;
            
        case ParameterScaling::Logarithmic:
            // Inverse log scaling
            scaled = (std::pow(10.0f, normalized * std::log10(1.1f)) - 0.1f) / 0.9f;
            break;
            
        case ParameterScaling::Exponential:
            // Inverse exponential scaling
            scaled = std::pow(normalized, 1.0f/3.0f);  // Cube root
            break;
            
        case ParameterScaling::Stepped:
            if (steps > 1) {
                // Convert from discrete steps
                int step = static_cast<int>(normalized * steps);
                scaled = static_cast<float>(step) / static_cast<float>(steps - 1);
            } else {
                scaled = normalized;
            }
            break;
    }
    
    // Convert to MIDI value (0-127)
    return static_cast<int>(std::clamp(scaled * 127.0f, 0.0f, 127.0f));
}

// Simple version for backward compatibility
float MidiManager::midiValueToParameter(int value) const {
    return midiValueToParameter(value, ParameterScaling::Linear, 0.0f, 1.0f, 0);
}

// Simple version for backward compatibility
int MidiManager::parameterToMidiValue(float value) const {
    return parameterToMidiValue(value, ParameterScaling::Linear, 0.0f, 1.0f, 0);
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
            
            // Determine parameter range and scaling based on parameter ID
            // This is where we would look up parameter metadata in a more complete system
            ParameterScaling scaling = ParameterScaling::Linear;
            float min = 0.0f;
            float max = 1.0f;
            int steps = 0;
            
            // Example parameter-specific mappings
            // In a real implementation, this would come from a parameter registry
            if (paramId == "filter_cutoff") {
                scaling = ParameterScaling::Logarithmic;
                min = 20.0f;    // 20Hz
                max = 20000.0f; // 20kHz
            } else if (paramId == "filter_resonance") {
                scaling = ParameterScaling::Exponential;
                min = 0.0f;
                max = 0.99f;
            } else if (paramId == "oscillator_type") {
                scaling = ParameterScaling::Stepped;
                steps = 5; // 5 waveform types
            }
            
            // Convert MIDI value to parameter value
            float paramValue = midiValueToParameter(value, scaling, min, max, steps);
            
            // Update the parameter in the synthesizer
            if (synthesizer_) {
                synthesizer_->setParameter(paramId, paramValue);
            }
            
            // Notify the listener
            if (listener_) {
                listener_->parameterChangedViaMidi(paramId, paramValue);
            }
        }
    }
}

} // namespace AIMusicHardware