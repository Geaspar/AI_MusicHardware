#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include "../wavetable/wavetable.h"  // For Wavetable class

namespace AIMusicHardware {

// Forward declarations
class Voice;
class WavetableOscillator;
class ModEnvelope;

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
    void setSampleRate(int sampleRate);
    
    // Shared wavetable management
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
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
    std::unique_ptr<Voice> createVoice();
    
    // Voice management
    std::vector<std::unique_ptr<Voice>> voices_;
    std::unordered_map<int, Voice*> activeNotes_; // Maps MIDI note to active voice
    
    // Basic settings
    int sampleRate_;
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

/**
 * Voice class with advanced state management.
 */
class Voice {
public:
    enum class State {
        Inactive,   // Voice is not in use
        Starting,   // Voice is starting but hasn't produced sound yet
        Playing,    // Voice is actively playing
        Released,   // Voice is in release stage
        Finished    // Voice has finished but not yet recycled
    };
    
    Voice(int sampleRate = 44100);
    ~Voice();
    
    // Note control
    void noteOn(int midiNote, float velocity);
    void noteOff();
    void reset();
    
    // Sound generation
    float generateSample();
    void process(float* buffer, int numFrames);

    // State access
    State getState() const { return state_; }
    bool isActive() const { return state_ != State::Inactive && state_ != State::Finished; }
    bool isReleased() const { return state_ == State::Released; }
    int getMidiNote() const { return midiNote_; }
    int getChannel() const { return channel_; }
    void setChannel(int channel) { channel_ = channel; }

    // Oscillator access
    WavetableOscillator* getOscillator() { return oscillator_.get(); }
    
    // Age tracking for voice stealing
    int getAge() const { return age_; }
    void incrementAge() { age_++; }
    
    // Amplitude for voice stealing
    float getCurrentAmplitude() const;
    
    // Oscillator access
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
    // Pitch adjustment
    void setPitchBend(float semitones);
    void setPressure(float pressure) { pressure_ = pressure; }
    
    // Sample rate control
    void setSampleRate(int sampleRate);
    
private:
    // Helper to convert MIDI note to frequency
    float midiNoteToFrequency(int midiNote) const;
    
    int midiNote_;
    float velocity_;
    float frequency_;
    float baseFrequency_;     // Frequency without any pitch bend
    int age_;                 // Number of samples this voice has been active
    int channel_ = 0;         // MIDI channel for this voice
    
    State state_;
    int sampleRate_;
    
    // MIDI expression parameters
    float pitchBendSemitones_ = 0.0f;  // Current pitch bend in semitones
    float pressure_ = 0.0f;            // Pressure/aftertouch (0.0-1.0)
    
    // Voice components
    std::unique_ptr<WavetableOscillator> oscillator_;
    std::unique_ptr<ModEnvelope> envelope_;
};

} // namespace AIMusicHardware