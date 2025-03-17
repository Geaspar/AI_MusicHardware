#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

#include "Sequencer.h"
#include "../audio/AudioEngine.h"
#include "../audio/Synthesizer.h"
#include "../midi/MidiInterface.h"
#include "../hardware/HardwareInterface.h"

namespace AIMusicHardware {

// Define MidiEvent struct for AdaptiveSequencer
enum class MidiEventType {
    NoteOn,
    NoteOff,
    ControlChange
};

struct MidiEvent {
    MidiEventType type;
    int note;
    int velocity;
    float time;
    int controller;
    int value;
};

// Forward declarations
class MusicState;
class StateTransition;
class TrackLayer;
class MixSnapshot;
class TransitionManager;
class Parameter;
class EventSystem;

/**
 * @brief Parameter class for dynamic system variables
 */
class Parameter {
public:
    Parameter(const std::string& name, float defaultValue = 0.0f, float minValue = 0.0f, float maxValue = 1.0f);
    
    std::string getName() const;
    float getValue() const;
    void setValue(float value);
    float getMin() const;
    float getMax() const;
    float getDefault() const;
    bool isBipolar() const;
    void setBipolar(bool bipolar);
    
    // Callback for parameter changes
    using ChangeCallback = std::function<void(const Parameter&, float oldValue, float newValue)>;
    void setChangeCallback(ChangeCallback callback);
    
private:
    std::string name_;
    float value_;
    float defaultValue_;
    float minValue_;
    float maxValue_;
    bool bipolar_;
    ChangeCallback changeCallback_;
    mutable std::mutex mutex_; // Make mutex mutable so it can be locked in const methods
};

/**
 * @brief Event system for trigger events
 */
class EventSystem {
public:
    using EventCallback = std::function<void(const std::string& eventName, const std::map<std::string, float>& eventData)>;
    
    EventSystem();
    
    void registerEvent(const std::string& eventName);
    void unregisterEvent(const std::string& eventName);
    bool isEventRegistered(const std::string& eventName) const;
    
    void addListener(const std::string& eventName, EventCallback callback);
    void removeListener(const std::string& eventName, void* owner);
    
    void triggerEvent(const std::string& eventName, const std::map<std::string, float>& eventData = {});
    
    // Time-based event scheduling
    void scheduleEvent(const std::string& eventName, float delayInBeats, 
                       const std::map<std::string, float>& eventData = {});
    void cancelScheduledEvents(const std::string& eventName);
    void processTick(float beatPosition);
    
private:
    struct ScheduledEvent {
        std::string eventName;
        float triggerBeat;
        std::map<std::string, float> eventData;
    };
    
    std::map<std::string, std::vector<EventCallback>> eventListeners_;
    std::vector<ScheduledEvent> scheduledEvents_;
    mutable std::mutex mutex_; // Make mutex mutable so it can be locked in const methods
};

/**
 * @brief A single layer of musical content
 */
class TrackLayer {
public:
    TrackLayer(const std::string& name);
    ~TrackLayer();
    
    void setPattern(const std::vector<MidiEvent>& pattern);
    const std::vector<MidiEvent>& getPattern() const;
    
    void setVolume(float volume);
    float getVolume() const;
    
    void setMuted(bool muted);
    bool isMuted() const;
    
    void setSolo(bool solo);
    bool isSolo() const;
    
    const std::string& getName() const;
    
private:
    std::string name_;
    std::vector<MidiEvent> pattern_;
    float volume_;
    bool muted_;
    bool solo_;
};

/**
 * @brief Snapshot of a particular mix configuration
 */
class MixSnapshot {
public:
    MixSnapshot(const std::string& name);
    ~MixSnapshot();
    
    void setLayerVolume(const std::string& layerName, float volume);
    float getLayerVolume(const std::string& layerName) const;
    
    void setLayerMuted(const std::string& layerName, bool muted);
    bool isLayerMuted(const std::string& layerName) const;
    
    const std::string& getName() const;
    
    std::map<std::string, float> getVolumeMap() const;
    std::map<std::string, bool> getMuteMap() const;
    
private:
    std::string name_;
    std::map<std::string, float> layerVolumes_;
    std::map<std::string, bool> layerMutes_;
};

/**
 * @brief Represents a musical state in the system
 */
class MusicState {
public:
    MusicState(const std::string& name);
    ~MusicState();
    
    void addLayer(std::shared_ptr<TrackLayer> layer);
    void removeLayer(const std::string& layerName);
    std::shared_ptr<TrackLayer> getLayer(const std::string& layerName);
    std::vector<std::shared_ptr<TrackLayer>> getAllLayers() const;
    
    void addSnapshot(std::shared_ptr<MixSnapshot> snapshot);
    void removeSnapshot(const std::string& snapshotName);
    std::shared_ptr<MixSnapshot> getSnapshot(const std::string& snapshotName);
    std::vector<std::shared_ptr<MixSnapshot>> getAllSnapshots() const;
    
    void setActiveSnapshot(const std::string& snapshotName);
    std::shared_ptr<MixSnapshot> getActiveSnapshot() const;
    
    const std::string& getName() const;
    
    // State metadata
    void setTempo(float bpm);
    float getTempo() const;
    
    void setTimeSignature(int numerator, int denominator);
    std::pair<int, int> getTimeSignature() const;
    
    void setLoopLength(int bars);
    int getLoopLength() const;
    
    // Parameter handling
    void addParameter(std::shared_ptr<Parameter> parameter);
    void removeParameter(const std::string& paramName);
    std::shared_ptr<Parameter> getParameter(const std::string& paramName);
    std::vector<std::shared_ptr<Parameter>> getAllParameters() const;
    
private:
    std::string name_;
    float tempo_;
    int timeSignatureNumerator_;
    int timeSignatureDenominator_;
    int loopLengthBars_;
    
    std::map<std::string, std::shared_ptr<TrackLayer>> layers_;
    std::map<std::string, std::shared_ptr<MixSnapshot>> snapshots_;
    std::shared_ptr<MixSnapshot> activeSnapshot_;
    std::map<std::string, std::shared_ptr<Parameter>> parameters_;
};

/**
 * @brief Handles transitions between states
 */
class StateTransition {
public:
    enum class TransitionType {
        Immediate,      // Instant switch
        Crossfade,      // Gradual volume crossfade
        MusicalSync,    // Wait for musical boundary
        Morph           // Parameter-based morphing
    };
    
    StateTransition(const std::string& name, std::shared_ptr<MusicState> fromState, 
                   std::shared_ptr<MusicState> toState);
    ~StateTransition();
    
    void setTransitionType(TransitionType type);
    TransitionType getTransitionType() const;
    
    void setDuration(float durationInBeats);
    float getDuration() const;
    
    void setSyncPoint(int bars, int beats = 1);
    std::pair<int, int> getSyncPoint() const;
    
    const std::string& getName() const;
    
    std::shared_ptr<MusicState> getFromState() const;
    std::shared_ptr<MusicState> getToState() const;
    
    // Transition conditions
    void setCondition(const std::string& paramName, float threshold, bool greaterThan = true);
    void clearCondition(const std::string& paramName);
    bool checkConditions() const;
    
private:
    std::string name_;
    std::weak_ptr<MusicState> fromState_;
    std::weak_ptr<MusicState> toState_;
    TransitionType type_;
    float durationInBeats_;
    int syncBarCount_;
    int syncBeatCount_;
    
    struct Condition {
        std::weak_ptr<Parameter> parameter;
        float threshold;
        bool greaterThan;
    };
    
    std::map<std::string, Condition> conditions_;
};

/**
 * @brief Manages transitions between states
 */
class TransitionManager {
public:
    TransitionManager();
    ~TransitionManager();
    
    void addTransition(std::shared_ptr<StateTransition> transition);
    void removeTransition(const std::string& transitionName);
    std::shared_ptr<StateTransition> getTransition(const std::string& transitionName);
    std::vector<std::shared_ptr<StateTransition>> getAllTransitions() const;
    
    void startTransition(const std::string& transitionName);
    void cancelTransition();
    bool isTransitioning() const;
    
    void update(float deltaTime);
    
private:
    std::map<std::string, std::shared_ptr<StateTransition>> transitions_;
    std::shared_ptr<StateTransition> activeTransition_;
    float transitionProgress_;
    mutable std::mutex mutex_; // Make mutex mutable so it can be locked in const methods
};

/**
 * @brief Main AdaptiveSequencer controller class
 */
class AdaptiveSequencer {
public:
    AdaptiveSequencer();
    ~AdaptiveSequencer();
    
    bool initialize(std::shared_ptr<AudioEngine> audioEngine, 
                   std::shared_ptr<Synthesizer> synthesizer,
                   std::shared_ptr<HardwareInterface> hardwareInterface = nullptr);
    void shutdown();
    
    // State management
    void addState(std::shared_ptr<MusicState> state);
    void removeState(const std::string& stateName);
    std::shared_ptr<MusicState> getState(const std::string& stateName);
    std::vector<std::shared_ptr<MusicState>> getAllStates() const;
    
    void setActiveState(const std::string& stateName);
    std::shared_ptr<MusicState> getActiveState() const;
    
    // Transitions
    void addTransition(std::shared_ptr<StateTransition> transition);
    void removeTransition(const std::string& transitionName);
    std::shared_ptr<StateTransition> getTransition(const std::string& transitionName);
    std::vector<std::shared_ptr<StateTransition>> getAllTransitions() const;
    
    // Global parameters
    void addParameter(std::shared_ptr<Parameter> parameter);
    void removeParameter(const std::string& paramName);
    std::shared_ptr<Parameter> getParameter(const std::string& paramName);
    std::vector<std::shared_ptr<Parameter>> getAllParameters() const;
    
    // Events
    void triggerEvent(const std::string& eventName, const std::map<std::string, float>& data = {});
    void registerEvent(const std::string& eventName);
    void addEventListener(const std::string& eventName, EventSystem::EventCallback callback);
    
    // Transport control
    void play();
    void stop();
    void pause();
    bool isPlaying() const;
    
    void setTempo(float bpm);
    float getTempo() const;
    
    // Hardware interface
    bool hasHardwareInterface() const;
    std::shared_ptr<HardwareInterface> getHardwareInterface() const;
    void mapControllerToParameter(int controllerId, const std::string& parameterName);
    
    // Update function to be called regularly
    void update(float deltaTime);
    
private:
    std::shared_ptr<AudioEngine> audioEngine_;
    std::shared_ptr<Synthesizer> synthesizer_;
    std::shared_ptr<Sequencer> sequencer_;
    std::shared_ptr<HardwareInterface> hardwareInterface_;
    
    std::map<std::string, std::shared_ptr<MusicState>> states_;
    std::shared_ptr<MusicState> activeState_;
    
    std::unique_ptr<TransitionManager> transitionManager_;
    std::unique_ptr<EventSystem> eventSystem_;
    
    std::map<std::string, std::shared_ptr<Parameter>> globalParameters_;
    
    bool isPlaying_;
    float tempo_;
    float currentBeat_;
    
    mutable std::mutex mutex_; // Make mutex mutable so it can be locked in const methods
    
    // Hardware control callbacks
    void onControlChange(int controllerId, float value);
    void onButtonPress(int buttonId, bool isPressed);
    void onPadPress(int padId, float pressure);
};

} // namespace AIMusicHardware