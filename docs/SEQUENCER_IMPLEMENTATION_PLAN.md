# Sequencer Implementation Plan

## Overview

This document provides a detailed implementation plan for enhancing the sequencer system in the AIMusicHardware project. Based on analysis of best-in-class sequencer systems from Vital, Arturia KeyStep Pro, Elektron Octatrack, and Analog Rhythm, this plan outlines the technical approach, code structure, and timeline for implementation.

## Architecture

The enhanced sequencer will use a modular architecture with these key components:

### Core Components

1. **SequencerEngine**: Central timing and transport management
2. **PatternManager**: Storage and organization of patterns
3. **TrackManager**: Multi-track playback and management
4. **StepData**: Note and trigger information 
5. **ParameterAutomation**: Parameter sequencing system
6. **LineGenerator**: Smooth curve generation (inspired by Vital)
7. **PerformanceControls**: Real-time control interface

## Detailed Implementation

### 1. SequencerEngine

The SequencerEngine handles all timing, playback position tracking, and transport operations.

```cpp
// SequencerEngine.h
class SequencerEngine {
public:
    enum SyncMode {
        kInternal,
        kMidiClock,
        kExternalClock,
        kHostSync
    };
    
    SequencerEngine(double bpm = 120.0, int ppq = 96);
    ~SequencerEngine();
    
    // Transport controls
    void start();
    void stop();
    void pause();
    void continuePlaying();
    void reset();
    
    // Position management
    void setPositionInBeats(double beats);
    double getPositionInBeats() const;
    void setLoopPoints(double startBeat, double endBeat);
    
    // Clock management
    void setSyncMode(SyncMode mode);
    void setTempo(double bpm);
    double getTempo() const;
    void setSwing(float amount, int division = 8);
    
    // Timing processing
    void processMidiClock(int clockCount);
    void processAudioCallback(double sampleTime, int numSamples);
    
    // Callbacks
    using PositionCallback = std::function<void(double positionInBeats, int bar, int beat, int tick)>;
    void setPositionCallback(PositionCallback callback);
    
private:
    // Clock state
    std::atomic<double> tempo_;
    std::atomic<bool> isPlaying_;
    std::atomic<bool> isPaused_;
    SyncMode syncMode_;
    
    // Position tracking
    double positionInBeats_;
    double loopStartBeat_;
    double loopEndBeat_;
    bool looping_;
    
    // PPQ (Pulses Per Quarter note) - timing resolution
    int ppq_;
    
    // Swing parameters
    float swingAmount_;
    int swingDivision_;
    
    // Timing helpers
    double lastSampleTime_;
    
    // Thread safety
    mutable std::mutex timingMutex_;
    
    // Callbacks
    PositionCallback positionCallback_;
};
```

### 2. Pattern and Step Data Structures

These structures define the core data model for sequences.

```cpp
// StepData.h
struct StepData {
    // Basic note properties
    bool active = false;
    int pitch = 60;  // MIDI note number
    float velocity = 1.0f;
    float duration = 0.9f;  // As fraction of step length
    
    // Advanced properties
    float probability = 1.0f;  // Trigger probability
    bool slide = false;  // Slide/portamento to next note
    bool accent = false;  // Accent flag
    
    // Conditional triggers
    int triggerCondition = 0;  // 0=always, 1=odd, 2=even, etc.
    int triggerCount = 1;  // How many times this step repeats (ratcheting)
    
    // Parameter locks (Elektron-style)
    std::map<int, float> parameterLocks;
};

// Pattern.h
class Pattern {
public:
    Pattern(const std::string& name = "New Pattern", int length = 16);
    ~Pattern();
    
    // Basic pattern properties
    void setName(const std::string& name);
    std::string getName() const;
    void setLength(int steps);
    int getLength() const;
    
    // Step access and manipulation
    StepData* getStep(int track, int step);
    void setStep(int track, int step, const StepData& data);
    void clearStep(int track, int step);
    void clearTrack(int track);
    void clearAll();
    
    // Track management
    int getNumTracks() const;
    void setNumTracks(int tracks);
    
    // Pattern operations
    void copy(const Pattern& source);
    void paste(int destTrack, int destStep, const Pattern& source, 
               int sourceTrack, int sourceStep, int numSteps);
    
    // Probability and conditions
    void randomize(int track, float amount);
    
    // Direction and playback modes
    enum class Direction {
        Forward,
        Reverse,
        PingPong,
        Random
    };
    
    void setDirection(Direction dir);
    Direction getDirection() const;
    
    // Serialization
    json saveToJson() const;
    void loadFromJson(const json& data);
    
private:
    std::string name_;
    int length_;
    std::vector<std::vector<StepData>> tracks_;
    Direction direction_;
    
    // Pattern timing properties
    float swingAmount_;
    int timeSignatureNumerator_;
    int timeSignatureDenominator_;
};
```

### 3. Advanced Parameter Sequencing

Implements Vital-style curve generation for smooth parameter modulation.

```cpp
// LineGenerator.h (Inspired by Vital)
class LineGenerator {
public:
    static constexpr int kMaxPoints = 64;
    static constexpr int kDefaultResolution = 2048;
    
    LineGenerator(int resolution = kDefaultResolution);
    ~LineGenerator();
    
    // Standard waveform templates
    void initLinear();
    void initTriangle();
    void initSquare();
    void initSaw();
    void initRandom();
    
    // Point editing
    void addPoint(int index, Point position);
    void removePoint(int index);
    void setPoint(int index, Point position);
    Point getPoint(int index) const;
    int getNumPoints() const;
    
    // Curve properties
    void setSmooth(bool smooth);
    void setLoop(bool loop);
    void setPower(int index, float power);
    float getPower(int index) const;
    
    // Value calculation
    float getValueAtPhase(float phase) const;
    
    // Buffer access for fast rendering
    const float* getBuffer() const;
    void render();
    
    // Serialization
    json saveToJson() const;
    void loadFromJson(const json& data);
    
private:
    std::string name_;
    std::vector<Point> points_;
    std::vector<float> powers_;
    int numPoints_;
    bool smooth_;
    bool loop_;
    
    // Cached rendering
    std::unique_ptr<float[]> buffer_;
    int resolution_;
    bool needsUpdate_;
    
    // Helper methods
    float getValueBetweenPoints(float x, int fromIndex, int toIndex) const;
    void checkIsLinear();
};

// ParameterSequence.h
class ParameterSequence {
public:
    enum InterpolationType {
        kStep,      // No interpolation between steps
        kLinear,    // Linear interpolation
        kCurve      // Curve-based (using LineGenerator)
    };
    
    ParameterSequence(int length = 16);
    ~ParameterSequence();
    
    // Basic properties
    void setLength(int length);
    int getLength() const;
    
    // Step values
    void setValue(int step, float value);
    float getValue(int step) const;
    
    // Continuous curve mode (Vital-style)
    void setInterpolationType(InterpolationType type);
    void setLineGenerator(std::shared_ptr<LineGenerator> generator);
    std::shared_ptr<LineGenerator> getLineGenerator();
    
    // Value calculation for playback
    float getValueAtPosition(float position) const;
    
    // Utility
    void clear();
    void randomize(float amount);
    
private:
    std::vector<float> stepValues_;
    std::shared_ptr<LineGenerator> lineGenerator_;
    InterpolationType interpolationType_;
    int length_;
};
```

### 4. Advanced Track System

Multi-track system supporting different track types for diverse sequencing needs.

```cpp
// Track.h
class Track {
public:
    enum TrackType {
        kMelodic,   // Standard note sequence
        kDrum,      // Percussion triggers
        kControl    // Pure parameter control (no notes)
    };
    
    Track(TrackType type = TrackType::kMelodic);
    virtual ~Track();
    
    // Track properties
    void setName(const std::string& name);
    std::string getName() const;
    void setType(TrackType type);
    TrackType getType() const;
    
    // Channel routing
    void setMidiChannel(int channel);
    int getMidiChannel() const;
    void setOutputDevice(int device);
    
    // Pattern access
    Pattern* getPattern(int index);
    void setCurrentPattern(int index);
    int getCurrentPatternIndex() const;
    
    // Parameter sequences
    void addParameterSequence(int parameterId, std::shared_ptr<ParameterSequence> sequence);
    std::shared_ptr<ParameterSequence> getParameterSequence(int parameterId);
    
    // Playback
    void setMuted(bool muted);
    bool isMuted() const;
    void setSolo(bool solo);
    bool isSolo() const;
    
    // Track-specific timing
    void setClockDivision(int division);  // e.g., 1=normal, 2=half-speed, 4=quarter-speed
    int getClockDivision() const;
    void setSwingAmount(float amount);
    float getSwingAmount() const;
    
    // Processing
    void process(double currentPosition, double deltaBeats);
    
    // Callbacks
    using NoteCallback = std::function<void(int pitch, float velocity, float duration, int channel)>;
    void setNoteCallback(NoteCallback callback);
    
private:
    std::string name_;
    TrackType type_;
    int midiChannel_;
    int outputDevice_;
    
    std::vector<std::unique_ptr<Pattern>> patterns_;
    int currentPatternIndex_;
    
    std::map<int, std::shared_ptr<ParameterSequence>> parameterSequences_;
    
    bool muted_;
    bool solo_;
    
    int clockDivision_;
    float swingAmount_;
    
    // Playback state
    double trackPosition_;
    std::vector<NoteEvent> activeNotes_;
    
    // Callbacks
    NoteCallback noteCallback_;
};
```

### 5. Performance and Scene Management

System for storing and recalling different states and performance setups.

```cpp
// Scene.h
class Scene {
public:
    Scene(const std::string& name = "New Scene");
    ~Scene();
    
    // Scene properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Pattern assignments
    void setPatternForTrack(int trackIndex, int patternIndex);
    int getPatternForTrack(int trackIndex) const;
    
    // Track states
    void setTrackMuted(int trackIndex, bool muted);
    bool isTrackMuted(int trackIndex) const;
    
    // Parameter snapshots
    void setParameterValue(int parameterId, float value);
    float getParameterValue(int parameterId) const;
    
    // Scene management
    void store(const SequencerEngine& engine);
    void recall(SequencerEngine& engine);
    
    // Serialization
    json saveToJson() const;
    void loadFromJson(const json& data);
    
private:
    std::string name_;
    std::map<int, int> trackPatterns_;  // Track index -> Pattern index
    std::map<int, bool> trackMutes_;    // Track index -> Mute state
    std::map<int, float> parameterValues_;  // Parameter ID -> Value
};
```

### 6. Top-Level Sequencer Class

Main interface that integrates all components.

```cpp
// AdvancedSequencer.h
class AdvancedSequencer {
public:
    AdvancedSequencer();
    ~AdvancedSequencer();
    
    bool initialize();
    void shutdown();
    
    // Engine access
    SequencerEngine* getEngine() { return engine_.get(); }
    
    // Pattern and track management
    int createPattern(const std::string& name = "New Pattern", int length = 16);
    void deletePattern(int patternIndex);
    Pattern* getPattern(int patternIndex);
    
    int createTrack(Track::TrackType type = Track::TrackType::kMelodic);
    void deleteTrack(int trackIndex);
    Track* getTrack(int trackIndex);
    
    // Parameter control
    void registerParameter(int parameterId, const std::string& name, 
                           float defaultValue, float minValue, float maxValue);
    void setParameterValue(int parameterId, float value);
    float getParameterValue(int parameterId) const;
    
    // Scene management
    int createScene(const std::string& name = "New Scene");
    void deleteScene(int sceneIndex);
    Scene* getScene(int sceneIndex);
    void recallScene(int sceneIndex);
    
    // Timeline and song arrangement
    void addPatternToTimeline(int patternIndex, double startBeat);
    void clearTimeline();
    
    // Pattern playback
    void playPattern(int patternIndex);
    void stopPlayback();
    
    // Project management
    bool saveToFile(const std::string& filename);
    bool loadFromFile(const std::string& filename);
    
    // Audio callback (for DAW integration)
    void processAudio(float* buffer, int numFrames, int numChannels, double sampleRate);
    
private:
    // Core components
    std::unique_ptr<SequencerEngine> engine_;
    std::vector<std::unique_ptr<Pattern>> patterns_;
    std::vector<std::unique_ptr<Track>> tracks_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    
    // Parameter management
    struct ParameterInfo {
        std::string name;
        float value;
        float defaultValue;
        float minValue;
        float maxValue;
    };
    std::map<int, ParameterInfo> parameters_;
    
    // Timeline arrangement
    struct TimelineEntry {
        int patternIndex;
        double startBeat;
    };
    std::vector<TimelineEntry> timeline_;
    
    // Audio engine integration
    std::vector<NoteEvent> pendingNotes_;
    std::mutex notesMutex_;
    
    // Processing callbacks
    void onEnginePositionChanged(double positionInBeats, int bar, int beat, int tick);
    void onTrackNoteTriggered(int trackIndex, int pitch, float velocity, float duration, int channel);
};
```

## Implementation Timeline

### Phase 1: Core Architecture (Weeks 1-2)
- Implement SequencerEngine with accurate timing
- Build basic Pattern and StepData classes
- Create fundamental Track system
- Design parameter registration system
- Implement basic MIDI note generation

#### Key Deliverables:
- Basic step sequencer with accurate timing
- Multi-pattern storage system
- Note entry and playback functionality

### Phase 2: Advanced Step Features (Weeks 3-4)
- Implement conditional triggers and probability
- Add multi-note polyphonic steps
- Create ratcheting/retriggering system
- Implement pattern chaining
- Add playback direction controls

#### Key Deliverables:
- Conditional trigger system
- Ratcheting and micro-timing
- Multiple playback directions
- Advanced step properties

### Phase 3: Parameter Sequencing (Weeks 5-6)
- Implement LineGenerator (Vital-style)
- Build parameter sequencing system
- Create parameter locks for steps
- Implement modulation routing

#### Key Deliverables:
- Parameter automation system
- Curve-based modulation
- Step-based parameter locks
- Smooth interpolation options

### Phase 4: Advanced Features (Weeks 7-8)
- Implement scene management
- Create song arrangement timeline
- Add polyrhythmic capabilities
- Implement MIDI I/O system
- Build randomization tools

#### Key Deliverables:
- Performance scene system
- Multi-track timeline
- External synchronization options
- Randomization algorithms

### Phase 5: UI Integration and Testing (Weeks 9-10)
- Design UI for pattern editing
- Create visual pattern and parameter displays
- Build performance controls
- Implement preset management

#### Key Deliverables:
- Grid editor interface
- Parameter visualization
- Performance UI controls
- Preset storage system

## Integration Points

### Audio Engine Integration
```cpp
void AdvancedSequencer::processAudio(float* buffer, int numFrames, int numChannels, double sampleRate) {
    // Process timing
    double sampleTime = 1.0 / sampleRate * numFrames;
    engine_->processAudioCallback(sampleTime, numFrames);
    
    // Process notes
    std::lock_guard<std::mutex> lock(notesMutex_);
    if (!pendingNotes_.empty()) {
        // Pass notes to audio engine
        for (const auto& note : pendingNotes_) {
            // Trigger note in audio engine
            // This depends on your audio engine implementation
        }
        pendingNotes_.clear();
    }
    
    // Process parameter modulation
    for (const auto& param : parameters_) {
        if (param.second.modulated) {
            // Apply parameter modulation to audio engine parameters
        }
    }
}
```

### MIDI Integration
```cpp
void AdvancedSequencer::onTrackNoteTriggered(int trackIndex, int pitch, float velocity, float duration, int channel) {
    // Add note to pending notes for audio engine
    {
        std::lock_guard<std::mutex> lock(notesMutex_);
        NoteEvent event;
        event.pitch = pitch;
        event.velocity = velocity;
        event.duration = duration;
        event.channel = channel;
        pendingNotes_.push_back(event);
    }
    
    // Send MIDI message if enabled
    if (midiOutputEnabled_) {
        // Send MIDI note on message
        // This depends on your MIDI implementation
    }
}
```

## Performance Considerations

1. **Thread Safety**: All timing-critical operations use proper mutex locking and atomic variables
2. **Memory Management**: Smart pointers for proper resource handling
3. **Processing Efficiency**: High-performance buffer calculations and vectorization
4. **CPU Usage**: Optimized rendering paths for LineGenerator
5. **Real-time Audio**: Lock-free pathways for critical audio operations

## Extensibility

This architecture is designed to be easily extended in several key areas:

1. **Additional Track Types**: New track types can be derived from the base Track class
2. **Custom Step Behaviors**: The StepData structure can be extended with new properties
3. **New Modulation Sources**: Additional generator types can be implemented
4. **Algorithm Extensions**: The pattern processing system allows for algorithmic composition extensions
5. **Custom UI Components**: Each component has clear data models for UI representation

## Conclusion

This implementation plan provides a detailed roadmap for developing a professional-grade sequencer system with features inspired by the best hardware and software sequencers on the market. The modular architecture allows for incremental development and testing, with a focus on performance, extensibility, and creative expression.

By following this plan, the AIMusicHardware project will gain a powerful sequencing engine capable of advanced parameter automation, complex pattern management, and professional-level music production capabilities.