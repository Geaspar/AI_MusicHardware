#include "../../../include/synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

// Static defaults for global minimums
float ModEnvelope::s_minAttackSeconds_ = 0.005f;  // 5 ms default
float ModEnvelope::s_minReleaseSeconds_ = 0.010f; // 10 ms default

ModEnvelope::ModEnvelope(int sampleRate)
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
      stageProgress_(0.0f),
      releaseStartValue_(0.0f) {
    updateRates();
}

ModEnvelope::~ModEnvelope() {
}

void ModEnvelope::noteOn() {
    stageProgress_ = 0.0f;
    // Ensure rates reflect current base parameters for a fresh note
    updateRates();
    currentStage_ = Stage::Attack;
}

void ModEnvelope::noteOff() {
    if (currentStage_ != Stage::Idle && currentStage_ != Stage::Killed) {
        // Capture the current value so release decays from the actual level
        releaseStartValue_ = value_;
        // Apply one-shot release override if requested
        if (releaseOverrideActive_) {
            float originalRelease = release_;
            release_ = releaseOverrideSeconds_;
            updateRates();
            // Revert base release after computing rates so future noteOns use the base value
            release_ = originalRelease;
            releaseOverrideActive_ = false;
        }
        stageProgress_ = 0.0f;
        currentStage_ = Stage::Release;
    }
}

void ModEnvelope::reset() {
    value_ = 0.0f;
    stageProgress_ = 0.0f;
    currentStage_ = Stage::Idle;
}

float ModEnvelope::generateValue() {
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
                value_ = releaseStartValue_ * (1.0f - curvedProgress);
            }
            break;
            
        case Stage::Killed:
            value_ = 0.0f;
            break;
    }
    
    return value_;
}

void ModEnvelope::setAttack(float seconds) {
    float minA = std::max(0.0005f, s_minAttackSeconds_);
    attack_ = std::max(minA, seconds);
    updateRates();
}

void ModEnvelope::setDecay(float seconds) {
    decay_ = std::max(0.001f, seconds); // Minimum 1ms decay
    updateRates();
}

void ModEnvelope::setSustain(float level) {
    sustain_ = std::clamp(level, 0.0f, 1.0f);
}

void ModEnvelope::setRelease(float seconds) {
    float minR = std::max(0.001f, s_minReleaseSeconds_);
    release_ = std::max(minR, seconds);
    updateRates();
}

void ModEnvelope::setReleaseOverrideOnce(float seconds) {
    releaseOverrideSeconds_ = std::max(0.010f, seconds);
    releaseOverrideActive_ = true;
}

void ModEnvelope::setAttackCurve(float curve) {
    attackCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void ModEnvelope::setDecayCurve(float curve) {
    decayCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void ModEnvelope::setReleaseCurve(float curve) {
    releaseCurve_ = std::clamp(curve, -1.0f, 1.0f);
}

void ModEnvelope::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    updateRates();
}

void ModEnvelope::updateRates() {
    // Calculate rates based on time values and sample rate
    attackRate_ = 1.0f / (attack_ * sampleRate_);
    decayRate_ = 1.0f / (decay_ * sampleRate_);
    releaseRate_ = 1.0f / (release_ * sampleRate_);
}

float ModEnvelope::applyCurve(float value, float curve) const {
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

void ModEnvelope::setGlobalMinimums(float minAttackSeconds, float minReleaseSeconds) {
    s_minAttackSeconds_ = std::max(0.0005f, minAttackSeconds);
    s_minReleaseSeconds_ = std::max(0.001f, minReleaseSeconds);
}

} // namespace AIMusicHardware
