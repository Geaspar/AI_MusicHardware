# Horizontal Re-sequencing Implementation Guide

## Overview

Horizontal re-sequencing is a technique from game audio middleware that allows for dynamic reordering of musical sections based on parameters or events. This document provides a detailed implementation plan for adding this capability to the AIMusicHardware sequencer.

## Core Concepts

### Musical Segments

A segment is a discrete section of music with a defined structure, typically containing multiple patterns across different tracks. Segments can be arranged in sequence, but unlike traditional linear arrangements, they can be reordered or switched dynamically during playback based on logic or input parameters.

### Exit Points & Entry Points

Exit points are specific locations within a segment where transitions to other segments can occur naturally. Similarly, entry points define where a new segment can begin. These points ensure musically coherent transitions.

### Transition Rules

Transition rules define the conditions and methods for moving from one segment to another. Rules can be based on parameters, events, or musical timing constraints.

### Segment Queue

A flexible system for scheduling upcoming segments, allowing for both immediate transitions and planned progressions.

## Implementation Architecture

### 1. MusicSegment Class

This class represents a discrete musical section with its own patterns, properties, and transition points.

```cpp
class MusicSegment {
public:
    MusicSegment(const std::string& name, double lengthInBeats);
    ~MusicSegment();
    
    // Basic properties
    void setName(const std::string& name);
    std::string getName() const;
    void setLength(double lengthInBeats);
    double getLength() const;
    
    // Pattern management
    void setPattern(int trackIndex, std::shared_ptr<Pattern> pattern);
    std::shared_ptr<Pattern> getPattern(int trackIndex) const;
    void clearPatterns();
    
    // Musical properties
    void setKey(int key); // 0=C, 1=C#, etc.
    int getKey() const;
    void setMode(int mode); // 0=major, 1=minor, etc.
    int getMode() const;
    void setTempo(double bpm);
    double getTempo() const;
    
    // Transition points
    void addExitPoint(double beatPosition, const std::string& name);
    void removeExitPoint(const std::string& name);
    std::vector<std::pair<double, std::string>> getExitPoints() const;
    double getExitPointPosition(const std::string& name) const;
    
    void addEntryPoint(double beatPosition, const std::string& name);
    void removeEntryPoint(const std::string& name);
    std::vector<std::pair<double, std::string>> getEntryPoints() const;
    double getEntryPointPosition(const std::string& name) const;
    
    // Tags for filtering and organization
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    std::vector<std::string> getTags() const;
    
    // Serialization
    json toJson() const;
    static std::shared_ptr<MusicSegment> fromJson(const json& data);
    
private:
    std::string name_;
    double lengthInBeats_;
    std::map<int, std::shared_ptr<Pattern>> trackPatterns_;
    int key_ = 0;  // C by default
    int mode_ = 0; // Major by default
    double tempo_ = 120.0;
    
    std::map<std::string, double> exitPoints_;  // name -> beat position
    std::map<std::string, double> entryPoints_; // name -> beat position
    std::vector<std::string> tags_;
};
```

### 2. SegmentTransition Class

This class defines the rules and methods for transitioning between segments.

```cpp
class SegmentTransition {
public:
    enum class TransitionType {
        Immediate,      // Switch immediately
        NextBeat,       // Wait for next beat
        NextBar,        // Wait for next bar
        ExitPoint,      // Wait for specific exit point
        Crossfade,      // Crossfade between segments
        MatchBeat,      // Align beats between segments
        Harmonic        // Find harmonically matching points
    };
    
    SegmentTransition(const std::string& fromSegment, const std::string& toSegment);
    ~SegmentTransition();
    
    // Basic properties
    void setFromSegment(const std::string& segmentName);
    std::string getFromSegment() const;
    void setToSegment(const std::string& segmentName);
    std::string getToSegment() const;
    
    // Transition configuration
    void setType(TransitionType type);
    TransitionType getType() const;
    void setFadeTime(double timeInBeats);
    double getFadeTime() const;
    
    // Exit/entry point specification
    void setExitPoint(const std::string& exitPointName);
    std::string getExitPoint() const;
    void setEntryPoint(const std::string& entryPointName);
    std::string getEntryPoint() const;
    
    // Condition management
    void addParameterCondition(const std::string& paramName, 
                             float threshold,
                             bool greaterThan = true,
                             float hysteresis = 0.0f);
    void removeParameterCondition(const std::string& paramName);
    bool checkParameterConditions(const ParameterManager& paramManager) const;
    
    // Event triggering
    void setTriggerEvent(const std::string& eventName);
    std::string getTriggerEvent() const;
    
    // Priority for resolving conflicts
    void setPriority(int priority);
    int getPriority() const;
    
    // Probability for random selection
    void setProbability(float probability); // 0.0-1.0
    float getProbability() const;
    
    // Serialization
    json toJson() const;
    static std::shared_ptr<SegmentTransition> fromJson(const json& data);
    
private:
    std::string fromSegment_;
    std::string toSegment_;
    TransitionType type_ = TransitionType::Immediate;
    double fadeTime_ = 0.0;
    std::string exitPoint_;
    std::string entryPoint_;
    
    struct ParameterCondition {
        std::string paramName;
        float threshold;
        bool greaterThan;
        float hysteresis;
        bool lastState;  // For hysteresis tracking
    };
    std::vector<ParameterCondition> parameterConditions_;
    
    std::string triggerEvent_;
    int priority_ = 0;
    float probability_ = 1.0f;
};
```

### 3. SegmentSequencer Class

This class manages the playback and sequencing of segments.

```cpp
class SegmentSequencer {
public:
    SegmentSequencer(ParameterManager& paramManager, EventSystem& eventSystem);
    ~SegmentSequencer();
    
    // Segment management
    void addSegment(std::shared_ptr<MusicSegment> segment);
    void removeSegment(const std::string& name);
    std::shared_ptr<MusicSegment> getSegment(const std::string& name);
    std::vector<std::string> getAllSegmentNames() const;
    
    // Transition management
    void addTransition(std::shared_ptr<SegmentTransition> transition);
    void removeTransition(const std::string& fromSegment, const std::string& toSegment);
    std::shared_ptr<SegmentTransition> getTransition(const std::string& fromSegment, 
                                                   const std::string& toSegment);
    
    // Playback control
    void play(const std::string& segmentName, const std::string& entryPoint = "");
    void stop();
    void pause();
    void resume();
    bool isPlaying() const;
    
    // Queue management
    void queueSegment(const std::string& segmentName, const std::string& entryPoint = "");
    void clearQueue();
    std::vector<std::string> getQueuedSegments() const;
    
    // Position information
    double getCurrentPosition() const; // Position in beats within current segment
    double getTotalPosition() const;   // Total position since start
    std::string getCurrentSegmentName() const;
    
    // Event handling
    void onEvent(const std::string& eventName);
    
    // Update method (called from main sequencer loop)
    void update(double deltaTime);
    
    // Callbacks
    using SegmentChangeCallback = std::function<void(const std::string& prevSegment, 
                                                  const std::string& newSegment)>;
    void setSegmentChangeCallback(SegmentChangeCallback callback);
    
    using ExitPointCallback = std::function<void(const std::string& segmentName, 
                                               const std::string& exitPointName)>;
    void setExitPointCallback(ExitPointCallback callback);
    
    // Serialization
    json toJson() const;
    void fromJson(const json& data);
    
private:
    // Available content
    std::map<std::string, std::shared_ptr<MusicSegment>> segments_;
    std::multimap<std::string, std::shared_ptr<SegmentTransition>> transitions_; // fromSegment -> transition
    
    // Playback state
    std::string currentSegment_;
    double positionInBeats_ = 0.0;
    double totalPosition_ = 0.0;
    bool playing_ = false;
    bool transitioning_ = false;
    std::shared_ptr<SegmentTransition> activeTransition_;
    double transitionProgress_ = 0.0;
    
    // Segment queue
    struct QueuedSegment {
        std::string name;
        std::string entryPoint;
    };
    std::vector<QueuedSegment> segmentQueue_;
    
    // External systems
    ParameterManager& parameterManager_;
    EventSystem& eventSystem_;
    
    // Callbacks
    SegmentChangeCallback segmentChangeCallback_;
    ExitPointCallback exitPointCallback_;
    
    // Internal methods
    void checkForTransitions();
    bool checkExitPoints();
    void startTransition(std::shared_ptr<SegmentTransition> transition);
    void completeTransition();
    void processActiveTransition(double deltaTime);
    void selectNextSegment();
};
```

### 4. Integration with Main Sequencer

This implementation would integrate with the existing sequencer architecture.

```cpp
class AdvancedSequencer {
    // ... existing code ...
    
    // Add horizontal re-sequencing components
    std::unique_ptr<SegmentSequencer> segmentSequencer_;
    
    // Add methods to manage segments
    void addSegment(std::shared_ptr<MusicSegment> segment);
    void removeSegment(const std::string& name);
    void playSegment(const std::string& name);
    
    // Integration with existing sequencer
    void processSegmentPlayback(double deltaTime);
    void onSegmentChange(const std::string& prevSegment, const std::string& newSegment);
    
    // ... existing code ...
};
```

## Implementation Details

### Pattern to Segment Conversion

To create segments from existing patterns:

```cpp
std::shared_ptr<MusicSegment> createSegmentFromPattern(const Pattern& pattern, const std::string& name) {
    auto segment = std::make_shared<MusicSegment>(name, pattern.getLength());
    
    // Create a new pattern for each track in the segment
    for (int trackIndex = 0; trackIndex < pattern.getNumTracks(); ++trackIndex) {
        auto patternCopy = std::make_shared<Pattern>(pattern);
        segment->setPattern(trackIndex, patternCopy);
    }
    
    // Add default exit points
    segment->addExitPoint(0.0, "start");
    segment->addExitPoint(pattern.getLength(), "end");
    
    // Add default entry points
    segment->addEntryPoint(0.0, "start");
    
    return segment;
}
```

### Transition Logic

The core logic for checking transitions:

```cpp
void SegmentSequencer::checkForTransitions() {
    if (!playing_ || transitioning_ || currentSegment_.empty())
        return;
    
    // Get current segment
    auto currentSegmentObj = segments_.find(currentSegment_);
    if (currentSegmentObj == segments_.end())
        return;
    
    // Check all transitions from current segment
    auto transitionRange = transitions_.equal_range(currentSegment_);
    std::vector<std::shared_ptr<SegmentTransition>> validTransitions;
    
    for (auto it = transitionRange.first; it != transitionRange.second; ++it) {
        auto transition = it->second;
        
        // Check parameter conditions
        if (!transition->checkParameterConditions(parameterManager_))
            continue;
        
        // Valid transition found
        validTransitions.push_back(transition);
    }
    
    // Sort transitions by priority
    std::sort(validTransitions.begin(), validTransitions.end(),
        [](const std::shared_ptr<SegmentTransition>& a, const std::shared_ptr<SegmentTransition>& b) {
            return a->getPriority() > b->getPriority();
        });
    
    // Apply probability filtering
    std::vector<std::shared_ptr<SegmentTransition>> probabilityFilteredTransitions;
    for (auto& transition : validTransitions) {
        float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        if (random < transition->getProbability()) {
            probabilityFilteredTransitions.push_back(transition);
        }
    }
    
    // If we have any valid transitions, start the highest priority one
    if (!probabilityFilteredTransitions.empty()) {
        startTransition(probabilityFilteredTransitions[0]);
    }
}
```

### Exit Point Checking

This method checks if we've reached an exit point:

```cpp
bool SegmentSequencer::checkExitPoints() {
    if (!playing_ || transitioning_ || currentSegment_.empty())
        return false;
    
    // Get current segment
    auto segmentIt = segments_.find(currentSegment_);
    if (segmentIt == segments_.end())
        return false;
    
    auto segment = segmentIt->second;
    
    // Check all exit points
    auto exitPoints = segment->getExitPoints();
    for (const auto& exitPoint : exitPoints) {
        const std::string& exitPointName = exitPoint.second;
        double exitPointPos = exitPoint.first;
        
        // Check if we're at or just passed this exit point
        double prevPos = positionInBeats_ - 0.01; // Small buffer to account for timing issues
        if (prevPos <= exitPointPos && positionInBeats_ >= exitPointPos) {
            // We hit an exit point
            if (exitPointCallback_) {
                exitPointCallback_(currentSegment_, exitPointName);
            }
            
            // Check for transitions that are waiting for this exit point
            auto transitionRange = transitions_.equal_range(currentSegment_);
            for (auto it = transitionRange.first; it != transitionRange.second; ++it) {
                auto transition = it->second;
                
                if (transition->getType() == SegmentTransition::TransitionType::ExitPoint && 
                    transition->getExitPoint() == exitPointName &&
                    transition->checkParameterConditions(parameterManager_)) {
                    startTransition(transition);
                    return true;
                }
            }
        }
    }
    
    return false;
}
```

### Segment Update Logic

The main update loop for the segment sequencer:

```cpp
void SegmentSequencer::update(double deltaTime) {
    if (!playing_)
        return;
    
    // Get current segment
    auto segmentIt = segments_.find(currentSegment_);
    if (segmentIt == segments_.end()) {
        // No current segment - try to play from queue
        selectNextSegment();
        return;
    }
    
    auto segment = segmentIt->second;
    double tempo = segment->getTempo();
    
    // Calculate beat delta
    double beatsPerSecond = tempo / 60.0;
    double beatDelta = deltaTime * beatsPerSecond;
    
    // Update position
    positionInBeats_ += beatDelta;
    totalPosition_ += beatDelta;
    
    // Check if we've hit an exit point
    checkExitPoints();
    
    // Check for parameter-driven transitions
    checkForTransitions();
    
    // Process active transition if any
    if (transitioning_ && activeTransition_) {
        processActiveTransition(deltaTime);
    }
    
    // Check if we've reached the end of the segment
    if (positionInBeats_ >= segment->getLength()) {
        // If we have a segment in queue, play it
        if (!segmentQueue_.empty()) {
            selectNextSegment();
        } 
        // Otherwise loop or stop
        else {
            positionInBeats_ = 0.0; // Loop back to beginning
        }
    }
}
```

## Practical Usage Examples

### 1. Basic Linear Arrangement

```cpp
// Create segments
auto introSegment = std::make_shared<MusicSegment>("Intro", 16.0);
auto verseSegment = std::make_shared<MusicSegment>("Verse", 32.0);
auto chorusSegment = std::make_shared<MusicSegment>("Chorus", 16.0);
auto outroSegment = std::make_shared<MusicSegment>("Outro", 8.0);

// Set patterns for each segment
// ...

// Add segments to sequencer
sequencer.addSegment(introSegment);
sequencer.addSegment(verseSegment);
sequencer.addSegment(chorusSegment);
sequencer.addSegment(outroSegment);

// Create transitions
auto introToVerseTransition = std::make_shared<SegmentTransition>("Intro", "Verse");
auto verseToChorusTransition = std::make_shared<SegmentTransition>("Verse", "Chorus");
auto chorusToVerseTransition = std::make_shared<SegmentTransition>("Chorus", "Verse");
auto chorusToOutroTransition = std::make_shared<SegmentTransition>("Chorus", "Outro");

// Queue segments for playback
sequencer.play("Intro");
sequencer.queueSegment("Verse");
sequencer.queueSegment("Chorus");
sequencer.queueSegment("Verse");
sequencer.queueSegment("Chorus");
sequencer.queueSegment("Outro");
```

### 2. Parameter-Driven Branching

```cpp
// Create segments
auto ambientSegment = std::make_shared<MusicSegment>("Ambient", 32.0);
auto tensionSegment = std::make_shared<MusicSegment>("Tension", 16.0);
auto actionSegment = std::make_shared<MusicSegment>("Action", 16.0);
auto resolveSegment = std::make_shared<MusicSegment>("Resolve", 16.0);

// Add segments to sequencer
sequencer.addSegment(ambientSegment);
sequencer.addSegment(tensionSegment);
sequencer.addSegment(actionSegment);
sequencer.addSegment(resolveSegment);

// Create parameter-driven transitions
auto ambientToTensionTransition = std::make_shared<SegmentTransition>("Ambient", "Tension");
ambientToTensionTransition->addParameterCondition("tension", 0.5f, true);

auto tensionToActionTransition = std::make_shared<SegmentTransition>("Tension", "Action");
tensionToActionTransition->addParameterCondition("intensity", 0.7f, true);

auto actionToResolveTransition = std::make_shared<SegmentTransition>("Action", "Resolve");
actionToResolveTransition->addParameterCondition("intensity", 0.3f, false);

auto resolveToAmbientTransition = std::make_shared<SegmentTransition>("Resolve", "Ambient");
resolveToAmbientTransition->addParameterCondition("tension", 0.2f, false);

// Add transitions to sequencer
sequencer.addTransition(ambientToTensionTransition);
sequencer.addTransition(tensionToActionTransition);
sequencer.addTransition(actionToResolveTransition);
sequencer.addTransition(resolveToAmbientTransition);

// Start playback
sequencer.play("Ambient");

// Later, in response to game events:
parameterManager.setParameterValue("tension", 0.6f);  // Will trigger ambient → tension
// ... gameplay intensifies
parameterManager.setParameterValue("intensity", 0.8f); // Will trigger tension → action
// ... gameplay calms down
parameterManager.setParameterValue("intensity", 0.2f); // Will trigger action → resolve
```

### 3. Exit Point-Based Transitions

```cpp
// Create segments with custom exit points
auto segmentA = std::make_shared<MusicSegment>("SegmentA", 16.0);
segmentA->addExitPoint(4.0, "phrase1");
segmentA->addExitPoint(8.0, "phrase2");
segmentA->addExitPoint(12.0, "phrase3");
segmentA->addExitPoint(16.0, "end");

auto segmentB = std::make_shared<MusicSegment>("SegmentB", 16.0);
segmentB->addEntryPoint(0.0, "start");
segmentB->addEntryPoint(8.0, "middle");

// Add segments
sequencer.addSegment(segmentA);
sequencer.addSegment(segmentB);

// Create transition that waits for a specific exit point
auto transitionAtPhrase2 = std::make_shared<SegmentTransition>("SegmentA", "SegmentB");
transitionAtPhrase2->setType(SegmentTransition::TransitionType::ExitPoint);
transitionAtPhrase2->setExitPoint("phrase2");
transitionAtPhrase2->setEntryPoint("middle");

// Add transition
sequencer.addTransition(transitionAtPhrase2);

// Start playback
sequencer.play("SegmentA");

// This will automatically transition to SegmentB at the "phrase2" exit point
// and start playing from the "middle" entry point
```

### 4. Event-Triggered Transitions

```cpp
// Create segments
auto mainThemeSegment = std::make_shared<MusicSegment>("MainTheme", 32.0);
auto bossThemeSegment = std::make_shared<MusicSegment>("BossTheme", 24.0);
auto victoryThemeSegment = std::make_shared<MusicSegment>("VictoryTheme", 16.0);

// Add segments
sequencer.addSegment(mainThemeSegment);
sequencer.addSegment(bossThemeSegment);
sequencer.addSegment(victoryThemeSegment);

// Create event-triggered transitions
auto mainToBossTransition = std::make_shared<SegmentTransition>("MainTheme", "BossTheme");
mainToBossTransition->setTriggerEvent("boss_appeared");
mainToBossTransition->setType(SegmentTransition::TransitionType::NextBar);

auto bossToVictoryTransition = std::make_shared<SegmentTransition>("BossTheme", "VictoryTheme");
bossToVictoryTransition->setTriggerEvent("boss_defeated");
bossToVictoryTransition->setType(SegmentTransition::TransitionType::Crossfade);
bossToVictoryTransition->setFadeTime(4.0);

// Add transitions
sequencer.addTransition(mainToBossTransition);
sequencer.addTransition(bossToVictoryTransition);

// Start with main theme
sequencer.play("MainTheme");

// Later, in response to game events:
eventSystem.triggerEvent("boss_appeared");  // Will trigger transition to boss theme at next bar
// ... boss battle happens
eventSystem.triggerEvent("boss_defeated");  // Will trigger crossfade transition to victory theme
```

## Implementation Timeline

### Phase 1: Core Framework (1 week)
- Create MusicSegment class
- Implement basic playback functions
- Add segment queue functionality

### Phase 2: Transition System (1 week)
- Implement SegmentTransition class
- Add parameter condition support
- Create transition execution logic

### Phase 3: Advanced Features (1 week)
- Add exit/entry point system
- Implement crossfading and advanced transitions
- Create harmonic transition support

### Phase 4: Integration & UI (1 week)
- Integrate with existing sequencer
- Create visualization interface
- Add preset library of transition types

## Best Practices

### Segment Design
1. **Musical Cohesion**: Design segments to be musically related to enable smooth transitions
2. **Transition Planning**: Place exit points at musically appropriate locations (phrase endings, etc.)
3. **Consistent Instrumentation**: Maintain some consistent elements across segments
4. **Balance Variation**: Create segments with varying intensity but compatible harmonies

### Transition Configuration
1. **Hysteresis**: Use hysteresis in parameter conditions to prevent rapid switching between segments
2. **Prioritization**: Assign priorities to transitions to resolve conflicts
3. **Crossfade Timing**: Match crossfade durations to musical timings (usually 1, 2, or 4 bars)
4. **Exit Point Placement**: Place exit points at musically natural transition locations

### Performance Considerations
1. **Preloading**: Ensure all segment patterns are prepared before playback
2. **Resource Management**: Limit the number of active segments in memory
3. **Thread Safety**: Implement proper locking for parameter and segment access
4. **Timing Accuracy**: Use high-precision timers for accurate beat tracking

## Conclusion

Implementing horizontal re-sequencing will dramatically enhance the AIMusicHardware sequencer, bringing game audio middleware capabilities to your music production environment. This system enables dynamic, responsive music that can adapt to parameters, events, or user input in real-time.

The implementation outlined above focuses on:
- Flexible segment management
- Sophisticated transition rules
- Musical coherence at transition points
- Performance and stability

By following this implementation guide, you'll add a powerful adaptive music system to your sequencer that enables creative applications far beyond traditional linear sequencing.