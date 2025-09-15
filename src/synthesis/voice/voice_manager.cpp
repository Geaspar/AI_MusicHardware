#include "../../../include/synthesis/voice/voice_manager.h"
#include "../../../include/synthesis/wavetable/wavetable.h"
#include "../../../include/synthesis/voice/realtime_wavetable_voice_v2.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

namespace AIMusicHardware {

// VoiceManager implementation
VoiceManager::VoiceManager(int sampleRate, int maxVoices)
    : sampleRate_(sampleRate),
      maxVoices_(maxVoices),
      stealMode_(StealMode::Oldest),
      pitchBendRange_(2.0f) {
    
    // Create initial voices
    for (int i = 0; i < maxVoices_; ++i) {
        voices_.push_back(createVoice());
    }
    
    // Create a default wavetable
    currentWavetable_ = std::make_shared<Wavetable>();
    currentWavetable_->initBasicWaveforms();
    
    // Assign wavetable to all voices
    for (auto& voice : voices_) {
        voice->setWavetable(currentWavetable_);
    }
    
    // Initialize default channel state for channel 0
    channelStates_[0] = ChannelState{};

    // Prepare a simple default SpectralTable for Hybrid V2 fallback
    defaultSpectralTable_ = std::make_unique<SpectralTable>();
    SpectralFrame frame;
    frame.fftSize = 2048;
    frame.sampleRate = sampleRate_;
    frame.bins.resize(static_cast<size_t>(frame.fftSize / 2 + 1));
    if (frame.bins.size() > 1) {
        frame.bins[1].magnitude = 1.0f; // sine fundamental
        frame.bins[1].phase = 0.0f;
    }
    frame.normalizationRms = 0.2f;
    defaultSpectralTable_->frames.push_back(frame);
}

VoiceManager::~VoiceManager() {
}

void VoiceManager::noteOn(int midiNote, float velocity, int channel) {
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        channelStates_[channel] = ChannelState{};
    }
    
    // Create a unique key for this note and channel
    int noteKey = (channel << 16) | midiNote;
    
    // Check if this note is already playing
    Voice* voice = findVoiceForNote(midiNote, channel);
    
    // If not playing, find an unused voice or steal one
    if (!voice) {
        // First look for an inactive voice
        for (auto& v : voices_) {
            if (!v->isActive()) {
                voice = v.get();
                break;
            }
        }
        
        // If all voices are in use, use voice stealing
        if (!voice) {
            voice = findVoiceToSteal();
        }
    }
    
    // Trigger the voice with this note
    if (voice) {
        // If the voice is currently active (voice stealing), apply a quick fade
        if (voice->isActive()) {
            // Apply a one-shot quick release to avoid clicks without altering base release
            if (auto* envelope = voice->getEnvelope()) {
                float qr = std::max(0.0f, quickReleaseOverrideSeconds_);
                if (fastRetriggerEnabled_) {
                    // Shorten quick release dramatically for fast retriggers to re-arm envelope quickly
                    qr = std::min(qr, 0.003f); // ~3ms
                }
                if (qr > 0.0f) envelope->setReleaseOverrideOnce(qr);
                voice->noteOff();
            }
        }
        
        voice->setChannel(channel);
        voice->noteOn(midiNote, velocity);
        
        // Apply any active pitch bend for this channel
        float pitchBendSemitones = channelStates_[channel].pitchBendValue * pitchBendRange_;
        voice->setPitchBend(pitchBendSemitones);
        
        activeNotes_[noteKey] = voice;
    }
}

void VoiceManager::noteOff(int midiNote, int channel) {
    // Create a unique key for this note and channel
    int noteKey = (channel << 16) | midiNote;
    
    // Find the voice playing this note on this channel
    auto it = activeNotes_.find(noteKey);
    if (it != activeNotes_.end()) {
        // Check for sustain pedal
        if (channelStates_[channel].sustainPedalDown) {
            // If sustain is active, mark the note as sustained but don't release it
            channelStates_[channel].sustainedNotes[midiNote] = true;
        } else {
            // Otherwise, release the note normally
            it->second->noteOff();
            // Don't remove from activeNotes_ immediately - let the cleanup process handle it
            // when the voice is completely finished (in Finished state)
        }
    }
}

void VoiceManager::allNotesOff(int channel) {
    if (channel < 0) {
        // Turn off all notes on all channels
        for (auto& voice : voices_) {
            voice->noteOff();
        }
        activeNotes_.clear();
        
        // Clear all sustained notes too
        for (auto& channelState : channelStates_) {
            channelState.second.sustainedNotes.clear();
        }
    } else {
        // Turn off notes for a specific channel only
        
        // First collect keys to remove to avoid modifying during iteration
        std::vector<int> keysToRemove;
        
        for (auto& pair : activeNotes_) {
            int noteChannel = pair.first >> 16;
            if (noteChannel == channel) {
                pair.second->noteOff();
                keysToRemove.push_back(pair.first);
            }
        }
        
        // Now remove the notes
        for (int key : keysToRemove) {
            activeNotes_.erase(key);
        }
        
        // Clear sustained notes for this channel
        if (channelStates_.find(channel) != channelStates_.end()) {
            channelStates_[channel].sustainedNotes.clear();
        }
    }
}

void VoiceManager::process(float* buffer, int numFrames) {
    // Clear output buffer
    std::fill(buffer, buffer + numFrames * 2, 0.0f);
    
    // Count active voices for dynamic gain adjustment
    int activeVoiceCount = 0;
    
    // Process each voice
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            activeVoiceCount++;
            voice->process(buffer, numFrames);
        }
    }
    
    // Gentle polyphony gain normalization to prevent clipping when many voices overlap
    {
        int loudCount = 0;
        for (auto& voice : voices_) {
            if (voice->isActive() && voice->getCurrentAmplitude() > 0.05f) {
                loudCount++;
            }
        }
        if (loudCount > 1) {
            float gain = 1.0f / std::sqrt(static_cast<float>(loudCount));
            for (int i = 0; i < numFrames * 2; ++i) {
                buffer[i] *= gain;
            }
        }
    }
    
    // Clean up voices that have finished their release phase
    for (auto it = activeNotes_.begin(); it != activeNotes_.end();) {
        if (it->second->getState() == Voice::State::Finished) {
            it = activeNotes_.erase(it);
        } else {
            ++it;
        }
    }
}

void VoiceManager::setMaxVoices(int maxVoices) {
    maxVoices_ = std::max(1, maxVoices);
    
    // Add voices if needed
    while (static_cast<int>(voices_.size()) < maxVoices_) {
        voices_.push_back(createVoice());
    }
    
    // Or remove excess voices
    while (static_cast<int>(voices_.size()) > maxVoices_) {
        // Find an inactive voice to remove
        for (auto it = voices_.begin(); it != voices_.end(); ++it) {
            if (!(*it)->isActive()) {
                voices_.erase(it);
                break;
            }
        }
        
        // If all voices are active, just remove the last one
        if (static_cast<int>(voices_.size()) > maxVoices_) {
            voices_.pop_back();
        }
    }
}

void VoiceManager::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update all voices
    for (auto& voice : voices_) {
        voice->setSampleRate(sampleRate);
    }
}

void VoiceManager::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    if (wavetable) {
        currentWavetable_ = wavetable;
        
        // Update all voices
        for (auto& voice : voices_) {
            voice->setWavetable(wavetable);
        }
    }
}

void VoiceManager::rebuildVoices() {
    // Stop and clear any active notes to avoid stale pointers
    for (auto& pair : activeNotes_) {
        if (pair.second) {
            pair.second->noteOff();
        }
    }
    activeNotes_.clear();
    // Clear sustained notes per channel
    for (auto& kv : channelStates_) {
        kv.second.sustainedNotes.clear();
    }
    
    // Recreate voices respecting current flags/services, keep count the same
    int count = static_cast<int>(voices_.size());
    voices_.clear();
    voices_.reserve(count);
    for (int i = 0; i < count; ++i) {
        auto v = createVoice();
        if (currentWavetable_) v->setWavetable(currentWavetable_);
        voices_.push_back(std::move(v));
    }
}

Voice* VoiceManager::findVoiceToSteal() {
    switch (stealMode_) {
        case StealMode::Oldest: {
            // Find the oldest voice that's not in release
            Voice* oldestVoice = nullptr;
            int oldestAge = -1;
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased() && voice->getAge() > oldestAge) {
                    oldestAge = voice->getAge();
                    oldestVoice = voice.get();
                }
            }
            
            // If no active non-released voice found, use any voice
            if (!oldestVoice && !voices_.empty()) {
                oldestVoice = voices_[0].get();
            }
            
            return oldestVoice;
        }
        
        case StealMode::Quietest: {
            // Find the quietest voice that's not in release
            Voice* quietestVoice = nullptr;
            float lowestAmp = 2.0f; // Higher than max amplitude (1.0)
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased()) {
                    float amp = voice->getCurrentAmplitude();
                    if (amp < lowestAmp) {
                        lowestAmp = amp;
                        quietestVoice = voice.get();
                    }
                }
            }
            
            // If no active non-released voice found, use any voice
            if (!quietestVoice && !voices_.empty()) {
                quietestVoice = voices_[0].get();
            }
            
            return quietestVoice;
        }
        
        case StealMode::Random: {
            // Choose a random voice that's not in release
            std::vector<Voice*> candidates;
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased()) {
                    candidates.push_back(voice.get());
                }
            }
            
            if (!candidates.empty()) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, static_cast<int>(candidates.size()) - 1);
                return candidates[dist(gen)];
            }
            
            // If no candidates, use the first voice
            if (!voices_.empty()) {
                return voices_[0].get();
            }
            
            return nullptr;
        }
        
        default:
            return !voices_.empty() ? voices_[0].get() : nullptr;
    }
}

void VoiceManager::applyHybridMorph(float morph01) {
    if (!hybridEnabled_) return;
    // COARSE: set morph on any V2 voice we have (later we propagate per-voice)
    for (auto& v : voices_) {
        if (auto* v2 = dynamic_cast<RealtimeWavetableVoiceV2*>(v.get())) {
            v2->setMorph01(std::clamp(morph01, 0.0f, 1.0f));
        }
    }
}

Voice* VoiceManager::findVoiceForNote(int midiNote, int channel) {
    // Create a unique key for this note and channel
    int noteKey = (channel << 16) | midiNote;
    
    auto it = activeNotes_.find(noteKey);
    return (it != activeNotes_.end()) ? it->second : nullptr;
}

void VoiceManager::sustainOn(int channel) {
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        channelStates_[channel] = ChannelState{};
    }
    
    // Activate sustain pedal
    channelStates_[channel].sustainPedalDown = true;
}

void VoiceManager::sustainOff(int channel) {
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        return; // No active channel state
    }
    
    // Deactivate sustain pedal
    channelStates_[channel].sustainPedalDown = false;
    
    // Release all sustained notes
    auto& sustainedNotes = channelStates_[channel].sustainedNotes;
    
    // Process all sustained notes for this channel
    for (auto& notePair : sustainedNotes) {
        int midiNote = notePair.first;
        int noteKey = (channel << 16) | midiNote;
        
        // Find and release the voice
        auto it = activeNotes_.find(noteKey);
        if (it != activeNotes_.end()) {
            it->second->noteOff();
            // Don't remove from activeNotes_ immediately - let the cleanup process handle it
        }
    }
    
    // Clear the sustained notes list
    sustainedNotes.clear();
}

void VoiceManager::setPitchBend(float value, int channel) {
    // Normalize value to range -1.0 to 1.0
    float normalizedValue = std::clamp(value, -1.0f, 1.0f);
    
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        channelStates_[channel] = ChannelState{};
    }
    
    // Store pitch bend value
    channelStates_[channel].pitchBendValue = normalizedValue;
    
    // Calculate bend in semitones
    float semitones = normalizedValue * pitchBendRange_;
    
    // Apply to all active voices for this channel
    for (auto& pair : activeNotes_) {
        int noteChannel = pair.first >> 16;
        if (noteChannel == channel) {
            pair.second->setPitchBend(semitones);
        }
    }
}

void VoiceManager::setAftertouch(int note, float pressure, int channel) {
    // Normalize pressure to range 0.0 to 1.0
    float normalizedPressure = std::clamp(pressure, 0.0f, 1.0f);
    
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        channelStates_[channel] = ChannelState{};
    }
    
    // Store aftertouch value
    channelStates_[channel].noteAftertouch[note] = normalizedPressure;
    
    // Apply to the specific voice
    int noteKey = (channel << 16) | note;
    auto it = activeNotes_.find(noteKey);
    if (it != activeNotes_.end()) {
        it->second->setPressure(normalizedPressure);
    }
}

void VoiceManager::setChannelPressure(float pressure, int channel) {
    // Normalize pressure to range 0.0 to 1.0
    float normalizedPressure = std::clamp(pressure, 0.0f, 1.0f);
    
    // Ensure we have a channel state for this channel
    if (channelStates_.find(channel) == channelStates_.end()) {
        channelStates_[channel] = ChannelState{};
    }
    
    // Store channel pressure value
    channelStates_[channel].channelPressure = normalizedPressure;
    
    // Apply to all active voices for this channel
    for (auto& pair : activeNotes_) {
        int noteChannel = pair.first >> 16;
        if (noteChannel == channel) {
            pair.second->setPressure(normalizedPressure);
        }
    }
}

void VoiceManager::resetAllControllers() {
    // Reset all controllers for all channels
    for (auto& channelPair : channelStates_) {
        ChannelState& state = channelPair.second;
        
        // Reset pitch bend
        state.pitchBendValue = 0.0f;
        state.channelPressure = 0.0f;
        
        // Reset all note-specific aftertouch values
        state.noteAftertouch.clear();
        
        // Don't release sustained notes or turn off sustain - that's a separate control
    }
    
    // Apply zero pitch bend to all active voices
    for (auto& pair : activeNotes_) {
        pair.second->setPitchBend(0.0f);
        pair.second->setPressure(0.0f);
    }
}

std::unique_ptr<Voice> VoiceManager::createVoice() {
    if (hybridEnabled_ && spectralCache_ && spectralWorkerPtr_ && *spectralWorkerPtr_) {
        auto v2 = std::make_unique<RealtimeWavetableVoiceV2>(spectralCache_, *spectralWorkerPtr_, sampleRate_);
        // Provide spectral table: prefer shared (from synth), else fallback default sine
        if (spectralTableShared_) v2->setSpectralTable(spectralTableShared_.get());
        else v2->setSpectralTable(defaultSpectralTable_.get());
        v2->setMinPhaseEnabled(hybridMinPhase_);
        return v2;
    } else {
        auto voice = std::make_unique<Voice>(sampleRate_);
        if (currentWavetable_) {
            voice->setWavetable(currentWavetable_);
        }
        return voice;
    }
}

} // namespace AIMusicHardware
