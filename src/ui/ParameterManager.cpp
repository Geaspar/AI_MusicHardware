#include "../../include/ui/ParameterManager.h"
#include "../../include/audio/Synthesizer.h"
#include <iostream>
#include <algorithm>

namespace AIMusicHardware {

ParameterManager::ParameterManager() 
    : synth_(nullptr) {
}

ParameterManager::~ParameterManager() {
}

bool ParameterManager::initialize() {
    // Initialize default parameters
    initializeDefaultParameters();
    return true;
}

void ParameterManager::connectSynthesizer(Synthesizer* synth) {
    synth_ = synth;
    
    // Update internal parameter cache from synthesizer
    if (synth_) {
        parameters_ = synth_->getAllParameters();
    }
}

float ParameterManager::getParameterValue(const std::string& parameterId) const {
    // If connected to a synthesizer, get the value directly
    if (synth_ && isValidParameter(parameterId)) {
        return synth_->getParameter(parameterId);
    }
    
    // Otherwise, look up in our parameter cache
    auto it = parameters_.find(parameterId);
    if (it != parameters_.end()) {
        return it->second;
    }
    
    // Parameter not found
    std::cerr << "Parameter not found: " << parameterId << std::endl;
    return 0.0f;
}

void ParameterManager::setParameterValue(const std::string& parameterId, float value) {
    // Update local cache
    parameters_[parameterId] = value;
    
    // If connected to a synthesizer, update it
    if (synth_) {
        synth_->setParameter(parameterId, value);
    }
    
    // Notify observers
    updateSynthesizer(parameterId);
    if (parameterChangedCallback_) {
        parameterChangedCallback_(parameterId);
    }
}

std::map<std::string, float> ParameterManager::getAllParameters() const {
    // If connected to a synthesizer, get all parameters from it
    if (synth_) {
        return synth_->getAllParameters();
    }
    
    // Otherwise, return our parameter cache
    return parameters_;
}

void ParameterManager::setAllParameters(const std::map<std::string, float>& parameters) {
    // Update local cache
    parameters_ = parameters;
    
    // If connected to a synthesizer, update all parameters
    if (synth_) {
        synth_->setAllParameters(parameters);
    }
    
    // Notify observers for each parameter
    for (const auto& [id, value] : parameters) {
        updateSynthesizer(id);
        if (parameterChangedCallback_) {
            parameterChangedCallback_(id);
        }
    }
}

void ParameterManager::addModulation(const std::string& source, const std::string& destination, float amount) {
    // Check if modulation already exists
    for (auto& mod : modulations_) {
        if (mod.source == source && mod.destination == destination) {
            // Update existing modulation
            mod.amount = amount;
            return;
        }
    }
    
    // Add new modulation
    modulations_.emplace_back(source, destination, amount);
    
    // Apply modulation to synthesizer if connected
    if (synth_) {
        // Note: We'll need to implement this in Synthesizer
        // synth_->setModulation(source, destination, amount);
    }
}

void ParameterManager::removeModulation(const std::string& source, const std::string& destination) {
    // Find and remove the modulation
    modulations_.erase(
        std::remove_if(modulations_.begin(), modulations_.end(), 
                     [&](const ModulationRouting& mod) {
                         return mod.source == source && mod.destination == destination;
                     }),
        modulations_.end());
    
    // Remove from synthesizer if connected
    if (synth_) {
        // Note: We'll need to implement this in Synthesizer
        // synth_->removeModulation(source, destination);
    }
}

std::vector<ModulationRouting> ParameterManager::getAllModulations() const {
    return modulations_;
}

void ParameterManager::setAllModulations(const std::vector<ModulationRouting>& modulations) {
    modulations_ = modulations;
    
    // Apply to synthesizer if connected
    if (synth_) {
        // Note: We'll need to implement this in Synthesizer
        // synth_->setAllModulations(modulations);
    }
}

void ParameterManager::setParameterChangedCallback(ParameterChangedCallback callback) {
    parameterChangedCallback_ = callback;
}

std::vector<std::string> ParameterManager::getAllParameterIds() const {
    std::vector<std::string> ids;
    
    // Get parameters from synthesizer if connected
    std::map<std::string, float> params;
    if (synth_) {
        params = synth_->getAllParameters();
    } else {
        params = parameters_;
    }
    
    // Extract IDs
    ids.reserve(params.size());
    for (const auto& [id, _] : params) {
        ids.push_back(id);
    }
    
    return ids;
}

std::string ParameterManager::getParameterDisplayName(const std::string& parameterId) const {
    // Convert parameter ID to display name (replace underscores with spaces, capitalize)
    std::string displayName = parameterId;
    
    // Replace underscores with spaces
    std::replace(displayName.begin(), displayName.end(), '_', ' ');
    
    // Capitalize first letter of each word
    bool capitalizeNext = true;
    for (char& c : displayName) {
        if (capitalizeNext && std::isalpha(c)) {
            c = std::toupper(c);
            capitalizeNext = false;
        } else if (c == ' ') {
            capitalizeNext = true;
        }
    }
    
    return displayName;
}

std::string ParameterManager::formatParameterValue(const std::string& parameterId, float value) const {
    // Format parameter value based on type
    
    // Integer parameters (like oscillator type)
    if (parameterId == "oscillator_type" || 
        parameterId == "voice_count" || 
        parameterId == "lfo1_shape" || 
        parameterId == "lfo2_shape") {
        return std::to_string(static_cast<int>(value));
    }
    
    // Percentage parameters
    if (parameterId.find("amount") != std::string::npos ||
        parameterId == "reverb_mix" ||
        parameterId == "filter_resonance") {
        return std::to_string(static_cast<int>(value * 100)) + "%";
    }
    
    // Frequency parameters
    if (parameterId == "filter_cutoff") {
        // Map 0-1 to 20-20000 Hz with logarithmic scaling
        float hz = 20.0f * std::pow(1000.0f, value);
        if (hz < 1000.0f) {
            return std::to_string(static_cast<int>(hz)) + " Hz";
        } else {
            return std::to_string(static_cast<int>(hz / 1000.0f)) + " kHz";
        }
    }
    
    if (parameterId.find("frequency") != std::string::npos ||
        parameterId.find("rate") != std::string::npos) {
        return std::to_string(static_cast<int>(value * 100) / 100.0f) + " Hz";
    }
    
    // Default formatting (2 decimal places)
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    return buffer;
}

void ParameterManager::getParameterRange(const std::string& parameterId, float& min, float& max) const {
    // Define parameter ranges
    
    // Oscillator type (0-3)
    if (parameterId == "oscillator_type") {
        min = 0.0f;
        max = 3.0f;
        return;
    }
    
    // Voice count (1-16)
    if (parameterId == "voice_count") {
        min = 1.0f;
        max = 16.0f;
        return;
    }
    
    // LFO shape (0-4)
    if (parameterId.find("lfo") != std::string::npos && 
        parameterId.find("shape") != std::string::npos) {
        min = 0.0f;
        max = 4.0f;
        return;
    }
    
    // LFO rate (0.01-20 Hz)
    if (parameterId.find("lfo") != std::string::npos && 
        parameterId.find("rate") != std::string::npos) {
        min = 0.01f;
        max = 20.0f;
        return;
    }
    
    // Filter cutoff (0-1, mapped to 20-20000 Hz)
    if (parameterId == "filter_cutoff") {
        min = 0.0f;
        max = 1.0f;
        return;
    }
    
    // Filter resonance (0-1)
    if (parameterId == "filter_resonance") {
        min = 0.0f;
        max = 1.0f;
        return;
    }
    
    // Default range (0-1)
    min = 0.0f;
    max = 1.0f;
}

void ParameterManager::initializeDefaultParameters() {
    // Initialize default parameters with sensible values
    parameters_["oscillator_type"] = 0.0f;    // Sine
    parameters_["voice_count"] = 8.0f;        // 8 voices
    parameters_["filter_cutoff"] = 1.0f;      // Fully open
    parameters_["filter_resonance"] = 0.5f;   // Medium resonance
    parameters_["envelope_attack"] = 0.01f;   // 10ms
    parameters_["envelope_decay"] = 0.1f;     // 100ms
    parameters_["envelope_sustain"] = 0.7f;   // 70%
    parameters_["envelope_release"] = 0.3f;   // 300ms
    parameters_["lfo1_rate"] = 1.0f;          // 1 Hz
    parameters_["lfo1_shape"] = 0.0f;         // Sine
    parameters_["lfo1_amount"] = 0.0f;        // No modulation
    parameters_["lfo2_rate"] = 0.5f;          // 0.5 Hz
    parameters_["lfo2_shape"] = 0.0f;         // Sine
    parameters_["lfo2_amount"] = 0.0f;        // No modulation
    parameters_["reverb_mix"] = 0.2f;         // 20% wet
    parameters_["volume"] = 0.7f;             // 70% volume
}

void ParameterManager::updateSynthesizer(const std::string& parameterId) {
    // This would apply any internal processing or conversions
    // before updating the synthesizer
}

bool ParameterManager::isValidParameter(const std::string& parameterId) const {
    // Check if a parameter is valid
    if (synth_) {
        // If connected to a synthesizer, check its parameter list
        auto params = synth_->getAllParameters();
        return params.find(parameterId) != params.end();
    }
    
    // Otherwise check our parameter cache
    return parameters_.find(parameterId) != parameters_.end();
}

} // namespace AIMusicHardware