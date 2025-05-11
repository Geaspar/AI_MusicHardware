# State-Based Music System Implementation

This document outlines the implementation of a State-Based Music System for the AIMusicHardware project, drawing inspiration from game audio middleware systems like FMOD and Wwise.

## Overview

A State-Based Music System provides a framework for creating and managing discrete musical states with smooth transitions between them. This approach is commonly used in game audio to dynamically adapt music to different gameplay contexts (exploration, combat, tension, etc.) while maintaining musical coherence.

## Core Components

### 1. MusicState Class

The `MusicState` class represents a distinct musical state, containing:
- Audio content (MIDI patterns, audio files)
- Transition rules
- Internal parameters
- Optional sub-states

```cpp
class MusicState {
public:
    using StateId = std::string;
    
    MusicState(const StateId& id, const std::string& name);
    virtual ~MusicState();
    
    // Core properties
    StateId getId() const { return id_; }
    std::string getName() const { return name_; }
    
    // Content management
    void addMidiPattern(const MidiPattern& pattern);
    void addAudioFile(const std::string& filePath);
    void clearContent();
    
    // State playback
    virtual void enter();  // Called when state becomes active
    virtual void exit();   // Called when state is exited
    virtual void update(); // Called every frame during active state
    
    // Parameter control
    void setParameter(const std::string& name, float value);
    float getParameter(const std::string& name) const;
    
    // Transition management
    void addTransitionRule(const StateId& targetState, 
                          std::function<bool()> condition);
    
private:
    StateId id_;
    std::string name_;
    std::vector<MidiPattern> midiPatterns_;
    std::vector<std::string> audioFiles_;
    std::map<std::string, float> parameters_;
    std::map<StateId, std::function<bool()>> transitionRules_;
};
```

### 2. StateTransition Class

The `StateTransition` class defines how music transitions between states:

```cpp
class StateTransition {
public:
    enum class Type {
        IMMEDIATE,      // Instant switch
        CROSSFADE,      // Volume crossfade
        SEQUENTIAL,     // Wait for musical phrase completion
        MUSICAL_SYNC    // Transition at specific musical position (beat/bar)
    };
    
    StateTransition(const MusicState::StateId& fromState,
                   const MusicState::StateId& toState,
                   Type type = Type::CROSSFADE);
    
    void setTransitionTime(float timeInSeconds);
    void setMusicalSyncPoint(int bar, int beat = 1);
    void setExitPoint(int measureIndex);
    
    // Custom transition functions
    void setCustomTransitionCallback(std::function<void(float progress)> callback);
    
    // Transition properties
    MusicState::StateId getFromState() const { return fromState_; }
    MusicState::StateId getToState() const { return toState_; }
    Type getType() const { return type_; }
    
    // Execute transition
    void start();
    void update(float deltaTime);
    bool isComplete() const;
    float getProgress() const;
    
private:
    MusicState::StateId fromState_;
    MusicState::StateId toState_;
    Type type_;
    float duration_ = 2.0f;     // Default 2-second transition
    float progress_ = 0.0f;
    bool isActive_ = false;
    
    // Musical sync parameters
    int syncBar_ = 1;
    int syncBeat_ = 1;
    int exitMeasure_ = -1;      // -1 means end of current phrase
    
    std::function<void(float progress)> customCallback_;
};
```

### 3. StateMachine Class

The `StateMachine` class manages the entire system:

```cpp
class StateMachine {
public:
    StateMachine();
    ~StateMachine();
    
    // State management
    void addState(std::unique_ptr<MusicState> state);
    MusicState* getState(const MusicState::StateId& id);
    void removeState(const MusicState::StateId& id);
    
    // Transition management
    void addTransition(std::unique_ptr<StateTransition> transition);
    StateTransition* findTransition(const MusicState::StateId& from, 
                                  const MusicState::StateId& to);
    
    // Machine control
    void setInitialState(const MusicState::StateId& id);
    void start();
    void stop();
    void update();
    
    // Direct control
    bool changeState(const MusicState::StateId& targetState, 
                    StateTransition::Type overrideTransitionType = StateTransition::Type::CROSSFADE);
    
    // Status
    MusicState* getCurrentState() const { return currentState_; }
    bool isTransitioning() const { return activeTransition_ != nullptr; }
    
    // Event callbacks
    void onStateChanged(std::function<void(MusicState*, MusicState*)> callback);
    
private:
    std::map<MusicState::StateId, std::unique_ptr<MusicState>> states_;
    std::vector<std::unique_ptr<StateTransition>> transitions_;
    MusicState* currentState_ = nullptr;
    StateTransition* activeTransition_ = nullptr;
    
    MusicState::StateId initialStateId_;
    std::function<void(MusicState*, MusicState*)> stateChangedCallback_;
    
    void evaluateTransitions();
    void updateCurrentTransition();
};
```

### 4. Integration with Sequencer and Audio Engine

The State Machine system integrates with our existing sequencer and audio engine:

```cpp
class MusicStateSequencer : public StateMachine {
public:
    MusicStateSequencer(Sequencer* sequencer, AudioEngine* audioEngine);
    
    // Additional integration methods
    void synchronizeWithSequencer();
    void bindParameterToSequencer(const std::string& parameterName, 
                                 Sequencer::ParameterId sequencerParameter);
    
    // Playback controls
    void play();
    void pause();
    void stop();
    
private:
    Sequencer* sequencer_;
    AudioEngine* audioEngine_;
    std::map<std::string, Sequencer::ParameterId> parameterBindings_;
};
```

## Implementation Details

### State Transitions

State transitions are a critical component of the system. Here's how different transition types work:

#### Immediate Transition
```cpp
void ImmediateTransition::execute() {
    // Stop current content
    fromState_->exit();
    
    // Start new content
    toState_->enter();
    
    // Mark transition as complete
    progress_ = 1.0f;
    isActive_ = false;
}
```

#### Crossfade Transition
```cpp
void CrossfadeTransition::update(float deltaTime) {
    if (!isActive_) return;
    
    // Calculate crossfade progress
    progress_ += deltaTime / duration_;
    if (progress_ > 1.0f) progress_ = 1.0f;
    
    // Apply volume changes to both states
    float fromVolume = 1.0f - progress_;
    float toVolume = progress_;
    
    // Apply volumes to sequencer tracks/audio files
    
    // Check if transition is complete
    if (progress_ >= 1.0f) {
        fromState_->exit();
        isActive_ = false;
    }
}
```

#### Sequential Transition
```cpp
void SequentialTransition::start() {
    isActive_ = true;
    progress_ = 0.0f;
    
    // Check if we need to wait for a specific exit point
    if (exitMeasure_ >= 0) {
        // Setup callback for when measure is reached
        sequencer_->onMeasureReached(exitMeasure_, [this]() {
            toState_->enter();
            fromState_->exit();
            progress_ = 1.0f;
            isActive_ = false;
        });
    } else {
        // Wait for current pattern to finish
        sequencer_->onPatternComplete([this]() {
            toState_->enter();
            fromState_->exit();
            progress_ = 1.0f;
            isActive_ = false;
        });
    }
}
```

#### Musical Sync Transition
```cpp
void MusicalSyncTransition::start() {
    isActive_ = true;
    progress_ = 0.0f;
    
    // Setup callback for when sync point is reached
    sequencer_->onPositionReached(syncBar_, syncBeat_, [this]() {
        toState_->enter();
        fromState_->exit();
        progress_ = 1.0f;
        isActive_ = false;
    });
}
```

### State Machine Evaluation

The state machine evaluates transitions on each update:

```cpp
void StateMachine::evaluateTransitions() {
    if (!currentState_ || activeTransition_)
        return;
        
    // Check all transition rules for current state
    for (auto& [targetStateId, condition] : currentState_->getTransitionRules()) {
        if (condition()) {
            // Condition met, trigger transition
            MusicState* targetState = getState(targetStateId);
            if (targetState) {
                StateTransition* transition = findTransition(
                    currentState_->getId(), targetState->getId());
                    
                if (transition) {
                    activeTransition_ = transition;
                    transition->start();
                    
                    // If the transition is immediate, update state now
                    if (transition->getType() == StateTransition::Type::IMMEDIATE) {
                        MusicState* oldState = currentState_;
                        currentState_ = targetState;
                        if (stateChangedCallback_)
                            stateChangedCallback_(oldState, currentState_);
                    }
                } else {
                    // No transition defined, use immediate transition
                    MusicState* oldState = currentState_;
                    currentState_ = targetState;
                    oldState->exit();
                    currentState_->enter();
                    if (stateChangedCallback_)
                        stateChangedCallback_(oldState, currentState_);
                }
                
                // Only trigger one transition at a time
                break;
            }
        }
    }
}
```

## Integration with Existing System

### Sequencer Integration

```cpp
void MusicStateSequencer::synchronizeWithSequencer() {
    // Ensure the sequencer is properly configured
    sequencer_->setTempoSource(Sequencer::TempoSource::INTERNAL);
    
    // Set callbacks for pattern processing
    sequencer_->onBeatChanged([this](int bar, int beat) {
        // Update states based on musical position
        if (currentState_)
            currentState_->update();
            
        // Update transitions
        updateCurrentTransition();
    });
}
```

### Audio Engine Integration

```cpp
void MusicStateSequencer::applyStateToAudioEngine(MusicState* state) {
    // Apply state parameters to audio engine
    for (const auto& [paramName, value] : state->getAllParameters()) {
        // Map state parameters to audio engine parameters
        if (parameterBindings_.find(paramName) != parameterBindings_.end()) {
            auto engineParam = parameterBindings_[paramName];
            audioEngine_->setParameter(engineParam, value);
        }
    }
}
```

## Practical Examples

### Basic State Setup

```cpp
// Create a music state machine
auto stateMachine = std::make_unique<MusicStateSequencer>(sequencer, audioEngine);

// Create states
auto exploreState = std::make_unique<MusicState>("explore", "Exploration Theme");
auto combatState = std::make_unique<MusicState>("combat", "Combat Theme");
auto tensionState = std::make_unique<MusicState>("tension", "Tension Theme");

// Add MIDI patterns to states
exploreState->addMidiPattern(loadMidiPattern("explore_main.mid"));
combatState->addMidiPattern(loadMidiPattern("combat_rhythm.mid"));
combatState->addMidiPattern(loadMidiPattern("combat_melody.mid"));
tensionState->addMidiPattern(loadMidiPattern("tension_pulse.mid"));

// Add transition rules
exploreState->addTransitionRule("combat", [&]() {
    return systemParameters.getValue("threat_level") > 0.7f;
});

combatState->addTransitionRule("explore", [&]() {
    return systemParameters.getValue("threat_level") < 0.3f;
});

combatState->addTransitionRule("tension", [&]() {
    return systemParameters.getValue("player_health") < 0.3f;
});

// Create transitions
auto exploreToCombat = std::make_unique<StateTransition>(
    "explore", "combat", StateTransition::Type::MUSICAL_SYNC);
exploreToCombat->setMusicalSyncPoint(5, 1); // Transition at bar 5, beat 1

auto combatToExplore = std::make_unique<StateTransition>(
    "combat", "explore", StateTransition::Type::CROSSFADE);
combatToExplore->setTransitionTime(4.0f); // 4-second crossfade

// Add states and transitions to state machine
stateMachine->addState(std::move(exploreState));
stateMachine->addState(std::move(combatState));
stateMachine->addState(std::move(tensionState));

stateMachine->addTransition(std::move(exploreToCombat));
stateMachine->addTransition(std::move(combatToExplore));

// Set initial state and start
stateMachine->setInitialState("explore");
stateMachine->start();
```

### Dynamic Control Example

```cpp
// Create callback for state changes
stateMachine->onStateChanged([](MusicState* oldState, MusicState* newState) {
    std::cout << "Music state changed from " 
              << oldState->getName() << " to " 
              << newState->getName() << std::endl;
              
    // Perhaps update UI or other game systems
    userInterface->setMoodColor(newState->getParameter("mood_color"));
});

// Update system parameters
void gameLoop() {
    // Update threat level based on enemies nearby
    float threatLevel = calculateThreatLevel();
    systemParameters.setValue("threat_level", threatLevel);
    
    // Update player health
    float healthPercent = player.getHealth() / player.getMaxHealth();
    systemParameters.setValue("player_health", healthPercent);
    
    // Update state machine (evaluates transitions)
    stateMachine->update();
}
```

### Sub-State Example

```cpp
// Create a parent state with multiple variations
auto dungeonState = std::make_unique<MusicState>("dungeon", "Dungeon Music");

// Create sub-states
auto dungeonNormal = std::make_unique<MusicState>("dungeon_normal", "Normal Dungeon");
auto dungeonSecret = std::make_unique<MusicState>("dungeon_secret", "Secret Room");
auto dungeonBoss = std::make_unique<MusicState>("dungeon_boss", "Boss Approach");

// Create composite state that manages sub-states
auto dungeonComposite = std::make_unique<CompositeState>("dungeon", "Dungeon Suite");
dungeonComposite->addSubState(std::move(dungeonNormal));
dungeonComposite->addSubState(std::move(dungeonSecret));
dungeonComposite->addSubState(std::move(dungeonBoss));

// Set sub-state transitions
dungeonComposite->addSubStateTransitionRule("dungeon_normal", "dungeon_secret", [&]() {
    return systemParameters.getValue("found_secret") > 0.5f;
});

dungeonComposite->addSubStateTransitionRule("dungeon_normal", "dungeon_boss", [&]() {
    return systemParameters.getValue("distance_to_boss") < 0.2f;
});

// Add to state machine
stateMachine->addState(std::move(dungeonComposite));
```

## Implementation Timeline

1. **Phase 1: Core Implementation (1-2 days)**
   - Implement basic `MusicState` class
   - Implement basic `StateTransition` class
   - Implement basic `StateMachine` class
   - Create unit tests for core functionality

2. **Phase 2: Transition Logic (1-2 days)**
   - Implement all transition types (IMMEDIATE, CROSSFADE, SEQUENTIAL, MUSICAL_SYNC)
   - Add transition execution logic
   - Create tests for transition behavior

3. **Phase 3: Integration (2-3 days)**
   - Integrate with existing Sequencer
   - Integrate with AudioEngine
   - Add parameter binding functionality
   - Create `MusicStateSequencer` class

4. **Phase 4: Advanced Features (2-3 days)**
   - Implement composite states
   - Add RTPC (Real-Time Parameter Control) support
   - Implement state blending for smoother transitions
   - Create visualization tools for state machine debugging

5. **Phase 5: Testing and Polishing (1-2 days)**
   - Create example implementations
   - Optimize performance
   - Documentation and cleanup

## Conclusion

The State-Based Music System provides a powerful framework for creating adaptive, responsive music that changes based on gameplay events or user interactions. This implementation draws from game audio middleware design patterns to create a flexible system that integrates with our existing sequencer and audio engine.

Key advantages of this approach:
- Clear separation of musical content into distinct states
- Flexible, rule-based transitions between states
- Multiple transition types for different musical situations
- Integration with existing parameter and sequencer systems
- Support for complex nested state hierarchies