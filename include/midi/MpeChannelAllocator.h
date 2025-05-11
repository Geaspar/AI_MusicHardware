#pragma once

#include "MpeConfiguration.h"
#include <array>
#include <mutex>
#include <unordered_map>
#include <cstdint>

namespace AIMusicHardware {

/**
 * @class MpeChannelAllocator
 * @brief Manages MPE channel allocation for notes
 * 
 * This class is responsible for allocating MIDI channels to notes in MPE mode,
 * handling channel rotation, and tracking which notes are assigned to which channels.
 */
class MpeChannelAllocator {
public:
    /**
     * @struct ChannelState
     * @brief Tracks the state of a MIDI channel in MPE mode
     */
    struct ChannelState {
        bool inUse = false;
        int noteNumber = -1;
        uint8_t noteId = 0;        // For tracking specific note instances
        uint32_t timestamp = 0;    // When this channel was allocated
        float pitchBend = 0.0f;    // -1.0 to 1.0
        float timbre = 0.5f;       // 0.0 to 1.0 (CC74)
        float pressure = 0.0f;     // 0.0 to 1.0 (Channel Pressure)
    };
    
    /**
     * @brief Constructor
     * 
     * @param mpeConfig Reference to MPE configuration
     */
    MpeChannelAllocator(const MpeConfiguration& mpeConfig);
    
    /**
     * @brief Allocate a channel for a note in MPE mode
     * 
     * @param note MIDI note number
     * @param velocity Note velocity (0-127)
     * @param lowerZone true to use Lower Zone, false for Upper Zone
     * @return Allocated channel (0-15) or -1 if allocation failed
     */
    int allocateChannel(int note, int velocity, bool lowerZone);
    
    /**
     * @brief Release a channel in MPE mode
     * 
     * @param channel MIDI channel to release (0-15)
     */
    void releaseChannel(int channel);
    
    /**
     * @brief Get the state of a channel
     * 
     * @param channel MIDI channel (0-15)
     * @return Reference to the channel state
     */
    ChannelState& getChannelState(int channel);
    
    /**
     * @brief Get the channel assigned to a note
     * 
     * @param note MIDI note number
     * @return Channel number (0-15) or -1 if not found
     */
    int getChannelForNote(int note) const;
    
    /**
     * @brief Get all channel states
     * 
     * @return Array of channel states
     */
    const std::array<ChannelState, 16>& getAllChannelStates() const;
    
    /**
     * @brief Reset all channel allocations
     */
    void reset();
    
    /**
     * @brief Update expression values for a channel
     * 
     * @param channel MIDI channel (0-15)
     * @param pitchBend Pitch bend value (-1.0 to 1.0)
     * @param timbre Timbre value (0.0 to 1.0)
     * @param pressure Pressure value (0.0 to 1.0)
     * @return true if the update was successful
     */
    bool updateExpression(int channel, float pitchBend, float timbre, float pressure);

private:
    const MpeConfiguration& mpeConfig_;
    std::array<ChannelState, 16> channelStates_;
    std::unordered_map<int, int> noteToChannelMap_;  // Maps note to channel
    uint32_t messageCounter_ = 0;  // For timestamps
    
    mutable std::mutex allocationMutex_;
};

} // namespace AIMusicHardware