#include "../../include/audio/Synthesizer.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

// Constants
constexpr float TWO_PI = 6.28318530718f;

// Helper function to convert MIDI note to frequency (A4 = 69 = 440Hz)
float midiNoteToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

// Voice implementation
Voice::Voice()
    : frequency_(440.0f),
      amplitude_(0.0f),
      phase_(0.0f),
      oscType_(OscillatorType::Sine),
      isActive_(false),
      envelope_(0.0f),
      attack_(0.01f),     // 10ms attack
      decay_(0.1f),       // 100ms decay
      sustain_(0.7f),     // 70% sustain level
      release_(0.5f),     // 500ms release
      envelopeValue_(0.0f),
      envelopeStage_(EnvelopeStage::Idle) {
}

Voice::~Voice() {
}

void Voice::setFrequency(float frequency) {
    frequency_ = frequency;
}

void Voice::setOscillatorType(OscillatorType type) {
    oscType_ = type;
}

void Voice::setAmplitude(float amplitude) {
    amplitude_ = std::clamp(amplitude, 0.0f, 1.0f);
}

void Voice::noteOn(int midiNote, float velocity) {
    frequency_ = midiNoteToFrequency(midiNote);
    amplitude_ = std::clamp(velocity, 0.0f, 1.0f);
    isActive_ = true;
    envelopeStage_ = EnvelopeStage::Attack;
    envelopeValue_ = 0.0f;
}

void Voice::noteOff() {
    if (isActive_) {
        envelopeStage_ = EnvelopeStage::Release;
    }
}

bool Voice::isActive() const {
    return isActive_;
}

float Voice::generateSample() {
    if (!isActive_) {
        return 0.0f;
    }
    
    // Update envelope
    switch (envelopeStage_) {
        case EnvelopeStage::Idle:
            envelopeValue_ = 0.0f;
            break;
            
        case EnvelopeStage::Attack:
            envelopeValue_ += 1.0f / (attack_ * 44100.0f); // assuming 44.1kHz
            if (envelopeValue_ >= 1.0f) {
                envelopeValue_ = 1.0f;
                envelopeStage_ = EnvelopeStage::Decay;
            }
            break;
            
        case EnvelopeStage::Decay:
            envelopeValue_ -= (1.0f - sustain_) / (decay_ * 44100.0f);
            if (envelopeValue_ <= sustain_) {
                envelopeValue_ = sustain_;
                envelopeStage_ = EnvelopeStage::Sustain;
            }
            break;
            
        case EnvelopeStage::Sustain:
            envelopeValue_ = sustain_;
            break;
            
        case EnvelopeStage::Release:
            envelopeValue_ -= sustain_ / (release_ * 44100.0f);
            if (envelopeValue_ <= 0.0f) {
                envelopeValue_ = 0.0f;
                envelopeStage_ = EnvelopeStage::Idle;
                isActive_ = false;
            }
            break;
    }
    
    // Generate waveform
    float sample = 0.0f;
    
    // Phase increment for this sample
    float phaseIncrement = frequency_ / 44100.0f;
    
    switch (oscType_) {
        case OscillatorType::Sine:
            sample = std::sin(phase_ * TWO_PI);
            break;
            
        case OscillatorType::Square:
            sample = (phase_ < 0.5f) ? 1.0f : -1.0f;
            break;
            
        case OscillatorType::Saw:
            sample = 2.0f * phase_ - 1.0f;
            break;
            
        case OscillatorType::Triangle:
            sample = (phase_ < 0.5f) ? (4.0f * phase_ - 1.0f) : (3.0f - 4.0f * phase_);
            break;
            
        case OscillatorType::Noise:
            sample = 2.0f * (static_cast<float>(rand()) / RAND_MAX) - 1.0f;
            break;
    }
    
    // Update phase
    phase_ += phaseIncrement;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
    }
    
    // Apply envelope and amplitude
    return sample * amplitude_ * envelopeValue_;
}

// Synthesizer implementation
Synthesizer::Synthesizer(int sampleRate)
    : sampleRate_(sampleRate),
      currentOscType_(OscillatorType::Sine) {
    
    // Create voices
    for (int i = 0; i < kMaxVoices; ++i) {
        voices_.push_back(std::make_unique<Voice>());
        voices_.back()->setOscillatorType(currentOscType_);
    }
}

Synthesizer::~Synthesizer() {
}

void Synthesizer::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

void Synthesizer::noteOn(int midiNote, float velocity) {
    // Find an inactive voice or the oldest one
    Voice* voice = nullptr;
    
    // First try to find an inactive voice
    for (auto& v : voices_) {
        if (!v->isActive()) {
            voice = v.get();
            break;
        }
    }
    
    // If all voices are active, we could implement voice stealing here
    // For now, just use the first voice as a fallback
    if (!voice && !voices_.empty()) {
        voice = voices_[0].get();
    }
    
    // Trigger the note
    if (voice) {
        voice->setOscillatorType(currentOscType_);
        voice->noteOn(midiNote, velocity);
    }
}

void Synthesizer::noteOff(int midiNote) {
    // Simple implementation - doesn't account for multiple voices with same note
    // In a real synth, we'd need to track which voices are playing which notes
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            // This is a simplification - we'd normally check if this voice is playing this note
            float voiceFreq = voice->generateSample(); // just to avoid unused variable warning
            float noteFreq = midiNoteToFrequency(midiNote);
            
            // If frequencies are close, this voice is playing our target note
            // This is imprecise but serves as an example
            if (std::abs(voiceFreq - noteFreq) < 0.1f) {
                voice->noteOff();
            }
        }
    }
}

void Synthesizer::allNotesOff() {
    for (auto& voice : voices_) {
        voice->noteOff();
    }
}

void Synthesizer::setOscillatorType(OscillatorType type) {
    currentOscType_ = type;
    
    // Update all inactive voices immediately
    // Active voices will be updated when they're retriggered
    for (auto& voice : voices_) {
        if (!voice->isActive()) {
            voice->setOscillatorType(type);
        }
    }
}

void Synthesizer::process(float* outputBuffer, int numFrames) {
    // Clear buffer
    std::fill(outputBuffer, outputBuffer + numFrames * 2, 0.0f);
    
    // Process each voice
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            for (int i = 0; i < numFrames; ++i) {
                float sample = voice->generateSample();
                // Apply to both channels (stereo)
                outputBuffer[i * 2] += sample;
                outputBuffer[i * 2 + 1] += sample;
            }
        }
    }
    
    // Prevent clipping - simple gain reduction
    for (int i = 0; i < numFrames * 2; ++i) {
        outputBuffer[i] = std::clamp(outputBuffer[i], -1.0f, 1.0f);
    }
}

} // namespace AIMusicHardware