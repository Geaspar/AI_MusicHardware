#pragma once

#include "voice_manager.h"
#include "stacked_voice.h"
// DetuneType enum is already included from stacked_voice.h

namespace AIMusicHardware {

/**
 * StackedVoiceManager extends VoiceManager to use StackedVoice instances
 * with multiple oscillators per voice.
 */
class StackedVoiceManager : public VoiceManager {
public:
    /**
     * @brief Constructor
     * 
     * @param sampleRate Audio sample rate
     * @param maxVoices Maximum number of voices
     * @param oscillatorsPerVoice Number of oscillators per voice (1-8)
     */
    StackedVoiceManager(int sampleRate = 44100, int maxVoices = 16, int oscillatorsPerVoice = 1);
    
    /**
     * @brief Destructor
     */
    ~StackedVoiceManager();
    
    /**
     * @brief Set the number of oscillators per voice
     * 
     * @param count Oscillator count (1-8)
     */
    void setOscillatorsPerVoice(int count);
    
    /**
     * @brief Get the number of oscillators per voice
     * 
     * @return int Oscillator count
     */
    int getOscillatorsPerVoice() const { return oscillatorsPerVoice_; }
    
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
     * @brief Configure unison settings
     * 
     * @param count Oscillator count (1-8)
     * @param detune Detune amount in cents
     * @param width Stereo width (0.0 to 1.0)
     * @param convergence Level convergence (0=equal, 1=center emphasized)
     */
    void configureUnison(int count, float detune, float width, float convergence);
    
    /**
     * @brief Set the oscillator frame position (waveform type)
     * 
     * @param position Position in wavetable (0.0 to 1.0)
     */
    void setOscillatorFramePosition(float position);
    
    /**
     * @brief Apply detune preset to all voices
     *
     * @param presetType 0=even, 1=center-weighted, 2=alternating
     * @param cents Maximum detune in cents
     */
    void applyDetunePreset(int presetType, float cents);

    /**
     * @brief Set detune distribution type for all voices
     *
     * @param type DetuneType enum value
     */
    void setDetuneType(DetuneType type);
    
    /**
     * @brief Access a stacked voice
     * 
     * @param index Voice index
     * @return StackedVoice* Pointer to the voice
     */
    StackedVoice* getStackedVoice(int index);
    
protected:
    /**
     * @brief Create a voice instance
     * 
     * Override to create StackedVoice instead of Voice
     * 
     * @return std::unique_ptr<Voice> New voice
     */
    std::unique_ptr<Voice> createVoice() override;
    
private:
    int oscillatorsPerVoice_ = 1;
    float detuneSpread_ = 10.0f;
    float stereoWidth_ = 0.5f;
    float convergence_ = 0.0f;
};

} // namespace AIMusicHardware