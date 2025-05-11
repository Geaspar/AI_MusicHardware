# Event System Implementation

This document outlines the implementation of an Event System for the AIMusicHardware project, inspired by event-driven architectures found in game audio middleware like FMOD and Wwise.

## Overview

An Event System provides a trigger-based mechanism for initiating musical changes at specific moments. Unlike continuous parameter control, events represent discrete occurrences that can trigger immediate or scheduled actions. Events are particularly useful for:

- Initiating state changes
- Triggering one-shot sounds or musical phrases
- Synchronizing musical elements with external actions
- Creating complex chains of musical responses
- Broadcasting notifications to multiple listeners

The Event System complements our Parameter System by handling discrete moments of change rather than continuous control.

## Core Components

### 1. Event Class

The foundation of the system is a lightweight `Event` class that represents a discrete occurrence:

```cpp
class Event {
public:
    using EventId = std::string;
    
    Event(const EventId& id);
    virtual ~Event();
    
    // Core properties
    EventId getId() const { return id_; }
    
    // Timestamp (when the event was created)
    double getTimestamp() const { return timestamp_; }
    
    // Payload handling
    template<typename T>
    void setPayload(const T& payload);
    
    template<typename T>
    T getPayload() const;
    
    bool hasPayload() const { return payload_.has_value(); }
    
    // Event cloning (for delayed dispatch)
    virtual std::unique_ptr<Event> clone() const;
    
private:
    EventId id_;
    double timestamp_;
    std::any payload_;
};
```

### 2. EventListener Interface

A simple listener interface for components that need to respond to events:

```cpp
class EventListener {
public:
    virtual ~EventListener() = default;
    
    // Called when an event is dispatched
    virtual void onEvent(const Event& event) = 0;
};
```

### 3. EventBus Class

The central messaging system that connects event producers with event consumers:

```cpp
class EventBus {
public:
    static EventBus& getInstance();
    
    // Event registration
    void addEventListener(const Event::EventId& eventId, EventListener* listener);
    void removeEventListener(const Event::EventId& eventId, EventListener* listener);
    
    // Event dispatch
    void dispatchEvent(const Event& event);
    
    // Scheduled events
    uint64_t scheduleEvent(const Event& event, double delayInSeconds);
    uint64_t scheduleMusicalEvent(const Event& event, int bar, int beat, int tick = 0);
    bool cancelScheduledEvent(uint64_t eventId);
    
    // Update (call this each frame to process delayed events)
    void update(double deltaTime);
    
    // Set musical time provider
    void setTimeProvider(std::function<std::tuple<int, int, int, double>()> timeProvider);
    
private:
    EventBus();
    ~EventBus();
    
    std::multimap<Event::EventId, EventListener*> listeners_;
    
    struct ScheduledEvent {
        uint64_t id;
        std::unique_ptr<Event> event;
        double scheduledTime;  // Absolute time in seconds
        bool isMusical;        // Whether this is a musical time event
        int bar;               // Target bar (if musical)
        int beat;              // Target beat (if musical)
        int tick;              // Target tick (if musical)
    };
    
    std::vector<ScheduledEvent> scheduledEvents_;
    uint64_t nextScheduledEventId_ = 1;
    
    // Musical time provider function returns tuple of (bar, beat, tick, timeInSeconds)
    std::function<std::tuple<int, int, int, double>()> timeProvider_;
    
    // Process events that are ready to fire
    void processScheduledEvents(double currentTime);
};
```

### 4. Common Event Types

Define some specialized event types for common musical scenarios:

```cpp
// Note event (for triggering notes)
class NoteEvent : public Event {
public:
    NoteEvent(int noteNumber, int velocity, int channel = 0);
    
    int getNoteNumber() const { return noteNumber_; }
    int getVelocity() const { return velocity_; }
    int getChannel() const { return channel_; }
    
    std::unique_ptr<Event> clone() const override;
    
private:
    int noteNumber_;
    int velocity_;
    int channel_;
};

// State change event (for the state-based music system)
class StateChangeEvent : public Event {
public:
    StateChangeEvent(const std::string& targetState);
    
    std::string getTargetState() const { return targetState_; }
    
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string targetState_;
};

// Pattern trigger event (for sequencer patterns)
class PatternEvent : public Event {
public:
    enum class Action {
        START,
        STOP,
        MUTE,
        UNMUTE
    };
    
    PatternEvent(const std::string& patternId, Action action);
    
    std::string getPatternId() const { return patternId_; }
    Action getAction() const { return action_; }
    
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string patternId_;
    Action action_;
};

// Transport event (for sequencer control)
class TransportEvent : public Event {
public:
    enum class Action {
        PLAY,
        STOP,
        PAUSE,
        CONTINUE,
        RECORD,
        REWIND
    };
    
    TransportEvent(Action action);
    
    Action getAction() const { return action_; }
    
    std::unique_ptr<Event> clone() const override;
    
private:
    Action action_;
};
```

### 5. EventTrigger Class

A configurable trigger that can fire events based on conditions:

```cpp
class EventTrigger {
public:
    enum class TriggerType {
        PARAMETER_THRESHOLD,   // Trigger when parameter crosses threshold
        PARAMETER_CHANGE,      // Trigger on any parameter change
        TIMER,                 // Trigger after time delay
        MUSICAL_POSITION,      // Trigger at specific bar/beat
        EXTERNAL               // Trigger from external source (e.g. UI)
    };
    
    EventTrigger(const std::string& name, TriggerType type);
    
    // Configuration
    void setEvent(std::unique_ptr<Event> event);
    
    // For threshold triggers
    void setParameterThreshold(Parameter* parameter, float threshold, 
                              bool triggerOnRising = true);
    
    // For timer triggers
    void setTimerInterval(double intervalInSeconds, bool repeat = false);
    
    // For musical position triggers
    void setMusicalPosition(int bar, int beat = 1);
    
    // For all triggers
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    
    // Manually activate trigger
    void activate();
    
    // Update (call every frame)
    void update(double deltaTime);
    
private:
    std::string name_;
    TriggerType type_;
    std::unique_ptr<Event> event_;
    bool enabled_ = true;
    
    // Parameter threshold settings
    Parameter* parameterToWatch_ = nullptr;
    float threshold_ = 0.0f;
    bool triggerOnRising_ = true;
    float lastParameterValue_ = 0.0f;
    
    // Timer settings
    double interval_ = 0.0f;
    double timer_ = 0.0f;
    bool repeat_ = false;
    
    // Musical position settings
    int targetBar_ = 1;
    int targetBeat_ = 1;
    int lastCheckedBar_ = 0;
    int lastCheckedBeat_ = 0;
    
    // Fire the configured event
    void fireEvent();
};
```

## Implementation Details

### Event Implementation

Basic implementation of the Event class:

```cpp
Event::Event(const EventId& id)
    : id_(id), timestamp_(getCurrentTimeInSeconds()) {
}

template<typename T>
void Event::setPayload(const T& payload) {
    payload_ = payload;
}

template<typename T>
T Event::getPayload() const {
    if (!payload_.has_value()) {
        throw std::runtime_error("Event has no payload");
    }
    
    try {
        return std::any_cast<T>(payload_);
    } catch (const std::bad_any_cast&) {
        throw std::runtime_error("Event payload type mismatch");
    }
}

std::unique_ptr<Event> Event::clone() const {
    auto clone = std::make_unique<Event>(id_);
    clone->payload_ = payload_;
    return clone;
}
```

### EventBus Implementation

Core event processing implementation:

```cpp
void EventBus::addEventListener(const Event::EventId& eventId, EventListener* listener) {
    if (!listener) return;
    listeners_.emplace(eventId, listener);
}

void EventBus::removeEventListener(const Event::EventId& eventId, EventListener* listener) {
    if (!listener) return;
    
    auto range = listeners_.equal_range(eventId);
    for (auto it = range.first; it != range.second; ) {
        if (it->second == listener) {
            it = listeners_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventBus::dispatchEvent(const Event& event) {
    // Notify all listeners registered for this event type
    auto range = listeners_.equal_range(event.getId());
    for (auto it = range.first; it != range.second; ++it) {
        it->second->onEvent(event);
    }
    
    // Also notify wildcard listeners (those listening to "*")
    auto wildcardRange = listeners_.equal_range("*");
    for (auto it = wildcardRange.first; it != wildcardRange.second; ++it) {
        it->second->onEvent(event);
    }
}

uint64_t EventBus::scheduleEvent(const Event& event, double delayInSeconds) {
    uint64_t eventId = nextScheduledEventId_++;
    
    ScheduledEvent scheduledEvent{
        eventId,
        event.clone(),
        getCurrentTimeInSeconds() + delayInSeconds,
        false,  // Not a musical event
        0, 0, 0 // No musical time
    };
    
    scheduledEvents_.push_back(std::move(scheduledEvent));
    return eventId;
}

uint64_t EventBus::scheduleMusicalEvent(const Event& event, int bar, int beat, int tick) {
    if (!timeProvider_) {
        // Fall back to immediate dispatch if no time provider
        dispatchEvent(event);
        return 0;
    }
    
    uint64_t eventId = nextScheduledEventId_++;
    
    ScheduledEvent scheduledEvent{
        eventId,
        event.clone(),
        0.0,     // Will be determined when the musical time is reached
        true,    // This is a musical event
        bar, beat, tick
    };
    
    scheduledEvents_.push_back(std::move(scheduledEvent));
    return eventId;
}

void EventBus::update(double deltaTime) {
    // Get current time
    double currentTime = getCurrentTimeInSeconds();
    
    // Process events scheduled by absolute time
    processScheduledEvents(currentTime);
    
    // Process events scheduled by musical time
    if (timeProvider_) {
        auto [currentBar, currentBeat, currentTick, _] = timeProvider_();
        
        // Check for musical events that should fire
        for (auto it = scheduledEvents_.begin(); it != scheduledEvents_.end(); ) {
            if (it->isMusical) {
                bool shouldFire = false;
                
                // Check if musical position has been reached or passed
                if (currentBar > it->bar || 
                    (currentBar == it->bar && currentBeat > it->beat) ||
                    (currentBar == it->bar && currentBeat == it->beat && currentTick >= it->tick)) {
                    shouldFire = true;
                }
                
                if (shouldFire) {
                    dispatchEvent(*(it->event));
                    it = scheduledEvents_.erase(it);
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
}

void EventBus::processScheduledEvents(double currentTime) {
    // Find events that are ready to fire
    for (auto it = scheduledEvents_.begin(); it != scheduledEvents_.end(); ) {
        if (!it->isMusical && it->scheduledTime <= currentTime) {
            // Event is ready, dispatch it
            dispatchEvent(*(it->event));
            it = scheduledEvents_.erase(it);
        } else {
            ++it;
        }
    }
}
```

### EventTrigger Implementation

```cpp
void EventTrigger::update(double deltaTime) {
    if (!enabled_ || !event_) {
        return;
    }
    
    switch (type_) {
        case TriggerType::PARAMETER_THRESHOLD: {
            if (parameterToWatch_) {
                float currentValue = 0.0f;
                
                if (auto* floatParam = dynamic_cast<FloatParameter*>(parameterToWatch_)) {
                    currentValue = floatParam->getValue();
                } else if (auto* intParam = dynamic_cast<IntParameter*>(parameterToWatch_)) {
                    currentValue = static_cast<float>(intParam->getValue());
                }
                
                // Check if threshold has been crossed
                bool thresholdCrossed = false;
                if (triggerOnRising_) {
                    thresholdCrossed = lastParameterValue_ < threshold_ && currentValue >= threshold_;
                } else {
                    thresholdCrossed = lastParameterValue_ > threshold_ && currentValue <= threshold_;
                }
                
                if (thresholdCrossed) {
                    fireEvent();
                }
                
                lastParameterValue_ = currentValue;
            }
            break;
        }
        
        case TriggerType::PARAMETER_CHANGE: {
            if (parameterToWatch_) {
                float currentValue = 0.0f;
                
                if (auto* floatParam = dynamic_cast<FloatParameter*>(parameterToWatch_)) {
                    currentValue = floatParam->getValue();
                } else if (auto* intParam = dynamic_cast<IntParameter*>(parameterToWatch_)) {
                    currentValue = static_cast<float>(intParam->getValue());
                }
                
                // Check if value has changed
                if (std::abs(currentValue - lastParameterValue_) > 0.00001f) {
                    fireEvent();
                }
                
                lastParameterValue_ = currentValue;
            }
            break;
        }
        
        case TriggerType::TIMER: {
            timer_ += deltaTime;
            if (timer_ >= interval_) {
                fireEvent();
                
                if (repeat_) {
                    timer_ = fmod(timer_, interval_); // Keep fractional remainder
                } else {
                    enabled_ = false; // Disable one-shot timer
                    timer_ = 0.0f;
                }
            }
            break;
        }
        
        case TriggerType::MUSICAL_POSITION: {
            // Get current musical position from sequencer
            auto& eventBus = EventBus::getInstance();
            if (eventBus.timeProvider_) {
                auto [currentBar, currentBeat, currentTick, _] = eventBus.timeProvider_();
                
                // Check if position has been reached
                bool positionReached = (currentBar == targetBar_ && currentBeat == targetBeat_) &&
                                      (lastCheckedBar_ != currentBar || lastCheckedBeat_ != currentBeat);
                
                if (positionReached) {
                    fireEvent();
                }
                
                lastCheckedBar_ = currentBar;
                lastCheckedBeat_ = currentBeat;
            }
            break;
        }
        
        case TriggerType::EXTERNAL:
            // Nothing to do, these are manually activated
            break;
    }
}

void EventTrigger::fireEvent() {
    auto& eventBus = EventBus::getInstance();
    eventBus.dispatchEvent(*event_);
}
```

## Integration with Existing System

### Sequencer Integration

```cpp
// In Sequencer.h
class Sequencer : public EventListener {
public:
    // ... existing code ...
    
    // EventListener implementation
    void onEvent(const Event& event) override;
    
    // Event-related methods
    void registerWithEventBus();
    void createEventTriggers();
    
    // Get musical time (for EventBus timeProvider)
    std::tuple<int, int, int, double> getMusicalTimePosition() const;
    
private:
    // ... existing code ...
    
    std::vector<std::unique_ptr<EventTrigger>> eventTriggers_;
};

// In Sequencer.cpp
void Sequencer::registerWithEventBus() {
    auto& eventBus = EventBus::getInstance();
    
    // Register for transport events
    eventBus.addEventListener("transport", this);
    
    // Register for pattern events
    eventBus.addEventListener("pattern", this);
    
    // Set this sequencer as the musical time provider
    eventBus.setTimeProvider([this]() {
        return this->getMusicalTimePosition();
    });
    
    // Create standard triggers
    createEventTriggers();
}

void Sequencer::onEvent(const Event& event) {
    // Handle transport events
    if (event.getId() == "transport") {
        if (auto* transportEvent = dynamic_cast<const TransportEvent*>(&event)) {
            switch (transportEvent->getAction()) {
                case TransportEvent::Action::PLAY:
                    play();
                    break;
                case TransportEvent::Action::STOP:
                    stop();
                    break;
                case TransportEvent::Action::PAUSE:
                    pause();
                    break;
                // ... handle other transport actions
            }
        }
    }
    // Handle pattern events
    else if (event.getId() == "pattern") {
        if (auto* patternEvent = dynamic_cast<const PatternEvent*>(&event)) {
            auto patternId = patternEvent->getPatternId();
            
            switch (patternEvent->getAction()) {
                case PatternEvent::Action::START:
                    startPattern(patternId);
                    break;
                case PatternEvent::Action::STOP:
                    stopPattern(patternId);
                    break;
                case PatternEvent::Action::MUTE:
                    mutePattern(patternId, true);
                    break;
                case PatternEvent::Action::UNMUTE:
                    mutePattern(patternId, false);
                    break;
            }
        }
    }
}

void Sequencer::createEventTriggers() {
    // Create a bar change trigger
    auto barTrigger = std::make_unique<EventTrigger>("barChange", EventTrigger::TriggerType::MUSICAL_POSITION);
    barTrigger->setEvent(std::make_unique<Event>("barChange"));
    
    // Create beat triggers for visualization
    for (int i = 1; i <= 4; ++i) {
        auto beatTrigger = std::make_unique<EventTrigger>(
            "beat" + std::to_string(i), 
            EventTrigger::TriggerType::MUSICAL_POSITION);
        
        beatTrigger->setMusicalPosition(0, i); // Any bar, specific beat
        beatTrigger->setEvent(std::make_unique<Event>("beat"));
        
        eventTriggers_.push_back(std::move(beatTrigger));
    }
    
    // Add triggers for patterns
    for (const auto& pattern : patterns_) {
        auto patternStartTrigger = std::make_unique<EventTrigger>(
            "patternStart_" + pattern.getId(),
            EventTrigger::TriggerType::MUSICAL_POSITION);
            
        patternStartTrigger->setMusicalPosition(pattern.getStartBar(), 1);
        
        auto patternEvent = std::make_unique<PatternEvent>(
            pattern.getId(), PatternEvent::Action::START);
            
        patternStartTrigger->setEvent(std::move(patternEvent));
        eventTriggers_.push_back(std::move(patternStartTrigger));
    }
}

std::tuple<int, int, int, double> Sequencer::getMusicalTimePosition() const {
    int currentBar = getCurrentBar();
    int currentBeat = getCurrentBeat();
    int currentTick = getCurrentTick();
    double currentTimeInSeconds = getCurrentTimeInSeconds();
    
    return std::make_tuple(currentBar, currentBeat, currentTick, currentTimeInSeconds);
}
```

### State Machine Integration

```cpp
// In StateMachine.h
class StateMachine : public EventListener {
public:
    // ... existing code ...
    
    // EventListener implementation
    void onEvent(const Event& event) override;
    
    // Register with event system
    void registerWithEventBus();
    
    // Create state change events
    Event createStateChangeEvent(const std::string& targetState) const;
    
private:
    // ... existing code ...
};

// In StateMachine.cpp
void StateMachine::registerWithEventBus() {
    auto& eventBus = EventBus::getInstance();
    
    // Register for state change events
    eventBus.addEventListener("stateChange", this);
}

void StateMachine::onEvent(const Event& event) {
    if (event.getId() == "stateChange") {
        if (auto* stateEvent = dynamic_cast<const StateChangeEvent*>(&event)) {
            std::string targetState = stateEvent->getTargetState();
            changeState(targetState);
        }
    }
}

Event StateMachine::createStateChangeEvent(const std::string& targetState) const {
    return StateChangeEvent(targetState);
}
```

### Audio Engine Integration

```cpp
// In AudioEngine.h
class AudioEngine : public EventListener {
public:
    // ... existing code ...
    
    // EventListener implementation
    void onEvent(const Event& event) override;
    
    // Register with event system
    void registerWithEventBus();
    
private:
    // ... existing code ...
};

// In AudioEngine.cpp
void AudioEngine::registerWithEventBus() {
    auto& eventBus = EventBus::getInstance();
    
    // Register for note events
    eventBus.addEventListener("note", this);
    
    // Register for audio control events
    eventBus.addEventListener("audio", this);
}

void AudioEngine::onEvent(const Event& event) {
    // Handle note events
    if (event.getId() == "note") {
        if (auto* noteEvent = dynamic_cast<const NoteEvent*>(&event)) {
            // Play the note
            int note = noteEvent->getNoteNumber();
            int velocity = noteEvent->getVelocity();
            int channel = noteEvent->getChannel();
            
            playNote(note, velocity, channel);
        }
    }
    // Handle audio control events
    else if (event.getId() == "audio") {
        // Process audio control events
        // ...
    }
}
```

### UI Integration

```cpp
// In UserInterface.h
class UserInterface : public EventListener {
public:
    // ... existing code ...
    
    // EventListener implementation
    void onEvent(const Event& event) override;
    
    // Register with event system
    void registerWithEventBus();
    
    // Fire UI events
    void fireButtonEvent(const std::string& buttonId);
    void fireSliderEvent(const std::string& sliderId, float value);
    
private:
    // ... existing code ...
};

// In UserInterface.cpp
void UserInterface::registerWithEventBus() {
    auto& eventBus = EventBus::getInstance();
    
    // Register for UI update events
    eventBus.addEventListener("beat", this);
    eventBus.addEventListener("barChange", this);
    eventBus.addEventListener("stateChange", this);
}

void UserInterface::onEvent(const Event& event) {
    // Update UI based on musical events
    if (event.getId() == "beat") {
        // Flash beat indicator
        updateBeatVisualizer();
    }
    else if (event.getId() == "barChange") {
        // Update bar counter
        updateBarCounter();
    }
    else if (event.getId() == "stateChange") {
        if (auto* stateEvent = dynamic_cast<const StateChangeEvent*>(&event)) {
            // Update state display
            updateStateDisplay(stateEvent->getTargetState());
        }
    }
}

void UserInterface::fireButtonEvent(const std::string& buttonId) {
    auto& eventBus = EventBus::getInstance();
    
    // Create custom UI event
    Event uiEvent("ui_button");
    uiEvent.setPayload(buttonId);
    
    // Dispatch the event
    eventBus.dispatchEvent(uiEvent);
}
```

## Practical Examples

### Basic Event Usage

```cpp
// Create and dispatch a simple event
Event simpleEvent("customEvent");
simpleEvent.setPayload<std::string>("Hello, Event System!");

EventBus::getInstance().dispatchEvent(simpleEvent);

// Create a note event
NoteEvent noteEvent(60, 100); // Middle C, velocity 100
EventBus::getInstance().dispatchEvent(noteEvent);

// Schedule an event for 2 seconds in the future
EventBus::getInstance().scheduleEvent(noteEvent, 2.0);

// Schedule an event to occur at bar 3, beat 2
EventBus::getInstance().scheduleMusicalEvent(noteEvent, 3, 2);
```

### Event Listener Example

```cpp
class MyComponent : public EventListener {
public:
    MyComponent() {
        // Register for events
        auto& eventBus = EventBus::getInstance();
        eventBus.addEventListener("customEvent", this);
        eventBus.addEventListener("note", this);
    }
    
    ~MyComponent() {
        // Clean up event registrations
        auto& eventBus = EventBus::getInstance();
        eventBus.removeEventListener("customEvent", this);
        eventBus.removeEventListener("note", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "customEvent") {
            try {
                std::string message = event.getPayload<std::string>();
                std::cout << "Received custom event with message: " << message << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error getting payload: " << e.what() << std::endl;
            }
        }
        else if (event.getId() == "note") {
            if (auto* noteEvent = dynamic_cast<const NoteEvent*>(&event)) {
                std::cout << "Received note event: note=" << noteEvent->getNoteNumber()
                          << ", velocity=" << noteEvent->getVelocity() << std::endl;
            }
        }
    }
};
```

### Event Trigger Example

```cpp
// Create a threshold trigger for a parameter
auto intensityParam = parameterManager.getParameterByPath("music/intensity");
auto& eventBus = EventBus::getInstance();

auto combatTrigger = std::make_unique<EventTrigger>(
    "combatStart", 
    EventTrigger::TriggerType::PARAMETER_THRESHOLD);
    
combatTrigger->setParameterThreshold(intensityParam, 0.7f, true); // Trigger when rising above 0.7
combatTrigger->setEvent(std::make_unique<StateChangeEvent>("combat"));

// Create a musical position trigger
auto stingerTrigger = std::make_unique<EventTrigger>(
    "stinger", 
    EventTrigger::TriggerType::MUSICAL_POSITION);
    
stingerTrigger->setMusicalPosition(4, 1); // Bar 4, beat 1
stingerTrigger->setEvent(std::make_unique<Event>("playSfx"));
stingerTrigger->getEvent()->setPayload<std::string>("stinger_brass.wav");
```

### Integration with State-Based Music System

```cpp
// Create state machine with event triggering
auto stateMachine = std::make_unique<StateMachine>();

// Define states
auto exploreState = std::make_unique<MusicState>("explore", "Exploration");
auto combatState = std::make_unique<MusicState>("combat", "Combat");
auto victoryState = std::make_unique<MusicState>("victory", "Victory");

// Register with event system
stateMachine->registerWithEventBus();

// Create UI buttons that trigger state changes
auto& ui = UserInterface::getInstance();

ui.createButton("Explore", [&]() {
    auto& eventBus = EventBus::getInstance();
    eventBus.dispatchEvent(StateChangeEvent("explore"));
});

ui.createButton("Combat", [&]() {
    auto& eventBus = EventBus::getInstance();
    eventBus.dispatchEvent(StateChangeEvent("combat"));
});

ui.createButton("Victory", [&]() {
    auto& eventBus = EventBus::getInstance();
    eventBus.dispatchEvent(StateChangeEvent("victory"));
});

// Create parameter threshold for automatic transition
auto& paramManager = ParameterManager::getInstance();
auto threatParam = paramManager.getParameterByPath("game/threatLevel");

auto combatTrigger = std::make_unique<EventTrigger>(
    "threatToCombat", 
    EventTrigger::TriggerType::PARAMETER_THRESHOLD);
    
combatTrigger->setParameterThreshold(threatParam, 0.8f, true);
combatTrigger->setEvent(std::make_unique<StateChangeEvent>("combat"));

auto exploreTrigger = std::make_unique<EventTrigger>(
    "threatToExplore", 
    EventTrigger::TriggerType::PARAMETER_THRESHOLD);
    
exploreTrigger->setParameterThreshold(threatParam, 0.2f, false);
exploreTrigger->setEvent(std::make_unique<StateChangeEvent>("explore"));
```

### Integration with Horizontal Re-sequencing

```cpp
// Create segment triggers based on events
auto& sequencer = horizontalResequencer.getSequencer();
sequencer.registerWithEventBus();

// Create event triggers for segment transitions
auto entranceTrigger = std::make_unique<EventTrigger>(
    "entranceSegment", 
    EventTrigger::TriggerType::EXTERNAL);
    
auto segmentEvent = std::make_unique<Event>("playSegment");
segmentEvent->setPayload<std::string>("entrance");
entranceTrigger->setEvent(std::move(segmentEvent));

// Register UI button to trigger segment
ui.createButton("Play Entrance", [&]() {
    entranceTrigger->activate();
});

// Create listener for segment events
class SegmentController : public EventListener {
public:
    SegmentController(HorizontalResequencer& resequencer)
        : resequencer_(resequencer) {
        auto& eventBus = EventBus::getInstance();
        eventBus.addEventListener("playSegment", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "playSegment") {
            try {
                std::string segmentId = event.getPayload<std::string>();
                resequencer_.playSegment(segmentId);
            } catch (const std::exception& e) {
                std::cerr << "Error playing segment: " << e.what() << std::endl;
            }
        }
    }
    
private:
    HorizontalResequencer& resequencer_;
};
```

### Integration with Vertical Remix System

```cpp
// Create layer triggers for vertical remix system
auto& verticalMixer = verticalRemixSystem.getLayerManager();

// Register mixer with event bus
class LayerController : public EventListener {
public:
    LayerController(LayerManager& layerManager)
        : layerManager_(layerManager) {
        auto& eventBus = EventBus::getInstance();
        eventBus.addEventListener("layerControl", this);
    }
    
    void onEvent(const Event& event) override {
        if (event.getId() == "layerControl") {
            try {
                auto data = event.getPayload<std::pair<std::string, float>>();
                std::string layerId = data.first;
                float volume = data.second;
                
                layerManager_.setLayerVolume(layerId, volume);
            } catch (const std::exception& e) {
                std::cerr << "Error controlling layer: " << e.what() << std::endl;
            }
        }
    }
    
private:
    LayerManager& layerManager_;
};

// Create UI controls that fire layer events
ui.createSlider("Drums Volume", 0.0f, 1.0f, [&](float value) {
    auto& eventBus = EventBus::getInstance();
    
    Event layerEvent("layerControl");
    layerEvent.setPayload<std::pair<std::string, float>>({"drums", value});
    
    eventBus.dispatchEvent(layerEvent);
});
```

## Implementation Timeline

1. **Phase 1: Core Event System (1-2 days)**
   - Implement `Event` base class
   - Implement `EventListener` interface
   - Implement `EventBus` singleton
   - Create unit tests for basic event functionality

2. **Phase 2: Specialized Event Types (1 day)**
   - Implement common event types (Note, State, Pattern, Transport)
   - Add event payload handling
   - Create serialization for events
   - Test event type functionality

3. **Phase 3: Event Scheduling (1-2 days)**
   - Implement time-based event scheduling
   - Implement musical-time event scheduling
   - Create test cases for scheduling
   - Integrate with sequencer timing

4. **Phase 4: Event Triggers (1-2 days)**
   - Implement `EventTrigger` class
   - Add various trigger types
   - Create test cases for triggers
   - Create builder pattern for easy trigger creation

5. **Phase 5: Integration with Existing Systems (2-3 days)**
   - Integrate with State Machine
   - Integrate with Sequencer
   - Integrate with Audio Engine
   - Integrate with User Interface
   - Connect with Parameter System

6. **Phase 6: Advanced Features (1-2 days)**
   - Add event filtering and routing
   - Implement event recording for debugging
   - Create visualization tools
   - Add performance optimizations

7. **Phase 7: Testing and Polishing (1-2 days)**
   - Create comprehensive integration tests
   - Optimize memory usage
   - Finalize documentation
   - Create example usage code

## Conclusion

The Event System provides a powerful mechanism for triggering musical changes at specific moments, complementing the continuous control offered by the Parameter System. By implementing this system, we gain:

1. **Discrete Triggering** - Ability to fire one-shot musical events
2. **Flexible Scheduling** - Events can be scheduled by time or musical position
3. **Threshold Triggering** - Automatic firing based on parameter values
4. **System Integration** - Connect UI, sequencer, and audio engine through events
5. **Loose Coupling** - Components can communicate without direct dependencies
6. **Extensibility** - New event types can be added as needed

This implementation integrates seamlessly with our previously implemented systems:

- **State-Based Music System** - Events trigger state transitions
- **Vertical Remix System** - Events control layer volumes
- **Horizontal Re-sequencing** - Events trigger segment changes
- **Parameter System** - Parameters can trigger events via thresholds

Together, these systems provide a complete framework for creating sophisticated adaptive music in the AIMusicHardware project.