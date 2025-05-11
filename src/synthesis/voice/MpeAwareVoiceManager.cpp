#include "../../../include/synthesis/voice/MpeAwareVoiceManager.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

MpeAwareVoiceManager::MpeAwareVoiceManager(int sampleRate, int maxVoices, const MpeConfiguration& mpeConfig)
    : VoiceManager(sampleRate, maxVoices),
      mpeConfig_(&mpeConfig),
      lowerZonePitchBendRange_(48.0f),
      upperZonePitchBendRange_(48.0f) {

    // We'll rely on the base class to create voices, but we'll override createVoice()
    // to ensure they're MPE voices
    setMaxVoices(maxVoices);
}

MpeAwareVoiceManager::~MpeAwareVoiceManager() {
    // Clean up any channel-to-voice mappings
    channelToVoiceMap_.clear();
}

void MpeAwareVoiceManager::noteOnWithExpression(int midiNote, float velocity, int channel,
                                              float pitchBend, float timbre, float pressure) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Skip if not a valid member channel
    if (!isMemberChannel(channel)) {
        return;
    }

    // First, call standard note on which handles voice allocation
    VoiceManager::noteOn(midiNote, velocity, channel);

    // Find the newly allocated voice
    Voice* baseVoice = findVoiceForMpeNote(midiNote, channel);
    if (!baseVoice) {
        return;  // Voice allocation failed
    }

    // Try to cast to MPE voice
    MpeVoice* mpeVoice = castToMpeVoice(baseVoice);
    if (!mpeVoice) {
        return;  // Not an MPE voice
    }

    // Add to channel map for quick lookups
    channelToVoiceMap_[channel] = mpeVoice;

    // Apply all expression parameters
    float pitchBendSemitones = pitchBend * (isLowerZoneChannel(channel) ?
                                            lowerZonePitchBendRange_ :
                                            upperZonePitchBendRange_);

    mpeVoice->updateExpression(pitchBendSemitones, timbre, pressure);
}

void MpeAwareVoiceManager::noteOn(int midiNote, float velocity, int channel) {
    // For MPE channels, use standard noteOn with default expression values
    if (isMemberChannel(channel)) {
        noteOnWithExpression(midiNote, velocity, channel, 0.0f, 0.5f, 0.0f);
    } else {
        // For non-MPE channels, use standard behavior
        VoiceManager::noteOn(midiNote, velocity, channel);
    }
}

void MpeAwareVoiceManager::noteOff(int midiNote, int channel) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Call standard noteOff
    VoiceManager::noteOff(midiNote, channel);

    // If this was an MPE channel, remove it from our channel map
    if (isMemberChannel(channel)) {
        channelToVoiceMap_.erase(channel);
    }
}

void MpeAwareVoiceManager::updateNotePitchBend(int channel, float pitchBend) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Convert normalized pitch bend (-1.0 to 1.0) to semitones based on zone
    float pitchBendRange = isLowerZoneChannel(channel) ?
                          lowerZonePitchBendRange_ :
                          upperZonePitchBendRange_;

    float pitchBendSemitones = pitchBend * pitchBendRange;

    // Find the voice for this channel
    MpeVoice* voice = findVoiceByChannel(channel);
    if (voice) {
        voice->setPitchBend(pitchBendSemitones);
    }
}

void MpeAwareVoiceManager::updateNoteTimbre(int channel, float timbre) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Find the voice for this channel
    MpeVoice* voice = findVoiceByChannel(channel);
    if (voice) {
        voice->setTimbre(timbre);
    }
}

void MpeAwareVoiceManager::updateNotePressure(int channel, float pressure) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Find the voice for this channel
    MpeVoice* voice = findVoiceByChannel(channel);
    if (voice) {
        voice->setPressure(pressure);
    }
}

void MpeAwareVoiceManager::updateNoteExpression(int channel, float pitchBend, float timbre, float pressure) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    // Find the voice for this channel
    MpeVoice* voice = findVoiceByChannel(channel);
    if (voice) {
        // Convert normalized pitch bend to semitones based on zone
        float pitchBendRange = isLowerZoneChannel(channel) ?
                              lowerZonePitchBendRange_ :
                              upperZonePitchBendRange_;

        float pitchBendSemitones = pitchBend * pitchBendRange;

        // Update all expression parameters at once
        voice->updateExpression(pitchBendSemitones, timbre, pressure);
    }
}

void MpeAwareVoiceManager::setMpeConfiguration(const MpeConfiguration& mpeConfig) {
    std::lock_guard<std::mutex> lock(mpeMutex_);
    mpeConfig_ = &mpeConfig;

    // Update pitch bend ranges from config
    if (mpeConfig_->getLowerZone().active) {
        lowerZonePitchBendRange_ = static_cast<float>(mpeConfig_->getLowerZone().pitchBendRange);
    }

    if (mpeConfig_->getUpperZone().active) {
        upperZonePitchBendRange_ = static_cast<float>(mpeConfig_->getUpperZone().pitchBendRange);
    }
}

void MpeAwareVoiceManager::setMpePitchBendRange(float semitones, bool lowerZone) {
    std::lock_guard<std::mutex> lock(mpeMutex_);

    if (lowerZone) {
        lowerZonePitchBendRange_ = semitones;
    } else {
        upperZonePitchBendRange_ = semitones;
    }
}

std::unique_ptr<Voice> MpeAwareVoiceManager::createVoice() {
    // Create an MPE-compatible voice
    return std::make_unique<MpeVoice>(getSampleRate());
}

Voice* MpeAwareVoiceManager::findVoiceForMpeNote(int midiNote, int channel) {
    // Create a unique key for this note and channel
    int noteKey = (channel << 16) | midiNote;

    // Iterate over all voices to find a matching one
    for (int i = 0; i < getMaxVoices(); ++i) {
        Voice* voice = getVoice(i);
        if (voice && voice->isActive() && voice->getMidiNote() == midiNote && voice->getChannel() == channel) {
            return voice;
        }
    }

    return nullptr;
}

MpeVoice* MpeAwareVoiceManager::findVoiceByChannel(int channel) {
    auto it = channelToVoiceMap_.find(channel);
    return (it != channelToVoiceMap_.end()) ? it->second : nullptr;
}

bool MpeAwareVoiceManager::isMasterChannel(int channel) const {
    if (!mpeConfig_) return false;

    const auto& lowerZone = mpeConfig_->getLowerZone();
    const auto& upperZone = mpeConfig_->getUpperZone();

    return (lowerZone.active && channel == lowerZone.masterChannel) ||
           (upperZone.active && channel == upperZone.masterChannel);
}

bool MpeAwareVoiceManager::isMemberChannel(int channel) const {
    if (!mpeConfig_) return false;

    const auto& lowerZone = mpeConfig_->getLowerZone();
    const auto& upperZone = mpeConfig_->getUpperZone();

    bool isLowerMember = lowerZone.active &&
                         channel >= lowerZone.startMemberChannel &&
                         channel <= lowerZone.endMemberChannel;

    bool isUpperMember = upperZone.active &&
                         channel >= upperZone.startMemberChannel &&
                         channel <= upperZone.endMemberChannel;

    return isLowerMember || isUpperMember;
}

bool MpeAwareVoiceManager::isLowerZoneChannel(int channel) const {
    if (!mpeConfig_) return false;

    const auto& lowerZone = mpeConfig_->getLowerZone();

    return lowerZone.active &&
           ((channel == lowerZone.masterChannel) ||
            (channel >= lowerZone.startMemberChannel &&
             channel <= lowerZone.endMemberChannel));
}

MpeVoice* MpeAwareVoiceManager::castToMpeVoice(Voice* voice) {
    if (!voice) return nullptr;

    // Use dynamic_cast to safely cast to MpeVoice
    return dynamic_cast<MpeVoice*>(voice);
}

} // namespace AIMusicHardware