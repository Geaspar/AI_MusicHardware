#include "../../include/effects/ReorderableEffectsChain.h"
#include "../../include/effects/AllEffects.h"
#include <iostream>

namespace AIMusicHardware {

ReorderableEffectsChain::ReorderableEffectsChain(int sampleRate)
    : sampleRate_(sampleRate) {
    // Initialize with a reasonable buffer size
    tempBuffer_.reserve(4096 * 2); // Stereo buffer
}

ReorderableEffectsChain::~ReorderableEffectsChain() {
    // Effects are automatically cleaned up by unique_ptr
}

void ReorderableEffectsChain::process(float* buffer, int numFrames) {
    // Process each enabled effect in the chain
    for (auto& effectInfo : effects_) {
        if (effectInfo.enabled && effectInfo.effect) {
            try {
                // Process the effect
                effectInfo.effect->process(buffer, numFrames);
            } catch (const std::exception& e) {
                // Log the error but continue processing
                std::cerr << "Error processing effect: " << e.what() << std::endl;
            }
        }
    }
}

int ReorderableEffectsChain::addEffect(std::unique_ptr<Effect> effect, int index) {
    if (!effect)
        return -1;
    
    // Set the sample rate for the new effect
    effect->setSampleRate(sampleRate_);
    
    // Determine the effect type
    std::string effectType = effect->getName();
    
    // Create the effect info
    EffectInfo info = {
        std::move(effect),
        effectType,
        true // Enable by default
    };
    
    // Insert at the specified position
    if (index < 0 || index >= static_cast<int>(effects_.size())) {
        // Add to the end
        effects_.push_back(std::move(info));
        return static_cast<int>(effects_.size()) - 1;
    } else {
        // Insert at the specified position
        effects_.insert(effects_.begin() + index, std::move(info));
        return index;
    }
}

void ReorderableEffectsChain::removeEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void ReorderableEffectsChain::moveEffect(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= effects_.size() || toIndex >= effects_.size() || fromIndex == toIndex)
        return;
    
    // Create a copy of the effect info
    EffectInfo info = std::move(effects_[fromIndex]);
    
    // Remove from the current position
    effects_.erase(effects_.begin() + fromIndex);
    
    // Adjust the target index if needed
    if (fromIndex < toIndex)
        toIndex--;
    
    // Insert at the new position
    effects_.insert(effects_.begin() + toIndex, std::move(info));
}

void ReorderableEffectsChain::setEffectEnabled(size_t index, bool enabled) {
    if (index < effects_.size()) {
        effects_[index].enabled = enabled;
    }
}

bool ReorderableEffectsChain::isEffectEnabled(size_t index) const {
    if (index < effects_.size()) {
        return effects_[index].enabled;
    }
    return false;
}

Effect* ReorderableEffectsChain::getEffect(size_t index) {
    if (index < effects_.size()) {
        return effects_[index].effect.get();
    }
    return nullptr;
}

size_t ReorderableEffectsChain::getNumEffects() const {
    return effects_.size();
}

void ReorderableEffectsChain::clearEffects() {
    effects_.clear();
}

void ReorderableEffectsChain::setSampleRate(int sampleRate) {
    if (sampleRate_ == sampleRate)
        return;
    
    sampleRate_ = sampleRate;
    
    // Update all effects
    for (auto& effectInfo : effects_) {
        if (effectInfo.effect) {
            effectInfo.effect->setSampleRate(sampleRate);
        }
    }
}

std::unique_ptr<Effect> ReorderableEffectsChain::createEffect(const std::string& type) {
    return createEffectComplete(type, sampleRate_);
}

std::string ReorderableEffectsChain::getEffectType(size_t index) const {
    if (index < effects_.size()) {
        return effects_[index].type;
    }
    return "";
}

} // namespace AIMusicHardware