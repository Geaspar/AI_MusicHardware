#include "../../include/effects/EffectProcessor.h"
#include <algorithm>

namespace AIMusicHardware {

// Base Effect implementation
Effect::Effect(int sampleRate) : sampleRate_(sampleRate) {
}

// Destructor is already marked virtual in the header
Effect::~Effect() {
    // Virtual destructor implementation
}

void Effect::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

// EffectProcessor implementation
EffectProcessor::EffectProcessor(int sampleRate) 
    : sampleRate_(sampleRate) {
    // Temp buffer will be initialized in initialize()
}

EffectProcessor::~EffectProcessor() {
}

bool EffectProcessor::initialize() {
    try {
        // Initialize temp buffer with a reasonable size, will be resized as needed
        tempBuffer_.reserve(4096 * 2); // Stereo buffer with reasonable initial capacity
        return true;
    } catch (const std::exception& e) {
        // Handle any exceptions during initialization
        return false;
    }
}

void EffectProcessor::addEffect(std::unique_ptr<Effect> effect) {
    if (effect) {
        effect->setSampleRate(sampleRate_); // Ensure the effect has the correct sample rate
        effects_.push_back(std::move(effect));
    }
}

void EffectProcessor::removeEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void EffectProcessor::clearEffects() {
    effects_.clear();
    effects_.shrink_to_fit(); // Release unused memory
}

Effect* EffectProcessor::getEffect(size_t index) {
    if (index >= effects_.size()) {
        return nullptr;
    }
    return effects_[index].get();
}

size_t EffectProcessor::getNumEffects() const {
    return effects_.size();
}

void EffectProcessor::process(float* buffer, int numFrames) {
    // Safety check for invalid input
    if (!buffer || numFrames <= 0) {
        return;
    }
    
    // Process each effect in the chain
    for (auto& effect : effects_) {
        if (!effect) {
            continue; // Skip null effects
        }
        
        // Make sure temp buffer is large enough - use reserve instead of resize for efficiency
        if (tempBuffer_.capacity() < static_cast<size_t>(numFrames * 2)) {
            tempBuffer_.reserve(numFrames * 2);
        }
        
        // Process effect
        try {
            effect->process(buffer, numFrames);
        } catch (const std::exception& e) {
            // Log or handle the exception
            // We continue processing to avoid interrupting the audio chain
        }
    }
}

void EffectProcessor::setSampleRate(int sampleRate) {
    // Skip if the sample rate hasn't changed
    if (sampleRate_ == sampleRate) {
        return;
    }
    
    sampleRate_ = sampleRate;
    for (auto& effect : effects_) {
        if (effect) {
            effect->setSampleRate(sampleRate);
        }
    }
}

} // namespace AIMusicHardware