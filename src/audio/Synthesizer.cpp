#include "../../include/audio/Synthesizer.h"
#include "../../include/synthesis/RealtimeWavetableVoice.h"
#include "../../include/synthesis/RealtimeWavetableVoice.h"
#include "../../include/sequencer/Sequencer.h"
#include "../../include/effects/Filter.h"
#include "../../include/effects/EffectProcessor.h"
#include "../../include/synthesis/RealtimeWavetableVoice.h"
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

    float getFrequency() const { return frequency_; }
    int getShapeIndex() const { return static_cast<int>(shape_); }
    
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
      
    // Create VoiceManager (use Standard so we can switch between Legacy and Hybrid V2 dynamically)
    setVoiceManagerType(VoiceManagerType::Standard);
    
    // Create default wavetable
    createDefaultWavetable();
    
    // Create modulation sources
    createModulationSources();

    // Hybrid wavetable scaffolding (disabled by default)
    spectralCache_ = std::make_shared<SpectralWavetableCache>(256);
    spectralWorker_ = std::make_unique<SpectralRenderWorker>(spectralCache_);
    spectralWorker_->setAsyncEnabled(true); // enable minimal async worker
    spectralWorker_->setPrewarmBreadth(1, 1); // default ±1 morph, ±1 pitch band
    hybridWavetableEnabled_ = false; // gate usage from settings later
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
        // Wire spectral services to voice manager but keep hybrid disabled
        if (voiceManager_) {
            voiceManager_->setSampleRate(getSampleRate());
            voiceManager_->setWavetable(currentWavetable_);
            voiceManager_->setPitchBendRange(2.0f);
            voiceManager_->setStealMode(VoiceManager::StealMode::Oldest);
            voiceManager_->setSpectralServices(spectralCache_, &spectralWorker_);
            // Provide spectral table converted from current legacy wavetable for Hybrid
            if (currentWavetable_) {
                auto spec = spectralFromWavetable(*currentWavetable_, getSampleRate());
                voiceManager_->setHybridSpectralTable(spec);
            }
            voiceManager_->enableHybridWavetable(false);
        }
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

    // Create MIDI-driven continuous sources
    class ValueSource : public ModulationSource {
    public:
        ValueSource(const std::string& name, bool bipolar, std::function<float()> getter)
            : ModulationSource(name), getter_(std::move(getter)) { setBipolar(bipolar); }
        float getValue() const override { return getter_(); }
        void update() override {}
    private:
        std::function<float()> getter_;
    };

    modulationMatrix_.addSource(std::make_unique<ValueSource>("ModWheel", false, [this]{ return modWheelValue_; }));
    modulationMatrix_.addSource(std::make_unique<ValueSource>("Aftertouch", false, [this]{ return aftertouchValue_; }));
    modulationMatrix_.addSource(std::make_unique<ValueSource>("Velocity", false, [this]{ return velocityValue_; }));
    // Envelope source returns average of active voices' envelope value (fallback to base sustain if none)
    modulationMatrix_.addSource(std::make_unique<ValueSource>("Envelope", false, [this]{
        if (!voiceManager_) return baseParameterValues_.count("envelope_sustain") ? baseParameterValues_.at("envelope_sustain") : 0.0f;
        float sum = 0.0f; int count = 0;
        for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
            if (auto* v = voiceManager_->getVoice(i)) {
                if (auto* env = v->getEnvelope(); env && v->isActive()) { sum += env->getCurrentValue(); ++count; }
            }
        }
        if (count == 0) return baseParameterValues_.count("envelope_sustain") ? baseParameterValues_.at("envelope_sustain") : 0.0f;
        return sum / static_cast<float>(count);
    }));
    
    // Store base parameter values
    baseParameterValues_["filter_cutoff"] = 1.0f; // Start with filter wide open (20kHz)
    baseParameterValues_["filter_resonance"] = 0.1f; // Low resonance by default
    baseParameterValues_["master_volume"] = 0.7f;
    baseParameterValues_["preset_output_trim_db"] = 0.0f;
    baseParameterValues_["pitch"] = 0.0f;
    baseParameterValues_["envelope_attack"] = 0.01f;
    baseParameterValues_["envelope_decay"] = 0.1f;
    baseParameterValues_["envelope_sustain"] = 0.7f;
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
            
            // If not found internally, check external effect processor (target LAST filter in chain)
            if (!foundFilter && externalEffectProcessor_) {
                for (size_t idx = externalEffectProcessor_->getNumEffects(); idx > 0; --idx) {
                    size_t i = idx - 1;
                    if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                        filter->setParameter("frequency", freq);
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
            
            // If not found internally, check external effect processor (target LAST filter in chain)
            if (!foundFilter && externalEffectProcessor_) {
                for (size_t idx = externalEffectProcessor_->getNumEffects(); idx > 0; --idx) {
                    size_t i = idx - 1;
                    if (auto* filter = dynamic_cast<Filter*>(externalEffectProcessor_->getEffect(i))) {
                        filter->setParameter("resonance", resonance);
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
    
    // Helper to add external FX parameter destinations (by effect name and parameter key)
    auto addFxParamDest = [this](const std::string& destDisplay,
                                 const std::string& effectName,
                                 const std::string& paramKey,
                                 float minVal,
                                 float maxVal) {
        auto setter = [this, effectName, paramKey, minVal, maxVal](float value) {
            float clamped = std::clamp(value, minVal, maxVal);
            if (!externalEffectProcessor_) return;
            // Target the first matching effect in chain
            for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                auto* fx = externalEffectProcessor_->getEffect(i);
                if (fx && fx->getName() == effectName) {
                    fx->setParameter(paramKey, clamped);
                    break;
                }
            }
        };
        auto getter = [this, effectName, paramKey, minVal]() -> float {
            if (!externalEffectProcessor_) return minVal;
            for (size_t i = 0; i < externalEffectProcessor_->getNumEffects(); ++i) {
                auto* fx = externalEffectProcessor_->getEffect(i);
                if (fx && fx->getName() == effectName) {
                    return fx->getParameter(paramKey);
                }
            }
            return minVal;
        };
        modulationMatrix_.addDestination(std::make_unique<ModulationDestination>(
            destDisplay, setter, getter, minVal, maxVal));
    };

    // Add destinations to modulation matrix
    modulationMatrix_.addDestination(std::move(filterCutoffDest));
    modulationMatrix_.addDestination(std::move(filterResDest));
    modulationMatrix_.addDestination(std::move(pitchDest));
    modulationMatrix_.addDestination(std::move(volumeDest));
    modulationMatrix_.addDestination(std::move(attackDest));
    modulationMatrix_.addDestination(std::move(releaseDest));

    // Wavetable Position destination (legacy + hybrid morph)
    modulationMatrix_.addDestination(std::make_unique<ModulationDestination>(
        "Wavetable Position",
        [this](float value) {
            float pos = std::clamp(value, 0.0f, 1.0f);
            // Legacy: set oscillator frame position on all voices
            if (voiceManager_) {
                for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                    if (auto* voice = voiceManager_->getVoice(i)) {
                        if (auto* osc = voice->getOscillator()) osc->setFramePosition(pos);
                    }
                }
                // Hybrid: set morph pos as well
                voiceManager_->applyHybridMorph(pos);
            }
        },
        [this]() {
            // Approximate by mapping currentOscType_ to frame position
            return oscTypeToFramePosition(currentOscType_);
        },
        0.0f, 1.0f
    ));
    if (auto* d = modulationMatrix_.getDestination("Wavetable Position")) d->setSmoothing(0.12f);
    // Enable gentle smoothing on synth parameter destinations
    if (auto* d = modulationMatrix_.getDestination("Filter Cutoff")) d->setSmoothing(0.1f);
    if (auto* d = modulationMatrix_.getDestination("Filter Res"))    d->setSmoothing(0.1f);
    if (auto* d = modulationMatrix_.getDestination("Volume"))        d->setSmoothing(0.1f);
    if (auto* d = modulationMatrix_.getDestination("Attack"))        d->setSmoothing(0.2f);
    if (auto* d = modulationMatrix_.getDestination("Release"))       d->setSmoothing(0.2f);

    // Classic Reverb (if present in chain)
    addFxParamDest("Reverb Room Size", "Reverb", "roomSize", 0.0f, 1.0f);
    addFxParamDest("Reverb Damping",   "Reverb", "damping",  0.0f, 1.0f);
    addFxParamDest("Reverb Width",     "Reverb", "width",    0.0f, 1.0f);
    addFxParamDest("Reverb Wet",       "Reverb", "wetLevel", 0.0f, 1.0f);

    // Delay
    addFxParamDest("Delay Time",     "Delay", "delayTime", 0.01f, 1.0f);
    addFxParamDest("Delay Feedback", "Delay", "feedback",  0.0f,  0.95f);
    addFxParamDest("Delay Mix",      "Delay", "mix",       0.0f,  1.0f);
    if (auto* d = modulationMatrix_.getDestination("Delay Time"))     d->setSmoothing(0.1f);
    if (auto* d = modulationMatrix_.getDestination("Delay Feedback")) d->setSmoothing(0.1f);
    if (auto* d = modulationMatrix_.getDestination("Delay Mix"))      d->setSmoothing(0.1f);

    // LowPassFilter FX (not the global filter): target first LowPassFilter in chain
    addFxParamDest("FX LPF Cutoff",    "LowPassFilter", "frequency", 20.0f, 20000.0f);
    addFxParamDest("FX LPF Resonance", "LowPassFilter", "resonance", 0.7f,  5.0f);

    // FDNReverb (Hall)
    addFxParamDest("Hall Predelay",   "FDNReverb (Hall)", "predelay_ms", 0.0f, 100.0f);
    addFxParamDest("Hall Size",       "FDNReverb (Hall)", "size",        0.5f, 2.0f);
    addFxParamDest("Hall Diffusion",  "FDNReverb (Hall)", "diffusion",   0.0f, 1.0f);
    addFxParamDest("Hall Mod Rate",   "FDNReverb (Hall)", "mod_rate",    0.05f, 1.0f);
    addFxParamDest("Hall Decay",      "FDNReverb (Hall)", "decay_rt60_s",0.2f, 20.0f);
    addFxParamDest("Hall High Damp",  "FDNReverb (Hall)", "high_damping",0.0f, 1.0f);
    addFxParamDest("Hall Bass Mult",  "FDNReverb (Hall)", "bass_mult",   0.5f, 2.0f);
    addFxParamDest("Hall Width",      "FDNReverb (Hall)", "stereo_width",0.0f, 1.0f);
    addFxParamDest("Hall Output Trim","FDNReverb (Hall)", "output_trim_db", -12.0f, 6.0f);
    addFxParamDest("Hall Mix",        "FDNReverb (Hall)", "mix", 0.0f, 1.0f);
    for (const char* name : {"Hall Predelay","Hall Size","Hall Diffusion","Hall Mod Rate","Hall Decay","Hall High Damp","Hall Bass Mult","Hall Width","Hall Output Trim","Hall Mix"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.1f);
    }

    // PlateReverb
    addFxParamDest("Plate Predelay",     "PlateReverb", "predelay_ms",    0.0f, 100.0f);
    addFxParamDest("Plate Decay",        "PlateReverb", "decay_rt60_s",   0.2f, 20.0f);
    addFxParamDest("Plate Diffusion",    "PlateReverb", "diffusion",      0.0f, 1.0f);
    addFxParamDest("Plate Mod Rate",     "PlateReverb", "mod_rate",       0.05f, 1.0f);
    addFxParamDest("Plate Mod Depth",    "PlateReverb", "mod_depth",      0.0f, 0.25f);
    addFxParamDest("Plate High Damp",    "PlateReverb", "high_damping",   0.0f, 1.0f);
    addFxParamDest("Plate Width",        "PlateReverb", "stereo_width",   0.0f, 1.0f);
    addFxParamDest("Plate Output Trim",  "PlateReverb", "output_trim_db", -12.0f, 6.0f);
    addFxParamDest("Plate Mix",          "PlateReverb", "mix",            0.0f, 1.0f);
    for (const char* name : {"Plate Predelay","Plate Decay","Plate Diffusion","Plate Mod Rate","Plate Mod Depth","Plate High Damp","Plate Width","Plate Output Trim","Plate Mix"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.12f);
    }
    

    // Classic Reverb extras
    addFxParamDest("Reverb Dry",      "Reverb", "dryLevel", 0.0f, 1.0f);
    for (const char* name : {"Reverb Room Size","Reverb Damping","Reverb Width","Reverb Wet","Reverb Dry"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.1f);
    }

    // Saturation
    addFxParamDest("Saturation Drive",      "Saturation", "drive", 1.0f, 20.0f);
    addFxParamDest("Saturation Tone",       "Saturation", "tone",  0.0f, 1.0f);
    addFxParamDest("Saturation Mix",        "Saturation", "mix",   0.0f, 1.0f);
    addFxParamDest("Saturation Output Trim", "Saturation", "output_trim_db", -12.0f, 6.0f);
    for (const char* name : {"Saturation Drive","Saturation Tone","Saturation Mix","Saturation Output Trim"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.1f);
    }

    // Distortion
    addFxParamDest("Distortion Drive", "Distortion", "drive", 1.0f, 20.0f);
    addFxParamDest("Distortion Tone",  "Distortion", "tone",  0.0f, 1.0f);
    addFxParamDest("Distortion Level", "Distortion", "level", 0.0f, 1.0f);
    addFxParamDest("Distortion Mix",   "Distortion", "mix",   0.0f, 1.0f);
    for (const char* name : {"Distortion Drive","Distortion Tone","Distortion Level","Distortion Mix"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.1f);
    }

    // BitCrusher
    addFxParamDest("BitCrusher BitDepth",    "BitCrusher", "bitDepth", 1.0f, 16.0f);
    addFxParamDest("BitCrusher SRR",         "BitCrusher", "sampleRateReduction", 0.01f, 1.0f);
    addFxParamDest("BitCrusher Drive",       "BitCrusher", "drive", 1.0f, 10.0f);
    addFxParamDest("BitCrusher Output Trim", "BitCrusher", "output_trim_db", -12.0f, 6.0f);
    addFxParamDest("BitCrusher Mix",         "BitCrusher", "mix", 0.0f, 1.0f);
    for (const char* name : {"BitCrusher BitDepth","BitCrusher SRR","BitCrusher Drive","BitCrusher Output Trim","BitCrusher Mix"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.15f);
    }

    // Compressor
    addFxParamDest("Compressor Threshold", "Compressor", "threshold", -60.0f, 0.0f);
    addFxParamDest("Compressor Ratio",     "Compressor", "ratio", 1.0f, 20.0f);
    addFxParamDest("Compressor Attack",    "Compressor", "attack", 0.001f, 1.0f);
    addFxParamDest("Compressor Release",   "Compressor", "release", 0.01f, 3.0f);
    addFxParamDest("Compressor Makeup",    "Compressor", "makeup", 0.0f, 24.0f);
    addFxParamDest("Compressor Knee",      "Compressor", "knee", 0.0f, 24.0f);
    for (const char* name : {"Compressor Threshold","Compressor Ratio","Compressor Attack","Compressor Release","Compressor Makeup","Compressor Knee"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.15f);
    }

    // Phaser
    addFxParamDest("Phaser Rate",     "Phaser", "rate", 0.05f, 10.0f);
    addFxParamDest("Phaser Depth",    "Phaser", "depth", 0.0f, 1.0f);
    addFxParamDest("Phaser Feedback", "Phaser", "feedback", 0.0f, 0.9f);
    addFxParamDest("Phaser Mix",      "Phaser", "mix", 0.0f, 1.0f);
    for (const char* name : {"Phaser Rate","Phaser Depth","Phaser Feedback","Phaser Mix"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.12f);
    }

    // EQ
    addFxParamDest("EQ Low Gain",  "EQ", "lowGain",  -24.0f, 24.0f);
    addFxParamDest("EQ Mid Gain",  "EQ", "midGain",  -24.0f, 24.0f);
    addFxParamDest("EQ High Gain", "EQ", "highGain", -24.0f, 24.0f);
    addFxParamDest("EQ Low Freq",  "EQ", "lowFreq",  20.0f, 1000.0f);
    addFxParamDest("EQ High Freq", "EQ", "highFreq", 1000.0f, 20000.0f);
    for (const char* name : {"EQ Low Gain","EQ Mid Gain","EQ High Gain","EQ Low Freq","EQ High Freq"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.12f);
    }

    // BassBoost
    addFxParamDest("BassBoost Freq",  "BassBoost", "frequency", 20.0f, 500.0f);
    addFxParamDest("BassBoost Gain",  "BassBoost", "gain",      0.0f, 24.0f);
    addFxParamDest("BassBoost Width", "BassBoost", "width",     0.1f, 5.0f);
    addFxParamDest("BassBoost Drive", "BassBoost", "drive",     1.0f, 3.0f);
    for (const char* name : {"BassBoost Freq","BassBoost Gain","BassBoost Width","BassBoost Drive"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.12f);
    }

    // Modulation (Chorus/Flanger)
    addFxParamDest("Chorus/Mod Rate",     "Modulation", "rate",     0.1f, 20.0f);
    addFxParamDest("Chorus/Mod Depth",    "Modulation", "depth",    0.0f, 1.0f);
    addFxParamDest("Chorus/Mod Feedback", "Modulation", "feedback", 0.0f, 0.7f);
    addFxParamDest("Chorus/Mod Spread",   "Modulation", "spread",   0.0f, 1.0f);
    addFxParamDest("Chorus/Mod Mix",      "Modulation", "mix",      0.0f, 1.0f);
    addFxParamDest("Chorus/Mod Output Trim", "Modulation", "output_trim_db", -12.0f, 6.0f);
    for (const char* name : {"Chorus/Mod Rate","Chorus/Mod Depth","Chorus/Mod Feedback","Chorus/Mod Spread","Chorus/Mod Mix","Chorus/Mod Output Trim"}) {
        if (auto* d = modulationMatrix_.getDestination(name)) d->setSmoothing(0.12f);
    }
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

void Synthesizer::setHybridWavetableEnabled(bool enabled) {
    hybridWavetableEnabled_ = enabled;
    std::cout << "[Synthesizer] setHybridWavetableEnabled(" << (enabled ? "ON" : "OFF") << ")" << std::endl;
    // Ensure we are using the Standard VoiceManager which supports Hybrid V2 gating
    if (voiceManagerType_ != VoiceManagerType::Standard) {
        setVoiceManagerType(VoiceManagerType::Standard);
        // Re-wire services after manager swap
        if (voiceManager_) {
            voiceManager_->setSampleRate(getSampleRate());
            voiceManager_->setWavetable(currentWavetable_);
            voiceManager_->setPitchBendRange(2.0f);
            voiceManager_->setStealMode(VoiceManager::StealMode::Oldest);
            voiceManager_->setSpectralServices(spectralCache_, &spectralWorker_);
        }
    }
    if (voiceManager_) {
        voiceManager_->enableHybridWavetable(enabled);
        voiceManager_->setHybridMinPhase(hybridTimbreMinPhase_);
        // Update spectral table from current wavetable when enabling Hybrid
        if (enabled && currentWavetable_) {
            auto spec = spectralFromWavetable(*currentWavetable_, getSampleRate());
            voiceManager_->setHybridSpectralTable(spec);
        }
        voiceManager_->rebuildVoices();
        voiceManager_->setWavetable(currentWavetable_);
        // Apply current waveform selection to Hybrid morph position
        if (enabled) {
            float framePos = oscTypeToFramePosition(currentOscType_);
            voiceManager_->applyHybridMorph(framePos);
            std::cout << "[Synthesizer] Hybrid enabled; applied morph position " << framePos << std::endl;
        }
        // Re-apply current oscillator waveform selection to legacy oscillator when returning to legacy
        if (!enabled) {
            float framePos = oscTypeToFramePosition(currentOscType_);
            for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                if (auto* voice = voiceManager_->getVoice(i)) {
                    if (auto* osc = voice->getOscillator()) {
                        osc->setFramePosition(framePos);
                    }
                }
            }
            std::cout << "[Synthesizer] Hybrid disabled; reapplied legacy frame position " << framePos << std::endl;
        }
    }
}

void Synthesizer::noteOn(int midiNote, float velocity, int channel) {
    if (voiceManager_) {
        voiceManager_->noteOn(midiNote, velocity, channel);
        velocityValue_ = std::clamp(velocity, 0.0f, 1.0f);
        
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
        // Trigger voice first
        voiceManager_->noteOn(midiNote, velocity, channel);

        // Find the newly triggered voice and apply per-note ADSR parameters
        for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
            if (auto* voice = voiceManager_->getVoice(i)) {
                if (voice->getMidiNote() == midiNote && voice->getChannel() == channel && voice->isActive()) {
                    if (auto* env = voice->getEnvelope()) {
                        env->setAttack(std::max(0.0f, legacyEnv.attack));
                        env->setDecay(std::max(0.0f, legacyEnv.decay));
                        env->setSustain(std::clamp(legacyEnv.sustain, 0.0f, 1.0f));
                        env->setRelease(std::max(0.0f, legacyEnv.release));
                    }
                    break;
                }
            }
        }
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
    aftertouchValue_ = std::clamp(pressure, 0.0f, 1.0f);
}

void Synthesizer::setChannelPressure(float pressure, int channel) {
    if (voiceManager_) {
        voiceManager_->setChannelPressure(pressure, channel);
        std::cout << "Channel pressure " << pressure << " for channel " << channel << std::endl;
    }
    aftertouchValue_ = std::clamp(pressure, 0.0f, 1.0f);
}

void Synthesizer::resetAllControllers() {
    if (voiceManager_) {
        voiceManager_->resetAllControllers();
        std::cout << "Resetting all controllers" << std::endl;
    }
}

bool Synthesizer::hasActiveVoices() const {
    if (!voiceManager_) return false;
    for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
        if (auto* v = voiceManager_->getVoice(i)) {
            if (v->isActive()) return true;
        }
    }
    return false;
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
        std::cout << "setParameter: oscillator_type = " << value << " (typeIndex = " << typeIndex << ")" << std::endl;
        if (typeIndex >= 0 && typeIndex <= 4) {
            setOscillatorType(static_cast<OscillatorType>(typeIndex));
        } else {
            std::cout << "ERROR: Invalid oscillator type index: " << typeIndex << std::endl;
        }
    }
    else if (paramId == "filter_cutoff") {
        // Store the base parameter value
        baseParameterValues_["filter_cutoff"] = value;
        cutoffTargetNorm_ = value;
        
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
    else if (paramId == "preset_output_trim_db") {
        // dB target; smoothing applied in process()
        baseParameterValues_["preset_output_trim_db"] = value;
        presetTrimTargetDb_ = value;
    }
    else if (paramId == "engine.hybrid_enabled") {
        setHybridWavetableEnabled(value >= 0.5f);
    }
    else if (paramId == "engine.timbre_min_phase") {
        setHybridTimbreMinPhase(value >= 0.5f);
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
        // Update base parameter value
        baseParameterValues_["envelope_decay"] = value;
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
        // Update base parameter value
        baseParameterValues_["envelope_sustain"] = value;
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
                    else if (paramName == "depth") {
                        baseParameterValues_[upperLfoName == "LFO1" ? "lfo1_depth" : "lfo2_depth"] = value;
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
        // Return stored normalized cutoff
        return baseParameterValues_.count("filter_cutoff") ? baseParameterValues_.at("filter_cutoff") : 1.0f;
    }
    else if (paramId == "filter_resonance") {
        // Return stored normalized resonance
        return baseParameterValues_.count("filter_resonance") ? baseParameterValues_.at("filter_resonance") : 0.1f;
    }
    else if (paramId == "master_volume") {
        // Return stored master volume value
        return baseParameterValues_.count("master_volume") ? baseParameterValues_.at("master_volume") : 0.7f;
    }
    else if (paramId == "preset_output_trim_db") {
        return baseParameterValues_.count("preset_output_trim_db") ? baseParameterValues_.at("preset_output_trim_db") : 0.0f;
    }
    else if (paramId == "engine.hybrid_enabled") {
        return hybridWavetableEnabled_ ? 1.0f : 0.0f;
    }
    else if (paramId == "engine.timbre_min_phase") {
        return hybridTimbreMinPhase_ ? 1.0f : 0.0f;
    }
    else if (paramId == "envelope_attack") {
        return baseParameterValues_.count("envelope_attack") ? baseParameterValues_.at("envelope_attack") : 0.01f;
    }
    else if (paramId == "envelope_decay") {
        return baseParameterValues_.count("envelope_decay") ? baseParameterValues_.at("envelope_decay") : 0.1f;
    }
    else if (paramId == "envelope_sustain") {
        return baseParameterValues_.count("envelope_sustain") ? baseParameterValues_.at("envelope_sustain") : 0.7f;
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

            // Convert to uppercase for lookup (sources are named LFO1/LFO2)
            std::string upperLfoName;
            for (char c : lfoName) upperLfoName += std::toupper(c);
            if (auto* source = modulationMatrix_.getSource(upperLfoName)) {
                if (auto* lfo = dynamic_cast<LfoSource*>(source)) {
                    if (paramName == "rate") {
                        return lfo->getFrequency();
                    }
                    else if (paramName == "shape") {
                        return static_cast<float>(lfo->getShapeIndex());
                    }
                    else if (paramName == "depth") {
                        return baseParameterValues_.count(upperLfoName == "LFO1" ? "lfo1_depth" : "lfo2_depth")
                            ? baseParameterValues_.at(upperLfoName == "LFO1" ? "lfo1_depth" : "lfo2_depth")
                            : 0.0f;
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
    // Pull real base values when available
    auto getBase = [&](const char* key, float fallback) -> float {
        auto it = baseParameterValues_.find(key);
        return (it != baseParameterValues_.end()) ? it->second : fallback;
    };
    parameters["master_volume"] = getBase("master_volume", getParameter("master_volume"));
    parameters["filter_cutoff"] = getBase("filter_cutoff", getParameter("filter_cutoff"));
    parameters["filter_resonance"] = getBase("filter_resonance", getParameter("filter_resonance"));
    parameters["preset_output_trim_db"] = getBase("preset_output_trim_db", 0.0f);
    parameters["engine.hybrid_enabled"] = isHybridWavetableEnabled() ? 1.0f : 0.0f;
    parameters["engine.timbre_min_phase"] = isHybridTimbreMinPhase() ? 1.0f : 0.0f;
    // Envelope ADSR
    parameters["envelope_attack"]  = getBase("envelope_attack",  getParameter("envelope_attack"));
    parameters["envelope_decay"]   = getBase("envelope_decay",   getParameter("envelope_decay"));
    parameters["envelope_sustain"] = getBase("envelope_sustain", getParameter("envelope_sustain"));
    parameters["envelope_release"] = getBase("envelope_release", getParameter("envelope_release"));
    // LFO Parameters (use current engine values)
    parameters["lfo1_rate"] = getParameter("lfo1_rate");
    parameters["lfo1_depth"] = getParameter("lfo1_depth");
    parameters["lfo1_shape"] = getParameter("lfo1_shape");
    parameters["lfo2_rate"] = getParameter("lfo2_rate");
    parameters["lfo2_depth"] = getParameter("lfo2_depth");
    parameters["lfo2_shape"] = getParameter("lfo2_shape");

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
    std::cout << "Synthesizer::setOscillatorType(" << static_cast<int>(type) << "), voiceManagerType = " << static_cast<int>(voiceManagerType_) << std::endl;

    if (voiceManagerType_ == VoiceManagerType::BandLimited) {
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
            std::cout << "Updated BandLimitedVoiceManager waveform to " << static_cast<int>(blWaveType) << std::endl;
        } else {
            std::cout << "ERROR: VoiceManager is not a BandLimitedVoiceManager!" << std::endl;
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
            // If hybrid enabled, also set morph position (approx map framePos->morph)
            if (hybridWavetableEnabled_) {
                // Our basic waves are 5 frames at 0.0, 0.25, 0.5, 0.75, 1.0, so morph ~= framePos
                voiceManager_->applyHybridMorph(framePos);
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

        // Ensure per-block LFO-driven pitch modulation reaches all voices (legacy and hybrid)
        if (voiceManager_) {
            if (auto* lfo1 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO1"))) {
                float lfo1Val = lfo1->getValue(); // expected in [-1,1]
                for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                    if (auto* voice = voiceManager_->getVoice(i)) {
                        if (voice->isActive()) {
                            voice->setPitchModulationValue("lfo1", lfo1Val);
                        }
                    }
                }
            }
            if (auto* lfo2 = dynamic_cast<LfoSource*>(modulationMatrix_.getSource("LFO2"))) {
                float lfo2Val = lfo2->getValue();
                for (int i = 0; i < voiceManager_->getMaxVoices(); ++i) {
                    if (auto* voice = voiceManager_->getVoice(i)) {
                        if (voice->isActive()) {
                            voice->setPitchModulationValue("lfo2", lfo2Val);
                        }
                    }
                }
            }
        }
        
        // Process voices for this block
        if (voiceManager_) {
            float* blockBuffer = buffer + (samplesProcessed * 2);
            voiceManager_->process(blockBuffer, samplesToProcess);
        }
        
        samplesProcessed += samplesToProcess;
    }
    
    // Process effects chain on the complete buffer
    effectChain_.process(buffer, numFrames);
    
    // Apply preset-level output trim (smoothed)
    {
        float targetLinear = std::pow(10.0f, (presetTrimTargetDb_) / 20.0f);
        // one-pole smoothing once per call
        presetTrimLinear_ = presetTrimLinear_ + presetTrimSmoothingAlpha_ * (targetLinear - presetTrimLinear_);
        for (int i = 0; i < numFrames * 2; ++i) {
            buffer[i] *= presetTrimLinear_;
        }
    }

    // Final limiter to prevent clipping and apply master volume (smoothed)
    const float linearVolume = baseParameterValues_.count("master_volume") ? baseParameterValues_.at("master_volume") : 0.7f;
    float targetDb = (linearVolume <= 0.0f) ? -120.0f : (-40.0f + (linearVolume * 46.0f));
    float targetLinear = (targetDb <= -100.0f) ? 0.0f : std::pow(10.0f, targetDb / 20.0f);
    masterVolumeSmoothed_ = masterVolumeSmoothed_ + masterVolumeSmoothingAlpha_ * (targetLinear - masterVolumeSmoothed_);
    for (int i = 0; i < numFrames * 2; ++i) {
        float sample = buffer[i] * masterVolumeSmoothed_;
        // Apply a brief cosine ramp after preset apply to avoid clicks
        if (paramApplyRampRemainingSamples_ > 0 && paramApplyRampTotalSamples_ > 0) {
            float t = 1.0f - (static_cast<float>(paramApplyRampRemainingSamples_) / static_cast<float>(paramApplyRampTotalSamples_));
            float ramp = 0.5f * (1.0f - std::cos(3.14159265359f * t)); // 0..1
            sample *= ramp;
        }
        buffer[i] = std::clamp(sample, -1.0f, 1.0f);
    }
    if (paramApplyRampRemainingSamples_ > 0) {
        paramApplyRampRemainingSamples_ = std::max(0, paramApplyRampRemainingSamples_ - numFrames);
    }

    // Smooth base filter cutoff target over time to avoid zippering on preset load
    // Note: This only updates the stored base value; the modulation destination continues to map to the active filter
    if (baseParameterValues_.count("filter_cutoff")) {
        cutoffSmoothedNorm_ = cutoffSmoothedNorm_ + cutoffSmoothingAlpha_ * (cutoffTargetNorm_ - cutoffSmoothedNorm_);
        baseParameterValues_["filter_cutoff"] = cutoffSmoothedNorm_;
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

std::optional<float> Synthesizer::getModAmount(const std::string& sourceName, const std::string& destName) const {
    return modulationMatrix_.getConnectionAmount(sourceName, destName);
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

void Synthesizer::setModWheel(float value) {
    modWheelValue_ = std::clamp(value, 0.0f, 1.0f);
}

void Synthesizer::setVoiceManagerType(VoiceManagerType type) {
    // Also recreate if we don't yet have a voice manager (e.g. first-time setup).
    if (voiceManagerType_ != type || !voiceManager_) {
        voiceManagerType_ = type;
        
        int maxVoices = voiceManager_ ? voiceManager_->getMaxVoices() : 16;
        std::cout << "Recreating VoiceManager with " << maxVoices << " voices" << std::endl;
        
        switch (type) {
            case VoiceManagerType::Standard:
                voiceManager_ = std::make_unique<VoiceManager>(sampleRate_, maxVoices);
                if (currentWavetable_) {
                    voiceManager_->setWavetable(currentWavetable_);
                }
                break;
            case VoiceManagerType::BandLimited:
                {
                    auto blVoiceManager = std::make_unique<BandLimitedVoiceManager>(
                        sampleRate_, maxVoices, oversamplingEnabled_);
                    
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
                }
                break;
            case VoiceManagerType::RealTime:
                voiceManager_ = std::make_unique<RealtimeWavetableVoiceManager>(sampleRate_, maxVoices);
                // Note: RealtimeWavetableVoiceManager uses FrequencyDomainWavetable, not the standard Wavetable.
                // A conversion or loading mechanism for frequency domain wavetables will be needed here.
                break;
        }
        
        std::cout << "VoiceManager type changed to " << static_cast<int>(type) << std::endl;
    }
}

void Synthesizer::setOversamplingEnabled(bool enable) {
    oversamplingEnabled_ = enable;
    
    if (voiceManagerType_ == VoiceManagerType::BandLimited) {
        if (auto* blVoiceManager = dynamic_cast<BandLimitedVoiceManager*>(voiceManager_.get())) {
            blVoiceManager->setOversamplingEnabled(enable);
        }
    }
    
    std::cout << "Oversampling " << (enable ? "enabled" : "disabled") << std::endl;
}

void Synthesizer::setOversamplingFactor(OversamplingProcessor::Factor factor) {
    oversamplingFactor_ = factor;
    
    if (voiceManagerType_ == VoiceManagerType::BandLimited) {
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
