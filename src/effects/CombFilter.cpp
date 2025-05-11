#include "../../include/effects/CombFilter.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

CombFilterModel::CombFilterModel(int sampleRate, Type type)
    : FilterModel(sampleRate), type_(type) {
    
    // Initialize default parameters
    parameters_["delay_time"] = 5.0f;      // Delay time in milliseconds (1-100ms)
    parameters_["feedback"] = 0.5f;        // Feedback amount (0-0.99)
    parameters_["mod_amount"] = 0.0f;      // Modulation amount (0-10ms)
    parameters_["mod_rate"] = 0.5f;        // Modulation rate in Hz (0.1-10Hz)
    parameters_["direct_mix"] = 1.0f;      // Mix of direct signal (0-1)
    
    // Initialize cached values
    delayTime_ = parameters_["delay_time"];
    feedback_ = parameters_["feedback"];
    modAmount_ = parameters_["mod_amount"];
    modRate_ = parameters_["mod_rate"];
    directMix_ = parameters_["direct_mix"];
    
    // Calculate derived values
    float maxDelayTime = delayTime_ + modAmount_ + 1.0f; // Add 1ms safety margin
    maxDelaySamples_ = (maxDelayTime / 1000.0f) * sampleRate;
    delaySamples_ = (delayTime_ / 1000.0f) * sampleRate;
    
    // Initialize delay lines (one per channel)
    for (int ch = 0; ch < 2; ++ch) {
        // Allocate delay line with appropriate size
        delayLines_[ch].resize(static_cast<size_t>(maxDelaySamples_ + 4));
        std::fill(delayLines_[ch].begin(), delayLines_[ch].end(), 0.0f);
        
        // Initialize positions and phases
        writePos_[ch] = 0;
        lfoPhase_[ch] = 0.0f;
    }
    
    // Calculate LFO phase increment
    lfoPhaseIncrement_ = modRate_ / sampleRate_;
}

CombFilterModel::~CombFilterModel() {
}

void CombFilterModel::setSampleRate(int sampleRate) {
    FilterModel::setSampleRate(sampleRate);
    updateParameters();
    
    // Reinitialize delay lines with new sample rate
    float maxDelayTime = delayTime_ + modAmount_ + 1.0f;
    maxDelaySamples_ = (maxDelayTime / 1000.0f) * sampleRate;
    
    for (int ch = 0; ch < 2; ++ch) {
        delayLines_[ch].resize(static_cast<size_t>(maxDelaySamples_ + 4));
        std::fill(delayLines_[ch].begin(), delayLines_[ch].end(), 0.0f);
        writePos_[ch] = 0;
    }
}

std::string CombFilterModel::getTypeName() const {
    switch (type_) {
        case Type::FeedForward: return "Comb Filter (FIR)";
        case Type::FeedBack: return "Comb Filter (IIR)";
        default: return "Comb Filter";
    }
}

void CombFilterModel::updateParameters() {
    // Cache parameter values for efficient processing
    delayTime_ = parameters_["delay_time"];
    feedback_ = parameters_["feedback"];
    modAmount_ = parameters_["mod_amount"];
    modRate_ = parameters_["mod_rate"];
    directMix_ = parameters_["direct_mix"];
    
    // Calculate derived values
    delaySamples_ = (delayTime_ / 1000.0f) * sampleRate_;
    lfoPhaseIncrement_ = modRate_ / sampleRate_;
    
    // Ensure delay lines are large enough
    float maxDelayTime = delayTime_ + modAmount_ + 1.0f;
    float newMaxDelaySamples = (maxDelayTime / 1000.0f) * sampleRate_;
    
    if (newMaxDelaySamples > maxDelaySamples_) {
        maxDelaySamples_ = newMaxDelaySamples;
        
        for (int ch = 0; ch < 2; ++ch) {
            delayLines_[ch].resize(static_cast<size_t>(maxDelaySamples_ + 4));
        }
    }
}

int CombFilterModel::calculateWritePos(int channel) {
    return writePos_[channel];
}

float CombFilterModel::calculateDelayTime(int channel) {
    // Base delay in samples
    float delay = delaySamples_;
    
    // Add modulation if enabled
    if (modAmount_ > 0.0f) {
        // Convert modulation amount from ms to samples
        float modSamples = (modAmount_ / 1000.0f) * sampleRate_;
        
        // Calculate LFO value: sine wave with stereo phase offset for channel 1
        float phase = lfoPhase_[channel];
        if (channel == 1) {
            // Add 90° phase offset for right channel to create stereo effect
            phase += 0.25f;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        
        // Sine wave modulation
        float lfoValue = std::sin(phase * 2.0f * PI);
        
        // Add modulation to delay time
        delay += lfoValue * modSamples;
    }
    
    // Ensure delay is positive and within buffer size
    return std::max(1.0f, std::min(delay, maxDelaySamples_ - 1.0f));
}

float CombFilterModel::getInterpolatedSample(int channel, float delaySamples) {
    // Convert to fixed and fractional parts
    int intDelay = static_cast<int>(delaySamples);
    float frac = delaySamples - intDelay;
    
    // Get buffer size and read position
    int bufferSize = static_cast<int>(delayLines_[channel].size());
    int readPos = writePos_[channel] - intDelay;
    
    // Handle buffer wrap-around
    if (readPos < 0) readPos += bufferSize;
    
    // Get two samples for linear interpolation
    float sample1 = delayLines_[channel][readPos];
    int nextPos = (readPos + 1) % bufferSize;
    float sample2 = delayLines_[channel][nextPos];
    
    // Linear interpolation between samples
    return sample1 + frac * (sample2 - sample1);
}

void CombFilterModel::process(float* buffer, int numFrames, int channels) {
    // Check if parameters need to be updated
    bool paramsChanged = false;
    static float lastDelay = 0.0f;
    static float lastFeedback = 0.0f;
    static float lastModAmount = 0.0f;
    static float lastModRate = 0.0f;
    
    if (lastDelay != parameters_["delay_time"] ||
        lastFeedback != parameters_["feedback"] ||
        lastModAmount != parameters_["mod_amount"] ||
        lastModRate != parameters_["mod_rate"]) {
        
        updateParameters();
        
        lastDelay = parameters_["delay_time"];
        lastFeedback = parameters_["feedback"];
        lastModAmount = parameters_["mod_amount"];
        lastModRate = parameters_["mod_rate"];
    }
    
    // Restrict feedback to avoid instability
    float safeFeedback = std::clamp(feedback_, -0.99f, 0.99f);
    
    // Process each sample
    for (int i = 0; i < numFrames * channels; i += channels) {
        for (int ch = 0; ch < std::min(channels, 2); ++ch) {
            // Get the input sample
            float input = buffer[i + ch];
            
            // Calculate current write position
            int writePos = calculateWritePos(ch);
            
            // Calculate current delay time (with modulation)
            float currentDelaySamples = calculateDelayTime(ch);
            
            // Get the delayed sample (with interpolation)
            float delayedSample = getInterpolatedSample(ch, currentDelaySamples);
            
            float output = 0.0f;
            
            // Process based on comb filter type
            if (type_ == Type::FeedForward) {
                // FIR Comb Filter: y[n] = x[n] + b * x[n-M]
                output = (directMix_ * input) + (feedback_ * delayedSample);
                
                // Write the input sample to the delay line
                delayLines_[ch][writePos] = input;
            }
            else { // Type::FeedBack
                // IIR Comb Filter: y[n] = x[n] + a * y[n-M]
                output = (directMix_ * input) + (safeFeedback * delayedSample);
                
                // Write the output sample to the delay line for feedback
                delayLines_[ch][writePos] = output;
            }
            
            // Update buffer with the output
            buffer[i + ch] = output;
            
            // Increment write position
            writePos_[ch] = (writePos_[ch] + 1) % delayLines_[ch].size();
            
            // Update LFO phase for next sample
            lfoPhase_[ch] += lfoPhaseIncrement_;
            if (lfoPhase_[ch] >= 1.0f) lfoPhase_[ch] -= 1.0f;
        }
    }
}

void CombFilterModel::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "delay_time") {
        parameters_["delay_time"] = std::clamp(value, 0.1f, 100.0f);
        recalculate = true;
    }
    else if (name == "feedback") {
        parameters_["feedback"] = std::clamp(value, -0.99f, 0.99f);
    }
    else if (name == "mod_amount") {
        parameters_["mod_amount"] = std::clamp(value, 0.0f, 20.0f);
        recalculate = true;
    }
    else if (name == "mod_rate") {
        parameters_["mod_rate"] = std::clamp(value, 0.01f, 10.0f);
        recalculate = true;
    }
    else if (name == "direct_mix") {
        parameters_["direct_mix"] = std::clamp(value, 0.0f, 1.0f);
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 1) {
            type_ = static_cast<Type>(typeInt);
        }
    }
    else {
        // For any other parameters, use base implementation
        FilterModel::setParameter(name, value);
    }
    
    if (recalculate) {
        updateParameters();
    }
}

} // namespace AIMusicHardware