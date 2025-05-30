#pragma once

#include "parameters/Parameter.h"
#include <atomic>
#include <array>
#include <functional>
#include <memory>

namespace AIMusicHardware {

/**
 * @brief Thread-safe queue for parameter updates
 * 
 * Lock-free single-producer single-consumer (SPSC) queue
 * for efficient communication between UI and audio threads.
 * Inspired by Vital's approach using moodycamel::ConcurrentQueue
 * but implemented with a simpler ring buffer for our use case.
 */
template<size_t Capacity = 1024>
class ParameterUpdateQueue {
public:
    /**
     * @brief Source of parameter change
     */
    enum class ChangeSource {
        UI,
        MIDI,
        IoT,
        Automation,
        Preset,
        Internal
    };

    /**
     * @brief Parameter change event
     */
    struct ParameterChange {
        Parameter::ParameterId id;
        float value;
        ChangeSource source;
        uint64_t timestamp; // For timing analysis
    };

    ParameterUpdateQueue() : writeIndex_(0), readIndex_(0) {}

    /**
     * @brief Push a parameter update (producer side)
     * @param change The parameter change to push
     * @return true if successful, false if queue is full
     */
    bool push(const ParameterChange& change) {
        size_t currentWrite = writeIndex_.load(std::memory_order_relaxed);
        size_t nextWrite = (currentWrite + 1) % Capacity;
        
        // Check if queue is full
        if (nextWrite == readIndex_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }
        
        // Write data
        buffer_[currentWrite] = change;
        
        // Update write index
        writeIndex_.store(nextWrite, std::memory_order_release);
        
        return true;
    }

    /**
     * @brief Pop a parameter update (consumer side)
     * @param change Output parameter for the change
     * @return true if successful, false if queue is empty
     */
    bool pop(ParameterChange& change) {
        size_t currentRead = readIndex_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }
        
        // Read data
        change = buffer_[currentRead];
        
        // Update read index
        size_t nextRead = (currentRead + 1) % Capacity;
        readIndex_.store(nextRead, std::memory_order_release);
        
        return true;
    }

    /**
     * @brief Check if queue is empty
     * @return true if empty
     */
    bool empty() const {
        return readIndex_.load(std::memory_order_acquire) == 
               writeIndex_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get approximate size (may be inaccurate during concurrent access)
     * @return Approximate number of items in queue
     */
    size_t size() const {
        size_t write = writeIndex_.load(std::memory_order_acquire);
        size_t read = readIndex_.load(std::memory_order_acquire);
        
        if (write >= read) {
            return write - read;
        } else {
            return Capacity - read + write;
        }
    }

    /**
     * @brief Clear the queue (not thread-safe, call only when no concurrent access)
     */
    void clear() {
        readIndex_.store(0, std::memory_order_release);
        writeIndex_.store(0, std::memory_order_release);
    }

private:
    alignas(64) std::atomic<size_t> writeIndex_; // Cache line aligned
    alignas(64) std::atomic<size_t> readIndex_;  // Cache line aligned
    std::array<ParameterChange, Capacity> buffer_;
};

/**
 * @brief Bidirectional parameter update system
 * 
 * Manages thread-safe communication between UI and audio threads
 * with separate queues for each direction.
 */
class ParameterUpdateSystem {
public:
    using ChangeCallback = std::function<void(const ParameterUpdateQueue<>::ParameterChange&)>;

    static ParameterUpdateSystem& getInstance();

    /**
     * @brief Push update from UI thread to audio thread
     * @param id Parameter ID
     * @param value New value
     * @param source Change source
     * @return true if successful
     */
    bool pushToAudio(const Parameter::ParameterId& id, float value, 
                     ParameterUpdateQueue<>::ChangeSource source = 
                     ParameterUpdateQueue<>::ChangeSource::UI);

    /**
     * @brief Push update from audio thread to UI thread
     * @param id Parameter ID
     * @param value New value
     * @param source Change source
     * @return true if successful
     */
    bool pushToUI(const Parameter::ParameterId& id, float value,
                  ParameterUpdateQueue<>::ChangeSource source = 
                  ParameterUpdateQueue<>::ChangeSource::Internal);

    /**
     * @brief Process all pending audio updates (call from audio thread)
     * @param callback Function to call for each update
     * @param maxUpdates Maximum number of updates to process
     * @return Number of updates processed
     */
    size_t processAudioUpdates(ChangeCallback callback, size_t maxUpdates = 64);

    /**
     * @brief Process all pending UI updates (call from UI thread)
     * @param callback Function to call for each update
     * @param maxUpdates Maximum number of updates to process
     * @return Number of updates processed
     */
    size_t processUIUpdates(ChangeCallback callback, size_t maxUpdates = 64);

    /**
     * @brief Enable/disable update logging for debugging
     * @param enable Whether to enable logging
     */
    void setLoggingEnabled(bool enable) { loggingEnabled_ = enable; }

    /**
     * @brief Get statistics about queue usage
     */
    struct Statistics {
        size_t audioQueueSize;
        size_t uiQueueSize;
        size_t totalAudioUpdates;
        size_t totalUIUpdates;
        size_t droppedAudioUpdates;
        size_t droppedUIUpdates;
    };
    
    Statistics getStatistics() const;

    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();

private:
    ParameterUpdateSystem() = default;
    ~ParameterUpdateSystem() = default;

    // Queues for bidirectional communication
    ParameterUpdateQueue<> audioQueue_; // UI -> Audio
    ParameterUpdateQueue<> uiQueue_;    // Audio -> UI
    
    // Statistics
    std::atomic<size_t> totalAudioUpdates_{0};
    std::atomic<size_t> totalUIUpdates_{0};
    std::atomic<size_t> droppedAudioUpdates_{0};
    std::atomic<size_t> droppedUIUpdates_{0};
    
    // Debug logging
    std::atomic<bool> loggingEnabled_{false};
    
    // Helper methods
    uint64_t getCurrentTimestamp() const;
    void logUpdate(const ParameterUpdateQueue<>::ParameterChange& change, bool toAudio);
};

} // namespace AIMusicHardware