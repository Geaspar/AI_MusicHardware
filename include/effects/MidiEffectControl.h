#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include "../midi/MidiManager.h"
#include "ReorderableEffectsChain.h"

namespace AIMusicHardware {

/**
 * MidiEffectControl provides MIDI control capabilities for effect parameters.
 * It bridges the MidiManager and ReorderableEffectsChain to allow parameters
 * to be controlled via MIDI CC messages.
 */
class MidiEffectControl : public MidiManager::Listener {
public:
    /**
     * Constructor
     * @param effectsChain The effects chain to control
     * @param midiManager The MIDI manager providing MIDI messages
     */
    MidiEffectControl(ReorderableEffectsChain* effectsChain, MidiManager* midiManager);
    ~MidiEffectControl();

    // MidiManager::Listener implementation
    void parameterChangedViaMidi(const std::string& paramId, float value) override;
    void pitchBendChanged(int channel, float value) override;
    void modWheelChanged(int channel, float value) override;
    void afterTouchChanged(int channel, float value) override;

    /**
     * Map a MIDI CC to an effect parameter
     * @param effectIndex The index of the effect in the chain
     * @param paramName The name of the parameter on the effect
     * @param channel The MIDI channel (0-15)
     * @param cc The MIDI CC number (0-127)
     * @return True if mapping was successful
     */
    bool mapEffectParameter(size_t effectIndex, const std::string& paramName, int channel, int cc);

    /**
     * Remove a MIDI mapping for an effect parameter
     * @param effectIndex The index of the effect in the chain
     * @param paramName The name of the parameter on the effect
     * @return True if a mapping was removed
     */
    bool unmapEffectParameter(size_t effectIndex, const std::string& paramName);

    /**
     * Start MIDI learning for a specific effect parameter
     * @param effectIndex The index of the effect in the chain
     * @param paramName The name of the parameter on the effect
     */
    void startMidiLearn(size_t effectIndex, const std::string& paramName);

    /**
     * Cancel the current MIDI learning session
     */
    void cancelMidiLearn();

    /**
     * Get all current MIDI mappings
     * @return Map of effect parameters to MIDI CCs
     */
    std::map<std::string, std::pair<int, int>> getMidiMappings() const;

    /**
     * Get parameter ID for effect parameter in format "effect{index}_{paramName}"
     * @param effectIndex The index of the effect
     * @param paramName The parameter name
     * @return Parameter ID string
     */
    static std::string getParameterId(size_t effectIndex, const std::string& paramName);

    /**
     * Parse a parameter ID to extract effect index and parameter name
     * @param paramId The parameter ID string
     * @return Pair of effect index and parameter name, or {-1, ""} if invalid
     */
    static std::pair<int, std::string> parseParameterId(const std::string& paramId);

private:
    ReorderableEffectsChain* effectsChain_;
    MidiManager* midiManager_;

    // Map of parameter IDs to effect index and parameter name
    std::map<std::string, std::pair<size_t, std::string>> parameterMap_;

    // Current parameter being learned
    std::string learnParamId_;
};

} // namespace AIMusicHardware