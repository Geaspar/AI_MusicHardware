#pragma once

#include <memory>
#include <string>
#include <vector>
#include <random>

namespace AIMusicHardware {

// Forward declarations
class WavetableOscillator;
class ModEnvelope;
class Wavetable;

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
    virtual ~Voice();
    
    // Note control
    void noteOn(int midiNote, float velocity);
    void noteOff();
    void reset();
    
    // Sound generation
    virtual float generateSample();
    virtual void process(float* buffer, int numFrames);

    // State access
    State getState() const { return state_; }
    bool isActive() const { return state_ != State::Inactive && state_ != State::Finished; }
    bool isReleased() const { return state_ == State::Released; }
    int getMidiNote() const { return midiNote_; }
    int getChannel() const { return channel_; }
    void setChannel(int channel) { channel_ = channel; }

    // Oscillator access
    WavetableOscillator* getOscillator() { return oscillator_.get(); }
    
    // Envelope access
    ModEnvelope* getEnvelope() { return envelope_.get(); }
    
    // Age tracking for voice stealing
    int getAge() const { return age_; }
    void incrementAge() { age_++; }
    
    // Amplitude for voice stealing
    float getCurrentAmplitude() const;
    
    // Oscillator access
    virtual void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
    // Pitch adjustment
    void setPitchBend(float semitones);
    void setPressure(float pressure) { pressure_ = pressure; }
    
    // Amplitude modulation
    void setAmplitudeModulation(float modulation) { amplitudeModulation_ = modulation; }
    float getAmplitudeModulation() const { return amplitudeModulation_; }
    
    // Unified pitch modulation system (Vital-style)
    struct PitchModulation {
        float basePitch = 0.0f;           // MIDI note number
        float pitchBend = 0.0f;           // Pitch wheel (-2 to +2 semitones typically)
        float lfo1ToPitch = 0.0f;         // LFO1 modulation amount in semitones
        float lfo2ToPitch = 0.0f;         // LFO2 modulation amount in semitones
        float envToPitch = 0.0f;          // Envelope modulation amount in semitones
        float velocityToPitch = 0.0f;     // Velocity modulation amount in semitones
        float randomToPitch = 0.0f;       // Random modulation amount in semitones
        float noteToPitch = 0.0f;         // Note tracking (keyboard follow)
        
        // Current modulation values (0-1 range, will be multiplied by amounts)
        float lfo1Value = 0.0f;
        float lfo2Value = 0.0f;
        float envValue = 0.0f;
        float velocityValue = 0.0f;
        float randomValue = 0.0f;
        
        // Calculate total pitch in semitones
        float calculateTotalPitch() const {
            return basePitch 
                 + pitchBend
                 + (lfo1ToPitch * lfo1Value)
                 + (lfo2ToPitch * lfo2Value)
                 + (envToPitch * envValue)
                 + (velocityToPitch * velocityValue)
                 + (randomToPitch * randomValue)
                 + noteToPitch;
        }
        
        // Smooth interpolation for audio-rate modulation
        float smoothedPitch = 0.0f;
        void updateSmoothedPitch(float targetPitch, float smoothingFactor = 0.995f) {
            smoothedPitch = smoothedPitch * smoothingFactor + targetPitch * (1.0f - smoothingFactor);
        }
    };
    
    // Set modulation amounts (in semitones)
    void setPitchModulationAmount(const std::string& source, float semitones);
    
    // Set modulation values (0-1 range, bipolar sources should be -1 to 1)
    void setPitchModulationValue(const std::string& source, float value);
    
    // Get current total pitch
    float getTotalPitch() const { return pitchMod_.calculateTotalPitch(); }
    
    // Sample rate control
    virtual void setSampleRate(int sampleRate);

protected:
    // Voice components that derived classes might need access to
    std::unique_ptr<WavetableOscillator> oscillator_;
    std::unique_ptr<ModEnvelope> envelope_;

    // State data that derived classes might need access to
    State state_;
    float velocity_;
    float frequency_;
    float baseFrequency_;     // Frequency without any pitch bend

    // Unified pitch modulation
    PitchModulation pitchMod_;

    // Helper to convert MIDI note to frequency
    float midiNoteToFrequency(int midiNote) const;

    // Internal frequency update
    void updateOscillatorFrequency();

private:
    int midiNote_;
    int age_;                 // Number of samples this voice has been active
    int channel_ = 0;         // MIDI channel for this voice
    int sampleRate_;

    // MIDI expression parameters
    float pressure_ = 0.0f;            // Pressure/aftertouch (0.0-1.0)
    float amplitudeModulation_ = 1.0f; // Amplitude modulation multiplier (0.0-1.0)
    
    // Random number generator for random pitch
    std::mt19937 rng_;
    std::uniform_real_distribution<float> randomDist_{-1.0f, 1.0f};
    
    // DC blocker filter state
    float dcBlockerX1_;  // Previous input
    float dcBlockerY1_;  // Previous output
};

} // namespace AIMusicHardware
