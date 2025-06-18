#include "../../../include/synthesis/voice/band_limited_voice.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <iostream>
#include <cmath>

namespace AIMusicHardware {

// Helper function to convert MIDI note to frequency
static float midiNoteToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

//==============================================================================
// BandLimitedVoice Implementation
//==============================================================================

BandLimitedVoice::BandLimitedVoice(int sampleRate, bool enableOversampling)
    : Voice(sampleRate),
      currentWaveform_(BandLimitedWavetable::WaveType::Saw),
      oversamplingEnabled_(enableOversampling),
      oversamplingFactor_(OversamplingProcessor::Factor::x1),
      dcBlockerX1_(0.0f),
      dcBlockerY1_(0.0f) {
    
    // Create band-limited oscillator
    bandLimitedOscillator_ = OscillatorFactory::createBandLimitedOscillator(
        sampleRate,
        currentWaveform_,
        oversamplingEnabled_
    );
    
    // Hide the base class oscillator since we're using our own
    oscillator_.reset();
}

BandLimitedVoice::~BandLimitedVoice() {
}

float BandLimitedVoice::generateSample() {
    if (!isActive()) {
        return 0.0f;
    }
    
    // Update age for voice stealing
    incrementAge();
    
    // Get envelope value
    float envelopeValue = envelope_->generateValue();
    
    // Check if voice is finished
    if (getState() == State::Released && envelopeValue < 0.001f) {
        state_ = State::Finished;
        return 0.0f;
    }
    
    // Update state if we're starting
    if (getState() == State::Starting && envelopeValue > 0.0f) {
        state_ = State::Playing;
    }
    
    // Update oscillator frequency based on pitch modulation
    updateOscillatorFrequency();
    
    // Generate oscillator sample
    float sample = bandLimitedOscillator_->generateSample();
    
    // Apply envelope and velocity
    sample *= envelopeValue * velocity_ * getAmplitudeModulation();
    
    // DC blocking filter (high-pass at ~20Hz)
    const float dcBlockerCutoff = 0.995f;
    float dcBlockerOutput = sample - dcBlockerX1_ + dcBlockerCutoff * dcBlockerY1_;
    dcBlockerX1_ = sample;
    dcBlockerY1_ = dcBlockerOutput;
    
    return dcBlockerOutput;
}

void BandLimitedVoice::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames; ++i) {
        buffer[i] = generateSample();
    }
}

void BandLimitedVoice::setWaveform(BandLimitedWavetable::WaveType waveType) {
    currentWaveform_ = waveType;
    bandLimitedOscillator_->setWaveform(waveType);
}

BandLimitedWavetable::WaveType BandLimitedVoice::getWaveform() const {
    return currentWaveform_;
}

void BandLimitedVoice::setOversamplingEnabled(bool enable) {
    oversamplingEnabled_ = enable;
    bandLimitedOscillator_->setOversamplingEnabled(enable);
}

bool BandLimitedVoice::isOversamplingEnabled() const {
    return oversamplingEnabled_;
}

void BandLimitedVoice::setOversamplingFactor(OversamplingProcessor::Factor factor) {
    oversamplingFactor_ = factor;
    bandLimitedOscillator_->setOversamplingFactor(factor);
}

OversamplingProcessor::Factor BandLimitedVoice::getOversamplingFactor() const {
    return oversamplingFactor_;
}

void BandLimitedVoice::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    // Ignore this call - we use band-limited wavetables instead
    // This is here for compatibility with the base Voice class
}

void BandLimitedVoice::setSampleRate(int sampleRate) {
    Voice::setSampleRate(sampleRate);
    
    // Recreate oscillator with new sample rate
    bandLimitedOscillator_ = OscillatorFactory::createBandLimitedOscillator(
        sampleRate,
        currentWaveform_,
        oversamplingEnabled_
    );
    bandLimitedOscillator_->setOversamplingFactor(oversamplingFactor_);
}

void BandLimitedVoice::updateOscillatorFrequency() {
    // Get total pitch from modulation system
    float totalPitch = getTotalPitch();
    
    // Convert to frequency
    float frequency = midiNoteToFrequency(static_cast<int>(totalPitch));
    
    // Update oscillator
    bandLimitedOscillator_->setFrequency(frequency);
}

//==============================================================================
// BandLimitedVoiceManager Implementation
//==============================================================================

BandLimitedVoiceManager::BandLimitedVoiceManager(int sampleRate, int maxVoices, bool enableOversampling)
    : VoiceManager(sampleRate, maxVoices),
      defaultWaveform_(BandLimitedWavetable::WaveType::Saw),
      defaultOversamplingEnabled_(enableOversampling),
      defaultOversamplingFactor_(OversamplingProcessor::Factor::x1) {
}

BandLimitedVoiceManager::~BandLimitedVoiceManager() {
}

void BandLimitedVoiceManager::setWaveform(BandLimitedWavetable::WaveType waveType) {
    defaultWaveform_ = waveType;
    
    // Update all existing voices
    for (auto& voice : voices_) {
        if (auto* blVoice = dynamic_cast<BandLimitedVoice*>(voice.get())) {
            blVoice->setWaveform(waveType);
        }
    }
}

void BandLimitedVoiceManager::setOversamplingEnabled(bool enable) {
    defaultOversamplingEnabled_ = enable;
    
    // Update all existing voices
    for (auto& voice : voices_) {
        if (auto* blVoice = dynamic_cast<BandLimitedVoice*>(voice.get())) {
            blVoice->setOversamplingEnabled(enable);
        }
    }
}

void BandLimitedVoiceManager::setOversamplingFactor(OversamplingProcessor::Factor factor) {
    defaultOversamplingFactor_ = factor;
    
    // Update all existing voices
    for (auto& voice : voices_) {
        if (auto* blVoice = dynamic_cast<BandLimitedVoice*>(voice.get())) {
            blVoice->setOversamplingFactor(factor);
        }
    }
}

std::unique_ptr<Voice> BandLimitedVoiceManager::createVoice() {
    auto voice = std::make_unique<BandLimitedVoice>(getSampleRate(), defaultOversamplingEnabled_);
    voice->setWaveform(defaultWaveform_);
    voice->setOversamplingFactor(defaultOversamplingFactor_);
    return voice;
}

} // namespace AIMusicHardware