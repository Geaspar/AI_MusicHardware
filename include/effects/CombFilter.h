#pragma once

#include "AdvancedFilter.h"
#include <vector>

namespace AIMusicHardware {

/**
 * @brief Comb Filter model
 * 
 * Implements both feed-forward and feedback comb filters with delay time
 * modulation for flanger and chorus-like effects.
 */
class CombFilterModel : public FilterModel {
public:
    enum class Type {
        FeedForward,  // Feed-forward (FIR) comb filter - creates notches
        FeedBack      // Feedback (IIR) comb filter - creates peaks
    };
    
    CombFilterModel(int sampleRate = 44100, Type type = Type::FeedForward);
    ~CombFilterModel() override;
    
    void process(float* buffer, int numFrames, int channels) override;
    void setParameter(const std::string& name, float value) override;
    void setSampleRate(int sampleRate) override;
    
    std::string getTypeName() const override;
    
private:
    // Updates internal calculations
    void updateParameters();
    
    // Calculate delay line write position with optional modulation
    int calculateWritePos(int channel);
    
    // Calculate interpolated delay line read position 
    float calculateDelayTime(int channel);
    
    // Get a sample from the delay line with interpolation
    float getInterpolatedSample(int channel, float delaySamples);
    
    Type type_;
    
    // Delay line - one per channel
    std::vector<float> delayLines_[2];
    int writePos_[2];       // Current write position
    
    // LFO for delay modulation
    float lfoPhase_[2];     // Current LFO phase (one per channel for stereo effects)
    float lfoPhaseIncrement_;  // Phase increment per sample
    
    // Cached parameter values for efficient processing
    float delayTime_;       // Base delay time in milliseconds
    float modAmount_;       // Modulation amount (+/- in milliseconds)
    float modRate_;         // LFO rate in Hz
    float feedback_;        // Feedback amount (-1 to 1)
    float directMix_;       // Direct/dry mix (0 to 1)
    float delaySamples_;    // Base delay time in samples
    float maxDelaySamples_; // Maximum delay time in samples
};

} // namespace AIMusicHardware