#include "synthesis/modulators/LFO.h"
#include <algorithm>

namespace AIMusicHardware {

LFO::LFO(float sampleRate) 
    : sampleRate_(sampleRate)
    , tempo_(120.0f)
    , currentPhase_(0.0f)
    , currentValue_(0.0f)
    , phaseIncrement_(0.0f)
    , randomGenerator_(std::random_device{}())
    , randomDist_(-1.0f, 1.0f)
    , lastRandomValue_(0.0f)
    , targetRandomValue_(0.0f)
    , randomPhaseThreshold_(0.0f) {
    
    // Initialize parameters with default values
    Parameters params;
    params.rate = 1.0f;
    params.depth = 1.0f;
    params.phase = 0.0f;
    params.shape = WaveShape::Sine;
    params.syncMode = SyncMode::Free;
    params.bipolar = true;
    parameters_.store(params);
    
    updatePhaseIncrement();
    rateSmoothing_.setSmoothingFactor(0.995f);
    depthSmoothing_.setSmoothingFactor(0.99f);
}

float LFO::process() {
    Parameters params = parameters_.load();
    
    // Smooth parameter changes
    rateSmoothing_.setTarget(params.rate);
    depthSmoothing_.setTarget(params.depth);
    
    float smoothedRate = rateSmoothing_.process();
    float smoothedDepth = depthSmoothing_.process();
    
    // Update phase increment if rate changed significantly
    if (std::abs(smoothedRate - params.rate) > 0.001f) {
        params.rate = smoothedRate;
        parameters_.store(params);
        updatePhaseIncrement();
    }
    
    // Generate waveform
    float value = 0.0f;
    switch (params.shape) {
        case WaveShape::Sine:
            value = generateSine(currentPhase_);
            break;
        case WaveShape::Triangle:
            value = generateTriangle(currentPhase_);
            break;
        case WaveShape::Saw:
            value = generateSaw(currentPhase_);
            break;
        case WaveShape::Square:
            value = generateSquare(currentPhase_);
            break;
        case WaveShape::Random:
            value = generateRandom(currentPhase_);
            break;
        case WaveShape::Smooth:
            value = generateSmooth(currentPhase_);
            break;
        case WaveShape::Count:
            // Should never happen, but handle it to avoid warning
            value = 0.0f;
            break;
    }
    
    // Apply depth
    value *= smoothedDepth;
    
    // Convert to unipolar if needed
    if (!params.bipolar) {
        value = (value + 1.0f) * 0.5f;
    }
    
    // Update phase
    currentPhase_ += phaseIncrement_;
    currentPhase_ = wrapPhase(currentPhase_);
    
    currentValue_ = value;
    return value;
}

void LFO::reset() {
    currentPhase_ = parameters_.load().phase;
    lastRandomValue_ = 0.0f;
    targetRandomValue_ = 0.0f;
    rateSmoothing_.reset(parameters_.load().rate);
    depthSmoothing_.reset(parameters_.load().depth);
}

void LFO::trigger() {
    if (parameters_.load().syncMode == SyncMode::Trigger) {
        reset();
    }
}

void LFO::setRate(float hz) {
    Parameters params = parameters_.load();
    params.rate = std::max(0.01f, std::min(hz, 50.0f));
    parameters_.store(params);
    updatePhaseIncrement();
}

void LFO::setDepth(float depth) {
    Parameters params = parameters_.load();
    params.depth = std::max(0.0f, std::min(depth, 1.0f));
    parameters_.store(params);
}

void LFO::setPhase(float phase) {
    Parameters params = parameters_.load();
    params.phase = std::max(0.0f, std::min(phase, 1.0f));
    parameters_.store(params);
}

void LFO::setShape(WaveShape shape) {
    Parameters params = parameters_.load();
    params.shape = shape;
    parameters_.store(params);
}

void LFO::setSyncMode(SyncMode mode) {
    Parameters params = parameters_.load();
    params.syncMode = mode;
    parameters_.store(params);
    updatePhaseIncrement();
}

void LFO::setBipolar(bool bipolar) {
    Parameters params = parameters_.load();
    params.bipolar = bipolar;
    parameters_.store(params);
}

void LFO::setTempo(float bpm) {
    tempo_.store(bpm);
    if (parameters_.load().syncMode == SyncMode::Tempo) {
        updatePhaseIncrement();
    }
}

void LFO::setSampleRate(float sampleRate) {
    sampleRate_.store(sampleRate);
    updatePhaseIncrement();
}

LFO::Parameters LFO::getParameters() const {
    return parameters_.load();
}

void LFO::setParameters(const Parameters& params) {
    parameters_.store(params);
    updatePhaseIncrement();
}

float LFO::generateSine(float phase) {
    return std::sin(phase * TWO_PI);
}

float LFO::generateTriangle(float phase) {
    if (phase < 0.25f) {
        return phase * 4.0f;
    } else if (phase < 0.75f) {
        return 2.0f - (phase * 4.0f);
    } else {
        return (phase * 4.0f) - 4.0f;
    }
}

float LFO::generateSaw(float phase) {
    return 2.0f * phase - 1.0f;
}

float LFO::generateSquare(float phase) {
    return phase < 0.5f ? 1.0f : -1.0f;
}

float LFO::generateRandom(float phase) {
    // Sample & Hold - change value when phase wraps
    if (phase < randomPhaseThreshold_) {
        lastRandomValue_ = randomDist_(randomGenerator_);
        randomPhaseThreshold_ = phase + 0.1f; // Prevent multiple triggers
    } else if (phase > 0.9f) {
        randomPhaseThreshold_ = 0.0f; // Reset threshold
    }
    return lastRandomValue_;
}

float LFO::generateSmooth(float phase) {
    // Smooth random - interpolate between random values
    if (phase < 0.01f) {
        lastRandomValue_ = targetRandomValue_;
        targetRandomValue_ = randomDist_(randomGenerator_);
    }
    
    // Cosine interpolation for smoothness
    float t = phase;
    float smoothT = 0.5f * (1.0f - std::cos(t * PI));
    return lastRandomValue_ * (1.0f - smoothT) + targetRandomValue_ * smoothT;
}

void LFO::updatePhaseIncrement() {
    float rate = parameters_.load().rate;
    float sampleRate = sampleRate_.load();
    
    if (parameters_.load().syncMode == SyncMode::Tempo) {
        // In tempo sync mode, rate is a multiplier of beat frequency
        float bpm = tempo_.load();
        float beatHz = bpm / 60.0f;
        phaseIncrement_ = (beatHz * rate) / sampleRate;
    } else {
        // Free running mode
        phaseIncrement_ = rate / sampleRate;
    }
}

float LFO::wrapPhase(float phase) {
    while (phase >= 1.0f) phase -= 1.0f;
    while (phase < 0.0f) phase += 1.0f;
    return phase;
}

} // namespace AIMusicHardware