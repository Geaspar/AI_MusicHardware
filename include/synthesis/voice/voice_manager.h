#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include "voice.h"
#include "../wavetable/wavetable.h"  // For Wavetable class
#include "synthesis/wavetable/hybrid_wavetable_cache.h" // for cache/worker refs
#include "synthesis/wavetable/hybrid_wavetable.h"

namespace AIMusicHardware {

/**
 * Voice allocation and management system.
 */
class VoiceManager {
public:
    enum class StealMode {
        Oldest,     // Steal the oldest playing voice
        Quietest,   // Steal the quietest voice
        Random      // Steal a random voice
    };
    
    VoiceManager(int sampleRate = 44100, int maxVoices = 16);
    ~VoiceManager();
    
    // Voice control
    void noteOn(int midiNote, float velocity, int channel = 0);
    void noteOff(int midiNote, int channel = 0);
    void allNotesOff(int channel = -1); // -1 for all channels
    
    // MIDI-specific control methods
    void sustainOn(int channel = 0);
    void sustainOff(int channel = 0);
    void setPitchBend(float value, int channel = 0);  // value range: -1.0 to 1.0
    void setAftertouch(int note, float pressure, int channel = 0);
    void setChannelPressure(float pressure, int channel = 0);
    void resetAllControllers();
    
    // Voice processing
    void process(float* buffer, int numFrames);
    
    // Voice allocation settings
    void setMaxVoices(int maxVoices);
    int getMaxVoices() const { return maxVoices_; }
    void setStealMode(StealMode mode) { stealMode_ = mode; }
    StealMode getStealMode() const { return stealMode_; }
    
    // Sample rate control
    virtual void setSampleRate(int sampleRate);
    int getSampleRate() const { return sampleRate_; }

    // Shared wavetable management
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    // Recreate voice objects with current settings (sample rate, wavetable, hybrid flag)
    void rebuildVoices();

    // Hybrid spectral wavetable scaffolding (off by default)
    void enableHybridWavetable(bool enable) { hybridEnabled_ = enable; }
    void setSpectralServices(std::shared_ptr<SpectralWavetableCache> cache,
                             std::unique_ptr<SpectralRenderWorker>* workerPtr) {
        spectralCache_ = std::move(cache);
        spectralWorkerPtr_ = workerPtr; // non-owning
    }

    void setHybridSpectralTable(std::shared_ptr<SpectralTable> table) { spectralTableShared_ = std::move(table); }
    void applyHybridMorph(float morph01);

    // Pitch bend range control (in semitones, default = 2.0)
    void setPitchBendRange(float semitones) { pitchBendRange_ = semitones; }
    float getPitchBendRange() const { return pitchBendRange_; }

    // Access individual voices for advanced control
    Voice* getVoice(int index) {
        if (index >= 0 && index < static_cast<int>(voices_.size())) {
            return voices_[index].get();
        }
        return nullptr;
    }
    
private:
    // Find voice to steal based on current policy
    Voice* findVoiceToSteal();
    
    // Find existing voice for a note
    Voice* findVoiceForNote(int midiNote, int channel = 0);
    
    // Create a new voice instance
    virtual std::unique_ptr<Voice> createVoice();

protected:
    // Voice management (made protected for derived classes)
    std::vector<std::unique_ptr<Voice>> voices_;
    int sampleRate_;
    bool hybridEnabled_ = false;
    std::shared_ptr<SpectralWavetableCache> spectralCache_;
    std::unique_ptr<SpectralRenderWorker>* spectralWorkerPtr_ = nullptr; // non-owning pointer to worker unique_ptr
    std::unique_ptr<SpectralTable> defaultSpectralTable_;
    std::shared_ptr<SpectralTable> spectralTableShared_;

private:
    std::unordered_map<int, Voice*> activeNotes_; // Maps MIDI note to active voice
    
    // Basic settings
    int maxVoices_;
    StealMode stealMode_;
    
    // Shared resources for all voices
    std::shared_ptr<Wavetable> currentWavetable_;
    
    // MIDI control state
    struct ChannelState {
        bool sustainPedalDown = false;
        float pitchBendValue = 0.0f;        // -1.0 to 1.0
        float channelPressure = 0.0f;       // 0.0 to 1.0
        std::unordered_map<int, bool> sustainedNotes;  // Notes held by sustain
        std::unordered_map<int, float> noteAftertouch;  // Per-note aftertouch
    };
    
    std::unordered_map<int, ChannelState> channelStates_;  // Maps channel to state
    
    // Pitch bend settings
    float pitchBendRange_ = 2.0f;  // Default +/- 2 semitones
};

} // namespace AIMusicHardware
