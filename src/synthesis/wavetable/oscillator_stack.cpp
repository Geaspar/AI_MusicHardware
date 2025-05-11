#include "../../../include/synthesis/wavetable/oscillator_stack.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace AIMusicHardware {

// Constants
constexpr int MAX_OSCILLATORS = 8;  // Maximum number of oscillators in a stack

OscillatorStack::OscillatorStack(int sampleRate, int numOscillators) 
    : sampleRate_(sampleRate), baseFrequency_(440.0f) {
    
    // Limit the number of oscillators
    numOscillators = std::clamp(numOscillators, 1, MAX_OSCILLATORS);
    
    // Initialize oscillators and configs
    for (int i = 0; i < numOscillators; ++i) {
        oscillators_.push_back(createOscillator());
        configs_.push_back(OscillatorConfig{}); // Default config
    }
}

OscillatorStack::~OscillatorStack() {
    // Smart pointers auto-cleanup
}

std::unique_ptr<WavetableOscillator> OscillatorStack::createOscillator() {
    auto osc = std::make_unique<WavetableOscillator>(sampleRate_);
    if (wavetable_) {
        osc->setWavetable(wavetable_);
    }
    return osc;
}

void OscillatorStack::setOscillatorCount(int count) {
    // Limit the count to valid range
    count = std::clamp(count, 1, MAX_OSCILLATORS);
    
    // If increasing, add new oscillators
    while (oscillators_.size() < static_cast<size_t>(count)) {
        oscillators_.push_back(createOscillator());
        configs_.push_back(OscillatorConfig{}); // Default config
        
        // Set the frequency for the new oscillator
        size_t index = oscillators_.size() - 1;
        applyDetune(static_cast<int>(index));
    }
    
    // If decreasing, remove oscillators
    while (oscillators_.size() > static_cast<size_t>(count)) {
        oscillators_.pop_back();
        configs_.pop_back();
    }
}

void OscillatorStack::setFrequency(float frequency) {
    // Store the base frequency
    baseFrequency_ = std::max(10.0f, frequency); // Minimum 10Hz to avoid issues
    
    // Update all oscillator frequencies with their detuning
    for (size_t i = 0; i < oscillators_.size(); ++i) {
        applyDetune(static_cast<int>(i));
    }
}

float OscillatorStack::calculateDetuneMultiplier(float cents) const {
    // Convert cents to frequency multiplier: 2^(cents/1200)
    return std::pow(2.0f, cents / 1200.0f);
}

void OscillatorStack::applyDetune(int index) {
    if (index < 0 || index >= static_cast<int>(oscillators_.size())) {
        return;
    }
    
    // Calculate detuned frequency
    float detuneMultiplier = calculateDetuneMultiplier(configs_[index].detune);
    float frequency = baseFrequency_ * detuneMultiplier;
    
    // Set the frequency
    oscillators_[index]->setFrequency(frequency);
}

void OscillatorStack::setDetune(int index, float cents) {
    if (index < 0 || index >= static_cast<int>(configs_.size())) {
        return;
    }
    
    // Limit detune to reasonable range (-100 to +100 cents, i.e., +/- semitone)
    cents = std::clamp(cents, -100.0f, 100.0f);
    
    // Update the config
    configs_[index].detune = cents;
    
    // Apply the change
    applyDetune(index);
}

void OscillatorStack::setDetuneSpread(float cents) {
    size_t count = oscillators_.size();
    if (count <= 1) {
        return; // Nothing to spread with only one oscillator
    }
    
    // Distribute detune values symmetrically
    for (size_t i = 0; i < count; ++i) {
        float normalizedPos = static_cast<float>(i) / (count - 1); // 0 to 1
        float detune = -cents / 2.0f + cents * normalizedPos;
        setDetune(static_cast<int>(i), detune);
    }
}

void OscillatorStack::setLevel(int index, float level) {
    if (index < 0 || index >= static_cast<int>(configs_.size())) {
        return;
    }
    
    // Clamp level between 0 and 1
    level = std::clamp(level, 0.0f, 1.0f);
    
    // Update the config
    configs_[index].level = level;
}

void OscillatorStack::setFramePosition(int index, float position) {
    if (index < 0 || index >= static_cast<int>(oscillators_.size())) {
        return;
    }
    
    // Clamp position between 0 and 1
    position = std::clamp(position, 0.0f, 1.0f);
    
    // Update the config
    configs_[index].framePosition = position;
    
    // Apply the change
    oscillators_[index]->setFramePosition(position);
}

void OscillatorStack::setAllFramePositions(float position) {
    // Clamp position between 0 and 1
    position = std::clamp(position, 0.0f, 1.0f);
    
    // Update all oscillators
    for (size_t i = 0; i < oscillators_.size(); ++i) {
        configs_[i].framePosition = position;
        oscillators_[i]->setFramePosition(position);
    }
}

void OscillatorStack::setPan(int index, float pan) {
    if (index < 0 || index >= static_cast<int>(configs_.size())) {
        return;
    }
    
    // Clamp pan between -1 and 1
    pan = std::clamp(pan, -1.0f, 1.0f);
    
    // Update the config
    configs_[index].pan = pan;
}

void OscillatorStack::setPhase(int index, float phase) {
    if (index < 0 || index >= static_cast<int>(oscillators_.size())) {
        return;
    }
    
    // Clamp phase between 0 and 1
    phase = std::clamp(phase, 0.0f, 1.0f);
    
    // Update the config
    configs_[index].phase = phase;
    
    // Apply the change
    oscillators_[index]->setPhase(phase);
}

void OscillatorStack::resetAllPhases() {
    for (size_t i = 0; i < oscillators_.size(); ++i) {
        oscillators_[i]->setPhase(configs_[i].phase);
    }
}

void OscillatorStack::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    // Store the wavetable
    wavetable_ = wavetable;
    
    // Update all oscillators
    for (auto& osc : oscillators_) {
        osc->setWavetable(wavetable);
    }
}

void OscillatorStack::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update all oscillators
    for (auto& osc : oscillators_) {
        osc->setSampleRate(sampleRate);
    }
}

float OscillatorStack::generateMonoSample() {
    float sample = 0.0f;
    
    // Mix all oscillators
    for (size_t i = 0; i < oscillators_.size(); ++i) {
        sample += oscillators_[i]->generateSample() * configs_[i].level;
    }
    
    // Normalize if more than one oscillator is active
    if (oscillators_.size() > 1) {
        // Simple normalization by oscillator count
        float normFactor = 1.0f / std::sqrt(static_cast<float>(oscillators_.size()));
        sample *= normFactor;
    }
    
    return sample;
}

void OscillatorStack::generateStereoSample(float& leftOut, float& rightOut) {
    float left = 0.0f;
    float right = 0.0f;
    
    // Mix all oscillators with panning
    for (size_t i = 0; i < oscillators_.size(); ++i) {
        float sample = oscillators_[i]->generateSample() * configs_[i].level;
        
        // Apply panning
        float pan = configs_[i].pan;
        float leftGain = std::cos((pan + 1.0f) * 0.25f * M_PI) * 1.414f; // Equal power panning
        float rightGain = std::sin((pan + 1.0f) * 0.25f * M_PI) * 1.414f;
        
        left += sample * leftGain;
        right += sample * rightGain;
    }
    
    // Normalize if more than one oscillator is active
    if (oscillators_.size() > 1) {
        // Simple normalization by oscillator count
        float normFactor = 1.0f / std::sqrt(static_cast<float>(oscillators_.size()));
        left *= normFactor;
        right *= normFactor;
    }
    
    leftOut = left;
    rightOut = right;
}

OscillatorConfig& OscillatorStack::getConfig(int index) {
    if (index < 0 || index >= static_cast<int>(configs_.size())) {
        throw std::out_of_range("Oscillator index out of range");
    }
    
    return configs_[index];
}

WavetableOscillator* OscillatorStack::getOscillator(int index) {
    if (index < 0 || index >= static_cast<int>(oscillators_.size())) {
        return nullptr;
    }
    
    return oscillators_[index].get();
}

void OscillatorStack::spreadParameter(std::function<void(float, int)> paramFn) {
    size_t count = oscillators_.size();
    if (count <= 1) {
        return; // Nothing to spread with only one oscillator
    }
    
    // Apply the parameter function across all oscillators
    for (size_t i = 0; i < count; ++i) {
        float normalizedPos = static_cast<float>(i) / (count - 1); // 0 to 1
        paramFn(normalizedPos, static_cast<int>(i));
    }
}

void OscillatorStack::applyDetunePreset(int presetType, float maxDetune) {
    size_t count = oscillators_.size();
    if (count <= 1) {
        return; // Nothing to spread with only one oscillator
    }
    
    switch (presetType) {
        case 0: // Even spread
            setDetuneSpread(maxDetune);
            break;
            
        case 1: { // Center-weighted
            // Create detune distribution that's more concentrated in the center
            for (size_t i = 0; i < count; ++i) {
                float normalizedPos = static_cast<float>(i) / (count - 1); // 0 to 1
                // Apply cubic function for center weighting
                float weight = (normalizedPos - 0.5f) * (normalizedPos - 0.5f) * 4.0f;
                float detune = -maxDetune / 2.0f + maxDetune * normalizedPos;
                detune *= weight; // Apply weighting
                setDetune(static_cast<int>(i), detune);
            }
            break;
        }
            
        case 2: { // Alternating
            // Create alternating pattern of detune values
            for (size_t i = 0; i < count; ++i) {
                float normalizedPos = static_cast<float>(i) / (count - 1); // 0 to 1
                float detune = -maxDetune / 2.0f + maxDetune * normalizedPos;
                // Flip sign for every other oscillator
                if (i % 2 == 1) {
                    detune = -detune;
                }
                setDetune(static_cast<int>(i), detune);
            }
            break;
        }
            
        default:
            // Default to even spread
            setDetuneSpread(maxDetune);
            break;
    }
}

void OscillatorStack::configUnison(int count, float detune, float width, float convergence) {
    // Set the oscillator count
    setOscillatorCount(count);
    
    if (count <= 1) {
        return; // No unison with just one oscillator
    }
    
    // Detune spread
    setDetuneSpread(detune);
    
    // Panning spread for stereo width
    spreadParameter([this, width](float pos, int idx) {
        // Map 0-1 to -width to +width
        float pan = (pos - 0.5f) * 2.0f * width;
        setPan(idx, pan);
    });
    
    // Level convergence (center oscillators louder if convergence > 0)
    if (convergence > 0.0f) {
        spreadParameter([this, convergence](float pos, int idx) {
            // Calculate distance from center (0 = center, 1 = edges)
            float centerDist = std::abs(pos - 0.5f) * 2.0f;
            
            // Apply convergence: higher values emphasize center oscillators
            float level = 1.0f - (centerDist * convergence * 0.5f);
            setLevel(idx, level);
        });
    } else {
        // All oscillators at equal level
        for (int i = 0; i < count; ++i) {
            setLevel(i, 1.0f);
        }
    }
    
    // Phase spread for richer sound
    spreadParameter([this](float pos, int idx) {
        setPhase(idx, pos);
    });
}

} // namespace AIMusicHardware