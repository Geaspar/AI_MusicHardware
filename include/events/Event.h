#pragma once

#include <string>
#include <memory>
#include <any>
#include <chrono>
#include <stdexcept>

namespace AIMusicHardware {

/**
 * @brief Base class for all events
 * 
 * Represents a discrete occurrence that can trigger immediate or scheduled actions.
 * Events can carry payload data and are used throughout the system for communication
 * between components.
 */
class Event {
public:
    using EventId = std::string;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    
    /**
     * @brief Constructor
     * 
     * @param id Unique identifier for the event type
     */
    Event(const EventId& id);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Event() = default;
    
    /**
     * @brief Get event type ID
     * 
     * @return EventId Event type identifier
     */
    EventId getId() const { return id_; }
    
    /**
     * @brief Get creation timestamp
     * 
     * @return double Time in seconds since program start
     */
    double getTimestamp() const;
    
    /**
     * @brief Get time point when event was created
     * 
     * @return TimePoint Creation time point
     */
    TimePoint getTimePoint() const { return timePoint_; }
    
    /**
     * @brief Set event payload data
     * 
     * @tparam T Payload data type
     * @param payload The data to attach
     */
    template<typename T>
    void setPayload(const T& payload) {
        payload_ = payload;
    }
    
    /**
     * @brief Get event payload data
     * 
     * @tparam T Expected payload type
     * @return T The payload data
     * @throws std::bad_any_cast if type doesn't match
     */
    template<typename T>
    T getPayload() const {
        try {
            return std::any_cast<T>(payload_);
        } catch (const std::bad_any_cast&) {
            throw std::runtime_error("Invalid payload type in event: " + id_);
        }
    }
    
    /**
     * @brief Check if event has payload data
     * 
     * @return true if has payload
     */
    bool hasPayload() const { return payload_.has_value(); }
    
    /**
     * @brief Create a copy of this event
     * 
     * @return std::unique_ptr<Event> Clone of this event
     */
    virtual std::unique_ptr<Event> clone() const;
    
private:
    EventId id_;          ///< Event type identifier
    TimePoint timePoint_; ///< Creation time point
    std::any payload_;    ///< Optional payload data
    
    // Static clock reference point for calculating seconds
    static const TimePoint startTime_;
};

/**
 * @brief Event for state changes in the system
 */
class StateChangeEvent : public Event {
public:
    /**
     * @brief Constructor
     * 
     * @param targetState Name of the target state
     */
    explicit StateChangeEvent(const std::string& targetState);
    
    /**
     * @brief Get target state
     * 
     * @return std::string Target state name
     */
    std::string getTargetState() const { return targetState_; }
    
    /**
     * @brief Clone this event
     * 
     * @return std::unique_ptr<Event> Clone of this event
     */
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string targetState_;
};

/**
 * @brief Event for musical pattern control
 */
class PatternEvent : public Event {
public:
    /**
     * @brief Pattern control actions
     */
    enum class Action {
        START,      ///< Start the pattern
        STOP,       ///< Stop the pattern
        PAUSE,      ///< Pause the pattern
        RESUME,     ///< Resume the pattern
        RESTART     ///< Restart from beginning
    };
    
    /**
     * @brief Constructor
     * 
     * @param patternId Pattern identifier
     * @param action Action to perform
     */
    PatternEvent(const std::string& patternId, Action action);
    
    /**
     * @brief Get pattern ID
     * 
     * @return std::string Pattern identifier
     */
    std::string getPatternId() const { return patternId_; }
    
    /**
     * @brief Get action
     * 
     * @return Action The control action
     */
    Action getAction() const { return action_; }
    
    /**
     * @brief Clone this event
     * 
     * @return std::unique_ptr<Event> Clone of this event
     */
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string patternId_;
    Action action_;
};

/**
 * @brief Event for parameter value changes
 */
class ParameterEvent : public Event {
public:
    /**
     * @brief Constructor
     * 
     * @param parameterId Parameter identifier
     * @param value New parameter value
     */
    ParameterEvent(const std::string& parameterId, float value);
    
    /**
     * @brief Get parameter ID
     * 
     * @return std::string Parameter identifier
     */
    std::string getParameterId() const { return parameterId_; }
    
    /**
     * @brief Get parameter value
     * 
     * @return float Parameter value
     */
    float getValue() const { return value_; }
    
    /**
     * @brief Clone this event
     * 
     * @return std::unique_ptr<Event> Clone of this event
     */
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string parameterId_;
    float value_;
};

/**
 * @brief Event for IoT messages
 */
class IoTEvent : public Event {
public:
    /**
     * @brief Constructor
     * 
     * @param topic IoT topic
     * @param payload IoT message payload
     */
    IoTEvent(const std::string& topic, const std::string& payload);
    
    /**
     * @brief Get IoT topic
     * 
     * @return std::string Topic string
     */
    std::string getTopic() const { return topic_; }
    
    /**
     * @brief Get IoT payload
     * 
     * @return std::string Payload string
     */
    std::string getPayload() const { return payload_; }
    
    /**
     * @brief Clone this event
     * 
     * @return std::unique_ptr<Event> Clone of this event
     */
    std::unique_ptr<Event> clone() const override;
    
private:
    std::string topic_;
    std::string payload_;
};

} // namespace AIMusicHardware