#pragma once

#include "AdvancedFilter.h"
#include <array>

namespace AIMusicHardware {

/**
 * @brief Moog Ladder Filter model
 * 
 * This implements a classic 4-pole Moog-style ladder filter with resonance compensation.
 * It provides lowpass and highpass modes with 24dB/octave slope.
 */
class LadderFilterModel : public FilterModel {
public:
    enum class Type {
        LowPass,
        HighPass
    };
    
    LadderFilterModel(int sampleRate = 44100, Type type = Type::LowPass);
    ~LadderFilterModel() override;
    
    void process(float* buffer, int numFrames, int channels) override;
    void setParameter(const std::string& name, float value) override;
    void setSampleRate(int sampleRate) override;
    
    std::string getTypeName() const override;
    
private:
    void calculateCoefficients();
    
    // Reset filter state
    void reset();
    
    Type type_;
    
    // Filter state (for each channel)
    std::array<float, 4> state_[2];   // 4 filter stages x 2 channels
    float delay_[2];                  // Additional delay for highpass mode
    
    // Cached coefficient calculations
    float cutoff_;        // Normalized cutoff frequency [0, 1]
    float resonance_;     // Resonance amount [0, 4]
    float drive_;         // Input drive amount
    
    // Internal coefficients
    float g_;             // Filter coefficient related to cutoff
    float resonanceComp_; // Resonance compensation amount
};

} // namespace AIMusicHardware