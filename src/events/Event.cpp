#include "../../include/events/Event.h"

namespace AIMusicHardware {

// Static initialization of start time
const Event::TimePoint Event::startTime_ = std::chrono::steady_clock::now();

Event::Event(const EventId& id)
    : id_(id),
      timePoint_(std::chrono::steady_clock::now()) {
}

double Event::getTimestamp() const {
    auto duration = timePoint_ - startTime_;
    return std::chrono::duration<double>(duration).count();
}

std::unique_ptr<Event> Event::clone() const {
    auto clonedEvent = std::make_unique<Event>(id_);
    if (hasPayload()) {
        clonedEvent->payload_ = payload_;
    }
    return clonedEvent;
}

// StateChangeEvent implementation
StateChangeEvent::StateChangeEvent(const std::string& targetState)
    : Event("state_change"),
      targetState_(targetState) {
    // Store target state in payload for generic access
    setPayload(targetState);
}

std::unique_ptr<Event> StateChangeEvent::clone() const {
    return std::make_unique<StateChangeEvent>(targetState_);
}

// PatternEvent implementation
PatternEvent::PatternEvent(const std::string& patternId, Action action)
    : Event("pattern_control"),
      patternId_(patternId),
      action_(action) {
    // Store pattern ID in payload for generic access
    setPayload(patternId);
}

std::unique_ptr<Event> PatternEvent::clone() const {
    return std::make_unique<PatternEvent>(patternId_, action_);
}

// ParameterEvent implementation
ParameterEvent::ParameterEvent(const std::string& parameterId, float value)
    : Event("parameter_change"),
      parameterId_(parameterId),
      value_(value) {
    // Store value in payload for generic access
    setPayload(value);
}

std::unique_ptr<Event> ParameterEvent::clone() const {
    return std::make_unique<ParameterEvent>(parameterId_, value_);
}

// IoTEvent implementation
IoTEvent::IoTEvent(const std::string& topic, const std::string& payload)
    : Event("iot_message"),
      topic_(topic),
      payload_(payload) {
    // Store payload as string for generic access
    setPayload(payload);
}

std::unique_ptr<Event> IoTEvent::clone() const {
    return std::make_unique<IoTEvent>(topic_, payload_);
}

} // namespace AIMusicHardware