#include "../../../include/synthesis/voice/voice_manager.h"
#include "../../../include/synthesis/wavetable/wavetable.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <algorithm>
#include <cmath>
#include <random>

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
      age_(0),
      state_(State::Inactive),
      sampleRate_(sampleRate) {
    
    // Create oscillator and envelope
    oscillator_ = std::make_unique<WavetableOscillator>(sampleRate);
    envelope_ = std::make_unique<ModEnvelope>(sampleRate);
    
    // Setup default envelope
    envelope_->setAttack(0.01f);     // 10ms attack
    envelope_->setDecay(0.1f);       // 100ms decay
    envelope_->setSustain(0.7f);     // 70% sustain
    envelope_->setRelease(0.5f);     // 500ms release
}

Voice::~Voice() {
}

void Voice::noteOn(int midiNote, float velocity) {
    midiNote_ = midiNote;
    velocity_ = std::clamp(velocity, 0.0f, 1.0f);
    frequency_ = midiNoteToFrequency(midiNote);
    age_ = 0;
    
    // Set oscillator frequency
    oscillator_->setFrequency(frequency_);
    
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
    
    // Generate oscillator sample
    float sample = oscillator_->generateSample();
    
    // Apply envelope
    float envValue = envelope_->generateValue();
    sample *= envValue * velocity_;
    
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

float Voice::midiNoteToFrequency(int midiNote) const {
    return midiNoteToFreq(midiNote);
}

// VoiceManager implementation
VoiceManager::VoiceManager(int sampleRate, int maxVoices)
    : sampleRate_(sampleRate),
      maxVoices_(maxVoices),
      stealMode_(StealMode::Oldest) {
    
    // Create initial voices
    for (int i = 0; i < maxVoices_; ++i) {
        voices_.push_back(createVoice());
    }
    
    // Create a default wavetable
    currentWavetable_ = std::make_shared<Wavetable>();
    currentWavetable_->initBasicWaveforms();
    
    // Assign wavetable to all voices
    for (auto& voice : voices_) {
        voice->setWavetable(currentWavetable_);
    }
}

VoiceManager::~VoiceManager() {
}

void VoiceManager::noteOn(int midiNote, float velocity) {
    // Check if this note is already playing
    Voice* voice = findVoiceForNote(midiNote);
    
    // If not playing, find an unused voice or steal one
    if (!voice) {
        // First look for an inactive voice
        for (auto& v : voices_) {
            if (!v->isActive()) {
                voice = v.get();
                break;
            }
        }
        
        // If all voices are in use, use voice stealing
        if (!voice) {
            voice = findVoiceToSteal();
        }
    }
    
    // Trigger the voice with this note
    if (voice) {
        voice->noteOn(midiNote, velocity);
        activeNotes_[midiNote] = voice;
    }
}

void VoiceManager::noteOff(int midiNote) {
    // Find the voice playing this note
    auto it = activeNotes_.find(midiNote);
    if (it != activeNotes_.end()) {
        it->second->noteOff();
        activeNotes_.erase(it);
    }
}

void VoiceManager::allNotesOff() {
    for (auto& voice : voices_) {
        voice->noteOff();
    }
    activeNotes_.clear();
}

void VoiceManager::process(float* buffer, int numFrames) {
    // Clear output buffer
    std::fill(buffer, buffer + numFrames * 2, 0.0f);
    
    // Count active voices for dynamic gain adjustment
    int activeVoiceCount = 0;
    
    // Process each voice
    for (auto& voice : voices_) {
        if (voice->isActive()) {
            activeVoiceCount++;
            voice->process(buffer, numFrames);
        }
    }
    
    // Apply output gain based on active voice count
    if (activeVoiceCount > 1) {
        float gain = 1.0f / std::sqrt(static_cast<float>(activeVoiceCount));
        for (int i = 0; i < numFrames * 2; ++i) {
            buffer[i] *= gain;
        }
    }
    
    // Clean up voices that have finished their release phase
    for (auto it = activeNotes_.begin(); it != activeNotes_.end();) {
        if (it->second->getState() == Voice::State::Finished) {
            it = activeNotes_.erase(it);
        } else {
            ++it;
        }
    }
}

void VoiceManager::setMaxVoices(int maxVoices) {
    maxVoices_ = std::max(1, maxVoices);
    
    // Add voices if needed
    while (static_cast<int>(voices_.size()) < maxVoices_) {
        voices_.push_back(createVoice());
    }
    
    // Or remove excess voices
    while (static_cast<int>(voices_.size()) > maxVoices_) {
        // Find an inactive voice to remove
        for (auto it = voices_.begin(); it != voices_.end(); ++it) {
            if (!(*it)->isActive()) {
                voices_.erase(it);
                break;
            }
        }
        
        // If all voices are active, just remove the last one
        if (static_cast<int>(voices_.size()) > maxVoices_) {
            voices_.pop_back();
        }
    }
}

void VoiceManager::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update all voices
    for (auto& voice : voices_) {
        voice->setSampleRate(sampleRate);
    }
}

void VoiceManager::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    if (wavetable) {
        currentWavetable_ = wavetable;
        
        // Update all voices
        for (auto& voice : voices_) {
            voice->setWavetable(wavetable);
        }
    }
}

Voice* VoiceManager::findVoiceToSteal() {
    switch (stealMode_) {
        case StealMode::Oldest: {
            // Find the oldest voice that's not in release
            Voice* oldestVoice = nullptr;
            int oldestAge = -1;
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased() && voice->getAge() > oldestAge) {
                    oldestAge = voice->getAge();
                    oldestVoice = voice.get();
                }
            }
            
            // If no active non-released voice found, use any voice
            if (!oldestVoice && !voices_.empty()) {
                oldestVoice = voices_[0].get();
            }
            
            return oldestVoice;
        }
        
        case StealMode::Quietest: {
            // Find the quietest voice that's not in release
            Voice* quietestVoice = nullptr;
            float lowestAmp = 2.0f; // Higher than max amplitude (1.0)
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased()) {
                    float amp = voice->getCurrentAmplitude();
                    if (amp < lowestAmp) {
                        lowestAmp = amp;
                        quietestVoice = voice.get();
                    }
                }
            }
            
            // If no active non-released voice found, use any voice
            if (!quietestVoice && !voices_.empty()) {
                quietestVoice = voices_[0].get();
            }
            
            return quietestVoice;
        }
        
        case StealMode::Random: {
            // Choose a random voice that's not in release
            std::vector<Voice*> candidates;
            
            for (auto& voice : voices_) {
                if (voice->isActive() && !voice->isReleased()) {
                    candidates.push_back(voice.get());
                }
            }
            
            if (!candidates.empty()) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(0, static_cast<int>(candidates.size()) - 1);
                return candidates[dist(gen)];
            }
            
            // If no candidates, use the first voice
            if (!voices_.empty()) {
                return voices_[0].get();
            }
            
            return nullptr;
        }
        
        default:
            return !voices_.empty() ? voices_[0].get() : nullptr;
    }
}

Voice* VoiceManager::findVoiceForNote(int midiNote) {
    auto it = activeNotes_.find(midiNote);
    return (it != activeNotes_.end()) ? it->second : nullptr;
}

std::unique_ptr<Voice> VoiceManager::createVoice() {
    auto voice = std::make_unique<Voice>(sampleRate_);
    if (currentWavetable_) {
        voice->setWavetable(currentWavetable_);
    }
    return voice;
}

} // namespace AIMusicHardware