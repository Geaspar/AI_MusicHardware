#include "synthesis/voice/voice.h"
#include "synthesis/wavetable/wavetable.h"
#include "synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <iostream>

namespace AIMusicHardware {

// Helper function to convert MIDI note to frequency (A4 = 69 = 440Hz)
float midiNoteToFreq(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

// Voice class implementation
Voice::Voice(int sampleRate)
    : midiNote_(-1),
      velocity_(0.0f),
      frequency_(440.0f),
      baseFrequency_(440.0f),
      age_(0),
      channel_(0),
      state_(State::Inactive),
      sampleRate_(sampleRate),
      pressure_(0.0f),
      amplitudeModulation_(1.0f),
      rng_(std::random_device{}()),
      dcBlockerX1_(0.0f),
      dcBlockerY1_(0.0f) {
    
    // Create oscillator and envelope
    oscillator_ = std::make_unique<WavetableOscillator>(sampleRate);
    envelope_ = std::make_unique<ModEnvelope>(sampleRate);
    
    // Setup default envelope with slightly slower attack to prevent clicks
    envelope_->setAttack(0.02f);     // 20ms attack (smoother)
    envelope_->setDecay(0.1f);       // 100ms decay
    envelope_->setSustain(0.7f);     // 70% sustain
    envelope_->setRelease(0.5f);     // 500ms release
}

Voice::~Voice() {
}

void Voice::noteOn(int midiNote, float velocity) {
    midiNote_ = midiNote;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    baseFrequency_ = midiNoteToFrequency(midiNote);
    
    // Initialize pitch modulation for this note
    pitchMod_.basePitch = static_cast<float>(midiNote);
    pitchMod_.velocityValue = velocity;
    pitchMod_.randomValue = randomDist_(rng_); // New random value per note
    pitchMod_.smoothedPitch = pitchMod_.basePitch; // Start at base pitch
    
    age_ = 0;
    
    // Update frequency with all modulations
    updateOscillatorFrequency();
    
    // Start envelope
    envelope_->noteOn();
    
    // Update state
    state_ = State::Starting;
    
}

void Voice::noteOff() {
    if (state_ != State::Inactive && state_ != State::Finished) {
        envelope_->noteOff();
        state_ = State::Released;
    }
}

void Voice::reset() {
    midiNote_ = -1;
    velocity_ = 0.0f;
    age_ = 0;
    
    // Reset modulation
    amplitudeModulation_ = 1.0f;
    
    // Reset pitch modulation to defaults
    pitchMod_ = PitchModulation(); // Reset to default values
    
    // Reset DC blocker state
    dcBlockerX1_ = 0.0f;
    dcBlockerY1_ = 0.0f;
    
    envelope_->reset();
    
    state_ = State::Inactive;
}

float Voice::generateSample() {
    // Only generate sound if voice is active
    if (state_ == State::Inactive || state_ == State::Finished) {
        return 0.0f;
    }
    
    // Increment age counter for voice stealing
    age_++;
    
    // Update envelope value for pitch modulation
    pitchMod_.envValue = envelope_->getCurrentValue();
    
    // Calculate total pitch (Vital-style unified calculation)
    float targetPitch = pitchMod_.calculateTotalPitch();
    
    // Smooth pitch changes to avoid clicks
    pitchMod_.updateSmoothedPitch(targetPitch);
    
    // Update frequency every 64 samples for efficiency (like Vital's block processing)
    if (age_ % 64 == 0) {
        updateOscillatorFrequency();
        
        // Debug output for pitch modulation (only first few times)
        if (age_ < 256 && pitchMod_.lfo1ToPitch != 0.0f) {
            std::cout << "Voice pitch mod - LFO1 amount: " << pitchMod_.lfo1ToPitch 
                      << ", LFO1 value: " << pitchMod_.lfo1Value 
                      << ", Total pitch: " << pitchMod_.smoothedPitch << std::endl;
        }
    }
    
    // Generate oscillator sample
    float sample = oscillator_->generateSample();
    
    // Apply envelope
    float envValue = envelope_->generateValue();
    sample *= envValue * velocity_;
    
    // Apply amplitude modulation
    sample *= amplitudeModulation_;
    
    // Apply lighter DC blocker to avoid aggressively removing low-level tails
    const float dc = 0.9995f; // gentler high-pass to preserve long releases
    float dcBlockerOutput = sample - dcBlockerX1_ + dc * dcBlockerY1_;
    dcBlockerX1_ = sample;
    dcBlockerY1_ = dcBlockerOutput;
    sample = dcBlockerOutput;
    
    // Debug output for first few samples (removed verbose logging)
    
    // Update state based on envelope
    if (state_ == State::Starting && envValue > 0.01f) {
        state_ = State::Playing;
    } else if (state_ == State::Released && !envelope_->isActive()) {
        state_ = State::Finished;
    }
    
    return sample;
}

void Voice::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames; ++i) {
        float sample = generateSample();
        
        // Apply to both channels (stereo)
        buffer[i * 2] += sample;
        buffer[i * 2 + 1] += sample;
    }
}

float Voice::getCurrentAmplitude() const {
    return envelope_->getCurrentValue() * velocity_;
}

void Voice::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    oscillator_->setWavetable(wavetable);
}

void Voice::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    oscillator_->setSampleRate(sampleRate);
    envelope_->setSampleRate(sampleRate);
}

void Voice::setPitchBend(float semitones) {
    pitchMod_.pitchBend = semitones;
    
    // Only update frequency if voice is active
    if (state_ != State::Inactive && state_ != State::Finished) {
        // Don't force update here, let generateSample handle it smoothly
    }
}

void Voice::setPitchModulationAmount(const std::string& source, float semitones) {
    if (source == "lfo1") {
        pitchMod_.lfo1ToPitch = semitones;
    } else if (source == "lfo2") {
        pitchMod_.lfo2ToPitch = semitones;
    } else if (source == "envelope") {
        pitchMod_.envToPitch = semitones;
    } else if (source == "velocity") {
        pitchMod_.velocityToPitch = semitones;
    } else if (source == "random") {
        pitchMod_.randomToPitch = semitones;
    } else if (source == "note") {
        pitchMod_.noteToPitch = semitones;
    }
}

void Voice::setPitchModulationValue(const std::string& source, float value) {
    if (source == "lfo1") {
        pitchMod_.lfo1Value = value;
    } else if (source == "lfo2") {
        pitchMod_.lfo2Value = value;
    }
    // Note: envelope, velocity, and random values are updated internally
}

void Voice::updateOscillatorFrequency() {
    // Use the smoothed pitch value for frequency calculation
    float totalPitchInSemitones = pitchMod_.smoothedPitch;
    
    // Convert total pitch (in MIDI note units) to frequency
    frequency_ = 440.0f * std::pow(2.0f, (totalPitchInSemitones - 69.0f) / 12.0f);
    
    // Update oscillator
    oscillator_->setFrequency(frequency_);
}

float Voice::midiNoteToFrequency(int midiNote) const {
    return midiNoteToFreq(midiNote);
}

} // namespace AIMusicHardware
