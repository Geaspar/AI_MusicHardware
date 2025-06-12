#pragma once

#include <cmath>
#include <atomic>
#include <random>
#include <array>

namespace AIMusicHardware {

class LFO {
public:
    enum class WaveShape {
        Sine,
        Triangle,
        Saw,
        Square,
        Random,      // Sample & Hold
        Smooth,      // Smooth Random
        Count
    };
    
    enum class SyncMode {
        Free,        // Free running
        Tempo,       // Sync to tempo
        Trigger      // Reset on trigger
    };
    
    struct Parameters {
        float rate = 1.0f;           // Hz in free mode, multiplier in tempo mode
        float depth = 1.0f;          // 0-1 modulation depth
        float phase = 0.0f;          // 0-1 phase offset
        WaveShape shape = WaveShape::Sine;
        SyncMode syncMode = SyncMode::Free;
        bool bipolar = true;         // true: -1 to 1, false: 0 to 1
    };
    
    LFO(float sampleRate);
    ~LFO() = default;
    
    // Main processing
    float process();
    void reset();
    void trigger();
    
    // Parameter setters
    void setRate(float hz);
    void setDepth(float depth);
    void setPhase(float phase);
    void setShape(WaveShape shape);
    void setSyncMode(SyncMode mode);
    void setBipolar(bool bipolar);
    void setTempo(float bpm);
    void setSampleRate(float sampleRate);
    
    // Parameter getters
    float getRate() const { return parameters_.load().rate; }
    float getDepth() const { return parameters_.load().depth; }
    float getPhase() const { return parameters_.load().phase; }
    WaveShape getShape() const { return parameters_.load().shape; }
    SyncMode getSyncMode() const { return parameters_.load().syncMode; }
    bool isBipolar() const { return parameters_.load().bipolar; }
    float getCurrentValue() const { return currentValue_; }
    float getCurrentPhase() const { return currentPhase_; }
    
    // Get all parameters at once (thread-safe)
    Parameters getParameters() const;
    void setParameters(const Parameters& params);
    
private:
    // Generate waveforms
    float generateSine(float phase);
    float generateTriangle(float phase);
    float generateSaw(float phase);
    float generateSquare(float phase);
    float generateRandom(float phase);
    float generateSmooth(float phase);
    
    // Internal state
    std::atomic<float> sampleRate_;
    std::atomic<float> tempo_;
    std::atomic<Parameters> parameters_;
    
    float currentPhase_;
    float currentValue_;
    float phaseIncrement_;
    
    // Random generation
    std::mt19937 randomGenerator_;
    std::uniform_real_distribution<float> randomDist_;
    float lastRandomValue_;
    float targetRandomValue_;
    float randomPhaseThreshold_;
    
    // Smoothing filter for parameter changes
    class SmoothingFilter {
    public:
        SmoothingFilter() : current_(0.0f), target_(0.0f), smoothing_(0.99f) {}
        
        void setTarget(float value) { target_ = value; }
        void setSmoothingFactor(float factor) { smoothing_ = factor; }
        void reset(float value) { current_ = target_ = value; }
        
        float process() {
            current_ = current_ * smoothing_ + target_ * (1.0f - smoothing_);
            return current_;
        }
        
    private:
        float current_;
        float target_;
        float smoothing_;
    };
    
    SmoothingFilter rateSmoothing_;
    SmoothingFilter depthSmoothing_;
    
    // Helper functions
    void updatePhaseIncrement();
    float wrapPhase(float phase);
    
    // Constants
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TWO_PI = 2.0f * PI;
};

} // namespace AIMusicHardware