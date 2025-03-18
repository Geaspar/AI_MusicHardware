#include "../../include/audio/Synthesizer.h"
#include "../../include/sequencer/Sequencer.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace AIMusicHardware {

// Helper LFO source for modulation
class LfoSource : public ModulationSource {
public:
    enum class WaveShape {
        Sine,
        Triangle,
        Saw,
        Square,
        Random
    };
    
    LfoSource(const std::string& name, int sampleRate = 44100)
        : ModulationSource(name),
          sampleRate_(sampleRate),
          frequency_(1.0f),  // 1 Hz default
          phase_(0.0f),
          shape_(WaveShape::Sine),
          value_(0.0f) {
    }
    
    float getValue() const override {
        return value_;
    }
    
    void update() override {
        // Update phase
        phase_ += frequency_ / sampleRate_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
        
        // Generate value based on wave shape
        switch (shape_) {
            case WaveShape::Sine:
                value_ = std::sin(phase_ * 2.0f * 3.14159265359f);
                break;
                
            case WaveShape::Triangle:
                value_ = (phase_ < 0.5f) ? 
                    (4.0f * phase_ - 1.0f) : 
                    (3.0f - 4.0f * phase_);
                break;
                
            case WaveShape::Saw:
                value_ = 2.0f * phase_ - 1.0f;
                break;
                
            case WaveShape::Square:
                value_ = (phase_ < 0.5f) ? 1.0f : -1.0f;
                break;
                
            case WaveShape::Random:
                if (phase_ < prevPhase_) {
                    // Generate new random value when phase wraps
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                    value_ = dist(gen);
                }
                break;
        }
        
        prevPhase_ = phase_;
    }
    
    void setFrequency(float freq) {
        frequency_ = std::clamp(freq, 0.01f, 20.0f); // Limit to reasonable range
    }
    
    void setShape(WaveShape shape) {
        shape_ = shape;
    }
    
    void setSampleRate(int sampleRate) {
        sampleRate_ = sampleRate;
    }
    
private:
    int sampleRate_;
    float frequency_;
    float phase_;
    float prevPhase_ = 0.0f;
    WaveShape shape_;
    float value_;
};

// Synthesizer implementation
Synthesizer::Synthesizer(int sampleRate)
    : Processor(sampleRate),
      currentOscType_(OscillatorType::Sine) {
      
    // Create VoiceManager
    voiceManager_ = std::make_unique<VoiceManager>(sampleRate);
    
    // Create default wavetable
    createDefaultWavetable();
    
    // Create modulation sources
    createModulationSources();
}

Synthesizer::~Synthesizer() {
}

void Synthesizer::createDefaultWavetable() {
    currentWavetable_ = std::make_shared<Wavetable>();
    currentWavetable_->initBasicWaveforms();
    
    if (voiceManager_) {
        voiceManager_->setWavetable(currentWavetable_);
    }
}

bool Synthesizer::initialize() {
    try {
        // Nothing to do here now, constructor handles initialization
        return true;
    } catch (const std::exception& e) {
        // Handle any exceptions during initialization
        return false;
    }
}

void Synthesizer::createModulationSources() {
    // Create LFO sources
    auto lfo1 = std::make_unique<LfoSource>("LFO1", sampleRate_);
    auto lfo2 = std::make_unique<LfoSource>("LFO2", sampleRate_);
    
    // Set different default shapes
    lfo1->setFrequency(1.0f);  // 1 Hz
    lfo2->setFrequency(0.5f);  // 0.5 Hz
    
    // Add to modulation matrix
    modulationMatrix_.addSource(std::move(lfo1));
    modulationMatrix_.addSource(std::move(lfo2));
}

void Synthesizer::setSampleRate(int sampleRate) {
    Processor::setSampleRate(sampleRate);
    
    // Update components
    if (voiceManager_) {
        voiceManager_->setSampleRate(sampleRate);
    }
    
    effectChain_.setSampleRate(sampleRate);
    
    // Update LFOs
    if (auto lfo1 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO1"))) {
        lfo1->setSampleRate(sampleRate);
    }
    
    if (auto lfo2 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO2"))) {
        lfo2->setSampleRate(sampleRate);
    }
}

void Synthesizer::noteOn(int midiNote, float velocity) {
    if (voiceManager_) {
        voiceManager_->noteOn(midiNote, velocity);
    }
}

void Synthesizer::noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& legacyEnv) {
    if (voiceManager_) {
        // Legacy support - first use standard noteOn
        voiceManager_->noteOn(midiNote, velocity);
        
        // Then find and update the envelope for this note
        // This is not implemented here since we don't have direct access to voice envelopes
        // through VoiceManager. In a real implementation, we'd need to extend VoiceManager
        // to support this or handle envelope mapping differently.
    }
}

void Synthesizer::noteOff(int midiNote) {
    if (voiceManager_) {
        voiceManager_->noteOff(midiNote);
    }
}

void Synthesizer::allNotesOff() {
    if (voiceManager_) {
        voiceManager_->allNotesOff();
    }
}

void Synthesizer::setOscillatorType(OscillatorType type) {
    currentOscType_ = type;
    
    // Convert oscillator type to wavetable frame position
    float framePos = oscTypeToFramePosition(type);
    
    // Update wavetable frame position for all voices
    // In a real implementation, we'd need to extend VoiceManager to support this
    // For now, createDefaultWavetable() already creates frames in the right order
}

float Synthesizer::oscTypeToFramePosition(OscillatorType type) {
    // Map oscillator type to frame position (0-1)
    switch (type) {
        case OscillatorType::Sine:
            return 0.0f;
        case OscillatorType::Saw:
            return 0.25f;
        case OscillatorType::Square:
            return 0.5f;
        case OscillatorType::Triangle:
            return 0.75f;
        case OscillatorType::Noise:
            return 1.0f;
        default:
            return 0.0f;
    }
}

void Synthesizer::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    if (wavetable) {
        currentWavetable_ = wavetable;
        
        if (voiceManager_) {
            voiceManager_->setWavetable(wavetable);
        }
    }
}

void Synthesizer::setVoiceCount(int count) {
    if (voiceManager_) {
        voiceManager_->setMaxVoices(count);
    }
}

int Synthesizer::getVoiceCount() const {
    return voiceManager_ ? voiceManager_->getMaxVoices() : 0;
}

void Synthesizer::process(float* buffer, int numFrames) {
    if (!enabled_) {
        return;
    }
    
    // Clear buffer
    std::fill(buffer, buffer + numFrames * 2, 0.0f);
    
    // Update modulation matrix
    modulationMatrix_.update();
    
    // Process voices through voice manager
    if (voiceManager_) {
        voiceManager_->process(buffer, numFrames);
    }
    
    // Process effects chain
    effectChain_.process(buffer, numFrames);
    
    // Final limiter to prevent clipping
    const float masterVolume = 0.7f;
    for (int i = 0; i < numFrames * 2; ++i) {
        buffer[i] *= masterVolume;
        buffer[i] = std::clamp(buffer[i], -1.0f, 1.0f);
    }
}

void Synthesizer::reset() {
    Processor::reset();
    
    // Reset all components
    if (voiceManager_) {
        voiceManager_->allNotesOff();
    }
    
    effectChain_.reset();
}

void Synthesizer::legacyEnvelopeToNew(const AIMusicHardware::Envelope& legacyEnv, 
                                     AIMusicHardware::ModEnvelope* newEnv) {
    if (!newEnv) {
        return;
    }
    
    // Map legacy envelope parameters to new envelope
    newEnv->setAttack(legacyEnv.attack);
    newEnv->setDecay(legacyEnv.decay);
    newEnv->setSustain(legacyEnv.sustain);
    newEnv->setRelease(legacyEnv.release);
}

void Synthesizer::addEffect(std::unique_ptr<Processor> effect) {
    if (effect) {
        effectChain_.addProcessor(std::move(effect));
    }
}

void Synthesizer::removeEffect(size_t index) {
    effectChain_.removeProcessor(index);
}

Processor* Synthesizer::getEffect(size_t index) {
    return effectChain_.getProcessor(index);
}

size_t Synthesizer::getNumEffects() const {
    return effectChain_.getNumProcessors();
}

} // namespace AIMusicHardware