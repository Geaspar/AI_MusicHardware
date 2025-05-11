#include "../../include/effects/MidiEffectControl.h"
#include <iostream>
#include <regex>

namespace AIMusicHardware {

MidiEffectControl::MidiEffectControl(ReorderableEffectsChain* effectsChain, MidiManager* midiManager)
    : effectsChain_(effectsChain), midiManager_(midiManager) {
    // Register as a MIDI listener
    if (midiManager_) {
        // MidiManager already owns the listener, so we don't need to delete it
        midiManager_->setListener(this);
    }
}

MidiEffectControl::~MidiEffectControl() {
    // Cleanup if needed
}

void MidiEffectControl::parameterChangedViaMidi(const std::string& paramId, float value) {
    // Check if this is a parameter we're responsible for
    auto result = parseParameterId(paramId);
    if (result.first >= 0) {
        int effectIndex = result.first;
        const std::string& paramName = result.second;
        
        // Update the effect parameter
        if (effectsChain_ && effectIndex < static_cast<int>(effectsChain_->getNumEffects())) {
            Effect* effect = effectsChain_->getEffect(effectIndex);
            if (effect) {
                effect->setParameter(paramName, value);
                
                // Provide user feedback
                std::cout << "MIDI updated " << effectsChain_->getEffectType(effectIndex) 
                          << " effect parameter: " << paramName << " = " << value << std::endl;
            }
        }
    }
}

void MidiEffectControl::pitchBendChanged(int channel, float value) {
    // Optional: Map pitch bend to effect parameters
    // For now, we just log the value
    std::cout << "Pitch bend on channel " << channel << ": " << value << std::endl;
}

void MidiEffectControl::modWheelChanged(int channel, float value) {
    // Optional: Map mod wheel to effect parameters
    // For now, we just log the value
    std::cout << "Mod wheel on channel " << channel << ": " << value << std::endl;
}

void MidiEffectControl::afterTouchChanged(int channel, float value) {
    // Optional: Map aftertouch to effect parameters
    // For now, we just log the value
    std::cout << "Aftertouch on channel " << channel << ": " << value << std::endl;
}

bool MidiEffectControl::mapEffectParameter(size_t effectIndex, const std::string& paramName, int channel, int cc) {
    if (!effectsChain_ || effectIndex >= effectsChain_->getNumEffects()) {
        return false;
    }
    
    // Get the effect and check if parameter exists
    Effect* effect = effectsChain_->getEffect(effectIndex);
    if (!effect) {
        return false;
    }
    
    // Create a parameter ID
    std::string paramId = getParameterId(effectIndex, paramName);
    
    // Store the mapping in our local map
    parameterMap_[paramId] = std::make_pair(effectIndex, paramName);
    
    // Create a MIDI mapping
    if (midiManager_) {
        // Add to MIDI manager mappings
        auto mappings = midiManager_->getMidiMappings();
        mappings[channel][cc] = paramId;
        midiManager_->setMidiMappings(mappings);
        
        std::cout << "Mapped " << effectsChain_->getEffectType(effectIndex) 
                  << " parameter '" << paramName 
                  << "' to MIDI channel " << channel 
                  << ", CC " << cc << std::endl;
                  
        return true;
    }
    
    return false;
}

bool MidiEffectControl::unmapEffectParameter(size_t effectIndex, const std::string& paramName) {
    // Create parameter ID
    std::string paramId = getParameterId(effectIndex, paramName);
    
    // Remove from our local map
    auto it = parameterMap_.find(paramId);
    if (it != parameterMap_.end()) {
        parameterMap_.erase(it);
        
        // Remove from MIDI manager
        if (midiManager_) {
            midiManager_->clearMidiLearn(paramId);
        }
        
        std::cout << "Unmapped " << effectsChain_->getEffectType(effectIndex) 
                  << " parameter '" << paramName << "' from MIDI control" << std::endl;
                  
        return true;
    }
    
    return false;
}

void MidiEffectControl::startMidiLearn(size_t effectIndex, const std::string& paramName) {
    if (!effectsChain_ || effectIndex >= effectsChain_->getNumEffects()) {
        std::cout << "Invalid effect index for MIDI learn" << std::endl;
        return;
    }
    
    // Get the effect and check if parameter exists
    Effect* effect = effectsChain_->getEffect(effectIndex);
    if (!effect) {
        std::cout << "Effect not found for MIDI learn" << std::endl;
        return;
    }
    
    // Create a parameter ID
    std::string paramId = getParameterId(effectIndex, paramName);
    
    // Store the parameter being learned
    learnParamId_ = paramId;
    
    // Start MIDI learn in the MIDI manager
    if (midiManager_) {
        midiManager_->armMidiLearn(paramId);
        
        std::cout << "MIDI Learn started for " << effectsChain_->getEffectType(effectIndex) 
                  << " parameter '" << paramName 
                  << "'. Move a MIDI controller..." << std::endl;
    }
}

void MidiEffectControl::cancelMidiLearn() {
    learnParamId_.clear();
    
    if (midiManager_) {
        midiManager_->cancelMidiLearn();
        std::cout << "MIDI Learn canceled" << std::endl;
    }
}

std::map<std::string, std::pair<int, int>> MidiEffectControl::getMidiMappings() const {
    std::map<std::string, std::pair<int, int>> result;
    
    if (midiManager_) {
        auto midiMappings = midiManager_->getMidiMappings();
        
        // Convert from MidiManager's format to our format
        for (const auto& channelMapping : midiMappings) {
            int channel = channelMapping.first;
            
            for (const auto& ccMapping : channelMapping.second) {
                int cc = ccMapping.first;
                const std::string& paramId = ccMapping.second;
                
                // Check if this is one of our effect parameters
                auto parsed = parseParameterId(paramId);
                if (parsed.first >= 0) {
                    // Store the mapping in our result map
                    result[paramId] = std::make_pair(channel, cc);
                }
            }
        }
    }
    
    return result;
}

std::string MidiEffectControl::getParameterId(size_t effectIndex, const std::string& paramName) {
    return "effect" + std::to_string(effectIndex) + "_" + paramName;
}

std::pair<int, std::string> MidiEffectControl::parseParameterId(const std::string& paramId) {
    // Use regex to extract effect index and parameter name
    std::regex pattern("effect(\\d+)_(.+)");
    std::smatch matches;
    
    if (std::regex_match(paramId, matches, pattern)) {
        if (matches.size() == 3) {
            int effectIndex = std::stoi(matches[1]);
            std::string paramName = matches[2];
            return std::make_pair(effectIndex, paramName);
        }
    }
    
    // Return invalid values if no match
    return std::make_pair(-1, "");
}

} // namespace AIMusicHardware