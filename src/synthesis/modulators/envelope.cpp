#include "../../../include/synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

Envelope::Envelope(int sampleRate)
    : attack_(0.01f),      // 10ms attack by default
      decay_(0.1f),        // 100ms decay by default
      sustain_(0.7f),      // 70% sustain level by default
      release_(0.5f),      // 500ms release by default
      attackCurve_(0.0f),  // Linear curve by default
      decayCurve_(0.0f),   // Linear curve by default
      releaseCurve_(0.0f), // Linear curve by default
      value_(0.0f),
      currentStage_(Stage::Idle),
      sampleRate_(sampleRate),
      stageProgress_(0.0f) {
    updateRates();
}

Envelope::~Envelope() {
}

void Envelope::noteOn() {
    stageProgress_ = 0.0f;
    currentStage_ = Stage::Attack;
}

void Envelope::noteOff() {
    if (currentStage_ != Stage::Idle && currentStage_ != Stage::Killed) {
        stageProgress_ = 0.0f;
        currentStage_ = Stage::Release;
    }
}

void Envelope::reset() {
    value_ = 0.0f;
    stageProgress_ = 0.0f;
    currentStage_ = Stage::Idle;
}

float Envelope::generateValue() {
    switch (currentStage_) {
        case Stage::Idle:
            value_ = 0.0f;
            break;
            
        case Stage::Attack:
            // Progress through attack stage
            stageProgress_ += attackRate_;
            if (stageProgress_ >= 1.0f) {
                stageProgress_ = 0.0f;
                currentStage_ = Stage::Decay;
                value_ = 1.0f; // Peak at full value
            } else {
                // Apply curve to the linear attack progression
                value_ = applyCurve(stageProgress_, attackCurve_);
            }
            break;
            
        case Stage::Decay:
            // Progress through decay stage
            stageProgress_ += decayRate_;
            if (stageProgress_ >= 1.0f) {
                stageProgress_ = 0.0f;
                currentStage_ = Stage::Sustain;
                value_ = sustain_;
            } else {
                // Apply curve to the linear decay progression and scale to sustain level
                float curvedProgress = applyCurve(stageProgress_, decayCurve_);
                value_ = 1.0f - curvedProgress * (1.0f - sustain_);
            }
            break;
            
        case Stage::Sustain:
            value_ = sustain_;
            break;
            
        case Stage::Release:
            // Progress through release stage
            stageProgress_ += releaseRate_;
            if (stageProgress_ >= 1.0f) {
                stageProgress_ = 0.0f;
                currentStage_ = Stage::Idle;
                value_ = 0.0f;
            } else {
                // Apply curve to the linear release progression and scale from sustain level
                float curvedProgress = applyCurve(stageProgress_, releaseCurve_);
                value_ = sustain_ * (1.0f - curvedProgress);
            }
            break;
            
        case Stage::Killed:
            value_ = 0.0f;
            break;
    }
    
    return value_;
}

void Envelope::setAttack(float seconds) {
    attack_ = std::max(0.001f, seconds); // Minimum 1ms attack
    updateRates();
}

void Envelope::setDecay(float seconds) {
    decay_ = std::max(0.001f, seconds); // Minimum 1ms decay
    updateRates();
}

void Envelope::setSustain(float level) {
    sustain_ = std::clamp(level, 0.0f, 1.0f);
}

void Envelope::setRelease(float seconds) {
    release_ = std::max(0.001f, seconds); // Minimum 1ms release
    updateRates();
}

void Envelope::setAttackCurve(float curve) {
    attackCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void Envelope::setDecayCurve(float curve) {
    decayCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void Envelope::setReleaseCurve(float curve) {
    releaseCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void Envelope::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    updateRates();
}

void Envelope::updateRates() {
    // Calculate rates based on time values and sample rate
    attackRate_ = 1.0f / (attack_ * sampleRate_);
    decayRate_ = 1.0f / (decay_ * sampleRate_);
    releaseRate_ = 1.0f / (release_ * sampleRate_);
}

float Envelope::applyCurve(float value, float curve) const {
    // Linear case - no curve applied
    if (curve == 0.0f) {
        return value;
    }
    
    // Apply exponential-like curve based on the curve parameter
    // Negative curve: slow start, fast finish
    // Positive curve: fast start, slow finish
    if (curve < 0.0f) {
        return std::pow(value, 1.0f + (-curve * 3.0f));
    } else {
        return 1.0f - std::pow(1.0f - value, 1.0f + (curve * 3.0f));
    }
}

} // namespace AIMusicHardware