#pragma once

#include "voice_manager.h"
#include "../wavetable/oscillator_stack.h"

namespace AIMusicHardware {

/**
 * Enum for different detune distribution types
 */
enum class DetuneType {
    Even,           // Evenly distributed detune
    CenterWeighted, // More oscillators near center, fewer at extremes
    Alternating,    // Alternate between positive and negative detune
    Random          // Random distribution within range
};

/**
 * StackedVoice extends the standard Voice with multiple oscillators for richer sound.
 * This enables unison, stacking, and multi-oscillator synthesis techniques.
 */
class StackedVoice : public Voice {
public:
    /**
     * @brief Constructor
     * 
     * @param sampleRate Audio sample rate
     * @param numOscillators Initial oscillator count
     */
    StackedVoice(int sampleRate = 44100, int numOscillators = 1);
    
    /**
     * @brief Destructor
     */
    ~StackedVoice() override;
    
    /**
     * @brief Generate a single audio sample
     * 
     * Overrides the base Voice class to use OscillatorStack
     * 
     * @return The generated audio sample
     */
    float generateSample() override;
    
    /**
     * @brief Process audio into a buffer
     * 
     * Enhanced version that supports stereo output with panning
     * 
     * @param buffer Audio buffer (interleaved stereo)
     * @param numFrames Number of frames to generate
     */
    void process(float* buffer, int numFrames) override;
    
    /**
     * @brief Set the number of oscillators
     * 
     * @param count Number of oscillators (1-8)
     */
    void setOscillatorCount(int count);
    
    /**
     * @brief Get the number of oscillators
     * 
     * @return int Oscillator count
     */
    int getOscillatorCount() const;
    
    /**
     * @brief Set the wavetable for all oscillators
     * 
     * @param wavetable Wavetable pointer
     */
    void setWavetable(std::shared_ptr<Wavetable> wavetable) override;
    
    /**
     * @brief Set the detune spread in cents
     * 
     * @param cents Maximum detune amount in cents
     */
    void setDetuneSpread(float cents);
    
    /**
     * @brief Set stereo width
     *
     * @param width Width (0.0=mono, 1.0=full stereo)
     */
    void setStereoWidth(float width);

    /**
     * @brief Set level convergence
     *
     * @param convergence Convergence amount (0.0=equal levels, 1.0=center focused)
     */
    void setConvergence(float convergence);

    /**
     * @brief Set detune distribution type
     *
     * @param type DetuneType enum value
     */
    void setDetuneType(DetuneType type);
    
    /**
     * @brief Configure all unison settings at once
     * 
     * @param count Oscillator count (1-8)
     * @param detune Detune amount in cents
     * @param width Stereo width (0.0 to 1.0)
     * @param convergence Level convergence (0=equal, 1=center emphasized)
     */
    void configureUnison(int count, float detune, float width, float convergence);
    
    /**
     * @brief Access the oscillator stack
     * 
     * @return OscillatorStack* Pointer to the oscillator stack
     */
    OscillatorStack* getOscillatorStack() { return oscillatorStack_.get(); }
    
    /**
     * @brief Set the frame position for all oscillators
     * 
     * @param position Position in wavetable (0.0 to 1.0)
     */
    void setFramePosition(float position);
    
    /**
     * @brief Apply detune preset
     * 
     * @param presetType 0=even, 1=center-weighted, 2=alternating
     * @param cents Maximum detune in cents
     */
    void applyDetunePreset(int presetType, float cents);
    
    /**
     * @brief Set sample rate
     * 
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(int sampleRate) override;
    
protected:
    /**
     * @brief Update the base frequency
     * 
     * Called when pitch changes due to MIDI note or pitch bend
     */
    void updateFrequency();
    
private:
    std::unique_ptr<OscillatorStack> oscillatorStack_;
    float stereoWidth_ = 0.5f;
    float detuneSpread_ = 10.0f; // Default 10 cents
    float convergence_ = 0.0f;   // Default equal levels
    int unisonCount_ = 1;        // Default single oscillator
};

} // namespace AIMusicHardware