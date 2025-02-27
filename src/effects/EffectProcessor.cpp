#include "../../include/effects/EffectProcessor.h"
#include <algorithm>

namespace AIMusicHardware {

// Base Effect implementation
Effect::Effect(int sampleRate) : sampleRate_(sampleRate) {
}

Effect::~Effect() {
}

void Effect::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

// EffectProcessor implementation
EffectProcessor::EffectProcessor(int sampleRate) 
    : sampleRate_(sampleRate) {
    // Initialize temp buffer with a reasonable size
    tempBuffer_.resize(4096);
}

EffectProcessor::~EffectProcessor() {
}

void EffectProcessor::addEffect(std::unique_ptr<Effect> effect) {
    effects_.push_back(std::move(effect));
}

void EffectProcessor::removeEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void EffectProcessor::clearEffects() {
    effects_.clear();
}

Effect* EffectProcessor::getEffect(size_t index) {
    if (index < effects_.size()) {
        return effects_[index].get();
    }
    return nullptr;
}

size_t EffectProcessor::getNumEffects() const {
    return effects_.size();
}

void EffectProcessor::process(float* buffer, int numFrames) {
    // Process each effect in the chain
    for (auto& effect : effects_) {
        // Make sure temp buffer is large enough
        if (tempBuffer_.size() < static_cast<size_t>(numFrames * 2)) {
            tempBuffer_.resize(numFrames * 2);
        }
        
        // Process effect
        effect->process(buffer, numFrames);
    }
}

void EffectProcessor::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    for (auto& effect : effects_) {
        effect->setSampleRate(sampleRate);
    }
}

} // namespace AIMusicHardware