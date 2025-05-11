#pragma once

#include "wavetable.h"
#include <vector>
#include <memory>
#include <functional>

namespace AIMusicHardware {

/**
 * OscillatorConfig stores configuration for a single oscillator in a stack
 */
struct OscillatorConfig {
    float detune = 0.0f;         // Detune in cents (-100 to +100)
    float pan = 0.0f;            // Panning (-1.0 left to 1.0 right)
    float level = 1.0f;          // Level (0.0 to 1.0)
    float phase = 0.0f;          // Phase offset (0.0 to 1.0)
    float framePosition = 0.0f;  // Position in wavetable (0.0 to 1.0)
};

/**
 * OscillatorStack manages multiple oscillators with various detuning configurations
 * for richer sound design capabilities.
 */
class OscillatorStack {
public:
    /**
     * @brief Construct a new Oscillator Stack
     * 
     * @param sampleRate Sample rate in Hz
     * @param numOscillators Initial number of oscillators (default: 1)
     */
    OscillatorStack(int sampleRate = 44100, int numOscillators = 1);
    
    /**
     * @brief Destructor
     */
    ~OscillatorStack();
    
    /**
     * @brief Set the number of oscillators in the stack
     * 
     * @param count Number of oscillators (1-8)
     */
    void setOscillatorCount(int count);
    
    /**
     * @brief Get the number of oscillators in the stack
     * 
     * @return int Oscillator count
     */
    int getOscillatorCount() const { return static_cast<int>(oscillators_.size()); }
    
    /**
     * @brief Set the base frequency for all oscillators
     * 
     * @param frequency Frequency in Hz
     */
    void setFrequency(float frequency);
    
    /**
     * @brief Get the base frequency
     * 
     * @return float Frequency in Hz
     */
    float getFrequency() const { return baseFrequency_; }
    
    /**
     * @brief Set the detune amount for a specific oscillator
     * 
     * @param index Oscillator index (0 to count-1)
     * @param cents Detune amount in cents (-100 to +100)
     */
    void setDetune(int index, float cents);
    
    /**
     * @brief Set the detune spread for all oscillators
     * 
     * @param cents Maximum detune amount in cents (will be distributed symmetrically)
     */
    void setDetuneSpread(float cents);
    
    /**
     * @brief Set the level (volume) for a specific oscillator
     * 
     * @param index Oscillator index (0 to count-1)
     * @param level Level (0.0 to 1.0)
     */
    void setLevel(int index, float level);
    
    /**
     * @brief Set the frame position for a specific oscillator
     * 
     * @param index Oscillator index (0 to count-1)
     * @param position Position in wavetable (0.0 to 1.0)
     */
    void setFramePosition(int index, float position);
    
    /**
     * @brief Set the frame position for all oscillators
     * 
     * @param position Position in wavetable (0.0 to 1.0)
     */
    void setAllFramePositions(float position);
    
    /**
     * @brief Set the panning for a specific oscillator
     * 
     * @param index Oscillator index (0 to count-1)
     * @param pan Panning (-1.0 left to 1.0 right)
     */
    void setPan(int index, float pan);
    
    /**
     * @brief Set the phase for a specific oscillator
     * 
     * @param index Oscillator index (0 to count-1)
     * @param phase Phase (0.0 to 1.0)
     */
    void setPhase(int index, float phase);
    
    /**
     * @brief Reset all phases to their configured phase offset
     */
    void resetAllPhases();
    
    /**
     * @brief Set the wavetable for all oscillators
     * 
     * @param wavetable Shared wavetable pointer
     */
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
    /**
     * @brief Set the sample rate for all oscillators
     * 
     * @param sampleRate Sample rate in Hz
     */
    void setSampleRate(int sampleRate);
    
    /**
     * @brief Generate mono sample from all oscillators
     * 
     * @return float Combined sample
     */
    float generateMonoSample();
    
    /**
     * @brief Generate stereo sample from all oscillators with panning
     * 
     * @param leftOut Output parameter for left channel
     * @param rightOut Output parameter for right channel
     */
    void generateStereoSample(float& leftOut, float& rightOut);
    
    /**
     * @brief Get oscillator configuration at given index
     * 
     * @param index Oscillator index
     * @return OscillatorConfig& Configuration reference
     */
    OscillatorConfig& getConfig(int index);
    
    /**
     * @brief Get the oscillator at the given index
     * 
     * @param index Oscillator index
     * @return WavetableOscillator* Oscillator pointer (nullptr if invalid index)
     */
    WavetableOscillator* getOscillator(int index);
    
    /**
     * @brief Apply a spread function across all oscillators
     * 
     * This allows for creating various distribution patterns for parameters.
     * 
     * @param paramFn Function taking normalized position (0-1) and oscillator index
     */
    void spreadParameter(std::function<void(float, int)> paramFn);
    
    /**
     * @brief Apply different preset spreading patterns
     * 
     * @param presetType 0=even, 1=center-weighted, 2=alternating
     * @param maxDetune Maximum detune amount in cents
     */
    void applyDetunePreset(int presetType, float maxDetune);
    
    /**
     * @brief Config preset for unison stacking
     * 
     * Configures a traditional unison stack with detuning, widening, and leveling
     * 
     * @param count Oscillator count (1-8)
     * @param detune Detune amount in cents
     * @param width Stereo width (0.0 to 1.0)
     * @param convergence Level convergence (0=equal, 1=center emphasized)
     */
    void configUnison(int count, float detune, float width, float convergence);

private:
    // Helper to calculate frequency from base frequency and detune
    float calculateDetuneMultiplier(float cents) const;
    
    // Apply configured detune to the actual frequency
    void applyDetune(int index);
    
    // Create a new oscillator with default settings
    std::unique_ptr<WavetableOscillator> createOscillator();

    std::vector<std::unique_ptr<WavetableOscillator>> oscillators_;
    std::vector<OscillatorConfig> configs_;
    
    float baseFrequency_ = 440.0f;
    int sampleRate_ = 44100;
    std::shared_ptr<Wavetable> wavetable_;
};

} // namespace AIMusicHardware