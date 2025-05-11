#pragma once

#include "Event.h"
#include "EventListener.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <cstdint>
#include <tuple>

namespace AIMusicHardware {

/**
 * @brief Central event dispatcher
 * 
 * The EventBus connects event producers with event consumers.
 * It allows components to communicate without direct dependencies.
 */
class EventBus {
public:
    /**
     * @brief Get singleton instance
     * 
     * @return EventBus& Reference to singleton instance
     */
    static EventBus& getInstance();
    
    /**
     * @brief Register event listener
     * 
     * @param eventId Event type to listen for
     * @param listener Listener to register
     */
    void addEventListener(const Event::EventId& eventId, EventListener* listener);
    
    /**
     * @brief Register event callback
     * 
     * @param eventId Event type to listen for
     * @param callback Function to call when event occurs
     * @return EventListener* Pointer to created callback listener
     */
    EventListener* addEventListener(const Event::EventId& eventId, 
                                   std::function<void(const Event&)> callback);
    
    /**
     * @brief Remove event listener
     * 
     * @param eventId Event type
     * @param listener Listener to remove
     */
    void removeEventListener(const Event::EventId& eventId, EventListener* listener);
    
    /**
     * @brief Dispatch event immediately
     * 
     * @param event Event to dispatch
     */
    void dispatchEvent(const Event& event);
    
    /**
     * @brief Schedule event for future dispatch
     * 
     * @param event Event to schedule
     * @param delayInSeconds Delay in seconds
     * @return uint64_t Scheduled event ID (can be used to cancel)
     */
    uint64_t scheduleEvent(const Event& event, double delayInSeconds);
    
    /**
     * @brief Schedule event based on musical time
     * 
     * @param event Event to schedule
     * @param bar Target bar number
     * @param beat Target beat number
     * @param tick Target tick number
     * @return uint64_t Scheduled event ID (can be used to cancel)
     */
    uint64_t scheduleMusicalEvent(const Event& event, int bar, int beat, int tick = 0);
    
    /**
     * @brief Cancel scheduled event
     * 
     * @param eventId Scheduled event ID from scheduleEvent()
     * @return true if event was found and canceled
     */
    bool cancelScheduledEvent(uint64_t eventId);
    
    /**
     * @brief Process scheduled events
     * 
     * Call this regularly (e.g., in audio thread) to dispatch delayed events.
     * 
     * @param deltaTime Time since last update in seconds
     */
    void update(double deltaTime);
    
    /**
     * @brief Set musical time provider function
     * 
     * This function should return current musical time as (bar, beat, tick, tempo).
     * 
     * @param timeProvider Function that returns (bar, beat, tick, tempo)
     */
    void setTimeProvider(std::function<std::tuple<int, int, int, double>()> timeProvider);
    
private:
    // Private constructor for singleton
    EventBus();
    ~EventBus();
    
    // Prevent copying
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    // Registered listeners
    std::map<Event::EventId, std::vector<EventListener*>> listeners_;
    
    // Owned callback listeners
    std::vector<std::unique_ptr<EventCallback>> ownedCallbacks_;
    
    // Scheduled events
    struct ScheduledEvent {
        uint64_t id;
        std::unique_ptr<Event> event;
        
        // Time-based scheduling
        bool isTimeBased;
        double triggerTime;
        
        // Musical-time scheduling
        bool isMusicalTimeBased;
        int targetBar;
        int targetBeat;
        int targetTick;
    };
    
    std::vector<ScheduledEvent> scheduledEvents_;
    uint64_t nextScheduledEventId_ = 1;
    
    // Musical time provider
    std::function<std::tuple<int, int, int, double>()> timeProvider_;
    
    // Total elapsed time in seconds (for time-based scheduling)
    double totalTime_ = 0.0;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper functions
    bool compareMusicalTimes(int bar1, int beat1, int tick1, 
                           int bar2, int beat2, int tick2) const;
                           
    void removeCompletedEvents();
};

} // namespace AIMusicHardware