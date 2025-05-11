#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "EffectProcessor.h"

namespace AIMusicHardware {

/**
 * ReorderableEffectsChain provides a flexible effects processing chain 
 * where effects can be enabled/disabled and reordered.
 * Inspired by Vital's approach to effects processing.
 */
class ReorderableEffectsChain {
public:
    ReorderableEffectsChain(int sampleRate = 44100);
    ~ReorderableEffectsChain();
    
    /**
     * Process audio through the entire effects chain
     * @param buffer The audio buffer to process (in-place)
     * @param numFrames The number of frames to process
     */
    void process(float* buffer, int numFrames);
    
    /**
     * Add an effect to the chain
     * @param effect The effect to add (ownership transferred to chain)
     * @param index Where to insert the effect (-1 means at the end)
     * @return Index of the newly added effect
     */
    int addEffect(std::unique_ptr<Effect> effect, int index = -1);
    
    /**
     * Remove an effect from the chain
     * @param index The index of the effect to remove
     */
    void removeEffect(size_t index);
    
    /**
     * Move an effect to a new position in the chain
     * @param fromIndex Current index of the effect
     * @param toIndex New index for the effect
     */
    void moveEffect(size_t fromIndex, size_t toIndex);
    
    /**
     * Enable or disable an effect
     * @param index The index of the effect
     * @param enabled Whether the effect should be enabled
     */
    void setEffectEnabled(size_t index, bool enabled);
    
    /**
     * Check if an effect is enabled
     * @param index The index of the effect
     * @return True if the effect is enabled, false otherwise
     */
    bool isEffectEnabled(size_t index) const;
    
    /**
     * Get a pointer to an effect in the chain
     * @param index The index of the effect
     * @return Pointer to the effect, or nullptr if index is invalid
     */
    Effect* getEffect(size_t index);
    
    /**
     * Get the number of effects in the chain
     * @return Number of effects
     */
    size_t getNumEffects() const;
    
    /**
     * Clear all effects from the chain
     */
    void clearEffects();
    
    /**
     * Set the sample rate for all effects
     * @param sampleRate The new sample rate
     */
    void setSampleRate(int sampleRate);
    
    /**
     * Create a new effect of the specified type
     * @param type The type of effect to create
     * @return A unique_ptr to the new effect, or nullptr if type is unknown
     */
    std::unique_ptr<Effect> createEffect(const std::string& type);
    
    /**
     * Get the effect type as a string
     * @param index The index of the effect
     * @return The type of the effect as a string, or empty string if index is invalid
     */
    std::string getEffectType(size_t index) const;
    
private:
    struct EffectInfo {
        std::unique_ptr<Effect> effect;
        std::string type;
        bool enabled;
    };
    
    std::vector<EffectInfo> effects_;
    int sampleRate_;
    std::vector<float> tempBuffer_;
};

} // namespace AIMusicHardware