#pragma once

#include "EffectProcessor.h"
#include <ctime>

namespace AIMusicHardware {

class Modulation : public Effect {
public:
    enum class WaveType {
        Sine,
        Triangle,
        Saw,
        Square,
        Random,
        SampleAndHold
    };
    
    Modulation(int sampleRate = 44100);
    ~Modulation() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    
    std::string getName() const override;
    
private:
    static constexpr int MAX_DELAY_SAMPLES = 48000; // Maximum 1 second at 48kHz
    
    float rate_;       // Modulation rate in Hz
    float depth_;      // Modulation depth (0-1)
    float feedback_;   // Feedback amount
    float spread_;     // Stereo spread
    WaveType waveType_; // Type of modulation wave
    
    float phase_;      // Current LFO phase (0-1)
    float lastPhase_;  // Previous phase value for edge detection
    
    float* leftDelay_; // Delay buffer for left channel
    float* rightDelay_; // Delay buffer for right channel
    int writePos_;     // Current write position in delay buffer
    
    float randomValue_;       // Current random value for Random and S&H waves
    float targetRandomValue_; // Target for smooth random
    int sampleHoldCounter_;   // Counter for sample and hold updates
};

} // namespace AIMusicHardware