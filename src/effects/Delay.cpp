#include "../../include/effects/EffectProcessor.h"
#include "../../include/effects/EffectUtils.h"
#include <cstring>
#include <algorithm>

namespace AIMusicHardware {

// Delay implementation
Delay::Delay(int sampleRate) 
    : Effect(sampleRate),
      delayTime_(0.5f),    // 500ms delay
      feedback_(0.5f),     // 50% feedback
      mix_(0.3f),          // 30% wet
      writePos_(0) {
    
    // Initialize delay buffer (max 2 seconds)
    delayBuffer_.resize(sampleRate_ * 4, 0.0f); // Stereo, so * 2 for channels, * 2 for 2 seconds
}

Delay::~Delay() {
}

void Delay::process(float* buffer, int numFrames) {
    // Process delay effect
    for (int i = 0; i < numFrames * 2; i += 2) { // Stereo processing
        // Calculate delay in samples
        int delaySamples = static_cast<int>(delayTime_ * sampleRate_) * 2; // * 2 for stereo
        delaySamples = delaySamples - (delaySamples % 2); // Ensure even for stereo
        
        // Calculate read position
        int readPos = writePos_ - delaySamples;
        if (readPos < 0) {
            readPos += delayBuffer_.size();
        }
        
        // Read from delay buffer
        float delayedSampleL = delayBuffer_[readPos];
        float delayedSampleR = delayBuffer_[readPos + 1];
        
        // Apply feedback and mix
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Write to delay buffer with feedback
        delayBuffer_[writePos_] = inputL + delayedSampleL * feedback_;
        delayBuffer_[writePos_ + 1] = inputR + delayedSampleR * feedback_;
        
        // Update write position
        writePos_ = (writePos_ + 2) % delayBuffer_.size();
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + delayedSampleL * mix_;
        buffer[i + 1] = inputR * (1.0f - mix_) + delayedSampleR * mix_;
    }
}

void Delay::setParameter(const std::string& name, float value) {
    if (name == "delayTime") {
        // Clamp to reasonable values (20ms - 2000ms)
        delayTime_ = clamp(value, 0.02f, 2.0f);
    }
    else if (name == "feedback") {
        // Clamp to avoid infinite feedback
        feedback_ = clamp(value, 0.0f, 0.95f);
    }
    else if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
    }
}

float Delay::getParameter(const std::string& name) const {
    if (name == "delayTime") {
        return delayTime_;
    }
    else if (name == "feedback") {
        return feedback_;
    }
    else if (name == "mix") {
        return mix_;
    }
    
    return 0.0f;
}

} // namespace AIMusicHardware