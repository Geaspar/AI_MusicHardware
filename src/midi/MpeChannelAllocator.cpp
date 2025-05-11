#include "../../include/midi/MpeChannelAllocator.h"
#include <algorithm>
#include <vector>

namespace AIMusicHardware {

MpeChannelAllocator::MpeChannelAllocator(const MpeConfiguration& mpeConfig)
    : mpeConfig_(mpeConfig) {
    
    // Initialize all channel states
    for (auto& state : channelStates_) {
        state.inUse = false;
        state.noteNumber = -1;
        state.noteId = 0;
        state.timestamp = 0;
        state.pitchBend = 0.0f;
        state.timbre = 0.5f;
        state.pressure = 0.0f;
    }
}

int MpeChannelAllocator::allocateChannel(int note, int velocity, bool lowerZone) {
    std::lock_guard<std::mutex> lock(allocationMutex_);
    
    // Get the appropriate zone configuration
    const MpeConfiguration::Zone& zone = lowerZone ? 
        mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
    
    if (!zone.active) {
        return -1;  // Zone not active
    }
    
    // Check if the note is already allocated
    auto it = noteToChannelMap_.find(note);
    if (it != noteToChannelMap_.end()) {
        // Note already allocated, reuse the channel
        int existingChannel = it->second;
        
        // Update the channel state
        channelStates_[existingChannel].timestamp = messageCounter_++;
        
        return existingChannel;
    }
    
    // Determine channel range (convert to 0-based)
    int startChannel = zone.startMemberChannel - 1;
    int endChannel = zone.endMemberChannel - 1;
    bool ascending = zone.ascending;
    
    // Try to find a free channel in the zone
    std::vector<std::pair<int, uint32_t>> candidates;  // channel, timestamp
    
    if (ascending) {
        // Scan channels in ascending order
        for (int ch = startChannel; ch <= endChannel; ch++) {
            if (!channelStates_[ch].inUse) {
                // Found unused channel
                channelStates_[ch].inUse = true;
                channelStates_[ch].noteNumber = note;
                channelStates_[ch].timestamp = messageCounter_++;
                channelStates_[ch].pitchBend = 0.0f;
                channelStates_[ch].timbre = 0.5f;
                channelStates_[ch].pressure = static_cast<float>(velocity) / 127.0f;
                
                // Store mapping
                noteToChannelMap_[note] = ch;
                
                return ch;
            }
            
            // Store as candidate for stealing if needed
            candidates.push_back({ch, channelStates_[ch].timestamp});
        }
    } else {
        // Scan channels in descending order
        for (int ch = startChannel; ch >= endChannel; ch--) {
            if (!channelStates_[ch].inUse) {
                // Found unused channel
                channelStates_[ch].inUse = true;
                channelStates_[ch].noteNumber = note;
                channelStates_[ch].timestamp = messageCounter_++;
                channelStates_[ch].pitchBend = 0.0f;
                channelStates_[ch].timbre = 0.5f;
                channelStates_[ch].pressure = static_cast<float>(velocity) / 127.0f;
                
                // Store mapping
                noteToChannelMap_[note] = ch;
                
                return ch;
            }
            
            // Store as candidate for stealing if needed
            candidates.push_back({ch, channelStates_[ch].timestamp});
        }
    }
    
    // No free channel, need to steal
    if (candidates.empty()) {
        return -1;  // No channels available in this zone
    }
    
    // Find oldest allocated channel
    std::sort(candidates.begin(), candidates.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    int channelToSteal = candidates[0].first;
    
    // Remove old mapping
    int oldNote = channelStates_[channelToSteal].noteNumber;
    if (oldNote >= 0) {
        noteToChannelMap_.erase(oldNote);
    }
    
    // Update channel state
    channelStates_[channelToSteal].inUse = true;
    channelStates_[channelToSteal].noteNumber = note;
    channelStates_[channelToSteal].timestamp = messageCounter_++;
    channelStates_[channelToSteal].pitchBend = 0.0f;
    channelStates_[channelToSteal].timbre = 0.5f;
    channelStates_[channelToSteal].pressure = static_cast<float>(velocity) / 127.0f;
    
    // Store new mapping
    noteToChannelMap_[note] = channelToSteal;
    
    return channelToSteal;
}

void MpeChannelAllocator::releaseChannel(int channel) {
    if (channel < 0 || channel >= 16) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(allocationMutex_);
    
    // Get the note assigned to this channel
    int note = channelStates_[channel].noteNumber;
    
    // Remove the note-to-channel mapping
    if (note >= 0) {
        noteToChannelMap_.erase(note);
    }
    
    // Reset channel state
    channelStates_[channel].inUse = false;
    channelStates_[channel].noteNumber = -1;
    channelStates_[channel].noteId = 0;
    channelStates_[channel].pitchBend = 0.0f;
    channelStates_[channel].timbre = 0.5f;
    channelStates_[channel].pressure = 0.0f;
}

MpeChannelAllocator::ChannelState& MpeChannelAllocator::getChannelState(int channel) {
    std::lock_guard<std::mutex> lock(allocationMutex_);
    return channelStates_[channel];
}

int MpeChannelAllocator::getChannelForNote(int note) const {
    std::lock_guard<std::mutex> lock(allocationMutex_);
    
    auto it = noteToChannelMap_.find(note);
    if (it != noteToChannelMap_.end()) {
        return it->second;
    }
    
    return -1;  // Note not allocated
}

const std::array<MpeChannelAllocator::ChannelState, 16>& MpeChannelAllocator::getAllChannelStates() const {
    std::lock_guard<std::mutex> lock(allocationMutex_);
    return channelStates_;
}

void MpeChannelAllocator::reset() {
    std::lock_guard<std::mutex> lock(allocationMutex_);
    
    // Clear all mappings
    noteToChannelMap_.clear();
    
    // Reset all channel states
    for (auto& state : channelStates_) {
        state.inUse = false;
        state.noteNumber = -1;
        state.noteId = 0;
        state.timestamp = 0;
        state.pitchBend = 0.0f;
        state.timbre = 0.5f;
        state.pressure = 0.0f;
    }
}

bool MpeChannelAllocator::updateExpression(int channel, float pitchBend, float timbre, float pressure) {
    if (channel < 0 || channel >= 16) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(allocationMutex_);
    
    // Only update expression for active channels
    if (!channelStates_[channel].inUse) {
        return false;
    }
    
    // Clamp values to valid ranges
    pitchBend = std::clamp(pitchBend, -1.0f, 1.0f);
    timbre = std::clamp(timbre, 0.0f, 1.0f);
    pressure = std::clamp(pressure, 0.0f, 1.0f);
    
    // Update expression values
    channelStates_[channel].pitchBend = pitchBend;
    channelStates_[channel].timbre = timbre;
    channelStates_[channel].pressure = pressure;
    
    return true;
}

} // namespace AIMusicHardware