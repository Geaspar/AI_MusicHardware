#include "../../../include/ui/parameters/ParameterManager.h"
#include "../../../include/audio/Synthesizer.h"
#include "../../../include/iot/IoTInterface.h"
#include <iostream>
#include <algorithm>

namespace AIMusicHardware {

// Singleton implementation
EnhancedParameterManager& EnhancedParameterManager::getInstance() {
    static EnhancedParameterManager instance;
    return instance;
}

EnhancedParameterManager::EnhancedParameterManager()
    : synth_(nullptr),
      iotInterface_(nullptr),
      iotAdapter_(nullptr) {
    
    // Create standard parameter groups
    rootGroup_.createGroup("synth", "Synthesizer");
    rootGroup_.createGroup("effects", "Effects");
    rootGroup_.createGroup("sequencer", "Sequencer");
}

EnhancedParameterManager::~EnhancedParameterManager() {
    // Cleanup - nothing to do as we use smart pointers
}

void EnhancedParameterManager::registerParameter(Parameter* parameter) {
    if (!parameter) return;
    
    // Add to global registry
    parameterRegistry_[parameter->getId()] = parameter;
    
    // If it's a float parameter with smoothing, add to smoothing list
    if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter)) {
        // Note: We'd need to track whether smoothing is enabled
        // For now, we'll just add all float parameters
        SmoothingInfo info;
        info.parameter = floatParam;
        info.updateInterval = 0.01f; // 10ms default
        info.lastUpdate = totalTime_;
        smoothingParameters_.push_back(info);
    }
}

void EnhancedParameterManager::unregisterParameter(Parameter* parameter) {
    if (!parameter) return;
    
    // Remove from global registry
    parameterRegistry_.erase(parameter->getId());
    
    // Remove from MIDI map
    for (auto it = midiCCMap_.begin(); it != midiCCMap_.end(); ) {
        if (it->second == parameter) {
            it = midiCCMap_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove from smoothing list
    if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter)) {
        smoothingParameters_.erase(
            std::remove_if(smoothingParameters_.begin(), smoothingParameters_.end(),
                         [floatParam](const SmoothingInfo& info) {
                             return info.parameter == floatParam;
                         }),
            smoothingParameters_.end());
    }
}

Parameter* EnhancedParameterManager::findParameter(const Parameter::ParameterId& id) {
    auto it = parameterRegistry_.find(id);
    if (it != parameterRegistry_.end()) {
        return it->second;
    }
    return nullptr;
}

Parameter* EnhancedParameterManager::getParameterByPath(const std::string& path) {
    return rootGroup_.getParameterByPath(path);
}

ParameterGroup* EnhancedParameterManager::getGroupByPath(const std::string& path) {
    return rootGroup_.getGroupByPath(path);
}

void EnhancedParameterManager::mapParameterToMidi(Parameter* parameter, int controller, int channel) {
    if (!parameter) return;
    
    // Only parameters that can be automated
    if (!parameter->isAutomatable()) return;
    
    // Create MIDI mapping
    std::pair<int, int> midiKey = {controller, channel};
    midiCCMap_[midiKey] = parameter;
}

void EnhancedParameterManager::unmapParameterFromMidi(Parameter* parameter) {
    if (!parameter) return;
    
    // Remove from MIDI map
    for (auto it = midiCCMap_.begin(); it != midiCCMap_.end(); ) {
        if (it->second == parameter) {
            it = midiCCMap_.erase(it);
        } else {
            ++it;
        }
    }
}

Parameter* EnhancedParameterManager::getParameterForMidiCC(int controller, int channel) {
    std::pair<int, int> midiKey = {controller, channel};
    auto it = midiCCMap_.find(midiKey);
    if (it != midiCCMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void EnhancedParameterManager::connectSynthesizer(Synthesizer* synth) {
    synth_ = synth;
    
    // Sync parameter values from synthesizer
    if (synth_) {
        syncFromSynthesizer();
    }
}

void EnhancedParameterManager::connectIoTInterface(IoTInterface* iotInterface) {
    iotInterface_ = iotInterface;
    
    // Update the IoT adapter
    if (iotInterface_) {
        iotAdapter_ = std::make_unique<IoTEventAdapter>(iotInterface_);
        iotAdapter_->start();
    } else {
        iotAdapter_.reset();
    }
}

void EnhancedParameterManager::mapIoTTopicToParameter(
    const std::string& topic, 
    Parameter* parameter,
    IoTParameterConverter::SensorType sensorType,
    float minValue,
    float maxValue) {
    
    if (!parameter || !iotInterface_ || !iotAdapter_) return;
    
    // Register with IoT adapter
    iotAdapter_->mapTopicToParameter(topic, parameter);
    iotAdapter_->registerSensorType(topic, sensorType, minValue, maxValue);
}

void EnhancedParameterManager::setIoTMappingMode(
    const std::string& topic,
    IoTParameterMappings::MappingMode mappingMode,
    float threshold,
    float exponent) {
    
    if (!iotInterface_ || !iotAdapter_) return;
    
    // Set mapping mode in IoT adapter
    iotAdapter_->setMappingMode(topic, mappingMode, threshold, exponent);
}

void EnhancedParameterManager::updateAutomation(float deltaTime) {
    // Update total time
    totalTime_ += deltaTime;
    
    // Update all parameter smoothing
    for (auto& info : smoothingParameters_) {
        if (info.parameter) {
            info.parameter->updateSmoothing(deltaTime);
        }
    }
    
    // Update synthesizer if connected
    if (synth_) {
        syncToSynthesizer();
    }
}

void EnhancedParameterManager::syncFromSynthesizer() {
    if (!synth_) return;
    
    // Get all parameters from synthesizer
    auto synthParams = synth_->getAllParameters();
    
    // Update parameter values (create if doesn't exist)
    for (const auto& [id, value] : synthParams) {
        // Find existing parameter
        Parameter* param = findParameter(id);
        
        // If found, update it
        if (param) {
            if (auto* floatParam = dynamic_cast<FloatParameter*>(param)) {
                floatParam->setValue(value, false); // Don't notify observers
            }
            else if (auto* intParam = dynamic_cast<IntParameter*>(param)) {
                intParam->setValue(static_cast<int>(value), false);
            }
            else if (auto* boolParam = dynamic_cast<BoolParameter*>(param)) {
                boolParam->setValue(value >= 0.5f, false);
            }
            // Enums and triggers would need special handling
        }
        else {
            // Create placeholder parameter in the root group
            auto* floatParam = rootGroup_.createParameter<FloatParameter>(id, id, value);
            
            // Register it (already done by createParameter, but to be safe)
            registerParameter(floatParam);
        }
    }
}

void EnhancedParameterManager::syncToSynthesizer() {
    if (!synth_) return;
    
    // Collect all parameter values to update synthesizer
    std::map<std::string, float> synthParams;
    
    // Iterate through all registered parameters
    for (const auto& [id, param] : parameterRegistry_) {
        if (param) {
            float value = 0.0f;
            
            // Convert parameter value to float based on type
            switch (param->getType()) {
                case Parameter::Type::FLOAT:
                    value = static_cast<FloatParameter*>(param)->getValue();
                    break;
                    
                case Parameter::Type::INT:
                    value = static_cast<float>(static_cast<IntParameter*>(param)->getValue());
                    break;
                    
                case Parameter::Type::BOOL:
                    value = static_cast<BoolParameter*>(param)->getValue() ? 1.0f : 0.0f;
                    break;
                    
                case Parameter::Type::ENUM:
                    value = static_cast<float>(static_cast<EnumParameter*>(param)->getValue());
                    break;
                    
                case Parameter::Type::TRIGGER:
                    // Triggers are transient, not reflected in synth params
                    continue;
            }
            
            synthParams[id] = value;
        }
    }
    
    // Update synthesizer with all parameters at once
    synth_->setAllParameters(synthParams);
}

} // namespace AIMusicHardware