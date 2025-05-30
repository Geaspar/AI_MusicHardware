# Sequencer Technical Specification

## Architecture Overview

The sequencer system follows a multi-layered architecture designed for real-time performance, adaptive behavior, and extensibility:

```
Application Layer
├── SongManager (Song/Section Management)
├── PerformanceController (Live Control)
└── AdaptiveEngine (Game Audio Logic)

Pattern Layer
├── PatternManager (Pattern Storage/Organization)
├── TrackManager (Multi-track Coordination)
└── VariationSystem (A/B Patterns)

Sequencing Engine
├── SequencerEngine (Core Timing/Transport)
├── StepProcessor (Step Execution)
└── ParameterAutomation (Automation Engine)

Audio Integration
├── AudioCallback (Sample-accurate Timing)
├── MIDIInterface (External Sync/Control)
└── EventSystem (Note/Parameter Events)
```

## Core Engine Implementation

### SequencerEngine

Central timing and transport management:

```cpp
class SequencerEngine {
public:
    struct EngineConfig {
        int sampleRate = 44100;
        int bufferSize = 256;
        bool sampleAccurateTiming = true;
        int lookaheadSamples = 64;
        bool enableMIDISync = false;
        float defaultTempo = 120.0f;
    };
    
    SequencerEngine(const EngineConfig& config);
    
    // Transport control
    void play();
    void stop();
    void pause();
    void record(bool enable);
    
    // Timing
    void setTempo(float bpm);
    void setTimeSignature(int numerator, int denominator);
    void setSwing(float amount);  // -1.0 to 1.0
    
    // Playback position
    struct PlaybackPosition {
        int64_t samplePosition;    // Absolute sample position
        double beatPosition;       // Beat position (fractional)
        int barPosition;          // Current bar
        int stepPosition;         // Current step in pattern
        int tickPosition;         // MIDI clock ticks
    };
    
    PlaybackPosition getPlaybackPosition() const;
    void setPlaybackPosition(const PlaybackPosition& pos);
    
    // Audio processing
    void processAudio(float** audioBuffers, int numChannels, int numSamples);
    
private:
    EngineConfig config_;
    
    // Timing state
    std::atomic<bool> isPlaying_{false};
    std::atomic<bool> isRecording_{false};
    std::atomic<float> tempo_{120.0f};
    
    // High-precision timing
    double samplesPerBeat_;
    double samplesPerTick_;
    int64_t totalSamples_ = 0;
    double fractionalSamples_ = 0.0;
    
    // MIDI sync
    std::unique_ptr<MIDIClockManager> midiClock_;
    
    // Pattern processing
    std::vector<std::unique_ptr<Pattern>> activePatterns_;
    std::mutex patternMutex_;
    
    // Audio thread callback
    void audioCallback(int numSamples);
    void processPatterns(int numSamples);
    void handleQuantizedEvents();
};
```

### Pattern System

```cpp
class Pattern {
public:
    struct PatternConfig {
        std::string name;
        int length = 16;           // Number of steps
        int subdivision = 16;      // 16th notes default
        float swing = 0.0f;
        int transpose = 0;
        Scale scale = Scale::Chromatic;
        float probability = 1.0f;
    };
    
    Pattern(const PatternConfig& config);
    
    // Step data management
    void setStep(int stepIndex, const StepData& data);
    StepData getStep(int stepIndex) const;
    void clearStep(int stepIndex);
    
    // Pattern properties
    void setLength(int steps);
    void setSwing(float amount);
    void setTranspose(int semitones);
    void setScale(const Scale& scale);
    
    // Automation tracks
    AutomationTrack* getAutomationTrack(const std::string& parameterId);
    void addAutomationTrack(const std::string& parameterId);
    
    // Variations
    void createVariation(const std::string& name);
    void switchToVariation(const std::string& name);
    std::vector<std::string> getVariationNames() const;
    
    // Pattern generation
    void generateEuclideanRhythm(int hits, int rotation = 0);
    void generateFromScale(const Scale& scale, float density = 0.5f);
    void applyGroove(const GrooveTemplate& groove);
    
    // Analysis
    struct PatternAnalysis {
        float density;           // Note density (0-1)
        float complexity;        // Rhythmic complexity
        float syncopation;       // Syncopation amount
        std::vector<int> accents; // Accent positions
    };
    
    PatternAnalysis analyze() const;
    
private:
    PatternConfig config_;
    std::vector<StepData> steps_;
    std::map<std::string, std::unique_ptr<AutomationTrack>> automationTracks_;
    std::map<std::string, std::vector<StepData>> variations_;
    std::string currentVariation_ = "main";
    
    // Real-time safe pattern switching
    std::atomic<bool> pendingVariationSwitch_{false};
    std::string pendingVariation_;
};
```

### StepData Structure

```cpp
struct StepData {
    // Note information
    bool trigger = false;
    int note = 60;              // MIDI note number
    int velocity = 127;         // MIDI velocity
    float gate = 1.0f;          // Gate length (0-1)
    
    // Timing
    float microTiming = 0.0f;   // Timing offset in beats (-0.5 to 0.5)
    float probability = 1.0f;   // Trigger probability (0-1)
    
    // Retrigger
    int retriggerCount = 0;     // Number of retriggers
    float retriggerRate = 0.25f; // Retrigger interval in beats
    
    // Conditions
    enum class Condition {
        Always,
        Every2nd,
        Every3rd,
        Every4th,
        FirstOfBar,
        LastOfBar,
        Random,
        Custom
    };
    
    Condition condition = Condition::Always;
    std::function<bool()> customCondition;
    
    // Parameter locks (per-step parameter overrides)
    std::map<std::string, float> parameterLocks;
    
    // Slide/portamento
    bool slide = false;
    float slideTime = 0.1f;     // Slide time in beats
    
    // Step state (runtime)
    mutable int playCount = 0;  // How many times this step has played
    mutable bool conditionMet = false;
};
```

### Track Management

```cpp
class Track {
public:
    enum class Type {
        Melodic,        // Note-based track
        Drum,          // Percussion track
        Automation,    // Parameter automation only
        Audio,         // Audio sample track
        MIDI           // External MIDI track
    };
    
    Track(const std::string& id, Type type);
    
    // Pattern assignment
    void assignPattern(std::shared_ptr<Pattern> pattern);
    void queuePattern(std::shared_ptr<Pattern> pattern, PatternTransition transition);
    
    // Track properties
    void setMute(bool muted);
    void setSolo(bool soloed);
    void setVolume(float volume);
    void setPan(float pan);
    
    // MIDI/Audio routing
    void setMIDIChannel(int channel);
    void setAudioOutput(int outputIndex);
    
    // Performance controls
    void setQuantization(Quantization quant);
    void setLaunchMode(LaunchMode mode);
    
    // Processing
    void processStep(int stepIndex, const PlaybackPosition& position);
    
private:
    std::string id_;
    Type type_;
    
    std::shared_ptr<Pattern> currentPattern_;
    std::shared_ptr<Pattern> queuedPattern_;
    PatternTransition queuedTransition_ = PatternTransition::None;
    
    // Track state
    bool muted_ = false;
    bool soloed_ = false;
    float volume_ = 1.0f;
    float pan_ = 0.0f;
    
    // Quantization
    Quantization quantization_ = Quantization::Bar;
    int quantizationCounter_ = 0;
    
    // Performance
    LaunchMode launchMode_ = LaunchMode::Trigger;
    
    // Audio/MIDI
    int midiChannel_ = 1;
    int audioOutput_ = 0;
};
```

## Adaptive Sequencing Implementation

### AdaptiveEngine

Game audio-inspired adaptive behavior:

```cpp
class AdaptiveEngine {
public:
    struct AdaptiveState {
        std::string name;
        float tempoMin = 120.0f;
        float tempoMax = 120.0f;
        std::vector<std::string> activePatterns;
        std::map<std::string, float> parameters;
        float transitionProbability = 1.0f;
    };
    
    struct StateTransition {
        std::string fromState;
        std::string toState;
        std::function<bool()> condition;
        float probability = 1.0f;
        float transitionTime = 1.0f;  // Bars
        TransitionType type = TransitionType::Crossfade;
    };
    
    AdaptiveEngine(SequencerEngine* sequencer);
    
    // State management
    void defineState(const std::string& name, const AdaptiveState& state);
    void setCurrentState(const std::string& name);
    void addStateTransition(const StateTransition& transition);
    
    // Parameter-driven adaptation
    void setAdaptationParameter(const std::string& name, float value);
    void mapParameterToTempo(const std::string& param, float minTempo, float maxTempo);
    void mapParameterToPatternSelection(const std::string& param, 
                                       const std::vector<std::string>& patterns);
    
    // Update (called from audio thread)
    void update();
    
private:
    SequencerEngine* sequencer_;
    
    std::map<std::string, AdaptiveState> states_;
    std::string currentState_;
    std::string targetState_;
    
    std::vector<StateTransition> transitions_;
    std::map<std::string, float> adaptationParameters_;
    
    // Transition state
    bool inTransition_ = false;
    float transitionProgress_ = 0.0f;
    float transitionDuration_ = 0.0f;
    
    void evaluateTransitions();
    void processTransition(float deltaTime);
};
```

### Horizontal Re-sequencing

Dynamic pattern modification:

```cpp
class HorizontalResequencer {
public:
    enum class ResequenceAction {
        None,
        DoubleTime,        // Play pattern at 2x speed
        HalfTime,          // Play pattern at 0.5x speed
        Reverse,           // Reverse pattern playback
        Skip,              // Skip certain steps
        Repeat,            // Repeat certain steps
        Shuffle,           // Randomize step order
        Stutter,           // Repeat current step multiple times
        Gate,              // Modify gate lengths
        Velocity,          // Modify velocities
        Custom             // User-defined transformation
    };
    
    struct ResequenceRule {
        std::string condition;           // Condition string (e.g., "energy > 0.8")
        ResequenceAction action;
        float probability = 1.0f;
        float intensity = 1.0f;         // How strong the effect is
        int duration = 1;               // Duration in bars
        std::function<void(Pattern&)> customAction;
    };
    
    HorizontalResequencer(Pattern* pattern);
    
    void addRule(const ResequenceRule& rule);
    void removeRule(const std::string& condition);
    void setEnabled(bool enabled);
    
    // Called each step to potentially modify pattern
    void processStep(int stepIndex, const std::map<std::string, float>& parameters);
    
private:
    Pattern* pattern_;
    std::vector<ResequenceRule> rules_;
    bool enabled_ = false;
    
    // State tracking
    std::map<ResequenceAction, int> activeActions_;
    std::map<ResequenceAction, int> actionCountdowns_;
    
    // Rule evaluation
    bool evaluateCondition(const std::string& condition, 
                          const std::map<std::string, float>& parameters);
    void applyAction(ResequenceAction action, float intensity);
};
```

### Vertical Remixing

Layer-based arrangement system:

```cpp
class VerticalRemixer {
public:
    struct Layer {
        std::string name;
        std::vector<std::string> trackIds;
        float intensity = 1.0f;         // 0-1, controls layer activity
        float fadeTime = 1.0f;          // Fade time in bars
        bool enabled = true;
    };
    
    VerticalRemixer(SequencerEngine* sequencer);
    
    // Layer management
    void createLayer(const std::string& name, const std::vector<std::string>& trackIds);
    void setLayerIntensity(const std::string& name, float intensity);
    void enableLayer(const std::string& name, bool enabled);
    
    // Parameter mapping
    void mapParameterToLayerIntensity(const std::string& parameter, 
                                     const std::string& layer);
    void setParameterValue(const std::string& parameter, float value);
    
    // Arrangement presets
    void saveArrangement(const std::string& name);
    void loadArrangement(const std::string& name);
    void morphToArrangement(const std::string& name, float time);
    
    // Update (called from audio thread)
    void update(float deltaTime);
    
private:
    SequencerEngine* sequencer_;
    
    std::map<std::string, Layer> layers_;
    std::map<std::string, std::string> parameterToLayerMap_;
    std::map<std::string, float> parameters_;
    
    // Arrangement morphing
    bool morphing_ = false;
    std::string targetArrangement_;
    float morphProgress_ = 0.0f;
    float morphDuration_ = 0.0f;
    std::map<std::string, float> startIntensities_;
    std::map<std::string, float> targetIntensities_;
    
    void updateLayerIntensities();
    void applyLayerIntensity(const Layer& layer, float intensity);
};
```

## Parameter Automation

### AutomationTrack

Smooth parameter automation with various curve types:

```cpp
class AutomationTrack {
public:
    enum class CurveType {
        Linear,
        Exponential,
        Logarithmic,
        Sine,
        Cosine,
        Bezier,
        Step,
        Custom
    };
    
    struct AutomationPoint {
        int step;                    // Step position
        float value;                 // Parameter value (0-1)
        CurveType curveType = CurveType::Linear;
        float tension = 0.0f;        // Curve tension (-1 to 1)
        bool locked = false;         // Prevent editing
    };
    
    AutomationTrack(const std::string& parameterId);
    
    // Point management
    void addPoint(int step, float value, CurveType curve = CurveType::Linear);
    void removePoint(int step);
    void movePoint(int fromStep, int toStep);
    void setPointValue(int step, float value);
    
    // Interpolation
    float getValueAtStep(float step) const;
    void setGlobalCurveType(CurveType type);
    void setCustomCurve(std::function<float(float)> curve);
    
    // Modulation
    void setLFOModulation(float frequency, float depth, float phase = 0.0f);
    void setEnvelopeFollower(const std::string& sourceParameter, float sensitivity);
    
private:
    std::string parameterId_;
    std::vector<AutomationPoint> points_;
    CurveType globalCurveType_ = CurveType::Linear;
    std::function<float(float)> customCurve_;
    
    // Modulation
    float lfoFrequency_ = 0.0f;
    float lfoDepth_ = 0.0f;
    float lfoPhase_ = 0.0f;
    
    std::string envelopeFollowerSource_;
    float envelopeFollowerSensitivity_ = 0.0f;
    
    // Interpolation helpers
    float interpolateLinear(float t, float a, float b) const;
    float interpolateExponential(float t, float a, float b) const;
    float interpolateBezier(float t, float a, float b, float tension) const;
};
```

## Performance Optimization

### Sample-Accurate Timing

```cpp
class SampleAccurateSequencer {
public:
    struct ScheduledEvent {
        int64_t sampleTime;          // Absolute sample time
        std::function<void()> action; // Event callback
        int priority = 0;            // Event priority
    };
    
    SampleAccurateSequencer(int sampleRate);
    
    // Event scheduling
    void scheduleEvent(int64_t sampleTime, std::function<void()> action, int priority = 0);
    void scheduleNoteOn(int64_t sampleTime, int note, int velocity, int channel = 1);
    void scheduleNoteOff(int64_t sampleTime, int note, int channel = 1);
    void scheduleParameterChange(int64_t sampleTime, const std::string& param, float value);
    
    // Processing
    void processEvents(int64_t currentSample, int numSamples);
    
private:
    int sampleRate_;
    
    // Event queue (sorted by sample time)
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, 
                       std::function<bool(const ScheduledEvent&, const ScheduledEvent&)>> eventQueue_;
    
    std::mutex eventMutex_;
    
    // High-resolution timing
    std::chrono::high_resolution_clock::time_point startTime_;
    
    void processEventQueue(int64_t startSample, int64_t endSample);
};
```

### Memory Management

```cpp
// Object pooling for real-time safety
template<typename T, size_t PoolSize = 1024>
class SequencerObjectPool {
public:
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!freeObjects_.empty()) {
            T* obj = freeObjects_.back();
            freeObjects_.pop_back();
            return obj;
        }
        return nullptr; // Pool exhausted
    }
    
    void release(T* obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        freeObjects_.push_back(obj);
    }
    
private:
    std::array<T, PoolSize> pool_;
    std::vector<T*> freeObjects_;
    std::mutex mutex_;
};

// Pre-allocated event pools
static SequencerObjectPool<MIDIEvent> midiEventPool;
static SequencerObjectPool<ParameterEvent> parameterEventPool;
```

### CPU Optimization

```cpp
class SequencerPerformanceManager {
public:
    struct PerformanceConfig {
        int maxActivePatterns = 16;
        int maxEventsPerBuffer = 256;
        bool enableLazyEvaluation = true;
        bool enableEventBatching = true;
        float cpuUsageThreshold = 0.8f;
    };
    
    SequencerPerformanceManager(const PerformanceConfig& config);
    
    // Performance monitoring
    struct PerformanceStats {
        float cpuUsage;              // Percentage
        float memoryUsage;           // MB
        float averageLatency;        // ms
        int activePatterns;
        int eventsPerSecond;
    };
    
    PerformanceStats getStats() const;
    
    // Adaptive quality
    void enableAdaptiveQuality(bool enable);
    void setCPUThreshold(float threshold);
    
private:
    PerformanceConfig config_;
    mutable PerformanceStats stats_;
    
    // Timing measurements
    std::chrono::high_resolution_clock::time_point lastMeasurement_;
    std::vector<float> cpuUsageHistory_;
    
    // Adaptive quality
    bool adaptiveQuality_ = false;
    float cpuThreshold_ = 0.8f;
    
    void measurePerformance();
    void adjustQuality();
};
```

## Testing Framework

### Pattern Testing

```cpp
class PatternTester {
public:
    static bool validatePattern(const Pattern& pattern) {
        // Test pattern consistency
        if (pattern.getLength() <= 0 || pattern.getLength() > 64) {
            return false;
        }
        
        // Test automation tracks
        for (const auto& track : pattern.getAutomationTracks()) {
            if (!validateAutomationTrack(*track.second)) {
                return false;
            }
        }
        
        return true;
    }
    
    static void runPerformanceTest(SequencerEngine& sequencer, int durationSeconds) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        while (std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::high_resolution_clock::now() - startTime).count() < durationSeconds) {
            
            sequencer.processAudio(nullptr, 0, 256);
            std::this_thread::sleep_for(std::chrono::microseconds(5805)); // ~256 samples at 44.1kHz
        }
        
        auto stats = sequencer.getPerformanceStats();
        std::cout << "Performance test results:\n";
        std::cout << "CPU usage: " << stats.cpuUsage << "%\n";
        std::cout << "Average latency: " << stats.averageLatency << " ms\n";
    }
};
```

This technical specification provides the complete implementation details for building a professional-grade sequencer system with adaptive capabilities.