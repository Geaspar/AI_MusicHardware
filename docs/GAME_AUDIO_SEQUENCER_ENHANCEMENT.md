# Game Audio Middleware Concepts for Sequencer Enhancement

## Overview

This document outlines a plan to augment the AIMusicHardware sequencer with concepts from game audio middleware (like FMOD and Wwise), creating a powerful adaptive music system. By incorporating these concepts, the sequencer will gain the ability to create dynamic, responsive musical experiences that adapt to user input and contextual parameters.

## Key Game Audio Middleware Concepts to Implement

### 1. State-Based Music System

Game audio middleware uses state machines to control the flow of musical content. Implementing a state system will allow the sequencer to seamlessly transition between different musical contexts.

#### Core Components:
- **Musical States**: Discrete musical sections defined by patterns, parameter settings, and mix configurations
- **State Transitions**: Rules governing how the system moves between states
- **State Graph**: Visual representation of all states and their connections for intuitive editing

#### Implementation Details:
```cpp
// State-based system
class MusicState {
public:
    MusicState(const std::string& name);
    ~MusicState();
    
    // State properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Content management
    void addPattern(int trackIndex, std::shared_ptr<Pattern> pattern);
    void setTempo(float tempo);
    void setTimeSignature(int numerator, int denominator);
    
    // Parameter settings
    void setParameterValue(const std::string& paramName, float value);
    float getParameterValue(const std::string& paramName) const;
    
    // Track settings
    void setTrackSettings(int trackIndex, const TrackSettings& settings);
    
    // Mix settings (volumes, mutes, panning)
    void setMixSnapshot(std::shared_ptr<MixSnapshot> snapshot);
    std::shared_ptr<MixSnapshot> getMixSnapshot() const;
    
    // State activation/deactivation
    void activate();
    void deactivate();
    
private:
    std::string name_;
    std::map<int, std::shared_ptr<Pattern>> trackPatterns_;
    std::map<std::string, float> parameterValues_;
    std::shared_ptr<MixSnapshot> mixSnapshot_;
    float tempo_;
    int timeSigNumerator_;
    int timeSigDenominator_;
    
    // State callbacks
    std::function<void()> onActivateCallback_;
    std::function<void()> onDeactivateCallback_;
};

class StateTransition {
public:
    enum TransitionType {
        kImmediate,      // Instant switch
        kNextBar,        // Wait until next bar
        kNextBeat,       // Wait until next beat
        kCrossfade,      // Gradual volume change
        kParametricBlend // Parameter-driven blend
    };
    
    StateTransition(MusicState* fromState, MusicState* toState);
    
    // Transition properties
    void setType(TransitionType type);
    void setDuration(float durationInBeats);
    
    // Transition conditions
    void addParameterCondition(const std::string& paramName, 
                              float threshold, bool greaterThan = true);
    void setTriggerEvent(const std::string& eventName);
    
    // Transition execution
    bool checkConditions() const;
    void execute();
    bool isComplete() const;
    float getProgress() const;
    
private:
    MusicState* fromState_;
    MusicState* toState_;
    TransitionType type_;
    float durationInBeats_;
    float progress_;
    bool active_;
    
    std::map<std::string, std::pair<float, bool>> parameterConditions_;
    std::string triggerEvent_;
};

class StateManager {
public:
    StateManager();
    ~StateManager();
    
    // State management
    void addState(std::shared_ptr<MusicState> state);
    void removeState(const std::string& stateName);
    std::shared_ptr<MusicState> getState(const std::string& stateName);
    
    // Transition management
    void addTransition(std::shared_ptr<StateTransition> transition);
    void removeTransition(int transitionId);
    
    // State execution
    void setActiveState(const std::string& stateName);
    std::shared_ptr<MusicState> getActiveState() const;
    
    // Update state system
    void update(float deltaTime);
    
private:
    std::map<std::string, std::shared_ptr<MusicState>> states_;
    std::vector<std::shared_ptr<StateTransition>> transitions_;
    std::shared_ptr<MusicState> activeState_;
    std::shared_ptr<StateTransition> activeTransition_;
};
```

### 2. Vertical Remix System (Layered Mixing)

Vertical remixing is a technique where different layers of a musical piece are mixed dynamically based on parameters. This allows for gradual changes in intensity, mood, or instrumentation without changing the core musical progression.

#### Core Components:
- **Track Layers**: Multiple stems/tracks that can be independently controlled
- **Volume Automation**: Parameter-driven control of layer volumes
- **Layer Groups**: Logical grouping of related layers for easier management
- **Mix Snapshots**: Predefined layer configurations that can be smoothly transitioned

#### Implementation Details:
```cpp
// Vertical remixing system
class LayerGroup {
public:
    LayerGroup(const std::string& name);
    
    // Layer management
    void addLayer(int trackIndex);
    void removeLayer(int trackIndex);
    
    // Group properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Volume control
    void setGroupVolume(float volume);
    float getGroupVolume() const;
    
    // Mute/solo
    void setMuted(bool muted);
    bool isMuted() const;
    
private:
    std::string name_;
    std::vector<int> trackIndices_;
    float volume_;
    bool muted_;
};

class MixSnapshot {
public:
    MixSnapshot(const std::string& name);
    
    // Snapshot properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Track settings
    void setTrackVolume(int trackIndex, float volume);
    float getTrackVolume(int trackIndex) const;
    void setTrackMuted(int trackIndex, bool muted);
    bool isTrackMuted(int trackIndex) const;
    
    // Group settings
    void setGroupVolume(const std::string& groupName, float volume);
    void setGroupMuted(const std::string& groupName, bool muted);
    
    // Snapshot transition
    void blendWith(const MixSnapshot& target, float blendFactor);
    
private:
    std::string name_;
    std::map<int, float> trackVolumes_;
    std::map<int, bool> trackMutes_;
    std::map<std::string, float> groupVolumes_;
    std::map<std::string, bool> groupMutes_;
};

class MixManager {
public:
    MixManager();
    
    // Snapshot management
    void addSnapshot(std::shared_ptr<MixSnapshot> snapshot);
    void removeSnapshot(const std::string& name);
    std::shared_ptr<MixSnapshot> getSnapshot(const std::string& name) const;
    
    // Layer group management
    void addLayerGroup(std::shared_ptr<LayerGroup> group);
    void removeLayerGroup(const std::string& name);
    
    // Transitioning
    void transitionToSnapshot(const std::string& name, float timeInSeconds);
    void updateTransition(float deltaTime);
    
    // Parameter-driven mixing
    void setParameterValue(const std::string& paramName, float value);
    void mapParameterToTrackVolume(const std::string& paramName, int trackIndex, 
                                  float minValue, float maxValue, 
                                  float minVolume, float maxVolume);
    
private:
    std::map<std::string, std::shared_ptr<MixSnapshot>> snapshots_;
    std::map<std::string, std::shared_ptr<LayerGroup>> layerGroups_;
    std::shared_ptr<MixSnapshot> activeSnapshot_;
    std::shared_ptr<MixSnapshot> targetSnapshot_;
    float transitionProgress_;
    float transitionDuration_;
    
    // Parameter mapping
    struct VolumeMapping {
        std::string paramName;
        int trackIndex;
        float minValue, maxValue;
        float minVolume, maxVolume;
    };
    std::vector<VolumeMapping> volumeMappings_;
};
```

### 3. Horizontal Re-sequencing

Horizontal re-sequencing allows for dynamic reordering of musical sections based on game events or parameters. This technique enables music to follow narrative or gameplay progression without jarring transitions.

#### Core Components:
- **Segment System**: Musical content divided into discrete segments that can be rearranged
- **Transition Rules**: Conditions for when and how to move between segments
- **Segment Queue**: Dynamic scheduling of upcoming segments
- **Smart Transitions**: Musically appropriate connections between segments

#### Implementation Details:
```cpp
// Horizontal re-sequencing system
class MusicSegment {
public:
    MusicSegment(const std::string& name, double lengthInBeats);
    
    // Segment properties
    void setName(const std::string& name);
    std::string getName() const;
    double getLengthInBeats() const;
    
    // Content management
    void setPattern(int trackIndex, std::shared_ptr<Pattern> pattern);
    std::shared_ptr<Pattern> getPattern(int trackIndex) const;
    
    // Musical properties
    void setKey(int key); // 0 = C, 1 = C#, etc.
    int getKey() const;
    void setMode(int mode); // 0 = major, 1 = minor, etc.
    int getMode() const;
    
    // Playback control
    void setIntensity(float intensity); // 0.0 - 1.0
    float getIntensity() const;
    
    // Transition points
    void addExitPoint(double beatPosition, const std::string& name);
    std::vector<std::pair<double, std::string>> getExitPoints() const;
    
private:
    std::string name_;
    double lengthInBeats_;
    int key_;
    int mode_;
    float intensity_;
    std::map<int, std::shared_ptr<Pattern>> trackPatterns_;
    std::vector<std::pair<double, std::string>> exitPoints_;
};

class SegmentTransition {
public:
    enum TransitionType {
        kCut,            // Immediate switch
        kFade,           // Simple crossfade
        kMatchBeat,      // Wait for beat alignment
        kContinuousBar,  // Wait for bar completion
        kExitPoint,      // Use defined exit point
        kHarmonic        // Match harmonically compatible points
    };
    
    SegmentTransition(const std::string& fromSegment, const std::string& toSegment);
    
    // Transition properties
    void setType(TransitionType type);
    TransitionType getType() const;
    
    // Transition timing
    void setExitPoint(const std::string& exitPointName);
    void setTransitionBars(int bars);
    
    // Conditions
    void addCondition(const std::string& paramName, float threshold, bool greaterThan = true);
    
private:
    std::string fromSegment_;
    std::string toSegment_;
    TransitionType type_;
    std::string exitPoint_;
    int transitionBars_;
    std::map<std::string, std::pair<float, bool>> conditions_;
};

class SegmentSequencer {
public:
    SegmentSequencer();
    
    // Segment management
    void addSegment(std::shared_ptr<MusicSegment> segment);
    void removeSegment(const std::string& name);
    std::shared_ptr<MusicSegment> getSegment(const std::string& name) const;
    
    // Transition management
    void addTransition(std::shared_ptr<SegmentTransition> transition);
    
    // Playback control
    void play(const std::string& segmentName);
    void queueSegment(const std::string& segmentName);
    void clearQueue();
    
    // Dynamic sequencing
    void update(float deltaTime);
    
private:
    std::map<std::string, std::shared_ptr<MusicSegment>> segments_;
    std::vector<std::shared_ptr<SegmentTransition>> transitions_;
    std::shared_ptr<MusicSegment> activeSegment_;
    std::vector<std::string> segmentQueue_;
    double segmentPosition_;
    bool transitioning_;
};
```

### 4. Parameter System

Game audio middleware relies heavily on parameter-driven control. A robust parameter system allows for smooth modulation of various aspects of the music system based on external inputs.

#### Core Components:
- **Global Parameters**: System-wide variables that affect musical behavior
- **Parameter Mapping**: Connections between parameters and musical properties
- **Parameter Automation**: Time or event-based parameter changes
- **RTPC (Real-Time Parameter Control)**: Dynamic parameter adjustment based on input

#### Implementation Details:
```cpp
// Parameter system
class Parameter {
public:
    Parameter(const std::string& name, float defaultValue, 
              float minValue = 0.0f, float maxValue = 1.0f);
    
    // Parameter properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Value management
    void setValue(float value);
    float getValue() const;
    void setRange(float minValue, float maxValue);
    
    // Advanced properties
    void setSmoothingTime(float timeInSeconds);
    void setBipolar(bool bipolar);
    void setDiscrete(bool discrete, int numSteps = 0);
    
    // Update smoothing
    void update(float deltaTime);
    
    // Callbacks for parameter changes
    using ChangeCallback = std::function<void(const std::string&, float, float)>;
    void setChangeCallback(ChangeCallback callback);
    
private:
    std::string name_;
    float value_;
    float targetValue_;
    float defaultValue_;
    float minValue_;
    float maxValue_;
    float smoothingTime_;
    float smoothingRate_;
    bool bipolar_;
    bool discrete_;
    int numSteps_;
    
    ChangeCallback changeCallback_;
    mutable std::mutex mutex_;
};

class ParameterManager {
public:
    ParameterManager();
    
    // Parameter management
    void addParameter(std::shared_ptr<Parameter> parameter);
    void removeParameter(const std::string& name);
    std::shared_ptr<Parameter> getParameter(const std::string& name) const;
    
    // Value control
    void setParameterValue(const std::string& name, float value);
    float getParameterValue(const std::string& name) const;
    
    // Parameter mapping
    void mapParameter(const std::string& paramName, 
                     std::function<void(float)> targetFunction,
                     float sourceMin = 0.0f, float sourceMax = 1.0f,
                     float targetMin = 0.0f, float targetMax = 1.0f);
    
    // Parameter modifiers
    void addParameterModifier(const std::string& paramName,
                             std::function<float(float)> modifierFunction);
    
    // Update all parameters
    void update(float deltaTime);
    
private:
    std::map<std::string, std::shared_ptr<Parameter>> parameters_;
    
    struct ParameterMapping {
        std::string paramName;
        std::function<void(float)> targetFunction;
        float sourceMin, sourceMax;
        float targetMin, targetMax;
    };
    std::vector<ParameterMapping> parameterMappings_;
    
    struct ParameterModifier {
        std::string paramName;
        std::function<float(float)> modifierFunction;
    };
    std::vector<ParameterModifier> parameterModifiers_;
};
```

### 5. Event System

Events are the triggers that drive state changes, parameter adjustments, and other musical behaviors in game audio middleware. An event system allows for time-based or input-driven control of the music system.

#### Core Components:
- **Event Registry**: Central catalog of available events
- **Event Triggers**: Conditions that cause events to fire
- **Event Listeners**: Components that respond to specific events
- **Event Queue**: System for scheduling future events

#### Implementation Details:
```cpp
// Event system
class Event {
public:
    Event(const std::string& name);
    
    // Event properties
    void setName(const std::string& name);
    std::string getName() const;
    
    // Event data
    void setData(const std::string& key, float value);
    float getData(const std::string& key) const;
    
    // Event timing
    void setTimestamp(double timeInSeconds);
    double getTimestamp() const;
    
private:
    std::string name_;
    std::map<std::string, float> data_;
    double timestamp_;
};

class EventSystem {
public:
    EventSystem();
    
    // Event registration
    void registerEvent(const std::string& eventName);
    void unregisterEvent(const std::string& eventName);
    
    // Event triggering
    void triggerEvent(const std::string& eventName);
    void triggerEvent(const std::string& eventName, 
                     const std::map<std::string, float>& data);
    
    // Event listening
    using EventCallback = std::function<void(const Event&)>;
    void addEventListener(const std::string& eventName, EventCallback callback);
    void removeEventListener(const std::string& eventName, void* owner);
    
    // Scheduled events
    void scheduleEvent(const std::string& eventName, double timeInSeconds);
    void scheduleEvent(const std::string& eventName, double timeInSeconds,
                      const std::map<std::string, float>& data);
    
    // Process scheduled events
    void update(double currentTime);
    
private:
    std::map<std::string, std::vector<EventCallback>> eventListeners_;
    
    struct ScheduledEvent {
        std::string eventName;
        std::map<std::string, float> data;
        double scheduledTime;
    };
    std::vector<ScheduledEvent> scheduledEvents_;
    
    mutable std::mutex mutex_;
};
```

### 6. RTPC (Real-Time Parameter Control)

Real-Time Parameter Control is a key concept from game audio middleware that allows for dynamic, continuous adjustment of musical properties based on gameplay metrics or user input.

#### Core Components:
- **Parameter Curves**: Non-linear mappings from input values to parameter values
- **Multi-Parameter Control**: Complex relationships between multiple inputs and outputs
- **Threshold Triggers**: Parameter-based event triggering
- **Follower Logic**: Parameters that track other parameters or audio signals

#### Implementation Details:
```cpp
// RTPC system
class ParameterCurve {
public:
    enum CurveType {
        kLinear,
        kExponential,
        kLogarithmic,
        kSCurve,
        kCustom
    };
    
    ParameterCurve(CurveType type = kLinear);
    
    // Curve configuration
    void setCurveType(CurveType type);
    void setCustomPoints(const std::vector<std::pair<float, float>>& points);
    
    // Value mapping
    float map(float inputValue);
    
private:
    CurveType type_;
    std::vector<std::pair<float, float>> customPoints_;
    
    // Helper methods for different curve types
    float mapLinear(float input);
    float mapExponential(float input);
    float mapLogarithmic(float input);
    float mapSCurve(float input);
    float mapCustom(float input);
};

class RtpcModule {
public:
    RtpcModule(ParameterManager* parameterManager);
    
    // RTPC mapping configuration
    void addParameterMapping(const std::string& sourceParam, 
                            const std::string& targetParam,
                            std::shared_ptr<ParameterCurve> curve = nullptr);
    
    // Threshold triggers
    void addThresholdTrigger(const std::string& paramName, 
                            float threshold, bool risingEdge,
                            const std::string& eventToTrigger);
    
    // Multi-parameter control
    void addMultiParameterControl(const std::vector<std::string>& sourceParams,
                                 const std::string& targetParam,
                                 std::function<float(const std::vector<float>&)> combiner);
    
    // Follower configuration
    void addFollower(const std::string& sourceParam, 
                    const std::string& targetParam,
                    float attackTime, float releaseTime);
    
    // Update RTPC relationships
    void update(float deltaTime);
    
private:
    ParameterManager* parameterManager_;
    
    struct ParameterMapping {
        std::string sourceParam;
        std::string targetParam;
        std::shared_ptr<ParameterCurve> curve;
    };
    std::vector<ParameterMapping> parameterMappings_;
    
    struct ThresholdTrigger {
        std::string paramName;
        float threshold;
        bool risingEdge;
        std::string eventToTrigger;
        bool lastState;
    };
    std::vector<ThresholdTrigger> thresholdTriggers_;
    
    struct MultiParameterControl {
        std::vector<std::string> sourceParams;
        std::string targetParam;
        std::function<float(const std::vector<float>&)> combinerFunction;
    };
    std::vector<MultiParameterControl> multiParameterControls_;
    
    struct Follower {
        std::string sourceParam;
        std::string targetParam;
        float attackTime;
        float releaseTime;
        float currentValue;
    };
    std::vector<Follower> followers_;
    
    EventSystem* eventSystem_;
};
```

## Integration with Existing Sequencer

To integrate these game audio middleware concepts with the existing sequencer architecture, we'll create adapter components and extend functionality in key areas.

### Integration Strategy

1. **Extend the Sequencer Base Class**:
   - Add support for state management
   - Incorporate event handling
   - Add parameter registration and tracking

2. **Create a Middleware Layer**:
   - Build adapter components that connect sequencer features to middleware concepts
   - Implement state transition logic
   - Add vertical and horizontal remixing capabilities

3. **Enhance the User Interface**:
   - Add state graph visualization
   - Create parameter mapping interfaces
   - Implement event management UI
   - Add mix snapshot controls

### Example Integration Code

```cpp
// Integration with existing sequencer
class AdaptiveSequencer : public AdvancedSequencer {
public:
    AdaptiveSequencer();
    ~AdaptiveSequencer();
    
    // Initialize middleware components
    bool initializeAdaptiveComponents();
    
    // State management
    void addState(std::shared_ptr<MusicState> state);
    void setState(const std::string& stateName);
    
    // Parameter system
    void registerParameter(const std::string& name, float defaultValue, 
                         float minValue = 0.0f, float maxValue = 1.0f);
    void setParameterValue(const std::string& name, float value);
    float getParameterValue(const std::string& name) const;
    
    // Event system
    void registerEvent(const std::string& eventName);
    void triggerEvent(const std::string& eventName);
    
    // Middleware component access
    StateManager* getStateManager() { return stateManager_.get(); }
    ParameterManager* getParameterManager() { return parameterManager_.get(); }
    EventSystem* getEventSystem() { return eventSystem_.get(); }
    MixManager* getMixManager() { return mixManager_.get(); }
    SegmentSequencer* getSegmentSequencer() { return segmentSequencer_.get(); }
    RtpcModule* getRtpcModule() { return rtpcModule_.get(); }
    
    // Update adaptive components
    void update(float deltaTime);
    
private:
    // Middleware components
    std::unique_ptr<StateManager> stateManager_;
    std::unique_ptr<ParameterManager> parameterManager_;
    std::unique_ptr<EventSystem> eventSystem_;
    std::unique_ptr<MixManager> mixManager_;
    std::unique_ptr<SegmentSequencer> segmentSequencer_;
    std::unique_ptr<RtpcModule> rtpcModule_;
    
    // Integration callbacks
    void onStateChanged(const std::string& newState);
    void onParameterChanged(const std::string& paramName, float oldValue, float newValue);
    void onEventTriggered(const Event& event);
};
```

## Implementation Plan

### Phase 1: Core Middleware Systems (2-3 weeks)
- Implement Parameter System
- Create Event System
- Build State Management framework

### Phase 2: Vertical Remixing (2 weeks)
- Implement Layer Group system
- Create Mix Snapshot functionality
- Add parameter-driven layer control

### Phase 3: Horizontal Re-sequencing (2 weeks)
- Build Segment Sequencer
- Implement transition rules
- Create segment scheduling system

### Phase 4: Integration and UI (2-3 weeks)
- Connect middleware components to existing sequencer
- Create visualization interfaces for states and transitions
- Add parameter mapping UI
- Implement event management interface

### Phase 5: Advanced Features and Testing (2 weeks)
- Add RTPC functionality
- Implement complex parameter curves
- Create example templates
- Comprehensive testing and optimization

## Use Cases and Applications

### 1. Interactive Performance
- Use parameter controls to modify song elements in real-time
- Build interactive installations with audience-driven musical shifts
- Create expressive solo instruments with state-based behavior

### 2. Dynamic Composition
- Design music that evolves based on compositional parameters
- Create generative pieces with rules for progression
- Build adaptive backing tracks for improvisation

### 3. Audio-Visual Integration
- Synchronize visual and audio elements through the parameter system
- Create responsive audiovisual installations
- Build soundtrack systems for interactive media

### 4. Educational Applications
- Create interactive lessons that evolve based on student progress
- Develop adaptive practice accompaniment
- Build music theory demonstration tools

### 5. Advanced Sound Design
- Create complex, evolving sound environments
- Design responsive sound installations
- Build interactive sound design tools

## Conclusion

By incorporating game audio middleware concepts into the AIMusicHardware sequencer, we can create a powerful system capable of generating dynamic, adaptive music. This enhancement brings professional game audio capabilities to music production, performance, and interactive media applications.

The implementation plan outlined above provides a structured approach to adding these features incrementally, ensuring that each component is properly tested and integrated with the existing sequencer architecture. The result will be a unique hybrid system that combines the best of traditional sequencing with the adaptive capabilities of game audio middleware.

## References

1. FMOD Studio Documentation: https://www.fmod.com/resources/documentation-studio
2. Wwise SDK Manual: https://www.audiokinetic.com/library/edge/
3. "The Game Audio Handbook" - Oxford Academic
4. "Interactive Game Music for Beginners with FMOD" - Orchestral Music School
5. "Vertical Re-Orchestration in FMOD Studio" - Alessandro Famà