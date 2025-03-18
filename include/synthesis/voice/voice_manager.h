#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

namespace AIMusicHardware {

// Forward declarations
class Voice;
class WavetableOscillator;
class Envelope;

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
    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    
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
    
private:
    // Find voice to steal based on current policy
    Voice* findVoiceToSteal();
    
    // Find existing voice for a note
    Voice* findVoiceForNote(int midiNote);
    
    // Create a new voice instance
    std::unique_ptr<Voice> createVoice();
    
    std::vector<std::unique_ptr<Voice>> voices_;
    std::unordered_map<int, Voice*> activeNotes_; // Maps MIDI note to active voice
    
    int sampleRate_;
    int maxVoices_;
    StealMode stealMode_;
    
    // Shared resources for all voices
    std::shared_ptr<Wavetable> currentWavetable_;
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
    
    // Age tracking for voice stealing
    int getAge() const { return age_; }
    void incrementAge() { age_++; }
    
    // Amplitude for voice stealing
    float getCurrentAmplitude() const;
    
    // Oscillator access
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
    // Sample rate control
    void setSampleRate(int sampleRate);
    
private:
    // Helper to convert MIDI note to frequency
    float midiNoteToFrequency(int midiNote) const;
    
    int midiNote_;
    float velocity_;
    float frequency_;
    int age_;         // Number of samples this voice has been active
    
    State state_;
    int sampleRate_;
    
    // Voice components
    std::unique_ptr<WavetableOscillator> oscillator_;
    std::unique_ptr<Envelope> envelope_;
};

} // namespace AIMusicHardware