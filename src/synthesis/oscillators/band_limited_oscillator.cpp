#include "../../../include/synthesis/oscillators/band_limited_oscillator.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace AIMusicHardware {

BandLimitedOscillator::BandLimitedOscillator(float sampleRate)
    : frequency_(440.0f),
      phase_(0.0f),
      phaseIncrement_(0.0f),
      amplitude_(1.0f),
      sampleRate_(sampleRate),
      oversamplingEnabled_(false) {
    
    // Create wavetable
    wavetable_ = std::make_unique<BandLimitedWavetable>(sampleRate);
    wavetable_->initWaveform(BandLimitedWavetable::WaveType::Saw);
    
    // Create oversampler (default 2x)
    oversampler_ = std::make_unique<OversamplingProcessor>(OversamplingProcessor::Factor::x2);
    
    // Calculate initial phase increment
    updatePhaseIncrement();
}

BandLimitedOscillator::~BandLimitedOscillator() {
}

void BandLimitedOscillator::setWaveform(BandLimitedWavetable::WaveType type) {
    wavetable_->initWaveform(type);
}

void BandLimitedOscillator::setFrequency(float frequency) {
    frequency_ = std::max(0.0f, std::min(frequency, sampleRate_ * 0.5f));
    updatePhaseIncrement();
}

void BandLimitedOscillator::setPhase(float phase) {
    // Ensure phase is in range [0, 1)
    phase_ = phase - std::floor(phase);
}

void BandLimitedOscillator::resetPhase() {
    phase_ = 0.0f;
}

void BandLimitedOscillator::setAmplitude(float amplitude) {
    amplitude_ = std::clamp(amplitude, 0.0f, 1.0f);
}

void BandLimitedOscillator::setOversamplingEnabled(bool enable) {
    oversamplingEnabled_ = enable;
    updatePhaseIncrement();
}

void BandLimitedOscillator::setOversamplingFactor(OversamplingProcessor::Factor factor) {
    oversampler_ = std::make_unique<OversamplingProcessor>(factor);
    updatePhaseIncrement();
}

float BandLimitedOscillator::generateSample() {
    if (oversamplingEnabled_ && oversampler_) {
        // Use oversampling
        float output = 0.0f;
        oversampler_->process(nullptr, &output, 1,
            [this]() { return generateSampleDirect(); });
        return output * amplitude_;
    } else {
        // Direct generation without oversampling
        return generateSampleDirect() * amplitude_;
    }
}

void BandLimitedOscillator::generateBlock(float* output, int numSamples) {
    if (oversamplingEnabled_ && oversampler_) {
        // Process with oversampling
        oversampler_->process(nullptr, output, numSamples,
            [this]() { return generateSampleDirect(); });
        
        // Apply amplitude
        for (int i = 0; i < numSamples; ++i) {
            output[i] *= amplitude_;
        }
    } else {
        // Direct generation
        for (int i = 0; i < numSamples; ++i) {
            output[i] = generateSampleDirect() * amplitude_;
        }
    }
}

void BandLimitedOscillator::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    wavetable_->setSampleRate(sampleRate);
    updatePhaseIncrement();
}

void BandLimitedOscillator::updatePhaseIncrement() {
    float effectiveSampleRate = sampleRate_;
    
    if (oversamplingEnabled_ && oversampler_) {
        effectiveSampleRate *= oversampler_->getFactor();
    }
    
    phaseIncrement_ = frequency_ / effectiveSampleRate;
    
}

float BandLimitedOscillator::generateSampleDirect() {
    // Get sample from wavetable
    float sample = wavetable_->getSample(phase_, frequency_);
    
    
    // Advance phase
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }
    
    return sample;
}

// OscillatorFactory implementation

std::unique_ptr<BandLimitedOscillator> OscillatorFactory::createBandLimitedOscillator(
    float sampleRate, BandLimitedWavetable::WaveType waveform, bool enableOversampling) {
    
    auto osc = std::make_unique<BandLimitedOscillator>(sampleRate);
    osc->setWaveform(waveform);
    osc->setOversamplingEnabled(enableOversampling);
    
    return osc;
}

std::unique_ptr<BandLimitedOscillator> OscillatorFactory::createLFO(
    float sampleRate, BandLimitedWavetable::WaveType waveform) {
    
    auto lfo = std::make_unique<BandLimitedOscillator>(sampleRate);
    lfo->setWaveform(waveform);
    lfo->setOversamplingEnabled(false);  // LFOs don't need oversampling
    
    return lfo;
}

} // namespace AIMusicHardware