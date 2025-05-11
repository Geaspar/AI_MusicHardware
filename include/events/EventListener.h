#pragma once

#include "Event.h"

namespace AIMusicHardware {

/**
 * @brief Interface for event listeners
 * 
 * Components that need to receive and handle events should implement this interface.
 * Event listeners register with the EventBus to receive specific event types.
 */
class EventListener {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~EventListener() = default;
    
    /**
     * @brief Called when an event is dispatched
     * 
     * @param event The dispatched event
     */
    virtual void onEvent(const Event& event) = 0;
};

/**
 * @brief Function-based event listener
 * 
 * Allows using function objects as event listeners without implementing
 * the full EventListener interface.
 */
class EventCallback : public EventListener {
public:
    using Callback = std::function<void(const Event&)>;
    
    /**
     * @brief Constructor
     * 
     * @param callback Function to call when event is received
     */
    explicit EventCallback(Callback callback) : callback_(callback) {}
    
    /**
     * @brief Event handler
     * 
     * @param event The dispatched event
     */
    void onEvent(const Event& event) override {
        if (callback_) {
            callback_(event);
        }
    }
    
private:
    Callback callback_;
};

} // namespace AIMusicHardware