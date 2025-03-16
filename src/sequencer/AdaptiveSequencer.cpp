#include "../../include/sequencer/AdaptiveSequencer.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

//------------------------------------------------------------------------------
// Parameter Implementation
//------------------------------------------------------------------------------

Parameter::Parameter(const std::string& name, float defaultValue, float minValue, float maxValue)
    : name_(name),
      value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      bipolar_(false) {
}

std::string Parameter::getName() const {
    return name_;
}

float Parameter::getValue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

void Parameter::setValue(float value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clamp the value to the valid range
    float newValue = std::max(minValue_, std::min(value, maxValue_));
    
    // Store the old value for the callback
    float oldValue = value_;
    
    // Update the value
    value_ = newValue;
    
    // Call the callback if it exists
    if (changeCallback_) {
        changeCallback_(*this, oldValue, newValue);
    }
}

float Parameter::getMin() const {
    return minValue_;
}

float Parameter::getMax() const {
    return maxValue_;
}

float Parameter::getDefault() const {
    return defaultValue_;
}

bool Parameter::isBipolar() const {
    return bipolar_;
}

void Parameter::setBipolar(bool bipolar) {
    bipolar_ = bipolar;
}

void Parameter::setChangeCallback(ChangeCallback callback) {
    changeCallback_ = callback;
}

//------------------------------------------------------------------------------
// EventSystem Implementation
//------------------------------------------------------------------------------

EventSystem::EventSystem() {
}

void EventSystem::registerEvent(const std::string& eventName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Only register if it doesn't already exist
    if (eventListeners_.find(eventName) == eventListeners_.end()) {
        eventListeners_[eventName] = std::vector<EventCallback>();
    }
}

void EventSystem::unregisterEvent(const std::string& eventName) {
    std::lock_guard<std::mutex> lock(mutex_);
    eventListeners_.erase(eventName);
}

bool EventSystem::isEventRegistered(const std::string& eventName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return eventListeners_.find(eventName) != eventListeners_.end();
}

void EventSystem::addListener(const std::string& eventName, EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Register the event if it doesn't exist
    if (eventListeners_.find(eventName) == eventListeners_.end()) {
        eventListeners_[eventName] = std::vector<EventCallback>();
    }
    
    // Add the callback
    eventListeners_[eventName].push_back(callback);
}

void EventSystem::removeListener(const std::string& eventName, void* owner) {
    // Note: This is a simplified implementation. In a real system, you'd need a way to 
    // identify which callbacks belong to which owner, which is beyond the scope of this example.
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (eventListeners_.find(eventName) != eventListeners_.end()) {
        // Clear all callbacks for this event (simplified)
        eventListeners_[eventName].clear();
    }
}

void EventSystem::triggerEvent(const std::string& eventName, const std::map<std::string, float>& eventData) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (eventListeners_.find(eventName) != eventListeners_.end()) {
        // Call all registered callbacks
        for (const auto& callback : eventListeners_[eventName]) {
            callback(eventName, eventData);
        }
    }
}

void EventSystem::scheduleEvent(const std::string& eventName, float delayInBeats, 
                               const std::map<std::string, float>& eventData) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create a scheduled event
    ScheduledEvent event;
    event.eventName = eventName;
    event.triggerBeat = delayInBeats;  // This would be currentBeat + delayInBeats in a real implementation
    event.eventData = eventData;
    
    // Add it to the list
    scheduledEvents_.push_back(event);
}

void EventSystem::cancelScheduledEvents(const std::string& eventName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove all scheduled events with the given name
    scheduledEvents_.erase(
        std::remove_if(scheduledEvents_.begin(), scheduledEvents_.end(),
            [&eventName](const ScheduledEvent& event) {
                return event.eventName == eventName;
            }),
        scheduledEvents_.end()
    );
}

void EventSystem::processTick(float beatPosition) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find events that should be triggered
    std::vector<ScheduledEvent> eventsToTrigger;
    
    // Copy events that should be triggered and remove them from the list
    scheduledEvents_.erase(
        std::remove_if(scheduledEvents_.begin(), scheduledEvents_.end(),
            [beatPosition, &eventsToTrigger](const ScheduledEvent& event) {
                if (event.triggerBeat <= beatPosition) {
                    eventsToTrigger.push_back(event);
                    return true;
                }
                return false;
            }),
        scheduledEvents_.end()
    );
    
    // Trigger the events (outside the loop to avoid recursive locks)
    for (const auto& event : eventsToTrigger) {
        // Release the lock while triggering
        mutex_.unlock();
        triggerEvent(event.eventName, event.eventData);
        mutex_.lock();
    }
}

//------------------------------------------------------------------------------
// TrackLayer Implementation
//------------------------------------------------------------------------------

TrackLayer::TrackLayer(const std::string& name)
    : name_(name),
      volume_(1.0f),
      muted_(false),
      solo_(false) {
}

TrackLayer::~TrackLayer() {
}

void TrackLayer::setPattern(const std::vector<MidiEvent>& pattern) {
    pattern_ = pattern;
}

const std::vector<MidiEvent>& TrackLayer::getPattern() const {
    return pattern_;
}

void TrackLayer::setVolume(float volume) {
    volume_ = std::max(0.0f, std::min(volume, 1.0f));
}

float TrackLayer::getVolume() const {
    return volume_;
}

void TrackLayer::setMuted(bool muted) {
    muted_ = muted;
}

bool TrackLayer::isMuted() const {
    return muted_;
}

void TrackLayer::setSolo(bool solo) {
    solo_ = solo;
}

bool TrackLayer::isSolo() const {
    return solo_;
}

const std::string& TrackLayer::getName() const {
    return name_;
}

//------------------------------------------------------------------------------
// MixSnapshot Implementation
//------------------------------------------------------------------------------

MixSnapshot::MixSnapshot(const std::string& name)
    : name_(name) {
}

MixSnapshot::~MixSnapshot() {
}

void MixSnapshot::setLayerVolume(const std::string& layerName, float volume) {
    layerVolumes_[layerName] = std::max(0.0f, std::min(volume, 1.0f));
}

float MixSnapshot::getLayerVolume(const std::string& layerName) const {
    auto it = layerVolumes_.find(layerName);
    if (it != layerVolumes_.end()) {
        return it->second;
    }
    return 1.0f;  // Default volume
}

void MixSnapshot::setLayerMuted(const std::string& layerName, bool muted) {
    layerMutes_[layerName] = muted;
}

bool MixSnapshot::isLayerMuted(const std::string& layerName) const {
    auto it = layerMutes_.find(layerName);
    if (it != layerMutes_.end()) {
        return it->second;
    }
    return false;  // Default not muted
}

const std::string& MixSnapshot::getName() const {
    return name_;
}

std::map<std::string, float> MixSnapshot::getVolumeMap() const {
    return layerVolumes_;
}

std::map<std::string, bool> MixSnapshot::getMuteMap() const {
    return layerMutes_;
}

//------------------------------------------------------------------------------
// MusicState Implementation
//------------------------------------------------------------------------------

MusicState::MusicState(const std::string& name)
    : name_(name),
      tempo_(120.0f),
      timeSignatureNumerator_(4),
      timeSignatureDenominator_(4),
      loopLengthBars_(4) {
}

MusicState::~MusicState() {
}

void MusicState::addLayer(std::shared_ptr<TrackLayer> layer) {
    layers_[layer->getName()] = layer;
}

void MusicState::removeLayer(const std::string& layerName) {
    layers_.erase(layerName);
}

std::shared_ptr<TrackLayer> MusicState::getLayer(const std::string& layerName) {
    auto it = layers_.find(layerName);
    if (it != layers_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<TrackLayer>> MusicState::getAllLayers() const {
    std::vector<std::shared_ptr<TrackLayer>> result;
    for (const auto& pair : layers_) {
        result.push_back(pair.second);
    }
    return result;
}

void MusicState::addSnapshot(std::shared_ptr<MixSnapshot> snapshot) {
    snapshots_[snapshot->getName()] = snapshot;
}

void MusicState::removeSnapshot(const std::string& snapshotName) {
    snapshots_.erase(snapshotName);
}

std::shared_ptr<MixSnapshot> MusicState::getSnapshot(const std::string& snapshotName) {
    auto it = snapshots_.find(snapshotName);
    if (it != snapshots_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<MixSnapshot>> MusicState::getAllSnapshots() const {
    std::vector<std::shared_ptr<MixSnapshot>> result;
    for (const auto& pair : snapshots_) {
        result.push_back(pair.second);
    }
    return result;
}

void MusicState::setActiveSnapshot(const std::string& snapshotName) {
    auto it = snapshots_.find(snapshotName);
    if (it != snapshots_.end()) {
        activeSnapshot_ = it->second;
    }
}

std::shared_ptr<MixSnapshot> MusicState::getActiveSnapshot() const {
    return activeSnapshot_;
}

const std::string& MusicState::getName() const {
    return name_;
}

void MusicState::setTempo(float bpm) {
    tempo_ = std::max(20.0f, std::min(bpm, 300.0f));
}

float MusicState::getTempo() const {
    return tempo_;
}

void MusicState::setTimeSignature(int numerator, int denominator) {
    timeSignatureNumerator_ = numerator;
    timeSignatureDenominator_ = denominator;
}

std::pair<int, int> MusicState::getTimeSignature() const {
    return {timeSignatureNumerator_, timeSignatureDenominator_};
}

void MusicState::setLoopLength(int bars) {
    loopLengthBars_ = std::max(1, bars);
}

int MusicState::getLoopLength() const {
    return loopLengthBars_;
}

void MusicState::addParameter(std::shared_ptr<Parameter> parameter) {
    parameters_[parameter->getName()] = parameter;
}

void MusicState::removeParameter(const std::string& paramName) {
    parameters_.erase(paramName);
}

std::shared_ptr<Parameter> MusicState::getParameter(const std::string& paramName) {
    auto it = parameters_.find(paramName);
    if (it != parameters_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Parameter>> MusicState::getAllParameters() const {
    std::vector<std::shared_ptr<Parameter>> result;
    for (const auto& pair : parameters_) {
        result.push_back(pair.second);
    }
    return result;
}

//------------------------------------------------------------------------------
// StateTransition Implementation
//------------------------------------------------------------------------------

StateTransition::StateTransition(const std::string& name, std::shared_ptr<MusicState> fromState, 
                               std::shared_ptr<MusicState> toState)
    : name_(name),
      fromState_(fromState),
      toState_(toState),
      type_(TransitionType::Immediate),
      durationInBeats_(0.0f),
      syncBarCount_(1),
      syncBeatCount_(1) {
}

StateTransition::~StateTransition() {
}

void StateTransition::setTransitionType(TransitionType type) {
    type_ = type;
}

StateTransition::TransitionType StateTransition::getTransitionType() const {
    return type_;
}

void StateTransition::setDuration(float durationInBeats) {
    durationInBeats_ = std::max(0.0f, durationInBeats);
}

float StateTransition::getDuration() const {
    return durationInBeats_;
}

void StateTransition::setSyncPoint(int bars, int beats) {
    syncBarCount_ = std::max(1, bars);
    syncBeatCount_ = std::max(1, beats);
}

std::pair<int, int> StateTransition::getSyncPoint() const {
    return {syncBarCount_, syncBeatCount_};
}

const std::string& StateTransition::getName() const {
    return name_;
}

std::shared_ptr<MusicState> StateTransition::getFromState() const {
    return fromState_.lock();
}

std::shared_ptr<MusicState> StateTransition::getToState() const {
    return toState_.lock();
}

void StateTransition::setCondition(const std::string& paramName, float threshold, bool greaterThan) {
    auto fromState = fromState_.lock();
    if (!fromState) return;
    
    auto param = fromState->getParameter(paramName);
    if (!param) return;
    
    Condition condition;
    condition.parameter = param;
    condition.threshold = threshold;
    condition.greaterThan = greaterThan;
    
    conditions_[paramName] = condition;
}

void StateTransition::clearCondition(const std::string& paramName) {
    conditions_.erase(paramName);
}

bool StateTransition::checkConditions() const {
    // Check each condition
    for (const auto& pair : conditions_) {
        const auto& condition = pair.second;
        
        auto param = condition.parameter.lock();
        if (!param) continue;
        
        float value = param->getValue();
        
        if (condition.greaterThan) {
            if (value <= condition.threshold) {
                return false;
            }
        } else {
            if (value >= condition.threshold) {
                return false;
            }
        }
    }
    
    // All conditions passed
    return true;
}

//------------------------------------------------------------------------------
// TransitionManager Implementation
//------------------------------------------------------------------------------

TransitionManager::TransitionManager()
    : transitionProgress_(0.0f) {
}

TransitionManager::~TransitionManager() {
}

void TransitionManager::addTransition(std::shared_ptr<StateTransition> transition) {
    std::lock_guard<std::mutex> lock(mutex_);
    transitions_[transition->getName()] = transition;
}

void TransitionManager::removeTransition(const std::string& transitionName) {
    std::lock_guard<std::mutex> lock(mutex_);
    transitions_.erase(transitionName);
}

std::shared_ptr<StateTransition> TransitionManager::getTransition(const std::string& transitionName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transitions_.find(transitionName);
    if (it != transitions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<StateTransition>> TransitionManager::getAllTransitions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<StateTransition>> result;
    for (const auto& pair : transitions_) {
        result.push_back(pair.second);
    }
    return result;
}

void TransitionManager::startTransition(const std::string& transitionName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = transitions_.find(transitionName);
    if (it != transitions_.end()) {
        activeTransition_ = it->second;
        transitionProgress_ = 0.0f;
    }
}

void TransitionManager::cancelTransition() {
    std::lock_guard<std::mutex> lock(mutex_);
    activeTransition_.reset();
    transitionProgress_ = 0.0f;
}

bool TransitionManager::isTransitioning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeTransition_ != nullptr;
}

void TransitionManager::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!activeTransition_) {
        return;
    }
    
    // Update progress based on the transition type
    switch (activeTransition_->getTransitionType()) {
        case StateTransition::TransitionType::Immediate:
            // Immediate transitions complete instantly
            transitionProgress_ = 1.0f;
            break;
            
        case StateTransition::TransitionType::Crossfade:
        case StateTransition::TransitionType::Morph:
            // Gradual transitions progress over time
            if (activeTransition_->getDuration() > 0.0f) {
                transitionProgress_ += deltaTime / activeTransition_->getDuration();
            } else {
                transitionProgress_ = 1.0f;
            }
            break;
            
        case StateTransition::TransitionType::MusicalSync:
            // Musical sync transitions wait for sync points
            // In a real implementation, this would check if we've reached the sync point
            // For now, we just advance to completion
            transitionProgress_ = 1.0f;
            break;
    }
    
    // Clamp progress to [0,1]
    transitionProgress_ = std::max(0.0f, std::min(transitionProgress_, 1.0f));
    
    // If we've reached the end of the transition, clear it
    if (transitionProgress_ >= 1.0f) {
        activeTransition_.reset();
        transitionProgress_ = 0.0f;
    }
}

//------------------------------------------------------------------------------
// AdaptiveSequencer Implementation
//------------------------------------------------------------------------------

AdaptiveSequencer::AdaptiveSequencer()
    : isPlaying_(false),
      tempo_(120.0f),
      currentBeat_(0.0f) {
    
    eventSystem_ = std::make_unique<EventSystem>();
    transitionManager_ = std::make_unique<TransitionManager>();
}

AdaptiveSequencer::~AdaptiveSequencer() {
    shutdown();
}

bool AdaptiveSequencer::initialize(std::shared_ptr<AudioEngine> audioEngine, 
                                  std::shared_ptr<Synthesizer> synthesizer,
                                  std::shared_ptr<HardwareInterface> hardwareInterface) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store the dependencies
    audioEngine_ = audioEngine;
    synthesizer_ = synthesizer;
    hardwareInterface_ = hardwareInterface;
    
    // Create the sequencer
    sequencer_ = std::make_shared<Sequencer>();
    
    // Set up hardware interface callbacks if available
    if (hardwareInterface_) {
        hardwareInterface_->setControlChangeCallback(
            [this](int controllerId, float value) {
                onControlChange(controllerId, value);
            }
        );
        
        hardwareInterface_->setButtonCallback(
            [this](int buttonId, bool isPressed) {
                onButtonPress(buttonId, isPressed);
            }
        );
        
        hardwareInterface_->setPadCallback(
            [this](int padId, float pressure) {
                onPadPress(padId, pressure);
            }
        );
    }
    
    return true;
}

void AdaptiveSequencer::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Stop playback
    stop();
    
    // Clean up resources
    states_.clear();
    activeState_.reset();
    globalParameters_.clear();
    
    // Release dependencies
    audioEngine_.reset();
    synthesizer_.reset();
    sequencer_.reset();
    hardwareInterface_.reset();
}

void AdaptiveSequencer::addState(std::shared_ptr<MusicState> state) {
    std::lock_guard<std::mutex> lock(mutex_);
    states_[state->getName()] = state;
}

void AdaptiveSequencer::removeState(const std::string& stateName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // If this is the active state, clear it
    if (activeState_ && activeState_->getName() == stateName) {
        activeState_.reset();
    }
    
    states_.erase(stateName);
}

std::shared_ptr<MusicState> AdaptiveSequencer::getState(const std::string& stateName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = states_.find(stateName);
    if (it != states_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<MusicState>> AdaptiveSequencer::getAllStates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<MusicState>> result;
    for (const auto& pair : states_) {
        result.push_back(pair.second);
    }
    return result;
}

void AdaptiveSequencer::setActiveState(const std::string& stateName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = states_.find(stateName);
    if (it != states_.end()) {
        // Store the old state for transition
        std::shared_ptr<MusicState> oldState = activeState_;
        
        // Set the new state
        activeState_ = it->second;
        
        // Update tempo if necessary
        if (activeState_) {
            tempo_ = activeState_->getTempo();
        }
        
        // Trigger a state change event
        std::map<std::string, float> eventData;
        if (oldState) {
            eventData["oldState"] = 1.0f;  // Just a placeholder, in a real system we'd use a better way to pass the state info
        }
        eventData["newState"] = 1.0f;  // Just a placeholder
        
        eventSystem_->triggerEvent("stateChanged", eventData);
    }
}

std::shared_ptr<MusicState> AdaptiveSequencer::getActiveState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeState_;
}

void AdaptiveSequencer::addTransition(std::shared_ptr<StateTransition> transition) {
    transitionManager_->addTransition(transition);
}

void AdaptiveSequencer::removeTransition(const std::string& transitionName) {
    transitionManager_->removeTransition(transitionName);
}

std::shared_ptr<StateTransition> AdaptiveSequencer::getTransition(const std::string& transitionName) {
    return transitionManager_->getTransition(transitionName);
}

std::vector<std::shared_ptr<StateTransition>> AdaptiveSequencer::getAllTransitions() const {
    return transitionManager_->getAllTransitions();
}

void AdaptiveSequencer::addParameter(std::shared_ptr<Parameter> parameter) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalParameters_[parameter->getName()] = parameter;
}

void AdaptiveSequencer::removeParameter(const std::string& paramName) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalParameters_.erase(paramName);
}

std::shared_ptr<Parameter> AdaptiveSequencer::getParameter(const std::string& paramName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = globalParameters_.find(paramName);
    if (it != globalParameters_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Parameter>> AdaptiveSequencer::getAllParameters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Parameter>> result;
    for (const auto& pair : globalParameters_) {
        result.push_back(pair.second);
    }
    return result;
}

void AdaptiveSequencer::triggerEvent(const std::string& eventName, const std::map<std::string, float>& data) {
    eventSystem_->triggerEvent(eventName, data);
}

void AdaptiveSequencer::registerEvent(const std::string& eventName) {
    eventSystem_->registerEvent(eventName);
}

void AdaptiveSequencer::addEventListener(const std::string& eventName, EventSystem::EventCallback callback) {
    eventSystem_->addListener(eventName, callback);
}

void AdaptiveSequencer::play() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isPlaying_) {
        isPlaying_ = true;
        
        // Update the sequencer with the active state's pattern
        if (activeState_) {
            // In a real implementation, we'd configure the sequencer with the patterns from the active state
        }
        
        // Start the sequencer
        // sequencer_->play();
        
        // Trigger event
        eventSystem_->triggerEvent("play");
    }
}

void AdaptiveSequencer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isPlaying_) {
        isPlaying_ = false;
        
        // Stop the sequencer
        // sequencer_->stop();
        
        // Reset the beat counter
        currentBeat_ = 0.0f;
        
        // Trigger event
        eventSystem_->triggerEvent("stop");
    }
}

void AdaptiveSequencer::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isPlaying_) {
        isPlaying_ = false;
        
        // Pause the sequencer
        // sequencer_->pause();
        
        // Trigger event
        eventSystem_->triggerEvent("pause");
    }
}

bool AdaptiveSequencer::isPlaying() const {
    return isPlaying_;
}

void AdaptiveSequencer::setTempo(float bpm) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    tempo_ = std::max(20.0f, std::min(bpm, 300.0f));
    
    // Update the active state's tempo
    if (activeState_) {
        activeState_->setTempo(tempo_);
    }
}

float AdaptiveSequencer::getTempo() const {
    return tempo_;
}

bool AdaptiveSequencer::hasHardwareInterface() const {
    return hardwareInterface_ != nullptr;
}

std::shared_ptr<HardwareInterface> AdaptiveSequencer::getHardwareInterface() const {
    return hardwareInterface_;
}

void AdaptiveSequencer::mapControllerToParameter(int controllerId, const std::string& parameterName) {
    if (hardwareInterface_) {
        hardwareInterface_->mapControllerToParameter(controllerId, parameterName);
    }
}

void AdaptiveSequencer::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isPlaying_) {
        return;
    }
    
    // Update the beat position
    // In a real implementation, this would be synchronized with the audio engine
    float beatsPerSecond = tempo_ / 60.0f;
    float beatAdvance = beatsPerSecond * deltaTime;
    currentBeat_ += beatAdvance;
    
    // Process scheduled events
    eventSystem_->processTick(currentBeat_);
    
    // Update transitions
    transitionManager_->update(deltaTime);
    
    // Check for auto-transitions
    if (!transitionManager_->isTransitioning()) {
        for (const auto& pair : transitionManager_->getAllTransitions()) {
            auto transition = pair;
            
            // Skip transitions that don't start from the current state
            if (transition->getFromState() != activeState_) {
                continue;
            }
            
            // Check if this transition's conditions are met
            if (transition->checkConditions()) {
                transitionManager_->startTransition(transition->getName());
                break;
            }
        }
    }
}

void AdaptiveSequencer::onControlChange(int controllerId, float value) {
    // Find the parameter mapped to this controller
    if (hardwareInterface_) {
        std::string paramName = hardwareInterface_->getMappedParameter(controllerId);
        if (!paramName.empty()) {
            // Try to find the parameter
            auto param = getParameter(paramName);
            if (param) {
                // Update the parameter value
                param->setValue(value);
            } else if (activeState_) {
                // If not found in global parameters, check the active state
                param = activeState_->getParameter(paramName);
                if (param) {
                    param->setValue(value);
                }
            }
        }
    }
}

void AdaptiveSequencer::onButtonPress(int buttonId, bool isPressed) {
    if (!isPressed) {
        return;  // Only respond to button presses, not releases
    }
    
    // In a real implementation, we'd check a mapping table to see what action to take
    // For now, we'll just show a placeholder implementation
    
    // Trigger an event for the button press
    std::map<std::string, float> eventData;
    eventData["buttonId"] = static_cast<float>(buttonId);
    
    eventSystem_->triggerEvent("buttonPressed", eventData);
}

void AdaptiveSequencer::onPadPress(int padId, float pressure) {
    // Trigger an event for the pad press
    std::map<std::string, float> eventData;
    eventData["padId"] = static_cast<float>(padId);
    eventData["pressure"] = pressure;
    
    eventSystem_->triggerEvent("padPressed", eventData);
}

} // namespace AIMusicHardware