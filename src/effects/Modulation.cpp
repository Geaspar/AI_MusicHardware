#include "../../include/effects/Modulation.h"
#include "../../include/effects/EffectUtils.h"
#include <cstring>
#include <cmath>
#include <cstdlib>

namespace AIMusicHardware {

Modulation::Modulation(int sampleRate)
    : Effect(sampleRate),
      rate_(2.0f),        // 2 Hz
      depth_(0.5f),       // 50% depth
      feedback_(0.0f),    // No feedback
      spread_(0.2f),      // 20% stereo spread
      waveType_(WaveType::Sine),
      phase_(0.0f),
      lastPhase_(0.0f),
      writePos_(0),
      randomValue_(0.0f),
      targetRandomValue_(0.0f),
      sampleHoldCounter_(0) {
    
    // Allocate delay buffers
    leftDelay_ = new float[MAX_DELAY_SAMPLES];
    rightDelay_ = new float[MAX_DELAY_SAMPLES];
    
    // Clear delay buffers
    std::memset(leftDelay_, 0, MAX_DELAY_SAMPLES * sizeof(float));
    std::memset(rightDelay_, 0, MAX_DELAY_SAMPLES * sizeof(float));
    
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Initialize S&H counter
    sampleHoldCounter_ = static_cast<int>(sampleRate_ / 30.0f);
}

Modulation::~Modulation() {
    delete[] leftDelay_;
    delete[] rightDelay_;
}

void Modulation::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    // Reset S&H counter when sample rate changes
    sampleHoldCounter_ = static_cast<int>(sampleRate_ / (rate_ * 2.0f));
}

std::string Modulation::getName() const { 
    switch (waveType_) {
        case WaveType::Sine: return "SineModulation";
        case WaveType::Triangle: return "TriangleModulation";
        case WaveType::Saw: return "SawModulation";
        case WaveType::Square: return "SquareModulation";
        case WaveType::Random: return "RandomModulation";
        case WaveType::SampleAndHold: return "S&HModulation";
        default: return "Modulation";
    }
}

void Modulation::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Store in delay buffer with feedback
        leftDelay_[writePos_] = inputL + feedback_ * leftDelay_[(writePos_ - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES];
        rightDelay_[writePos_] = inputR + feedback_ * rightDelay_[(writePos_ - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES];
        
        // Generate LFO value based on wave type
        float lfoValue = 0.0f;
        
        switch (waveType_) {
            case WaveType::Sine:
                lfoValue = 0.5f + 0.5f * std::sin(phase_ * TWO_PI);
                break;
                
            case WaveType::Triangle:
                lfoValue = (phase_ < 0.5f) ? 2.0f * phase_ : 2.0f * (1.0f - phase_);
                break;
                
            case WaveType::Saw:
                lfoValue = phase_;
                break;
                
            case WaveType::Square:
                lfoValue = (phase_ < 0.5f) ? 1.0f : 0.0f;
                break;
                
            case WaveType::Random:
                // Generate smooth random values (Korg ER-1 style)
                if (phase_ < lastPhase_) {
                    // At cycle reset, set new target
                    targetRandomValue_ = static_cast<float>(std::rand()) / RAND_MAX;
                }
                // Interpolate between current and target
                randomValue_ += 0.01f * (targetRandomValue_ - randomValue_);
                lfoValue = randomValue_;
                break;
                
            case WaveType::SampleAndHold:
                // Update S&H value at regular intervals
                if (--sampleHoldCounter_ <= 0) {
                    randomValue_ = static_cast<float>(std::rand()) / RAND_MAX;
                    sampleHoldCounter_ = static_cast<int>(sampleRate_ / (rate_ * 2.0f));
                }
                lfoValue = randomValue_;
                break;
        }
        
        // Store last phase for edge detection
        lastPhase_ = phase_;
        
        // Update phase for next sample
        phase_ += rate_ / sampleRate_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
        
        // Calculate modulated delay time (in samples) for classic chorus/flanger effect
        float delayMs = 1.0f + depth_ * 10.0f * lfoValue; // 1-11ms delay range (typical for chorus/flanger)
        float delaySamples = delayMs * sampleRate_ / 1000.0f;
        
        // Add stereo offset for right channel
        float delaySamplesR = delaySamples * (1.0f + spread_);
        
        // Calculate read positions with fractional interpolation
        float readPosL = static_cast<float>(writePos_) - delaySamples;
        if (readPosL < 0.0f) {
            readPosL += MAX_DELAY_SAMPLES;
        }
        
        float readPosR = static_cast<float>(writePos_) - delaySamplesR;
        if (readPosR < 0.0f) {
            readPosR += MAX_DELAY_SAMPLES;
        }
        
        // Linear interpolation for non-integer delay
        int readPosIntL = static_cast<int>(readPosL);
        float fractionL = readPosL - readPosIntL;
        int nextPosL = (readPosIntL + 1) % MAX_DELAY_SAMPLES;
        
        int readPosIntR = static_cast<int>(readPosR);
        float fractionR = readPosR - readPosIntR;
        int nextPosR = (readPosIntR + 1) % MAX_DELAY_SAMPLES;
        
        // Read from delay buffer with interpolation
        float delayedL = leftDelay_[readPosIntL] * (1.0f - fractionL) + leftDelay_[nextPosL] * fractionL;
        float delayedR = rightDelay_[readPosIntR] * (1.0f - fractionR) + rightDelay_[nextPosR] * fractionR;
        
        // Update write position
        writePos_ = (writePos_ + 1) % MAX_DELAY_SAMPLES;
        
        // Mix original and delayed signals (50/50 for classic chorus/flanger)
        buffer[i] = 0.5f * (inputL + delayedL);
        buffer[i + 1] = 0.5f * (inputR + delayedR);
    }
}

void Modulation::setParameter(const std::string& name, float value) {
    if (name == "rate") {
        rate_ = clamp(value, 0.1f, 20.0f); // 0.1 to 20 Hz
        // Update S&H counter when rate changes
        sampleHoldCounter_ = static_cast<int>(sampleRate_ / (rate_ * 2.0f));
    }
    else if (name == "depth") {
        depth_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "feedback") {
        feedback_ = clamp(value, 0.0f, 0.7f); // Limit feedback to avoid runaway
    }
    else if (name == "spread") {
        spread_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "waveType") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 5) {
            waveType_ = static_cast<WaveType>(typeInt);
        }
    }
}

float Modulation::getParameter(const std::string& name) const {
    if (name == "rate") {
        return rate_;
    }
    else if (name == "depth") {
        return depth_;
    }
    else if (name == "feedback") {
        return feedback_;
    }
    else if (name == "spread") {
        return spread_;
    }
    else if (name == "waveType") {
        return static_cast<float>(waveType_);
    }
    return 0.0f;
}

} // namespace AIMusicHardware