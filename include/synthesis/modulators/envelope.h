#pragma once

#include <string>
#include <algorithm> // For std::clamp

namespace AIMusicHardware {

/**
 * Advanced ADSR envelope with curve control.
 * This is different from the simple Envelope struct in sequencer/Sequencer.h
 */
class ModEnvelope {
public:
    enum class Stage {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release,
        Killed
    };
    
    ModEnvelope(int sampleRate = 44100);
    ~ModEnvelope();
    
    // Envelope control
    void noteOn();
    void noteOff();
    void reset();
    
    // Generate next envelope value
    float generateValue();
    float getCurrentValue() const { return value_; }
    
    // Parameter setters
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level); // 0.0 - 1.0
    void setRelease(float seconds);
    
    // Curve control for more natural envelopes
    void setAttackCurve(float curve); // -1.0 (slow start) to 1.0 (fast start)
    void setDecayCurve(float curve);   // -1.0 (slow decay) to 1.0 (fast decay)
    void setReleaseCurve(float curve); // -1.0 (slow release) to 1.0 (fast release)
    
    // State information
    Stage getCurrentStage() const { return currentStage_; }
    bool isActive() const { return currentStage_ != Stage::Idle && currentStage_ != Stage::Killed; }
    
    // Sample rate management
    void setSampleRate(int sampleRate);
    
private:
    // Calculate rates based on time values
    void updateRates();
    
    // Apply curve to linear progression
    float applyCurve(float value, float curve) const;
    
    float attack_;        // seconds
    float decay_;         // seconds
    float sustain_;       // level (0-1)
    float release_;       // seconds
    
    float attackCurve_;   // curve shape (-1 to 1)
    float decayCurve_;    // curve shape (-1 to 1)
    float releaseCurve_;  // curve shape (-1 to 1)
    
    float attackRate_;    // calculated rate for current sample rate
    float decayRate_;     // calculated rate for current sample rate
    float releaseRate_;   // calculated rate for current sample rate
    
    float value_;         // current envelope value
    Stage currentStage_;  // current envelope stage
    int sampleRate_;      // current sample rate
    
    float stageProgress_; // progress through current stage (0-1)
};

} // namespace AIMusicHardware