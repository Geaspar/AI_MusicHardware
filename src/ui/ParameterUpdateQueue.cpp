#include "../../include/ui/ParameterUpdateQueue.h"
#include <chrono>
#include <iostream>
#include <iomanip>

namespace AIMusicHardware {

ParameterUpdateSystem& ParameterUpdateSystem::getInstance() {
    static ParameterUpdateSystem instance;
    return instance;
}

bool ParameterUpdateSystem::pushToAudio(const Parameter::ParameterId& id, float value,
                                        ParameterUpdateQueue<>::ChangeSource source) {
    ParameterUpdateQueue<>::ParameterChange change{
        id, value, source, getCurrentTimestamp()
    };
    
    bool success = audioQueue_.push(change);
    
    if (success) {
        totalAudioUpdates_.fetch_add(1, std::memory_order_relaxed);
        if (loggingEnabled_) {
            logUpdate(change, true);
        }
    } else {
        droppedAudioUpdates_.fetch_add(1, std::memory_order_relaxed);
        if (loggingEnabled_) {
            std::cerr << "WARNING: Dropped audio update for parameter " << id << std::endl;
        }
    }
    
    return success;
}

bool ParameterUpdateSystem::pushToUI(const Parameter::ParameterId& id, float value,
                                     ParameterUpdateQueue<>::ChangeSource source) {
    ParameterUpdateQueue<>::ParameterChange change{
        id, value, source, getCurrentTimestamp()
    };
    
    bool success = uiQueue_.push(change);
    
    if (success) {
        totalUIUpdates_.fetch_add(1, std::memory_order_relaxed);
        if (loggingEnabled_) {
            logUpdate(change, false);
        }
    } else {
        droppedUIUpdates_.fetch_add(1, std::memory_order_relaxed);
        if (loggingEnabled_) {
            std::cerr << "WARNING: Dropped UI update for parameter " << id << std::endl;
        }
    }
    
    return success;
}

size_t ParameterUpdateSystem::processAudioUpdates(ChangeCallback callback, size_t maxUpdates) {
    if (!callback) return 0;
    
    size_t processed = 0;
    ParameterUpdateQueue<>::ParameterChange change;
    
    while (processed < maxUpdates && audioQueue_.pop(change)) {
        callback(change);
        processed++;
    }
    
    return processed;
}

size_t ParameterUpdateSystem::processUIUpdates(ChangeCallback callback, size_t maxUpdates) {
    if (!callback) return 0;
    
    size_t processed = 0;
    ParameterUpdateQueue<>::ParameterChange change;
    
    while (processed < maxUpdates && uiQueue_.pop(change)) {
        callback(change);
        processed++;
    }
    
    return processed;
}

ParameterUpdateSystem::Statistics ParameterUpdateSystem::getStatistics() const {
    return Statistics{
        audioQueue_.size(),
        uiQueue_.size(),
        totalAudioUpdates_.load(std::memory_order_relaxed),
        totalUIUpdates_.load(std::memory_order_relaxed),
        droppedAudioUpdates_.load(std::memory_order_relaxed),
        droppedUIUpdates_.load(std::memory_order_relaxed)
    };
}

void ParameterUpdateSystem::resetStatistics() {
    totalAudioUpdates_.store(0, std::memory_order_relaxed);
    totalUIUpdates_.store(0, std::memory_order_relaxed);
    droppedAudioUpdates_.store(0, std::memory_order_relaxed);
    droppedUIUpdates_.store(0, std::memory_order_relaxed);
}

uint64_t ParameterUpdateSystem::getCurrentTimestamp() const {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

void ParameterUpdateSystem::logUpdate(const ParameterUpdateQueue<>::ParameterChange& change, bool toAudio) {
    const char* sourceStr = "Unknown";
    switch (change.source) {
        case ParameterUpdateQueue<>::ChangeSource::UI: sourceStr = "UI"; break;
        case ParameterUpdateQueue<>::ChangeSource::MIDI: sourceStr = "MIDI"; break;
        case ParameterUpdateQueue<>::ChangeSource::IoT: sourceStr = "IoT"; break;
        case ParameterUpdateQueue<>::ChangeSource::Automation: sourceStr = "Automation"; break;
        case ParameterUpdateQueue<>::ChangeSource::Preset: sourceStr = "Preset"; break;
        case ParameterUpdateQueue<>::ChangeSource::Internal: sourceStr = "Internal"; break;
    }
    
    std::cout << "[ParameterUpdate] "
              << (toAudio ? "UI->Audio" : "Audio->UI") << " | "
              << "ID: " << change.id << " | "
              << "Value: " << std::fixed << std::setprecision(3) << change.value << " | "
              << "Source: " << sourceStr << " | "
              << "Time: " << change.timestamp << "μs"
              << std::endl;
}

} // namespace AIMusicHardware