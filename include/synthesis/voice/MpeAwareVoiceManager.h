#pragma once

#include "voice_manager.h"
#include "MpeVoice.h"
#include "../../../include/midi/MpeConfiguration.h"
#include "../../../include/midi/MpeChannelAllocator.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace AIMusicHardware {

/**
 * @class MpeAwareVoiceManager
 * @brief Specialized voice manager for MPE with per-note expression
 *
 * Extends the standard VoiceManager with MPE capabilities, handling the three
 * expression dimensions (pitch bend, timbre/CC74, pressure) on a per-note basis
 * and managing voice allocation according to MPE rules.
 */
class MpeAwareVoiceManager : public VoiceManager {
public:
    /**
     * @brief Constructor
     *
     * @param sampleRate Audio sample rate
     * @param maxVoices Maximum number of voices
     * @param mpeConfig Reference to MPE configuration
     */
    MpeAwareVoiceManager(int sampleRate = 44100, int maxVoices = 16, const MpeConfiguration& mpeConfig = MpeConfiguration());

    /**
     * @brief Destructor
     */
    ~MpeAwareVoiceManager();

    /**
     * @brief Specialized note on handling for MPE with expression values
     *
     * @param midiNote MIDI note number
     * @param velocity Note velocity (0.0 to 1.0)
     * @param channel MPE member channel
     * @param pitchBend Initial pitch bend value (-1.0 to 1.0)
     * @param timbre Initial timbre value (0.0 to 1.0)
     * @param pressure Initial pressure value (0.0 to 1.0)
     */
    void noteOnWithExpression(int midiNote, float velocity, int channel,
                             float pitchBend, float timbre, float pressure);

    /**
     * @brief Override standard note on to handle MPE channels properly
     *
     * @param midiNote MIDI note number
     * @param velocity Note velocity (0.0 to 1.0)
     * @param channel MIDI channel
     */
    void noteOn(int midiNote, float velocity, int channel = 0);

    /**
     * @brief Override standard note off to handle MPE channels properly
     *
     * @param midiNote MIDI note number
     * @param channel MIDI channel
     */
    void noteOff(int midiNote, int channel = 0);

    /**
     * @brief Update pitch bend for a specific MPE channel
     *
     * @param channel MPE member channel
     * @param pitchBend Pitch bend value (-1.0 to 1.0)
     */
    void updateNotePitchBend(int channel, float pitchBend);

    /**
     * @brief Update timbre for a specific MPE channel
     *
     * @param channel MPE member channel
     * @param timbre Timbre value (0.0 to 1.0)
     */
    void updateNoteTimbre(int channel, float timbre);

    /**
     * @brief Update pressure for a specific MPE channel
     *
     * @param channel MPE member channel
     * @param pressure Pressure value (0.0 to 1.0)
     */
    void updateNotePressure(int channel, float pressure);

    /**
     * @brief Update all expression values for a specific MPE channel
     *
     * @param channel MPE member channel
     * @param pitchBend Pitch bend value (-1.0 to 1.0)
     * @param timbre Timbre value (0.0 to 1.0)
     * @param pressure Pressure value (0.0 to 1.0)
     */
    void updateNoteExpression(int channel, float pitchBend, float timbre, float pressure);

    /**
     * @brief Set the MPE configuration reference
     *
     * @param mpeConfig Reference to MPE configuration
     */
    void setMpeConfiguration(const MpeConfiguration& mpeConfig);

    /**
     * @brief Set the pitch bend range for MPE
     *
     * @param semitones Pitch bend range in semitones
     * @param lowerZone Whether to set for lower zone (true) or upper zone (false)
     */
    void setMpePitchBendRange(float semitones, bool lowerZone);

    /**
     * @brief Find a voice based on its MPE channel
     *
     * @param channel MPE member channel
     * @return Pointer to the voice or nullptr if not found
     */
    MpeVoice* findVoiceByChannel(int channel);

protected:
    /**
     * @brief Create specialized MPE voices
     *
     * @return A new MPE voice
     */
    std::unique_ptr<Voice> createVoice();

private:
    // Reference to MPE configuration
    const MpeConfiguration* mpeConfig_;

    // Channel to voice mapping for quick lookups
    std::unordered_map<int, MpeVoice*> channelToVoiceMap_;

    // Pitch bend range for each zone
    float lowerZonePitchBendRange_ = 48.0f;  // Default ±48 semitones for MPE
    float upperZonePitchBendRange_ = 48.0f;

    // Thread safety
    std::mutex mpeMutex_;

    // Helper to determine if a channel is an MPE master channel
    bool isMasterChannel(int channel) const;

    // Helper to determine if a channel is an MPE member channel
    bool isMemberChannel(int channel) const;

    // Helper to determine which zone a channel belongs to
    bool isLowerZoneChannel(int channel) const;

    // Helper to find a voice for a specific note
    Voice* findVoiceForMpeNote(int midiNote, int channel);

    // Helper for safer casting
    MpeVoice* castToMpeVoice(Voice* voice);
};

} // namespace AIMusicHardware