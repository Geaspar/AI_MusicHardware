#include "../../../include/synthesis/voice/stacked_voice.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

StackedVoice::StackedVoice(int sampleRate, int numOscillators)
    : Voice(sampleRate),
      stereoWidth_(0.5f),
      detuneSpread_(10.0f),
      convergence_(0.0f),
      unisonCount_(std::clamp(numOscillators, 1, 8)) {
    
    // Create the oscillator stack
    oscillatorStack_ = std::make_unique<OscillatorStack>(sampleRate, unisonCount_);
    
    // Apply default unison settings
    configureUnison(unisonCount_, detuneSpread_, stereoWidth_, convergence_);
}

StackedVoice::~StackedVoice() {
    // Smart pointers handle cleanup
}

float StackedVoice::generateSample() {
    // Skip processing if voice is inactive
    if (getState() == State::Inactive || getState() == State::Finished) {
        return 0.0f;
    }
    
    // Increment the voice age for voice stealing
    incrementAge();
    
    // Get sample from the oscillator stack
    float sample = oscillatorStack_->generateMonoSample();
    
    // Apply envelope
    float envelopeValue = envelope_->generateValue();
    sample *= envelopeValue * velocity_;
    
    // Update state based on envelope
    if (getState() == State::Starting && envelopeValue > 0.01f) {
        state_ = State::Playing;
    } else if (getState() == State::Released && !envelope_->isActive()) {
        state_ = State::Finished;
    }
    
    return sample;
}

void StackedVoice::process(float* buffer, int numFrames) {
    // Skip processing if voice is inactive
    if (getState() == State::Inactive || getState() == State::Finished) {
        return;
    }
    
    // Process with stereo output
    for (int i = 0; i < numFrames; ++i) {
        // We need to get the envelope value in each iteration
        float envelopeValue = envelope_->generateValue();
        
        // Get stereo sample from the oscillator stack
        float left = 0.0f, right = 0.0f;
        oscillatorStack_->generateStereoSample(left, right);
        
        // Apply envelope and velocity
        left *= envelopeValue * velocity_;
        right *= envelopeValue * velocity_;
        
        // Add to buffer (interleaved stereo)
        buffer[i * 2] += left;
        buffer[i * 2 + 1] += right;
        
        // Update state based on envelope
        if (getState() == State::Starting && envelopeValue > 0.01f) {
            state_ = State::Playing;
        } else if (getState() == State::Released && !envelope_->isActive()) {
            state_ = State::Finished;
            break; // Stop processing if finished
        }
        
        // Increment age
        incrementAge();
    }
}

void StackedVoice::setOscillatorCount(int count) {
    unisonCount_ = std::clamp(count, 1, 8);
    oscillatorStack_->setOscillatorCount(unisonCount_);
    configureUnison(unisonCount_, detuneSpread_, stereoWidth_, convergence_);
}

int StackedVoice::getOscillatorCount() const {
    return oscillatorStack_->getOscillatorCount();
}

void StackedVoice::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    // Call base class implementation
    Voice::setWavetable(wavetable);
    
    // Also set wavetable for the oscillator stack
    oscillatorStack_->setWavetable(wavetable);
}

void StackedVoice::setDetuneSpread(float cents) {
    detuneSpread_ = std::clamp(cents, 0.0f, 100.0f);
    oscillatorStack_->setDetuneSpread(detuneSpread_);
}

void StackedVoice::setStereoWidth(float width) {
    stereoWidth_ = std::clamp(width, 0.0f, 1.0f);

    // Apply the width to oscillator panning
    oscillatorStack_->spreadParameter([this, width](float pos, int idx) {
        // Map 0-1 to -width to +width
        float pan = (pos - 0.5f) * 2.0f * width;
        oscillatorStack_->setPan(idx, pan);
    });
}

void StackedVoice::setConvergence(float convergence) {
    convergence_ = std::clamp(convergence, 0.0f, 1.0f);

    // Apply convergence to oscillator levels (center oscillators louder if convergence > 0)
    if (convergence > 0.0f) {
        oscillatorStack_->spreadParameter([this, convergence](float pos, int idx) {
            // Calculate distance from center (0 at center, 1 at edges)
            float centerDist = std::abs(pos - 0.5f) * 2.0f;
            // Apply convergence - higher values at center
            float level = 1.0f - (centerDist * convergence * 0.5f);
            oscillatorStack_->setLevel(idx, level);
        });
    } else {
        // Reset all oscillators to equal level
        for (int i = 0; i < oscillatorStack_->getOscillatorCount(); ++i) {
            oscillatorStack_->setLevel(i, 1.0f);
        }
    }
}

void StackedVoice::configureUnison(int count, float detune, float width, float convergence) {
    unisonCount_ = count;
    detuneSpread_ = detune;
    stereoWidth_ = width;
    convergence_ = convergence;
    
    // Apply all settings to the stack
    oscillatorStack_->configUnison(count, detune, width, convergence);
}

void StackedVoice::setFramePosition(float position) {
    oscillatorStack_->setAllFramePositions(position);
}

void StackedVoice::applyDetunePreset(int presetType, float cents) {
    detuneSpread_ = cents;
    oscillatorStack_->applyDetunePreset(presetType, cents);
}

void StackedVoice::setDetuneType(DetuneType type) {
    // Convert the enum to the integer type expected by OscillatorStack
    int presetType = 0;

    switch (type) {
        case DetuneType::Even:
            presetType = 0;
            break;
        case DetuneType::CenterWeighted:
            presetType = 1;
            break;
        case DetuneType::Alternating:
            presetType = 2;
            break;
        case DetuneType::Random:
            presetType = 3;
            break;
    }

    // Apply the preset with current detune spread
    oscillatorStack_->applyDetunePreset(presetType, detuneSpread_);
}

void StackedVoice::setSampleRate(int sampleRate) {
    // Call the base class implementation
    Voice::setSampleRate(sampleRate);
    
    // Update the oscillator stack
    oscillatorStack_->setSampleRate(sampleRate);
}

void StackedVoice::updateFrequency() {
    // Access the protected frequency_ member from Voice and set it in the oscillator stack
    oscillatorStack_->setFrequency(frequency_);
}

} // namespace AIMusicHardware