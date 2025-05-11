# RTPC (Real-Time Parameter Control) Implementation

This document outlines the implementation of an advanced Real-Time Parameter Control (RTPC) system for the AIMusicHardware project, inspired by the sophisticated parameter mapping capabilities found in game audio middleware like FMOD and Wwise.

## Overview

The RTPC system extends our existing Parameter System with advanced mapping capabilities, providing:

1. **Non-linear Relationships** - Map input values to output values using various curve types
2. **Multi-parameter Interactions** - Combine multiple parameters to generate a single control value
3. **Complex Modulation Sources** - Use envelopes, LFOs, and other modulation sources as inputs
4. **Conditional Logic** - Apply different mappings based on conditions
5. **Time-based Smoothing** - Advanced filtering options for parameter changes

While our basic Parameter System focuses on direct control with simple linear mappings, the RTPC system enables much more sophisticated and musical control of synthesis, effects, and sequencing parameters.

## Core Components

### 1. Curve Class Hierarchy

At the heart of the RTPC system is a flexible curve representation:

```cpp
class Curve {
public:
    virtual ~Curve() = default;
    
    // Evaluate curve at a point between 0.0 and 1.0
    virtual float evaluate(float x) const = 0;
    
    // Clone the curve (for copying)
    virtual std::unique_ptr<Curve> clone() const = 0;
    
    // Serialize curve to JSON
    virtual nlohmann::json toJson() const = 0;
    
    // Deserialize from JSON
    static std::unique_ptr<Curve> fromJson(const nlohmann::json& json);
};

// Linear curve
class LinearCurve : public Curve {
public:
    LinearCurve(float startValue = 0.0f, float endValue = 1.0f);
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    float startValue_;
    float endValue_;
};

// Exponential curve
class ExponentialCurve : public Curve {
public:
    ExponentialCurve(float exponent = 2.0f, float startValue = 0.0f, float endValue = 1.0f);
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    float exponent_;
    float startValue_;
    float endValue_;
};

// S-curve (sigmoid)
class SCurve : public Curve {
public:
    SCurve(float steepness = 1.0f, float startValue = 0.0f, float endValue = 1.0f);
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    float steepness_;
    float startValue_;
    float endValue_;
};

// Step curve (staircase)
class StepCurve : public Curve {
public:
    StepCurve(int steps = 4, float startValue = 0.0f, float endValue = 1.0f);
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    int steps_;
    float startValue_;
    float endValue_;
};

// Cubic Bézier curve
class BezierCurve : public Curve {
public:
    BezierCurve(float x1 = 0.42f, float y1 = 0.0f, 
                float x2 = 0.58f, float y2 = 1.0f);
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    float x1_, y1_, x2_, y2_;
};

// Multi-segment curve (consists of multiple curve segments)
class MultiSegmentCurve : public Curve {
public:
    struct Segment {
        float startX;
        float endX;
        std::unique_ptr<Curve> curve;
    };
    
    MultiSegmentCurve();
    
    void addSegment(float startX, float endX, std::unique_ptr<Curve> curve);
    void removeSegment(size_t index);
    size_t getSegmentCount() const { return segments_.size(); }
    
    float evaluate(float x) const override;
    std::unique_ptr<Curve> clone() const override;
    nlohmann::json toJson() const override;
    
private:
    std::vector<Segment> segments_;
};
```

### 2. ParameterMapping Class

The `ParameterMapping` class defines how source parameters are mapped to target parameters:

```cpp
class ParameterMapping {
public:
    using MappingId = std::string;
    
    ParameterMapping(const MappingId& id, const std::string& name);
    ~ParameterMapping();
    
    // Mapping properties
    MappingId getId() const { return id_; }
    std::string getName() const { return name_; }
    
    // Configure source parameter(s)
    void setSourceParameter(Parameter* sourceParam);
    void addSourceParameter(Parameter* sourceParam);
    void removeSourceParameter(Parameter* sourceParam);
    
    // Configure target parameter
    void setTargetParameter(Parameter* targetParam);
    Parameter* getTargetParameter() const { return targetParam_; }
    
    // Set curve for mapping
    void setCurve(std::unique_ptr<Curve> curve);
    Curve* getCurve() const { return curve_.get(); }
    
    // Set source and target ranges
    void setSourceRange(float min, float max);
    void setTargetRange(float min, float max);
    
    // Invert mapping
    void setInverted(bool inverted);
    bool isInverted() const { return inverted_; }
    
    // Bypass control
    void setBypass(bool bypass);
    bool isBypassed() const { return bypassed_; }
    
    // Smoothing configuration
    void setSmoothing(float timeInSeconds);
    float getSmoothing() const { return smoothingTimeInSeconds_; }
    
    // Update mapping (call this from main processing loop)
    void update(float deltaTime);
    
    // Serialize/deserialize
    nlohmann::json toJson() const;
    static std::unique_ptr<ParameterMapping> fromJson(const nlohmann::json& json);
    
private:
    MappingId id_;
    std::string name_;
    
    // Source parameters
    struct SourceParamInfo {
        Parameter* parameter;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float weight = 1.0f;
        bool inverted = false;
    };
    
    std::vector<SourceParamInfo> sourceParams_;
    
    // Target parameter
    Parameter* targetParam_ = nullptr;
    float targetMinValue_ = 0.0f;
    float targetMaxValue_ = 1.0f;
    
    // Mapping curve
    std::unique_ptr<Curve> curve_;
    
    // Processing state
    bool inverted_ = false;
    bool bypassed_ = false;
    float smoothingTimeInSeconds_ = 0.0f;
    float currentOutputValue_ = 0.0f;
    
    // Calculate combined input from all source parameters
    float calculateCombinedInput() const;
    
    // Map input value through curve to output value
    float mapValue(float inputValue) const;
};
```

### 3. ModulationSource Class Hierarchy

Modulation sources provide dynamic inputs to parameter mappings:

```cpp
class ModulationSource {
public:
    using SourceId = std::string;
    
    ModulationSource(const SourceId& id, const std::string& name);
    virtual ~ModulationSource();
    
    // Source properties
    SourceId getId() const { return id_; }
    std::string getName() const { return name_; }
    
    // Get current output value (0.0 to 1.0)
    virtual float getValue() const = 0;
    
    // Update source (call each processing block)
    virtual void update(double deltaTime) = 0;
    
    // Reset the source
    virtual void reset() = 0;
    
    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    
    // Serialize/deserialize
    virtual nlohmann::json toJson() const = 0;
    static std::unique_ptr<ModulationSource> fromJson(const nlohmann::json& json);
    
protected:
    SourceId id_;
    std::string name_;
    bool enabled_ = true;
};

// LFO (Low Frequency Oscillator) modulation source
class LFO : public ModulationSource {
public:
    enum class Waveform {
        SINE,
        TRIANGLE,
        SQUARE,
        SAWTOOTH,
        REVERSE_SAWTOOTH,
        RANDOM
    };
    
    LFO(const SourceId& id, const std::string& name);
    
    // Waveform configuration
    void setWaveform(Waveform waveform);
    Waveform getWaveform() const { return waveform_; }
    
    // Frequency control
    void setFrequency(float frequencyInHz);
    float getFrequency() const { return frequencyInHz_; }
    
    // Phase control
    void setPhase(float phase);  // 0.0 to 1.0
    float getPhase() const { return phase_; }
    
    // Sync to tempo
    void syncToTempo(bool sync, float beatsPerCycle = 1.0f);
    bool isSyncedToTempo() const { return syncToTempo_; }
    
    // ModulationSource interface
    float getValue() const override;
    void update(double deltaTime) override;
    void reset() override;
    nlohmann::json toJson() const override;
    
private:
    Waveform waveform_ = Waveform::SINE;
    float frequencyInHz_ = 1.0f;
    float phase_ = 0.0f;
    float currentPhase_ = 0.0f;
    
    bool syncToTempo_ = false;
    float beatsPerCycle_ = 1.0f;
    
    // For random waveform
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    float randomTimer_ = 0.0f;
    
    // Calculate value based on current phase and waveform
    float calculateValue(float phase) const;
};

// Envelope modulation source
class Envelope : public ModulationSource {
public:
    Envelope(const SourceId& id, const std::string& name);
    
    // ADSR parameters
    void setAttack(float timeInSeconds);
    void setDecay(float timeInSeconds);
    void setSustain(float level);  // 0.0 to 1.0
    void setRelease(float timeInSeconds);
    
    float getAttack() const { return attackTime_; }
    float getDecay() const { return decayTime_; }
    float getSustain() const { return sustainLevel_; }
    float getRelease() const { return releaseTime_; }
    
    // Envelope control
    void noteOn();
    void noteOff();
    
    // ModulationSource interface
    float getValue() const override;
    void update(double deltaTime) override;
    void reset() override;
    nlohmann::json toJson() const override;
    
private:
    enum class Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };
    
    float attackTime_ = 0.1f;
    float decayTime_ = 0.2f;
    float sustainLevel_ = 0.7f;
    float releaseTime_ = 0.5f;
    
    Stage currentStage_ = Stage::IDLE;
    float currentValue_ = 0.0f;
    float stageProgress_ = 0.0f;
};

// Follower (follows an audio signal level)
class Follower : public ModulationSource {
public:
    Follower(const SourceId& id, const std::string& name);
    
    // Configure follower
    void setAttackTime(float timeInSeconds);
    void setReleaseTime(float timeInSeconds);
    float getAttackTime() const { return attackTime_; }
    float getReleaseTime() const { return releaseTime_; }
    
    // Process audio to update follower
    void processAudio(const float* audioBuffer, int numSamples, int channel = 0);
    
    // ModulationSource interface
    float getValue() const override;
    void update(double deltaTime) override;
    void reset() override;
    nlohmann::json toJson() const override;
    
private:
    float attackTime_ = 0.01f;
    float releaseTime_ = 0.1f;
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    
    // Envelope follower state
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    // Calculate coefficients based on time constants
    void updateCoefficients(float sampleRate);
};
```

### 4. ModulationMatrix Class

The `ModulationMatrix` class manages the connections between modulation sources and targets:

```cpp
class ModulationMatrix {
public:
    ModulationMatrix();
    ~ModulationMatrix();
    
    // Source management
    void addModulationSource(std::unique_ptr<ModulationSource> source);
    ModulationSource* getModulationSource(const ModulationSource::SourceId& id);
    void removeModulationSource(const ModulationSource::SourceId& id);
    
    // Mapping management
    void addParameterMapping(std::unique_ptr<ParameterMapping> mapping);
    ParameterMapping* getParameterMapping(const ParameterMapping::MappingId& id);
    void removeParameterMapping(const ParameterMapping::MappingId& id);
    
    // Connect modulation source to parameter mapping
    void connectSourceToMapping(const ModulationSource::SourceId& sourceId,
                              const ParameterMapping::MappingId& mappingId,
                              float depth = 1.0f);
                              
    void disconnectSourceFromMapping(const ModulationSource::SourceId& sourceId,
                                   const ParameterMapping::MappingId& mappingId);
    
    // Update all modulation (call this from audio thread)
    void update(double deltaTime);
    
    // Serialize/deserialize
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& json);
    
private:
    std::map<ModulationSource::SourceId, std::unique_ptr<ModulationSource>> sources_;
    std::map<ParameterMapping::MappingId, std::unique_ptr<ParameterMapping>> mappings_;
    
    // Connection information
    struct Connection {
        ModulationSource::SourceId sourceId;
        ParameterMapping::MappingId mappingId;
        float depth;  // Amount of modulation (0.0 to 1.0)
    };
    
    std::vector<Connection> connections_;
};
```

### 5. RTParameterProcessor Class

A higher-level class that combines parameter handling with RTPC capabilities:

```cpp
class RTParameterProcessor {
public:
    RTParameterProcessor();
    ~RTParameterProcessor();
    
    // Parameter creation (delegates to ParameterManager)
    template<typename T, typename... Args>
    T* createParameter(Args&&... args);
    
    // Modulation source creation
    template<typename T, typename... Args>
    T* createModulationSource(Args&&... args);
    
    // Mapping creation
    ParameterMapping* createMapping(const std::string& id, const std::string& name);
    
    // Preset management
    void savePreset(const std::string& name);
    void loadPreset(const std::string& name);
    std::vector<std::string> getPresetNames() const;
    
    // Process all parameter modulation
    void process(double deltaTime);
    
private:
    ParameterManager& paramManager_;
    ModulationMatrix modulationMatrix_;
    
    // Cache of created parameters
    std::map<std::string, Parameter*> parameters_;
    
    // Preset storage
    std::map<std::string, nlohmann::json> presets_;
};
```

## Implementation Details

### Curve Implementations

```cpp
// LinearCurve implementation
float LinearCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    return startValue_ + x * (endValue_ - startValue_);
}

// ExponentialCurve implementation
float ExponentialCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    float expValue = std::pow(x, exponent_);
    return startValue_ + expValue * (endValue_ - startValue_);
}

// SCurve implementation
float SCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    
    // Sigmoid function: 1 / (1 + e^(-steepness * (x - 0.5)))
    float sigmoid = 1.0f / (1.0f + std::exp(-steepness_ * 12.0f * (x - 0.5f)));
    
    // Normalize sigmoid to 0-1 range
    float normalizedSigmoid = (sigmoid - 1.0f / (1.0f + std::exp(steepness_ * 6.0f))) / 
                             (1.0f / (1.0f + std::exp(-steepness_ * 6.0f)) - 
                              1.0f / (1.0f + std::exp(steepness_ * 6.0f)));
    
    return startValue_ + normalizedSigmoid * (endValue_ - startValue_);
}

// StepCurve implementation
float StepCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    
    // Determine which step we're in
    float stepSize = 1.0f / steps_;
    int currentStep = static_cast<int>(x / stepSize);
    
    // Calculate value for this step
    float stepValue = static_cast<float>(currentStep) / steps_;
    
    return startValue_ + stepValue * (endValue_ - startValue_);
}

// BezierCurve implementation
float BezierCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    
    // Use De Casteljau's algorithm for cubic Bézier curve
    float t = x;
    float invT = 1.0f - t;
    
    // Control points are P0(0,0), P1(x1,y1), P2(x2,y2), P3(1,1)
    float p0x = 0.0f, p0y = 0.0f;
    float p3x = 1.0f, p3y = 1.0f;
    
    // Compute the y value for given x using cubic Bézier formula
    // B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
    float y = invT * invT * invT * p0y +
              3.0f * invT * invT * t * y1_ +
              3.0f * invT * t * t * y2_ +
              t * t * t * p3y;
    
    return y;
}

// MultiSegmentCurve implementation
float MultiSegmentCurve::evaluate(float x) const {
    x = std::clamp(x, 0.0f, 1.0f);
    
    // Find which segment contains x
    for (const auto& segment : segments_) {
        if (x >= segment.startX && x <= segment.endX) {
            // Normalize x to segment's local space (0-1)
            float localX = (x - segment.startX) / (segment.endX - segment.startX);
            
            // Evaluate using this segment's curve
            return segment.curve->evaluate(localX);
        }
    }
    
    // If no segment found or empty, return input value
    return x;
}
```

### Parameter Mapping Implementation

```cpp
// ParameterMapping implementation
ParameterMapping::ParameterMapping(const MappingId& id, const std::string& name)
    : id_(id), name_(name), curve_(std::make_unique<LinearCurve>()) {
}

void ParameterMapping::setSourceParameter(Parameter* sourceParam) {
    sourceParams_.clear();
    
    if (sourceParam) {
        SourceParamInfo info;
        info.parameter = sourceParam;
        
        // Set appropriate range based on parameter type
        if (auto* floatParam = dynamic_cast<FloatParameter*>(sourceParam)) {
            info.minValue = floatParam->getMinValue();
            info.maxValue = floatParam->getMaxValue();
        } else if (auto* intParam = dynamic_cast<IntParameter*>(sourceParam)) {
            info.minValue = static_cast<float>(intParam->getMinValue());
            info.maxValue = static_cast<float>(intParam->getMaxValue());
        }
        
        sourceParams_.push_back(info);
    }
}

void ParameterMapping::addSourceParameter(Parameter* sourceParam) {
    if (sourceParam) {
        SourceParamInfo info;
        info.parameter = sourceParam;
        
        // Set appropriate range
        if (auto* floatParam = dynamic_cast<FloatParameter*>(sourceParam)) {
            info.minValue = floatParam->getMinValue();
            info.maxValue = floatParam->getMaxValue();
        } else if (auto* intParam = dynamic_cast<IntParameter*>(sourceParam)) {
            info.minValue = static_cast<float>(intParam->getMinValue());
            info.maxValue = static_cast<float>(intParam->getMaxValue());
        }
        
        sourceParams_.push_back(info);
    }
}

void ParameterMapping::setSourceRange(float min, float max) {
    if (!sourceParams_.empty()) {
        sourceParams_[0].minValue = min;
        sourceParams_[0].maxValue = max;
    }
}

void ParameterMapping::setTargetRange(float min, float max) {
    targetMinValue_ = min;
    targetMaxValue_ = max;
}

float ParameterMapping::calculateCombinedInput() const {
    if (sourceParams_.empty()) {
        return 0.0f;
    }
    
    if (sourceParams_.size() == 1) {
        // Single parameter - simple normalization
        const auto& info = sourceParams_[0];
        Parameter* param = info.parameter;
        
        float value = 0.0f;
        if (auto* floatParam = dynamic_cast<FloatParameter*>(param)) {
            value = floatParam->getValue();
        } else if (auto* intParam = dynamic_cast<IntParameter*>(param)) {
            value = static_cast<float>(intParam->getValue());
        } else if (auto* boolParam = dynamic_cast<BoolParameter*>(param)) {
            value = boolParam->getValue() ? 1.0f : 0.0f;
        }
        
        // Normalize to 0-1 range
        float normalizedValue = (value - info.minValue) / (info.maxValue - info.minValue);
        
        // Apply inversion if needed
        if (info.inverted) {
            normalizedValue = 1.0f - normalizedValue;
        }
        
        return std::clamp(normalizedValue, 0.0f, 1.0f);
    } 
    else {
        // Multiple parameters - weighted average
        float combinedValue = 0.0f;
        float totalWeight = 0.0f;
        
        for (const auto& info : sourceParams_) {
            Parameter* param = info.parameter;
            
            float value = 0.0f;
            if (auto* floatParam = dynamic_cast<FloatParameter*>(param)) {
                value = floatParam->getValue();
            } else if (auto* intParam = dynamic_cast<IntParameter*>(param)) {
                value = static_cast<float>(intParam->getValue());
            } else if (auto* boolParam = dynamic_cast<BoolParameter*>(param)) {
                value = boolParam->getValue() ? 1.0f : 0.0f;
            }
            
            // Normalize to 0-1 range
            float normalizedValue = (value - info.minValue) / (info.maxValue - info.minValue);
            
            // Apply inversion if needed
            if (info.inverted) {
                normalizedValue = 1.0f - normalizedValue;
            }
            
            combinedValue += normalizedValue * info.weight;
            totalWeight += info.weight;
        }
        
        // Avoid division by zero
        if (totalWeight > 0.0f) {
            combinedValue /= totalWeight;
        }
        
        return std::clamp(combinedValue, 0.0f, 1.0f);
    }
}

float ParameterMapping::mapValue(float inputValue) const {
    // Apply global inversion if needed
    if (inverted_) {
        inputValue = 1.0f - inputValue;
    }
    
    // Map through curve
    float mappedValue = curve_->evaluate(inputValue);
    
    // Map to target range
    float result = targetMinValue_ + mappedValue * (targetMaxValue_ - targetMinValue_);
    
    return result;
}

void ParameterMapping::update(float deltaTime) {
    if (bypassed_ || !targetParam_) {
        return;
    }
    
    // Get input value
    float inputValue = calculateCombinedInput();
    
    // Map to output value
    float outputValue = mapValue(inputValue);
    
    // Apply smoothing if enabled
    if (smoothingTimeInSeconds_ > 0.0f) {
        float smoothingFactor = deltaTime / smoothingTimeInSeconds_;
        if (smoothingFactor > 1.0f) smoothingFactor = 1.0f;
        
        currentOutputValue_ += smoothingFactor * (outputValue - currentOutputValue_);
    } else {
        currentOutputValue_ = outputValue;
    }
    
    // Apply to target parameter
    if (auto* floatParam = dynamic_cast<FloatParameter*>(targetParam_)) {
        floatParam->setValue(currentOutputValue_);
    } else if (auto* intParam = dynamic_cast<IntParameter*>(targetParam_)) {
        intParam->setValue(static_cast<int>(std::round(currentOutputValue_)));
    }
}
```

### Modulation Source Implementations

```cpp
// LFO implementation
float LFO::calculateValue(float phase) const {
    phase = std::fmod(phase, 1.0f);
    
    switch (waveform_) {
        case Waveform::SINE:
            return 0.5f + 0.5f * std::sin(phase * 2.0f * M_PI - M_PI_2);
            
        case Waveform::TRIANGLE:
            if (phase < 0.5f) {
                return phase * 2.0f; // Rising phase (0 to 1)
            } else {
                return 2.0f - phase * 2.0f; // Falling phase (1 to 0)
            }
            
        case Waveform::SQUARE:
            return phase < 0.5f ? 0.0f : 1.0f;
            
        case Waveform::SAWTOOTH:
            return phase;
            
        case Waveform::REVERSE_SAWTOOTH:
            return 1.0f - phase;
            
        case Waveform::RANDOM:
            // Using stored value for random (updated in update())
            return currentValue_;
            
        default:
            return 0.0f;
    }
}

void LFO::update(double deltaTime) {
    if (!enabled_) return;
    
    // Update phase
    float effectiveFrequency = frequencyInHz_;
    
    // If synced to tempo, calculate frequency from tempo
    if (syncToTempo_) {
        // Assume we have a way to get current tempo
        float tempo = 120.0f; // This should come from the sequencer
        effectiveFrequency = tempo / 60.0f / beatsPerCycle_;
    }
    
    currentPhase_ += static_cast<float>(deltaTime * effectiveFrequency);
    
    // Keep phase in 0-1 range
    currentPhase_ = std::fmod(currentPhase_, 1.0f);
    
    // For random waveform, update random value when needed
    if (waveform_ == Waveform::RANDOM) {
        randomTimer_ -= static_cast<float>(deltaTime);
        
        if (randomTimer_ <= 0.0f) {
            // Generate new target value
            targetValue_ = static_cast<float>(rand()) / RAND_MAX;
            
            // Reset timer (hold this value for a fraction of the LFO period)
            randomTimer_ = 1.0f / effectiveFrequency * 0.25f;
        }
        
        // Smooth between values for less clicking
        float smoothingFactor = static_cast<float>(deltaTime * 10.0f);
        if (smoothingFactor > 1.0f) smoothingFactor = 1.0f;
        
        currentValue_ += smoothingFactor * (targetValue_ - currentValue_);
    }
}

float LFO::getValue() const {
    if (!enabled_) return 0.0f;
    
    if (waveform_ == Waveform::RANDOM) {
        return currentValue_;
    }
    
    // Calculate based on current phase
    return calculateValue(currentPhase_ + phase_);
}

// Envelope implementation
void Envelope::noteOn() {
    currentStage_ = Stage::ATTACK;
    stageProgress_ = 0.0f;
}

void Envelope::noteOff() {
    // Only go to release if not already in IDLE
    if (currentStage_ != Stage::IDLE) {
        currentStage_ = Stage::RELEASE;
        stageProgress_ = 0.0f;
    }
}

void Envelope::update(double deltaTime) {
    if (!enabled_) return;
    
    // Update envelope based on current stage
    switch (currentStage_) {
        case Stage::IDLE:
            currentValue_ = 0.0f;
            break;
            
        case Stage::ATTACK:
            if (attackTime_ > 0.0f) {
                stageProgress_ += static_cast<float>(deltaTime) / attackTime_;
                
                if (stageProgress_ >= 1.0f) {
                    currentValue_ = 1.0f;
                    currentStage_ = Stage::DECAY;
                    stageProgress_ = 0.0f;
                } else {
                    // Linear attack
                    currentValue_ = stageProgress_;
                }
            } else {
                // Zero attack time
                currentValue_ = 1.0f;
                currentStage_ = Stage::DECAY;
                stageProgress_ = 0.0f;
            }
            break;
            
        case Stage::DECAY:
            if (decayTime_ > 0.0f) {
                stageProgress_ += static_cast<float>(deltaTime) / decayTime_;
                
                if (stageProgress_ >= 1.0f) {
                    currentValue_ = sustainLevel_;
                    currentStage_ = Stage::SUSTAIN;
                    stageProgress_ = 0.0f;
                } else {
                    // Linear decay from 1.0 to sustain
                    currentValue_ = 1.0f - stageProgress_ * (1.0f - sustainLevel_);
                }
            } else {
                // Zero decay time
                currentValue_ = sustainLevel_;
                currentStage_ = Stage::SUSTAIN;
                stageProgress_ = 0.0f;
            }
            break;
            
        case Stage::SUSTAIN:
            currentValue_ = sustainLevel_;
            break;
            
        case Stage::RELEASE:
            if (releaseTime_ > 0.0f) {
                stageProgress_ += static_cast<float>(deltaTime) / releaseTime_;
                
                if (stageProgress_ >= 1.0f) {
                    currentValue_ = 0.0f;
                    currentStage_ = Stage::IDLE;
                    stageProgress_ = 0.0f;
                } else {
                    // Linear release from sustain level to 0
                    currentValue_ = sustainLevel_ * (1.0f - stageProgress_);
                }
            } else {
                // Zero release time
                currentValue_ = 0.0f;
                currentStage_ = Stage::IDLE;
                stageProgress_ = 0.0f;
            }
            break;
    }
}

float Envelope::getValue() const {
    if (!enabled_) return 0.0f;
    return currentValue_;
}

// Follower implementation
void Follower::updateCoefficients(float sampleRate) {
    // Coefficients for first-order IIR filter
    attackCoeff_ = std::exp(-1.0f / (attackTime_ * sampleRate));
    releaseCoeff_ = std::exp(-1.0f / (releaseTime_ * sampleRate));
}

void Follower::processAudio(const float* audioBuffer, int numSamples, int channel) {
    if (!enabled_) return;
    
    float sampleRate = 44100.0f; // This should come from the audio engine
    updateCoefficients(sampleRate);
    
    // Process all samples to get peak value
    float maxLevel = 0.0f;
    
    for (int i = 0; i < numSamples; i++) {
        float sample = std::abs(audioBuffer[i]);
        if (sample > maxLevel) {
            maxLevel = sample;
        }
    }
    
    // Update target value based on peak
    targetValue_ = maxLevel;
}

void Follower::update(double deltaTime) {
    if (!enabled_) return;
    
    // Update follower value using attack/release coefficients
    if (targetValue_ > currentValue_) {
        // Attack phase
        currentValue_ = attackCoeff_ * currentValue_ + (1.0f - attackCoeff_) * targetValue_;
    } else {
        // Release phase
        currentValue_ = releaseCoeff_ * currentValue_ + (1.0f - releaseCoeff_) * targetValue_;
    }
}

float Follower::getValue() const {
    if (!enabled_) return 0.0f;
    return currentValue_;
}
```

### Modulation Matrix Implementation

```cpp
void ModulationMatrix::update(double deltaTime) {
    // Update all modulation sources
    for (auto& [id, source] : sources_) {
        source->update(deltaTime);
    }
    
    // Apply modulation to parameter mappings
    for (const auto& connection : connections_) {
        ModulationSource* source = getModulationSource(connection.sourceId);
        ParameterMapping* mapping = getParameterMapping(connection.mappingId);
        
        if (source && mapping) {
            // Get source value
            float sourceValue = source->getValue();
            
            // Apply depth
            float modulationAmount = sourceValue * connection.depth;
            
            // Apply to mapping
            // (Implementation depends on how we want to apply modulation -
            // could be additive, multiplicative, or through a separate modulation input)
        }
    }
    
    // Update all parameter mappings
    for (auto& [id, mapping] : mappings_) {
        mapping->update(static_cast<float>(deltaTime));
    }
}
```

## Integration with Existing System

### Integration with Parameter System

The RTPC system builds on top of our existing Parameter System, extending it with advanced mapping capabilities:

```cpp
// In ParameterManager.h
class ParameterManager {
public:
    // ... existing code ...
    
    // RTPC integration
    void setModulationMatrix(ModulationMatrix* matrix);
    ModulationMatrix* getModulationMatrix() const { return modulationMatrix_; }
    
    // Create parameter mapping
    ParameterMapping* createMapping(const std::string& id, const std::string& name);
    
    // Create modulation source
    template<typename T, typename... Args>
    T* createModulationSource(Args&&... args);
    
private:
    // ... existing code ...
    
    ModulationMatrix* modulationMatrix_ = nullptr;
};
```

### Integration with Audio Engine

```cpp
// In AudioEngine.h
class AudioEngine {
public:
    // ... existing code ...
    
    // RTPC integration
    void setRTParameterProcessor(RTParameterProcessor* processor);
    
    // Followers need audio input
    void processFollowers(const float* buffer, int numSamples, int numChannels);
    
    // Update modulation
    void updateModulation(double deltaTime);
    
private:
    // ... existing code ...
    
    RTParameterProcessor* rtProcessor_ = nullptr;
};

// In AudioEngine.cpp
void AudioEngine::processFollowers(const float* buffer, int numSamples, int numChannels) {
    if (!rtProcessor_) return;
    
    auto& matrix = rtProcessor_->getModulationMatrix();
    
    // Process all follower modulation sources
    for (auto& [id, source] : matrix.getSources()) {
        if (auto* follower = dynamic_cast<Follower*>(source.get())) {
            follower->processAudio(buffer, numSamples, 0); // Use first channel
        }
    }
}

void AudioEngine::updateModulation(double deltaTime) {
    if (rtProcessor_) {
        rtProcessor_->process(deltaTime);
    }
}
```

### Integration with State Machine System

```cpp
// In StateMachine.h
class StateMachine {
public:
    // ... existing code ...
    
    // RTPC integration - store mappings with states
    void addRTPCMapping(const std::string& stateName, ParameterMapping* mapping, 
                       float stateValue);
    
    // When state changes, update RTPC mappings
    void updateRTPCMappings();
    
private:
    // ... existing code ...
    
    // State-specific RTPC mappings
    struct StateRTPCMapping {
        std::string stateName;
        ParameterMapping* mapping;
        float value;
    };
    
    std::vector<StateRTPCMapping> stateMappings_;
};

// In StateMachine.cpp
void StateMachine::updateRTPCMappings() {
    std::string currentState = getCurrentState()->getName();
    
    // Update all mappings based on current state
    for (const auto& mapping : stateMappings_) {
        if (mapping.stateName == currentState) {
            // This mapping is active for this state
            mapping.mapping->setBypass(false);
            
            // TODO: Apply state-specific value or mapping
        } else {
            // This mapping is inactive
            mapping.mapping->setBypass(true);
        }
    }
}
```

### Integration with Sequencer

```cpp
// In Sequencer.h
class Sequencer {
public:
    // ... existing code ...
    
    // RTPC integration
    void setModulationMatrix(ModulationMatrix* matrix);
    
    // Create time-synced LFOs and other modulation sources
    LFO* createTempoSyncedLFO(const std::string& id, const std::string& name, 
                            float beatsPerCycle = 1.0f);
    
    // Update modulation sources with tempo info
    void updateModulationSources();
    
private:
    // ... existing code ...
    
    ModulationMatrix* modulationMatrix_ = nullptr;
    std::vector<LFO*> tempoSyncedLFOs_;
};

// In Sequencer.cpp
LFO* Sequencer::createTempoSyncedLFO(const std::string& id, const std::string& name, 
                                   float beatsPerCycle) {
    if (!modulationMatrix_) return nullptr;
    
    auto lfo = std::make_unique<LFO>(id, name);
    lfo->syncToTempo(true, beatsPerCycle);
    
    LFO* result = lfo.get();
    modulationMatrix_->addModulationSource(std::move(lfo));
    tempoSyncedLFOs_.push_back(result);
    
    return result;
}

void Sequencer::updateModulationSources() {
    float currentTempo = getTempo();
    
    // Update tempo-synced LFOs
    for (auto* lfo : tempoSyncedLFOs_) {
        // Update LFO with current tempo info if needed
    }
}
```

## Practical Examples

### Basic Curve Mapping Example

```cpp
// Create a parameter for filter cutoff
auto& paramManager = ParameterManager::getInstance();
auto* cutoffParam = paramManager.createParameter<FloatParameter>(
    "filter/cutoff", "Filter Cutoff", 1000.0f);
cutoffParam->setRange(20.0f, 20000.0f);

// Create a parameter for modulation wheel
auto* modWheelParam = paramManager.createParameter<FloatParameter>(
    "midi/modWheel", "Mod Wheel", 0.0f);
modWheelParam->setRange(0.0f, 127.0f);

// Create an exponential mapping from mod wheel to cutoff
auto& rtProcessor = RTParameterProcessor::getInstance();
auto* mapping = rtProcessor.createMapping("modWheel_to_cutoff", "ModWheel -> Cutoff");
mapping->setSourceParameter(modWheelParam);
mapping->setTargetParameter(cutoffParam);

// Use exponential curve for more musical filter control
auto curve = std::make_unique<ExponentialCurve>(2.0f);
mapping->setCurve(std::move(curve));

// Set ranges (map MIDI 0-127 to frequency 100-10000)
mapping->setSourceRange(0.0f, 127.0f);
mapping->setTargetRange(100.0f, 10000.0f);

// Add smoothing to avoid zipper noise
mapping->setSmoothing(0.1f);
```

### Multi-Parameter Mapping Example

```cpp
// Create velocity and note parameters
auto* velocityParam = paramManager.createParameter<FloatParameter>(
    "midi/velocity", "Note Velocity", 64.0f);
velocityParam->setRange(1.0f, 127.0f);

auto* noteParam = paramManager.createParameter<IntParameter>(
    "midi/note", "Note Number", 60);
noteParam->setRange(0, 127);

// Create filter cutoff parameter
auto* cutoffParam = paramManager.createParameter<FloatParameter>(
    "filter/cutoff", "Filter Cutoff", 1000.0f);
cutoffParam->setRange(20.0f, 20000.0f);

// Create a mapping that combines velocity and note
auto* mapping = rtProcessor.createMapping("note_velocity_to_cutoff", "Note+Vel -> Cutoff");
mapping->addSourceParameter(velocityParam);
mapping->addSourceParameter(noteParam);

// Adjust weights (velocity matters more than note number)
mapping->setSourceParameterWeight(velocityParam, 0.7f);
mapping->setSourceParameterWeight(noteParam, 0.3f);

mapping->setTargetParameter(cutoffParam);
mapping->setTargetRange(100.0f, 18000.0f);

// Use S-curve for more natural response
auto curve = std::make_unique<SCurve>(1.5f);
mapping->setCurve(std::move(curve));
```

### LFO Modulation Example

```cpp
// Create oscillator detune parameter
auto* detuneParam = paramManager.createParameter<FloatParameter>(
    "oscillator/detune", "Oscillator Detune", 0.0f);
detuneParam->setRange(-100.0f, 100.0f);  // Cents

// Create LFO
auto* lfo = rtProcessor.createModulationSource<LFO>("vibrato_lfo", "Vibrato LFO");
lfo->setWaveform(LFO::Waveform::SINE);
lfo->setFrequency(5.0f);  // 5 Hz

// Create mapping for detune
auto* mapping = rtProcessor.createMapping("lfo_to_detune", "LFO -> Detune");
mapping->setTargetParameter(detuneParam);

// Connect LFO to mapping with depth control
rtProcessor.getModulationMatrix().connectSourceToMapping(
    "vibrato_lfo", "lfo_to_detune", 0.5f);  // 50% depth

// This will create vibrato effect by modulating detune at 5 Hz
```

### Envelope Follower Example

```cpp
// Create a compressor threshold parameter
auto* thresholdParam = paramManager.createParameter<FloatParameter>(
    "compressor/threshold", "Threshold", -20.0f);
thresholdParam->setRange(-60.0f, 0.0f);  // dB

// Create an envelope follower
auto* follower = rtProcessor.createModulationSource<Follower>(
    "kick_follower", "Kick Drum Follower");
follower->setAttackTime(0.001f);   // Fast attack
follower->setReleaseTime(0.2f);    // Moderate release

// Create a mapping that will make threshold follow the kick drum
auto* mapping = rtProcessor.createMapping("kick_to_threshold", "Kick -> Threshold");
mapping->setTargetParameter(thresholdParam);

// Connect follower to mapping (using negative depth for "ducking" effect)
rtProcessor.getModulationMatrix().connectSourceToMapping(
    "kick_follower", "kick_to_threshold", -1.0f);  // -100% depth = invert

// When kick hits, threshold will drop, causing more compression
```

### Multi-segment Curve Example

```cpp
// Create a multi-segment curve for complex response
auto curve = std::make_unique<MultiSegmentCurve>();

// Add segments
curve->addSegment(0.0f, 0.2f, std::make_unique<ExponentialCurve>(3.0f));
curve->addSegment(0.2f, 0.8f, std::make_unique<LinearCurve>());
curve->addSegment(0.8f, 1.0f, std::make_unique<SCurve>(2.0f));

// Create mapping using this curve
auto* mapping = rtProcessor.createMapping("expression_to_filter", "Expression -> Filter");
mapping->setCurve(std::move(curve));

// This creates a response that is:
// - Very gradual at first (exponential)
// - Linear in the middle range
// - Gradually tapers off at the end (S-curve)
```

### State-dependent Modulation Example

```cpp
// Create combat intensity parameter
auto* intensityParam = paramManager.createParameter<FloatParameter>(
    "combat/intensity", "Combat Intensity", 0.0f);
intensityParam->setRange(0.0f, 1.0f);

// Create LFO with different settings for different states
auto* pulseLFO = rtProcessor.createModulationSource<LFO>("pulse_lfo", "Pulse LFO");

// Create mapping to filter cutoff
auto* filterCutoff = paramManager.getParameterByPath("filter/cutoff");
auto* mapping = rtProcessor.createMapping("pulse_to_cutoff", "Pulse -> Cutoff");
mapping->setTargetParameter(filterCutoff);
mapping->setTargetRange(200.0f, 5000.0f);

// Connect LFO to mapping
rtProcessor.getModulationMatrix().connectSourceToMapping(
    "pulse_lfo", "pulse_to_cutoff", 0.7f);  // 70% depth

// Now register with state machine for combat state
stateMachine.addRTPCContext("explore", [pulseLFO]() {
    // Slow, gentle pulse in exploration mode
    pulseLFO->setFrequency(0.25f);
    pulseLFO->setWaveform(LFO::Waveform::SINE);
});

stateMachine.addRTPCContext("combat", [pulseLFO]() {
    // Faster, sharper pulse in combat mode
    pulseLFO->setFrequency(4.0f);
    pulseLFO->setWaveform(LFO::Waveform::TRIANGLE);
});

// The LFO behavior will change based on the current state
```

## Implementation Timeline

1. **Phase 1: Core Curve Implementation (1-2 days)**
   - Implement `Curve` base class
   - Implement common curve types (Linear, Exponential, SCurve, etc.)
   - Create unit tests for curve behavior
   - Implement MultiSegmentCurve

2. **Phase 2: Parameter Mapping (1-2 days)**
   - Implement `ParameterMapping` class
   - Implement source/target parameter handling
   - Add curve application
   - Add smoothing
   - Create tests for parameter mapping

3. **Phase 3: Modulation Sources (2-3 days)**
   - Implement `ModulationSource` base class
   - Implement LFO class
   - Implement Envelope class
   - Implement Follower class
   - Create tests for modulation sources

4. **Phase 4: Modulation Matrix (1-2 days)**
   - Implement `ModulationMatrix` class
   - Add source/mapping connections
   - Implement depth control
   - Create test cases

5. **Phase 5: RTParameterProcessor (1 day)**
   - Implement high-level processor class
   - Add preset management
   - Add serialization/deserialization

6. **Phase 6: Integration (2-3 days)**
   - Integrate with Parameter System
   - Integrate with Audio Engine
   - Integrate with Sequencer
   - Integrate with State Machine
   - Create integration tests

7. **Phase 7: Optimization and Polishing (1-2 days)**
   - Optimize performance
   - Add threading safety
   - Refine API
   - Create example usage code

## Conclusion

The RTPC system builds on our existing Parameter System to provide advanced control capabilities that are essential for expressive musical applications. Key features include:

1. **Non-linear Response** - Use various curve types for more musical parameter mapping
2. **Complex Parameter Relationships** - Combine multiple parameters into sophisticated control signals
3. **Dynamic Modulation** - LFOs, envelopes, and followers create time-varying control signals
4. **Hierarchical Control** - Parameters can modulate other parameters, creating complex behaviors
5. **Musical Context Awareness** - Mappings can respond differently based on musical state

This system integrates with our previously implemented components:

- **State-Based Music System** - Different RTPC mappings can be active in different states
- **Vertical Remix System** - Layer volumes can be controlled via sophisticated mappings
- **Horizontal Re-sequencing** - Segment transitions can be influenced by parameter mappings
- **Parameter System** - Basic parameters are extended with advanced mapping capabilities
- **Event System** - Events can trigger changes in RTPC mappings or modulation sources

The RTPC system represents the final piece of our game audio middleware-inspired architecture, providing the sophisticated control necessary for truly adaptive and expressive music.