#include "../../include/effects/BitCrusher.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

BitCrusher::BitCrusher(int sampleRate)
    : Effect(sampleRate),
      bitDepth_(8.0f),      // 8-bit quantization
      sampleRateReduction_(0.5f), // Half the original sample rate
      mix_(1.0f),          // 100% wet
      drive_(1.0f),        // Default drive
      holdCounter_(0),
      holdL_(0.0f),
      holdR_(0.0f) {
}

BitCrusher::~BitCrusher() {
}

void BitCrusher::process(float* buffer, int numFrames) {
    // Calculate actual sampling interval
    int samplingInterval = static_cast<int>(1.0f / sampleRateReduction_);
    samplingInterval = std::max(1, std::min(samplingInterval, 100)); // Safety limits
    
    // Calculate bit depth quantization steps
    float numLevels = std::pow(2.0f, bitDepth_);
    float quantizationStep = 2.0f / numLevels;
    
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Apply drive for more aggressive crushing
        if (drive_ > 1.0f) {
            inputL = std::tanh(inputL * drive_);
            inputR = std::tanh(inputR * drive_);
        }
        
        // Sample rate reduction
        if (holdCounter_ >= samplingInterval) {
            // Take new sample
            holdL_ = inputL;
            holdR_ = inputR;
            holdCounter_ = 0;
        } else {
            // Hold previous sample
            holdCounter_++;
        }
        
        // Bit depth reduction (quantization)
        float quantizedL = std::floor(holdL_ / quantizationStep + 0.5f) * quantizationStep;
        float quantizedR = std::floor(holdR_ / quantizationStep + 0.5f) * quantizationStep;
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + quantizedL * mix_;
        buffer[i + 1] = inputR * (1.0f - mix_) + quantizedR * mix_;
    }
}

void BitCrusher::setParameter(const std::string& name, float value) {
    if (name == "bitDepth") {
        bitDepth_ = clamp(value, 1.0f, 16.0f);
    }
    else if (name == "sampleRateReduction") {
        sampleRateReduction_ = clamp(value, 0.01f, 1.0f);
    }
    else if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "drive") {
        drive_ = clamp(value, 1.0f, 10.0f);
    }
}

float BitCrusher::getParameter(const std::string& name) const {
    if (name == "bitDepth") {
        return bitDepth_;
    }
    else if (name == "sampleRateReduction") {
        return sampleRateReduction_;
    }
    else if (name == "mix") {
        return mix_;
    }
    else if (name == "drive") {
        return drive_;
    }
    return 0.0f;
}

} // namespace AIMusicHardware