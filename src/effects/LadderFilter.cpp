#include "../../include/effects/LadderFilter.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

LadderFilterModel::LadderFilterModel(int sampleRate, Type type)
    : FilterModel(sampleRate), type_(type) {
    
    // Initialize default parameters
    parameters_["frequency"] = 1000.0f;
    parameters_["resonance"] = 0.0f;     // 0-1 range, will be scaled internally
    parameters_["drive"] = 1.0f;         // Input drive/gain, adds saturation
    parameters_["poles"] = 4.0f;         // Number of filter poles (1-4)
    
    // Initialize cached values
    cutoff_ = 0.0f;
    resonance_ = 0.0f;
    drive_ = 1.0f;
    
    reset();
    calculateCoefficients();
}

LadderFilterModel::~LadderFilterModel() {
}

void LadderFilterModel::reset() {
    // Reset all filter states
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 4; ++i) {
            state_[ch][i] = 0.0f;
        }
        delay_[ch] = 0.0f;
    }
}

void LadderFilterModel::setSampleRate(int sampleRate) {
    FilterModel::setSampleRate(sampleRate);
    calculateCoefficients();
}

std::string LadderFilterModel::getTypeName() const {
    switch (type_) {
        case Type::LowPass: return "Ladder Low Pass";
        case Type::HighPass: return "Ladder High Pass";
        default: return "Ladder Filter";
    }
}

void LadderFilterModel::calculateCoefficients() {
    float frequency = parameters_["frequency"];
    resonance_ = parameters_["resonance"] * 3.99f; // Scale to [0, ~4]
    drive_ = parameters_["drive"];
    
    // Convert frequency from Hz to normalized [0,1]
    cutoff_ = 2.0f * frequency / sampleRate_;
    if (cutoff_ > 1.0f) cutoff_ = 1.0f;
    
    // Calculate the filter coefficient 'g'
    // This approximation works well and avoids expensive transcendentals
    g_ = 0.9892f * cutoff_ - 0.4342f * cutoff_ * cutoff_ + 0.1381f * cutoff_ * cutoff_ * cutoff_ - 0.0202f * cutoff_ * cutoff_ * cutoff_ * cutoff_;
    
    // Resonance compensation
    resonanceComp_ = 0.0f;
    if (resonance_ > 0.0f) {
        // Apply a small compensation to prevent bass loss at high resonance
        resonanceComp_ = 0.005f * resonance_;
    }
}

void LadderFilterModel::process(float* buffer, int numFrames, int channels) {
    // Process parameters - update coefficients if needed
    float frequency = parameters_["frequency"];
    float resonance = parameters_["resonance"];
    float drive = parameters_["drive"];
    int poles = static_cast<int>(parameters_["poles"]);
    
    // Clamp poles to valid range
    poles = std::clamp(poles, 1, 4);
    
    // Check if parameters have changed
    static float lastFreq = 0.0f;
    static float lastRes = 0.0f;
    static float lastDrive = 0.0f;
    
    if (frequency != lastFreq || resonance != lastRes || drive != lastDrive) {
        calculateCoefficients();
        lastFreq = frequency;
        lastRes = resonance;
        lastDrive = drive;
    }
    
    // Process audio samples
    for (int i = 0; i < numFrames * channels; i += channels) {
        for (int ch = 0; ch < std::min(channels, 2); ++ch) {
            // Get input sample
            float input = buffer[i + ch];
            
            // Apply input drive (subtle saturation)
            input *= drive_;
            if (drive_ > 1.0f && std::abs(input) > 1.0f) {
                // Soft clipping for distortion
                input = std::tanh(input);
            }
            
            // Apply resonance feedback
            float feedback = resonance_ * (1.0f - 0.15f * g_) * state_[ch][3];
            
            // Apply compensation to prevent bass loss at high resonance
            float compensatedInput = input + resonanceComp_ * input;
            
            // Input with feedback
            float x = compensatedInput - feedback;
            
            // Ladder filter core - four cascaded one-pole filters
            for (int stage = 0; stage < 4; ++stage) {
                // One-pole lowpass filter: y[n] = g*x[n] + (1-g)*y[n-1]
                x = g_ * x + (1.0f - g_) * state_[ch][stage];
                state_[ch][stage] = x;
            }
            
            float output = 0.0f;
            
            // Apply filter type and pole count
            if (type_ == Type::LowPass) {
                // For lowpass, use the output of the relevant pole
                switch (poles) {
                    case 1: output = state_[ch][0]; break;
                    case 2: output = state_[ch][1]; break;
                    case 3: output = state_[ch][2]; break;
                    case 4: output = state_[ch][3]; break;
                }
            }
            else { // HighPass
                // For highpass, subtract the lowpass from the delayed input
                delay_[ch] = input;
                
                // Use the appropriate pole output
                switch (poles) {
                    case 1: output = delay_[ch] - state_[ch][0]; break;
                    case 2: output = delay_[ch] - state_[ch][1]; break;
                    case 3: output = delay_[ch] - state_[ch][2]; break;
                    case 4: output = delay_[ch] - state_[ch][3]; break;
                }
            }
            
            // Write output
            buffer[i + ch] = output;
        }
    }
}

void LadderFilterModel::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "frequency") {
        parameters_["frequency"] = clamp(value, 20.0f, 20000.0f);
        recalculate = true;
    }
    else if (name == "resonance") {
        parameters_["resonance"] = clamp(value, 0.0f, 1.0f);
        recalculate = true;
    }
    else if (name == "drive") {
        parameters_["drive"] = clamp(value, 0.5f, 10.0f);
        recalculate = true;
    }
    else if (name == "poles") {
        parameters_["poles"] = clamp(value, 1.0f, 4.0f);
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 1) {
            type_ = static_cast<Type>(typeInt);
        }
    }
    else {
        // For any other parameters, use base implementation
        FilterModel::setParameter(name, value);
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

} // namespace AIMusicHardware