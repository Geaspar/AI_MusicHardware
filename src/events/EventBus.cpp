#include "../../include/events/EventBus.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

// Singleton implementation
EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

EventBus::EventBus() : totalTime_(0.0) {
}

EventBus::~EventBus() {
    // Clean up owned callback listeners
    ownedCallbacks_.clear();
    
    // Clean up scheduled events
    scheduledEvents_.clear();
}

void EventBus::addEventListener(const Event::EventId& eventId, EventListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if listener already registered
    auto& listenersList = listeners_[eventId];
    if (std::find(listenersList.begin(), listenersList.end(), listener) == listenersList.end()) {
        // Add listener if not already registered
        listenersList.push_back(listener);
    }
}

EventListener* EventBus::addEventListener(const Event::EventId& eventId, 
                                         std::function<void(const Event&)> callback) {
    if (!callback) return nullptr;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create owned callback listener
    auto callbackListener = std::make_unique<EventCallback>(callback);
    EventListener* listenerPtr = callbackListener.get();
    
    // Register listener
    listeners_[eventId].push_back(listenerPtr);
    
    // Store ownership of callback listener
    ownedCallbacks_.push_back(std::move(callbackListener));
    
    return listenerPtr;
}

void EventBus::removeEventListener(const Event::EventId& eventId, EventListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find and remove listener
    auto it = listeners_.find(eventId);
    if (it != listeners_.end()) {
        auto& listenersList = it->second;
        listenersList.erase(
            std::remove(listenersList.begin(), listenersList.end(), listener),
            listenersList.end());
    }
    
    // Check if the listener is one of our owned callbacks
    auto ownedIt = std::find_if(ownedCallbacks_.begin(), ownedCallbacks_.end(),
                             [listener](const std::unique_ptr<EventCallback>& callback) {
                                 return callback.get() == listener;
                             });
                             
    if (ownedIt != ownedCallbacks_.end()) {
        // Remove owned callback
        ownedCallbacks_.erase(ownedIt);
    }
}

void EventBus::dispatchEvent(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get event ID
    const auto& eventId = event.getId();
    
    // Dispatch to all registered listeners
    auto it = listeners_.find(eventId);
    if (it != listeners_.end()) {
        // Make a copy of listeners to avoid issues if listeners modify the list
        auto listenersCopy = it->second;
        
        // Release lock during notification
        mutex_.unlock();
        
        // Notify all listeners
        for (auto listener : listenersCopy) {
            if (listener) {
                try {
                    listener->onEvent(event);
                } catch (const std::exception& e) {
                    std::cerr << "Exception in event listener: " << e.what() << std::endl;
                }
            }
        }
        
        // Re-acquire lock
        mutex_.lock();
    }
}

uint64_t EventBus::scheduleEvent(const Event& event, double delayInSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create scheduled event
    ScheduledEvent scheduledEvent;
    scheduledEvent.id = nextScheduledEventId_++;
    scheduledEvent.event = event.clone();
    scheduledEvent.isTimeBased = true;
    scheduledEvent.isMusicalTimeBased = false;
    scheduledEvent.triggerTime = totalTime_ + delayInSeconds;
    
    // Add to scheduled events list
    scheduledEvents_.push_back(std::move(scheduledEvent));
    
    return scheduledEvent.id;
}

uint64_t EventBus::scheduleMusicalEvent(const Event& event, int bar, int beat, int tick) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create scheduled event
    ScheduledEvent scheduledEvent;
    scheduledEvent.id = nextScheduledEventId_++;
    scheduledEvent.event = event.clone();
    scheduledEvent.isTimeBased = false;
    scheduledEvent.isMusicalTimeBased = true;
    scheduledEvent.targetBar = bar;
    scheduledEvent.targetBeat = beat;
    scheduledEvent.targetTick = tick;
    
    // Add to scheduled events list
    scheduledEvents_.push_back(std::move(scheduledEvent));
    
    return scheduledEvent.id;
}

bool EventBus::cancelScheduledEvent(uint64_t eventId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find scheduled event
    auto it = std::find_if(scheduledEvents_.begin(), scheduledEvents_.end(),
                         [eventId](const ScheduledEvent& scheduledEvent) {
                             return scheduledEvent.id == eventId;
                         });
                         
    if (it != scheduledEvents_.end()) {
        // Remove scheduled event
        scheduledEvents_.erase(it);
        return true;
    }
    
    return false;
}

void EventBus::update(double deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update total time
    totalTime_ += deltaTime;
    
    // Process time-based scheduled events
    for (auto& scheduledEvent : scheduledEvents_) {
        if (scheduledEvent.isTimeBased && totalTime_ >= scheduledEvent.triggerTime) {
            // Time to trigger this event
            if (scheduledEvent.event) {
                // Copy event for dispatching (we'll remove it after)
                std::unique_ptr<Event> eventCopy = scheduledEvent.event->clone();
                
                // Release lock during dispatch
                mutex_.unlock();
                
                // Dispatch event
                dispatchEvent(*eventCopy);
                
                // Re-acquire lock
                mutex_.lock();
            }
            
            // Mark for removal
            scheduledEvent.event.reset();
        }
    }
    
    // Process musical-time-based scheduled events
    if (timeProvider_) {
        // Get current musical time
        auto [currentBar, currentBeat, currentTick, tempo] = timeProvider_();
        
        for (auto& scheduledEvent : scheduledEvents_) {
            if (scheduledEvent.isMusicalTimeBased) {
                // Check if we've reached the target musical time
                if (compareMusicalTimes(currentBar, currentBeat, currentTick,
                                     scheduledEvent.targetBar, scheduledEvent.targetBeat, scheduledEvent.targetTick)) {
                    // Time to trigger this event
                    if (scheduledEvent.event) {
                        // Copy event for dispatching (we'll remove it after)
                        std::unique_ptr<Event> eventCopy = scheduledEvent.event->clone();
                        
                        // Release lock during dispatch
                        mutex_.unlock();
                        
                        // Dispatch event
                        dispatchEvent(*eventCopy);
                        
                        // Re-acquire lock
                        mutex_.lock();
                    }
                    
                    // Mark for removal
                    scheduledEvent.event.reset();
                }
            }
        }
    }
    
    // Remove completed events
    removeCompletedEvents();
}

void EventBus::setTimeProvider(std::function<std::tuple<int, int, int, double>()> timeProvider) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeProvider_ = timeProvider;
}

bool EventBus::compareMusicalTimes(int bar1, int beat1, int tick1, 
                                 int bar2, int beat2, int tick2) const {
    // Compare musical times (bar, beat, tick)
    // Returns true if time1 >= time2
    
    if (bar1 > bar2) return true;
    if (bar1 < bar2) return false;
    
    // Same bar
    if (beat1 > beat2) return true;
    if (beat1 < beat2) return false;
    
    // Same bar and beat
    return tick1 >= tick2;
}

void EventBus::removeCompletedEvents() {
    // Remove events that have been triggered (event == nullptr)
    scheduledEvents_.erase(
        std::remove_if(scheduledEvents_.begin(), scheduledEvents_.end(),
                     [](const ScheduledEvent& scheduledEvent) {
                         return !scheduledEvent.event;
                     }),
        scheduledEvents_.end());
}

} // namespace AIMusicHardware