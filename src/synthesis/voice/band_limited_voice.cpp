#include "../../../include/synthesis/voice/band_limited_voice.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

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
    
    // Don't null out the base class oscillator - it's used by the base Voice class
    // The base oscillator will exist but we won't use it for audio generation
}

BandLimitedVoice::~BandLimitedVoice() {
}

void BandLimitedVoice::noteOn(int midiNote, float velocity) {
    // Call base class noteOn to handle all the state setup
    Voice::noteOn(midiNote, velocity);
    
    // Also set frequency on our band-limited oscillator
    float freq = midiNoteToFrequency(midiNote);
    bandLimitedOscillator_->setFrequency(freq);
    
}

float BandLimitedVoice::generateSample() {
    // Call base class generateSample to handle all the voice state,
    // pitch modulation, and frequency updates
    float baseClassSample = Voice::generateSample();
    
    // If voice is not active, return immediately
    if (!isActive()) {
        return 0.0f;
    }
    
    // Now that the base class has updated frequency_, sync our oscillator
    bandLimitedOscillator_->setFrequency(frequency_);
    
    // Generate our own sample using the band-limited oscillator
    float sample = bandLimitedOscillator_->generateSample();
    
    // The base class already applied envelope and velocity,
    // so we need to extract the envelope value to apply it ourselves
    float envelopeValue = envelope_->getCurrentValue();
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
    std::cout << "BandLimitedVoice::setWaveform(" << static_cast<int>(waveType) << ")" << std::endl;
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


//==============================================================================
// BandLimitedVoiceManager Implementation
//==============================================================================

BandLimitedVoiceManager::BandLimitedVoiceManager(int sampleRate, int maxVoices, bool enableOversampling)
    : VoiceManager(sampleRate, maxVoices),
      defaultWaveform_(BandLimitedWavetable::WaveType::Saw),
      defaultOversamplingEnabled_(enableOversampling),
      defaultOversamplingFactor_(OversamplingProcessor::Factor::x1) {
    std::cout << "BandLimitedVoiceManager created with " << maxVoices << " voices" << std::endl;
    
    // The base class constructor created regular Voice instances
    // We need to replace them with BandLimitedVoice instances
    voices_.clear();
    
    // Create band-limited voices
    for (int i = 0; i < maxVoices; ++i) {
        voices_.push_back(createVoice());
    }
    
    std::cout << "Created " << voices_.size() << " BandLimitedVoice instances" << std::endl;
}

BandLimitedVoiceManager::~BandLimitedVoiceManager() {
}

void BandLimitedVoiceManager::setWaveform(BandLimitedWavetable::WaveType waveType) {
    defaultWaveform_ = waveType;
    std::cout << "BandLimitedVoiceManager::setWaveform(" << static_cast<int>(waveType) << ") - updating " << voices_.size() << " voices" << std::endl;
    
    // Update all existing voices
    int updatedCount = 0;
    for (auto& voice : voices_) {
        if (auto* blVoice = dynamic_cast<BandLimitedVoice*>(voice.get())) {
            blVoice->setWaveform(waveType);
            updatedCount++;
        }
    }
    std::cout << "Updated " << updatedCount << " BandLimitedVoice instances" << std::endl;
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