#include "../../include/audio/Synthesizer.h"
#include "../../include/sequencer/Sequencer.h"
#include "../../include/effects/Filter.h"
#include "../../include/effects/EffectProcessor.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

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
        // Update phase - for now assume 64 sample blocks
        // TODO: Make this more flexible by passing samples per update
        const float samplesPerUpdate = 64.0f;
        float phaseIncrement = (frequency_ * samplesPerUpdate) / sampleRate_;
        phase_ += phaseIncrement;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
            // Debug: print when LFO completes a cycle
            if (getName() == "LFO1" && frequency_ != lastDebugFreq_) {
                std::cout << "LFO1 cycling at " << frequency_ << " Hz" << std::endl;
                lastDebugFreq_ = frequency_;
            }
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
    mutable float lastDebugFreq_ = -1.0f;
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
    
    // Store raw pointers before moving to modulation matrix
    lfo1_ = lfo1.get();
    lfo2_ = lfo2.get();
    
    // Set different default shapes
    lfo1->setFrequency(1.0f);  // 1 Hz
    lfo2->setFrequency(0.5f);  // 0.5 Hz
    
    // Add to modulation matrix
    modulationMatrix_.addSource(std::move(lfo1));
    modulationMatrix_.addSource(std::move(lfo2));
    
    // Store base parameter values
    baseParameterValues_["filter_cutoff"] = 1.0f; // Start with filter wide open (20kHz)
    baseParameterValues_["filter_resonance"] = 0.1f; // Low resonance by default
    baseParameterValues_["master_volume"] = 0.7f;
    baseParameterValues_["pitch"] = 0.0f;
    baseParameterValues_["envelope_attack"] = 0.01f;
    baseParameterValues_["envelope_release"] = 0.5f;
    
    // Create modulation destinations
    // Filter cutoff destination
    auto filterCutoffDest = std::make_unique<ModulationDestination>(
        "Filter Cutoff",
        [this](float value) { 
            // This is called with the MODULATED value
            // Map 0-1 to 20Hz-20kHz logarithmically
            float freq = 20.0f * std::pow(1000.0f, value);
            
            // Apply to filter - first check internal effect chain
            bool foundFilter = false;
            for (size_t i = 0; i < effectChain_.getNumProcessors(); ++i) {
                if (auto* filter = dynamic_cast<Filter*>(effectChain_.getProcessor(i))) {
                    filter->setParameter("frequency", freq);
                    foundFilter = true;
                    break;
                }
            }
            
            // If not found internally, check external effect processor
            if (!foundFilter && externalEffectProcessor_) {
                for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                    if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                        filter->setParameter("frequency", freq);
                        // std::cout << "Setting filter cutoff to " << freq << " Hz (normalized: " << value << ")" << std::endl;
                        break;
                    }
                }
            }
        },
        [this]() { return baseParameterValues_["filter_cutoff"]; },
        0.0f, 1.0f
    );
    
    // Filter resonance destination
    auto filterResDest = std::make_unique<ModulationDestination>(
        "Filter Res",
        [this](float value) { 
            // Map 0-1 to reasonable resonance range (0.7-10)
            float resonance = 0.7f + value * 9.3f;
            
            // Apply to filter - first check internal effect chain
            bool foundFilter = false;
            for (size_t i = 0; i < effectChain_.getNumProcessors(); ++i) {
                if (auto* filter = dynamic_cast<Filter*>(effectChain_.getProcessor(i))) {
                    filter->setParameter("resonance", resonance);
                    foundFilter = true;
                    break;
                }
            }
            
            // If not found internally, check external effect processor
            if (!foundFilter && externalEffectProcessor_) {
                for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                    if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                        filter->setParameter("resonance", resonance);
                        // std::cout << "Setting filter resonance to " << resonance << " (normalized: " << value << ")" << std::endl;
                        break;
                    }
                }
            }
        },
        [this]() { return baseParameterValues_["filter_resonance"]; },
        0.0f, 1.0f
    );
    
    // Pitch destination (in semitones) - now updates LFO1 pitch modulation
    auto pitchDest = std::make_unique<ModulationDestination>(
        "Pitch",
        [this](float value) { 
            // value is the LFO output in the range [-1, 1]
            // Store this as the LFO1 value for pitch modulation
            if (voiceManager_) {
                // Only log when there are active voices
                bool hasActiveVoices = false;
                
                // Update LFO1 value for all voices
                for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                    if (auto* voice = voiceManager_->getVoice(i)) {
                        if (voice->isActive()) {
                            hasActiveVoices = true;
                            voice->setPitchModulationValue("lfo1", value);
                        }
                    }
                }
                
                // Debug output only when there are active voices
                // if (hasActiveVoices) {
                //     std::cout << "Pitch modulation LFO value: " << value << std::endl;
                // }
            }
        },
        [this]() { return 0.0f; }, // LFO center value
        -1.0f, 1.0f
    );
    
    // Volume destination - per-voice amplitude modulation
    auto volumeDest = std::make_unique<ModulationDestination>(
        "Volume",
        [this](float value) { 
            // Apply amplitude modulation to all active voices
            if (voiceManager_) {
                for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                    if (auto* voice = voiceManager_->getVoice(i)) {
                        // Set amplitude modulation on each voice
                        // This would multiply with the voice's envelope output
                        voice->setAmplitudeModulation(value);
                    }
                }
            }
            // std::cout << "Modulating voice amplitude to " << value << std::endl;
        },
        [this]() { return 1.0f; }, // Base amplitude is always 1.0
        0.0f, 1.0f
    );
    
    // Attack time destination
    auto attackDest = std::make_unique<ModulationDestination>(
        "Attack",
        [this](float value) { 
            // value is already the attack time in seconds (0.001-2.0)
            // Only update if it's different from the base value
            if (std::abs(value - baseParameterValues_["envelope_attack"]) > 0.001f) {
                setParameter("envelope_attack", value);
            }
        },
        [this]() { return baseParameterValues_["envelope_attack"]; },
        0.001f, 2.0f
    );
    
    // Release time destination
    auto releaseDest = std::make_unique<ModulationDestination>(
        "Release",
        [this](float value) { 
            // value is already the release time in seconds (0.01-4.0)
            // Only update if it's different from the base value
            if (std::abs(value - baseParameterValues_["envelope_release"]) > 0.001f) {
                setParameter("envelope_release", value);
            }
        },
        [this]() { return baseParameterValues_["envelope_release"]; },
        0.01f, 4.0f
    );
    
    // Add destinations to modulation matrix
    modulationMatrix_.addDestination(std::move(filterCutoffDest));
    modulationMatrix_.addDestination(std::move(filterResDest));
    modulationMatrix_.addDestination(std::move(pitchDest));
    modulationMatrix_.addDestination(std::move(volumeDest));
    modulationMatrix_.addDestination(std::move(attackDest));
    modulationMatrix_.addDestination(std::move(releaseDest));
}

void Synthesizer::setSampleRate(int sampleRate) {
    // Call base class method
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

void Synthesizer::noteOn(int midiNote, float velocity, int channel) {
    if (voiceManager_) {
        voiceManager_->noteOn(midiNote, velocity, channel);
        
        // Apply global pitch modulation amounts to the newly triggered voice
        for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
            if (auto* voice = voiceManager_->getVoice(i)) {
                if (voice->getMidiNote() == midiNote && voice->getChannel() == channel && voice->isActive()) {
                    // Apply all stored global pitch modulation amounts
                    for (const auto& [source, amount] : globalPitchModAmounts_) {
                        if (amount != 0.0f) {
                            voice->setPitchModulationAmount(source, amount);
                        }
                    }
                    break; // Found the voice
                }
            }
        }
    }
}

void Synthesizer::noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& legacyEnv, int channel) {
    if (voiceManager_) {
        // Legacy support - first use standard noteOn
        voiceManager_->noteOn(midiNote, velocity, channel);
        
        // Then find and update the envelope for this note
        // This is not implemented here since we don't have direct access to voice envelopes
        // through VoiceManager. In a real implementation, we'd need to extend VoiceManager
        // to support this or handle envelope mapping differently.
    }
}

void Synthesizer::noteOff(int midiNote, int channel) {
    if (voiceManager_) {
        voiceManager_->noteOff(midiNote, channel);
    }
}

void Synthesizer::allNotesOff(int channel) {
    if (voiceManager_) {
        voiceManager_->allNotesOff(channel);
    }
}

void Synthesizer::sustainOn(int channel) {
    if (voiceManager_) {
        voiceManager_->sustainOn(channel);
        std::cout << "Sustain pedal on for channel " << channel << std::endl;
    }
}

void Synthesizer::sustainOff(int channel) {
    if (voiceManager_) {
        voiceManager_->sustainOff(channel);
        std::cout << "Sustain pedal off for channel " << channel << std::endl;
    }
}

void Synthesizer::setPitchBend(float value, int channel) {
    if (voiceManager_) {
        voiceManager_->setPitchBend(value, channel);
        std::cout << "Pitch bend value " << value << " for channel " << channel << std::endl;
    }
}

void Synthesizer::setAftertouch(int note, float pressure, int channel) {
    if (voiceManager_) {
        voiceManager_->setAftertouch(note, pressure, channel);
        std::cout << "Aftertouch for note " << note << " with pressure " << pressure 
                  << " on channel " << channel << std::endl;
    }
}

void Synthesizer::setChannelPressure(float pressure, int channel) {
    if (voiceManager_) {
        voiceManager_->setChannelPressure(pressure, channel);
        std::cout << "Channel pressure " << pressure << " for channel " << channel << std::endl;
    }
}

void Synthesizer::resetAllControllers() {
    if (voiceManager_) {
        voiceManager_->resetAllControllers();
        std::cout << "Resetting all controllers" << std::endl;
    }
}

void Synthesizer::setParameter(const std::string& paramId, float value) {
    // We'll need a parameter system for this in the future
    // For now, just handle a few basic parameters

    if (paramId == "oscillator_frame") {
        if (voiceManager_) {
            // Update oscillator frame position in all voices
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* osc = voice->getOscillator()) {
                        osc->setFramePosition(value);
                    }
                }
            }
            std::cout << "Setting oscillator frame to " << value << std::endl;
        }
    }
    else if (paramId == "oscillator_type") {
        // Convert 0-4 float value to oscillator type
        int typeIndex = static_cast<int>(value);
        if (typeIndex >= 0 && typeIndex <= 4) {
            setOscillatorType(static_cast<OscillatorType>(typeIndex));
        }
    }
    else if (paramId == "filter_cutoff") {
        // Store the base parameter value
        baseParameterValues_["filter_cutoff"] = value;
        
        // Apply directly to the filter if no modulation is active
        // The modulation matrix will handle this when modulation is active
        float freq = 20.0f * std::pow(1000.0f, value);
        bool foundFilter = false;
        for (size_t i = 0; i < effectChain_.getNumProcessors(); ++i) {
            if (auto* filter = dynamic_cast<Filter*>(effectChain_.getProcessor(i))) {
                filter->setParameter("frequency", freq);
                foundFilter = true;
                break;
            }
        }
        
        // If not found internally, check external effect processor
        if (!foundFilter && externalEffectProcessor_) {
            for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                    filter->setParameter("frequency", freq);
                    break;
                }
            }
        }
        std::cout << "Setting filter cutoff to " << value << " (freq: " << freq << " Hz)" << std::endl;
    }
    else if (paramId == "filter_resonance") {
        // Store the base parameter value
        baseParameterValues_["filter_resonance"] = value;
        
        // Apply directly to the filter if no modulation is active
        float resonance = 0.7f + value * 9.3f;
        bool foundFilter = false;
        for (size_t i = 0; i < effectChain_.getNumProcessors(); ++i) {
            if (auto* filter = dynamic_cast<Filter*>(effectChain_.getProcessor(i))) {
                filter->setParameter("resonance", resonance);
                foundFilter = true;
                break;
            }
        }
        
        // If not found internally, check external effect processor
        if (!foundFilter && externalEffectProcessor_) {
            for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                    filter->setParameter("resonance", resonance);
                    break;
                }
            }
        }
        std::cout << "Setting filter resonance to " << value << " (resonance: " << resonance << ")" << std::endl;
    }
    else if (paramId == "master_volume") {
        // Update base parameter value
        baseParameterValues_["master_volume"] = value;
        std::cout << "Setting master volume to " << value << std::endl;
    }
    else if (paramId == "envelope_attack") {
        // Update base parameter value
        baseParameterValues_["envelope_attack"] = value;
        
        // Update all voices' attack time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setAttack(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope attack to " << value << " seconds" << std::endl;
    }
    else if (paramId == "envelope_decay") {
        // Update all voices' decay time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setDecay(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope decay to " << value << " seconds" << std::endl;
    }
    else if (paramId == "envelope_sustain") {
        // Update all voices' sustain level
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setSustain(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope sustain to " << value << std::endl;
    }
    else if (paramId == "envelope_release") {
        // Update base parameter value
        baseParameterValues_["envelope_release"] = value;
        
        // Update all voices' release time
        if (voiceManager_) {
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* envelope = voice->getEnvelope()) {
                        envelope->setRelease(value);
                    }
                }
            }
        }
        std::cout << "Setting envelope release to " << value << " seconds" << std::endl;
    }
    else if (paramId == "voice_count") {
        // Set number of voices
        int count = static_cast<int>(value);
        if (count > 0) {
            setVoiceCount(count);
        }
    }
    else if (paramId.find("lfo") == 0) {
        // Handle LFO parameters (lfo1_rate, lfo1_shape, etc.)
        std::cout << "Setting " << paramId << " to " << value << std::endl;

        // Parse LFO index and parameter name
        size_t underscorePos = paramId.find('_');
        if (underscorePos != std::string::npos && underscorePos < paramId.size() - 1) {
            std::string lfoName = paramId.substr(0, underscorePos); // "lfo1", "lfo2", etc.
            std::string paramName = paramId.substr(underscorePos + 1); // "rate", "shape", etc.

            // Convert to uppercase for LFO name lookup (e.g., "lfo1" -> "LFO1")
            std::string upperLfoName;
            for (char c : lfoName) {
                upperLfoName += std::toupper(c);
            }

            if (auto* source = modulationMatrix_.getSource(upperLfoName)) {
                if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
                    if (paramName == "rate") {
                        lfo->setFrequency(value);
                        std::cout << "Set " << upperLfoName << " frequency to " << value << " Hz" << std::endl;
                    }
                    else if (paramName == "shape") {
                        // Convert 0-4 float value to LFO shape
                        int shapeIndex = static_cast<int>(value);
                        if (shapeIndex >= 0 && shapeIndex <= 4) {
                            lfo->setShape(static_cast<LfoSource::WaveShape>(shapeIndex));
                        }
                    }
                }
            }
        }
    }
    else {
        std::cout << "Unknown parameter: " << paramId << std::endl;
    }
}

float Synthesizer::getParameter(const std::string& paramId) const {
    // Getting actual parameters

    if (paramId == "oscillator_frame") {
        // In a real implementation, we'd get this from the first voice or from a stored value
        // For now, return based on current oscillator type
        return oscTypeToFramePosition(currentOscType_);
    }
    else if (paramId == "oscillator_type") {
        return static_cast<float>(currentOscType_);
    }
    else if (paramId == "filter_cutoff") {
        // For future implementation - will need to add filter to VoiceManager
        return 1.0f; // Default value
    }
    else if (paramId == "filter_resonance") {
        // For future implementation - will need to add filter to VoiceManager
        return 0.5f; // Default value
    }
    else if (paramId == "master_volume") {
        // Return stored master volume value
        return baseParameterValues_.count("master_volume") ? baseParameterValues_.at("master_volume") : 0.7f;
    }
    else if (paramId == "envelope_attack") {
        return baseParameterValues_.count("envelope_attack") ? baseParameterValues_.at("envelope_attack") : 0.01f;
    }
    else if (paramId == "envelope_decay") {
        return 0.1f; // Default 100ms decay
    }
    else if (paramId == "envelope_sustain") {
        return 0.7f; // Default 70% sustain
    }
    else if (paramId == "envelope_release") {
        return baseParameterValues_.count("envelope_release") ? baseParameterValues_.at("envelope_release") : 0.5f;
    }
    else if (paramId == "voice_count") {
        return static_cast<float>(getVoiceCount());
    }
    else if (paramId.find("lfo") == 0) {
        // Handle LFO parameters (lfo1_rate, lfo1_shape, etc.)

        // Parse LFO index and parameter name
        size_t underscorePos = paramId.find('_');
        if (underscorePos != std::string::npos && underscorePos < paramId.size() - 1) {
            std::string lfoName = paramId.substr(0, underscorePos); // "lfo1", "lfo2", etc.
            std::string paramName = paramId.substr(underscorePos + 1); // "rate", "shape", etc.

            if (auto* source = modulationMatrix_.getSource(lfoName)) {
                if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
                    if (paramName == "rate") {
                        // LFO frequency is private, we would need a getter
                        return 1.0f; // Default value
                    }
                    else if (paramName == "shape") {
                        // LFO shape is private, we would need a getter
                        return 0.0f; // Default value (sine)
                    }
                }
            }
        }
    }

    // Unknown parameter
    return 0.0f;
}

std::map<std::string, float> Synthesizer::getAllParameters() const {
    std::map<std::string, float> parameters;

    // Store core synthesizer parameters
    parameters["oscillator_type"] = static_cast<float>(currentOscType_);
    parameters["oscillator_frame"] = oscTypeToFramePosition(currentOscType_);
    parameters["voice_count"] = static_cast<float>(getVoiceCount());
    parameters["master_volume"] = 0.7f; // Default value for now

    // Future parameters to add when implemented:
    parameters["filter_cutoff"] = 1.0f;
    parameters["filter_resonance"] = 0.5f;

    // LFO Parameters
    parameters["lfo1_rate"] = 1.0f;
    parameters["lfo1_shape"] = 0.0f; // Sine
    parameters["lfo2_rate"] = 0.5f;
    parameters["lfo2_shape"] = 0.0f; // Sine

    // Add modulation connections when implemented

    return parameters;
}

void Synthesizer::setAllParameters(const std::map<std::string, float>& parameters) {
    // Apply all parameters at once
    for (const auto& [paramId, value] : parameters) {
        setParameter(paramId, value);
    }
}

void Synthesizer::setOscillatorType(OscillatorType type) {
    currentOscType_ = type;

    if (useBandLimitedOscillators_) {
        // Convert to band-limited waveform type
        BandLimitedWavetable::WaveType blWaveType = BandLimitedWavetable::WaveType::Saw;
        switch (type) {
            case OscillatorType::Sine:
                blWaveType = BandLimitedWavetable::WaveType::Sine;
                break;
            case OscillatorType::Saw:
                blWaveType = BandLimitedWavetable::WaveType::Saw;
                break;
            case OscillatorType::Square:
                blWaveType = BandLimitedWavetable::WaveType::Square;
                break;
            case OscillatorType::Triangle:
                blWaveType = BandLimitedWavetable::WaveType::Triangle;
                break;
            default:
                break;
        }
        
        // Update band-limited voice manager
        if (auto* blVoiceManager = dynamic_cast<BandLimitedVoiceManager*>(voiceManager_.get())) {
            blVoiceManager->setWaveform(blWaveType);
        }
    } else {
        // Convert oscillator type to wavetable frame position
        float framePos = oscTypeToFramePosition(type);

        // Update frame position in all active voices
        if (voiceManager_) {
            // Iterate through all voices and set their oscillator's frame position
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* osc = voice->getOscillator()) {
                        osc->setFramePosition(framePos);
                    }
                }
            }
        }
    }

    std::cout << "Oscillator type changed to " << static_cast<int>(type) << std::endl;
}

float Synthesizer::oscTypeToFramePosition(OscillatorType type) const {
    // Map oscillator type to frame position (0-1)
    // Wavetable stores: 0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=Noise
    // We need to map to exact frame positions, not interpolated positions
    switch (type) {
        case OscillatorType::Sine:
            return 0.0f;    // Frame 0
        case OscillatorType::Square:
            return 0.5f;    // Frame 2 (normalized: 2/4 = 0.5)
        case OscillatorType::Saw:
            return 0.25f;   // Frame 1 (normalized: 1/4 = 0.25)
        case OscillatorType::Triangle:
            return 0.75f;   // Frame 3 (normalized: 3/4 = 0.75)
        case OscillatorType::Noise:
            return 1.0f;    // Frame 4 (normalized: 4/4 = 1.0)
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
    
    // Process in blocks of 64 samples for smoother LFO modulation
    const int blockSize = 64;
    int samplesProcessed = 0;
    
    while (samplesProcessed < numFrames) {
        int samplesToProcess = std::min(blockSize, numFrames - samplesProcessed);
        
        // Update modulation matrix once per block (every 64 samples)
        // This gives smooth modulation without causing crashes
        modulationMatrix_.update();
        
        // Process voices for this block
        if (voiceManager_) {
            float* blockBuffer = buffer + (samplesProcessed * 2);
            voiceManager_->process(blockBuffer, samplesToProcess);
        }
        
        samplesProcessed += samplesToProcess;
    }
    
    // Process effects chain on the complete buffer
    effectChain_.process(buffer, numFrames);
    
    // Final limiter to prevent clipping
    const float linearVolume = baseParameterValues_.count("master_volume") ? baseParameterValues_.at("master_volume") : 0.7f;
    
    // Convert linear slider value to logarithmic gain
    // Use a more musical taper: -40dB to +6dB range for more usable volume
    float masterVolume;
    if (linearVolume <= 0.0f) {
        masterVolume = 0.0f;  // -inf dB
    } else {
        // Map 0-1 to -40dB to +6dB range (more headroom)
        float dbValue = -40.0f + (linearVolume * 46.0f);
        masterVolume = std::pow(10.0f, dbValue / 20.0f);
    }
    
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

void Synthesizer::connectModulation(const std::string& sourceName, const std::string& destName, float amount) {
    modulationMatrix_.connect(sourceName, destName, amount);
    std::cout << "Connected " << sourceName << " to " << destName << " with amount " << amount << std::endl;
}

void Synthesizer::disconnectModulation(const std::string& sourceName, const std::string& destName) {
    modulationMatrix_.disconnect(sourceName, destName);
    std::cout << "Disconnected " << sourceName << " from " << destName << std::endl;
}

void Synthesizer::setLFORate(int lfoIndex, float rate) {
    std::string lfoName = "LFO" + std::to_string(lfoIndex + 1);
    if (auto* source = modulationMatrix_.getSource(lfoName)) {
        if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
            lfo->setFrequency(rate);
            std::cout << "Set " << lfoName << " rate to " << rate << " Hz" << std::endl;
        }
    }
}

void Synthesizer::setLFOShape(int lfoIndex, int shape) {
    std::string lfoName = "LFO" + std::to_string(lfoIndex + 1);
    if (auto* source = modulationMatrix_.getSource(lfoName)) {
        if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
            lfo->setShape(static_cast<LfoSource::WaveShape>(shape));
            std::cout << "Set " << lfoName << " shape to " << shape << std::endl;
        }
    }
}

void Synthesizer::setLFODepth(int lfoIndex, float depth) {
    std::string lfoName = "LFO" + std::to_string(lfoIndex + 1);
    if (auto* source = modulationMatrix_.getSource(lfoName)) {
        if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
            // LFO depth is handled by modulation amount in the connection
            // This could be used to scale the LFO output directly if needed
            std::cout << "Set " << lfoName << " depth to " << depth << std::endl;
        }
    }
}

void Synthesizer::setGlobalPitchModulationAmount(const std::string& source, float semitones) {
    // Store the global pitch modulation amount
    globalPitchModAmounts_[source] = semitones;
    
    // Apply to all existing voices
    if (voiceManager_) {
        for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
            if (auto* voice = voiceManager_->getVoice(i)) {
                voice->setPitchModulationAmount(source, semitones);
            }
        }
    }
    
    std::cout << "Set global pitch modulation for " << source << " to " << semitones << " semitones" << std::endl;
}

void Synthesizer::enableBandLimitedOscillators(bool enable) {
    if (useBandLimitedOscillators_ != enable) {
        useBandLimitedOscillators_ = enable;
        
        // Recreate voice manager with appropriate type
        int maxVoices = voiceManager_ ? voiceManager_->getMaxVoices() : 16;
        
        if (enable) {
            // Create band-limited voice manager
            auto blVoiceManager = std::make_unique<BandLimitedVoiceManager>(
                sampleRate_, maxVoices, oversamplingEnabled_);
            
            // Set initial waveform based on current oscillator type
            BandLimitedWavetable::WaveType waveType = BandLimitedWavetable::WaveType::Saw;
            switch (currentOscType_) {
                case OscillatorType::Sine:
                    waveType = BandLimitedWavetable::WaveType::Sine;
                    break;
                case OscillatorType::Saw:
                    waveType = BandLimitedWavetable::WaveType::Saw;
                    break;
                case OscillatorType::Square:
                    waveType = BandLimitedWavetable::WaveType::Square;
                    break;
                case OscillatorType::Triangle:
                    waveType = BandLimitedWavetable::WaveType::Triangle;
                    break;
                default:
                    break;
            }
            blVoiceManager->setWaveform(waveType);
            blVoiceManager->setOversamplingFactor(oversamplingFactor_);
            
            voiceManager_ = std::move(blVoiceManager);
        } else {
            // Create standard voice manager
            voiceManager_ = std::make_unique<VoiceManager>(sampleRate_, maxVoices);
            if (currentWavetable_) {
                voiceManager_->setWavetable(currentWavetable_);
            }
        }
        
        std::cout << "Band-limited oscillators " << (enable ? "enabled" : "disabled") << std::endl;
    }
}

void Synthesizer::setOversamplingEnabled(bool enable) {
    oversamplingEnabled_ = enable;
    
    // Update existing band-limited voice manager if active
    if (useBandLimitedOscillators_) {
        if (auto* blVoiceManager = dynamic_cast<BandLimitedVoiceManager*>(voiceManager_.get())) {
            blVoiceManager->setOversamplingEnabled(enable);
        }
    }
    
    std::cout << "Oversampling " << (enable ? "enabled" : "disabled") << std::endl;
}

void Synthesizer::setOversamplingFactor(OversamplingProcessor::Factor factor) {
    oversamplingFactor_ = factor;
    
    // Update existing band-limited voice manager if active
    if (useBandLimitedOscillators_) {
        if (auto* blVoiceManager = dynamic_cast<BandLimitedVoiceManager*>(voiceManager_.get())) {
            blVoiceManager->setOversamplingFactor(factor);
        }
    }
    
    int factorInt = 1;
    switch (factor) {
        case OversamplingProcessor::Factor::x2: factorInt = 2; break;
        case OversamplingProcessor::Factor::x4: factorInt = 4; break;
        case OversamplingProcessor::Factor::x8: factorInt = 8; break;
        default: break;
    }
    std::cout << "Oversampling factor set to " << factorInt << "x" << std::endl;
}

} // namespace AIMusicHardware