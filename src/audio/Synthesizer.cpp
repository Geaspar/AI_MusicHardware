#include "../../include/audio/Synthesizer.h"
#include "../../include/sequencer/Sequencer.h"
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
    
    // Default envelope values remain unchanged
}

void Voice::noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env) {
    frequency_ = midiNoteToFrequency(midiNote);
    amplitude_ = std::clamp(velocity, 0.0f, 1.0f);
    isActive_ = true;
    
    // Use the provided envelope parameters
    attack_ = env.attack;
    decay_ = env.decay;
    sustain_ = env.sustain;
    release_ = env.release;
    
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
    
    // Use a constant sample rate for now
    // In a real implementation, this would be passed from the Synthesizer
    const float sampleRate = 44100.0f;
    
    // Update envelope
    switch (envelopeStage_) {
        case EnvelopeStage::Idle:
            envelopeValue_ = 0.0f;
            break;
            
        case EnvelopeStage::Attack:
            // Faster attack for better responsiveness
            envelopeValue_ += 1.0f / (attack_ * sampleRate);
            if (envelopeValue_ >= 1.0f) {
                envelopeValue_ = 1.0f;
                envelopeStage_ = EnvelopeStage::Decay;
            }
            break;
            
        case EnvelopeStage::Decay:
            envelopeValue_ -= (1.0f - sustain_) / (decay_ * sampleRate);
            if (envelopeValue_ <= sustain_) {
                envelopeValue_ = sustain_;
                envelopeStage_ = EnvelopeStage::Sustain;
            }
            break;
            
        case EnvelopeStage::Sustain:
            envelopeValue_ = sustain_;
            break;
            
        case EnvelopeStage::Release:
            // Ensure release always completes by using an absolute rate rather than relative to sustain
            float releaseRate = envelopeValue_ / (release_ * sampleRate);
            envelopeValue_ -= releaseRate;
            
            if (envelopeValue_ <= 0.001f) {  // Small threshold to ensure complete silence
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
    
    // Voices will be created in initialize()
}

bool Synthesizer::initialize() {
    try {
        // Create voices
        for (int i = 0; i < kMaxVoices; ++i) {
            voices_.push_back(std::make_unique<Voice>());
            if (!voices_.back()) {
                return false;
            }
            voices_.back()->setOscillatorType(currentOscType_);
        }
        return true;
    } catch (const std::exception& e) {
        // Handle any exceptions during initialization
        return false;
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

void Synthesizer::noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env) {
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
    
    // Trigger the note with custom envelope
    if (voice) {
        voice->setOscillatorType(currentOscType_);
        voice->noteOn(midiNote, velocity, env);
    }
}

void Synthesizer::noteOff(int midiNote) {
    // Simple implementation - for now, just turn off all active voices
    // In a real synth, we'd need to track which voices are playing which notes
    // so we can only turn off the specific voice playing this note
    
    // For now, the simplest solution is to call allNotesOff
    // In a real implementation, we would want to track which voices are playing which notes
    
    // Turn off all active voices since we can't identify which one is playing the note
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            voice->noteOff();
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
    
    // Count active voices for dynamic gain adjustment
    int activeVoiceCount = 0;
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            activeVoiceCount++;
        }
    }
    
    // Calculate gain reduction based on active voices
    float gainReduction = 1.0f;
    if (activeVoiceCount > 0) {
        // Apply a gain reduction when multiple voices are active
        // 1 voice = 1.0, 2 voices = 0.7, 3+ voices = 0.5 (approximate)
        gainReduction = 1.0f / (1.0f + 0.3f * activeVoiceCount);
    }
    
    // Process each voice
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            for (int i = 0; i < numFrames; ++i) {
                float sample = voice->generateSample() * gainReduction;
                // Apply to both channels (stereo)
                outputBuffer[i * 2] += sample;
                outputBuffer[i * 2 + 1] += sample;
            }
        }
    }
    
    // Additional master volume reduction to prevent distortion
    const float masterVolume = 0.7f;
    
    // Prevent clipping - apply gain and limiter
    for (int i = 0; i < numFrames * 2; ++i) {
        outputBuffer[i] *= masterVolume;
        outputBuffer[i] = std::clamp(outputBuffer[i], -1.0f, 1.0f);
    }
}

} // namespace AIMusicHardware