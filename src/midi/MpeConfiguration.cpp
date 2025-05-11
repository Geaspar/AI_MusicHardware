#include "../../include/midi/MpeConfiguration.h"
#include <algorithm>

namespace AIMusicHardware {

MpeConfiguration::MpeConfiguration() {
    // Initialize Lower Zone (Master: Channel 1, Members: Channels 2-8)
    lowerZone_.active = false;
    lowerZone_.masterChannel = 1;
    lowerZone_.startMemberChannel = 2;
    lowerZone_.endMemberChannel = 8;
    lowerZone_.ascending = true;
    lowerZone_.pitchBendRange = 48;
    lowerZone_.defaultTimbre = 64;
    
    // Initialize Upper Zone (Master: Channel 16, Members: Channels 15-9)
    upperZone_.active = false;
    upperZone_.masterChannel = 16;
    upperZone_.startMemberChannel = 15;
    upperZone_.endMemberChannel = 9;
    upperZone_.ascending = false;
    upperZone_.pitchBendRange = 48;
    upperZone_.defaultTimbre = 64;
    
    // Initialize all channels as active
    activeChannels_.fill(true);
}

void MpeConfiguration::setLowerZone(bool active, int memberChannels) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    lowerZone_.active = active;
    
    if (active) {
        // Clamp member channels to valid range (1-15)
        memberChannels = std::clamp(memberChannels, 1, 15);
        
        // Master is channel 1, members start at 2
        lowerZone_.startMemberChannel = 2;
        lowerZone_.endMemberChannel = 1 + memberChannels;
        
        // Make sure we don't conflict with upper zone
        if (upperZone_.active) {
            // Calculate max usable end channel
            int maxEndChannel = std::min(lowerZone_.endMemberChannel, upperZone_.endMemberChannel);
            lowerZone_.endMemberChannel = std::min(lowerZone_.endMemberChannel, maxEndChannel);
        }
    }
}

void MpeConfiguration::setUpperZone(bool active, int memberChannels) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    upperZone_.active = active;
    
    if (active) {
        // Clamp member channels to valid range (1-15)
        memberChannels = std::clamp(memberChannels, 1, 15);
        
        // Master is channel 16, members go downward
        upperZone_.startMemberChannel = 15;
        upperZone_.endMemberChannel = 16 - memberChannels;
        
        // Make sure we don't conflict with lower zone
        if (lowerZone_.active) {
            // Calculate min usable end channel
            int minEndChannel = std::max(upperZone_.endMemberChannel, lowerZone_.endMemberChannel);
            upperZone_.endMemberChannel = std::max(upperZone_.endMemberChannel, minEndChannel);
        }
    }
}

MpeConfiguration::Zone MpeConfiguration::getLowerZone() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return lowerZone_;
}

MpeConfiguration::Zone MpeConfiguration::getUpperZone() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return upperZone_;
}

bool MpeConfiguration::isActive() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return lowerZone_.active || upperZone_.active;
}

bool MpeConfiguration::isInLowerZone(int channel) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (!lowerZone_.active) {
        return false;
    }
    
    // Convert to 1-based channel for comparison
    int channel1Based = channel + 1;
    
    // Check if it's the master channel
    if (channel1Based == lowerZone_.masterChannel) {
        return true;
    }
    
    // Check if it's in the member channel range
    if (lowerZone_.ascending) {
        return channel1Based >= lowerZone_.startMemberChannel && 
               channel1Based <= lowerZone_.endMemberChannel;
    } else {
        return channel1Based <= lowerZone_.startMemberChannel && 
               channel1Based >= lowerZone_.endMemberChannel;
    }
}

bool MpeConfiguration::isInUpperZone(int channel) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    if (!upperZone_.active) {
        return false;
    }
    
    // Convert to 1-based channel for comparison
    int channel1Based = channel + 1;
    
    // Check if it's the master channel
    if (channel1Based == upperZone_.masterChannel) {
        return true;
    }
    
    // Check if it's in the member channel range
    if (upperZone_.ascending) {
        return channel1Based >= upperZone_.startMemberChannel && 
               channel1Based <= upperZone_.endMemberChannel;
    } else {
        return channel1Based <= upperZone_.startMemberChannel && 
               channel1Based >= upperZone_.endMemberChannel;
    }
}

bool MpeConfiguration::isMasterChannel(int channel) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // Convert to 1-based channel for comparison
    int channel1Based = channel + 1;
    
    return (lowerZone_.active && channel1Based == lowerZone_.masterChannel) ||
           (upperZone_.active && channel1Based == upperZone_.masterChannel);
}

bool MpeConfiguration::isMemberChannel(int channel) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // Convert to 1-based channel for comparison
    int channel1Based = channel + 1;
    
    // Check lower zone member channels
    if (lowerZone_.active) {
        if (lowerZone_.ascending) {
            if (channel1Based >= lowerZone_.startMemberChannel && 
                channel1Based <= lowerZone_.endMemberChannel) {
                return true;
            }
        } else {
            if (channel1Based <= lowerZone_.startMemberChannel && 
                channel1Based >= lowerZone_.endMemberChannel) {
                return true;
            }
        }
    }
    
    // Check upper zone member channels
    if (upperZone_.active) {
        if (upperZone_.ascending) {
            if (channel1Based >= upperZone_.startMemberChannel && 
                channel1Based <= upperZone_.endMemberChannel) {
                return true;
            }
        } else {
            if (channel1Based <= upperZone_.startMemberChannel && 
                channel1Based >= upperZone_.endMemberChannel) {
                return true;
            }
        }
    }
    
    return false;
}

const MpeConfiguration::Zone* MpeConfiguration::getZoneForChannel(int channel) const {
    if (isInLowerZone(channel)) {
        return &lowerZone_;
    } else if (isInUpperZone(channel)) {
        return &upperZone_;
    }
    
    return nullptr;
}

void MpeConfiguration::processRpn(int channel, int rpnMsb, int rpnLsb, int dataMsb, int dataLsb) {
    // Check if this is MPE Configuration message (RPN 0x6)
    if (rpnMsb == 0x00 && rpnLsb == 0x06) {
        // This is MPE Configuration
        int memberChannels = dataMsb;
        
        // Process based on channel
        if (channel == 0) {  // Channel 1 (0-based)
            // Configure Lower Zone
            setLowerZone(memberChannels > 0, memberChannels);
        } else if (channel == 15) {  // Channel 16 (0-based)
            // Configure Upper Zone
            setUpperZone(memberChannels > 0, memberChannels);
        }
    }
}

void MpeConfiguration::setActiveChannels(const std::array<bool, 16>& activeChannels) {
    std::lock_guard<std::mutex> lock(configMutex_);
    activeChannels_ = activeChannels;
}

std::array<bool, 16> MpeConfiguration::getActiveChannels() const {
    std::lock_guard<std::mutex> lock(configMutex_);
    return activeChannels_;
}

void MpeConfiguration::setPitchBendRange(int semitones, bool isLowerZone) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    // Ensure pitch bend range is reasonable
    semitones = std::clamp(semitones, 1, 96);
    
    if (isLowerZone) {
        lowerZone_.pitchBendRange = semitones;
    } else {
        upperZone_.pitchBendRange = semitones;
    }
}

int MpeConfiguration::getPitchBendRange(bool isLowerZone) const {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    return isLowerZone ? lowerZone_.pitchBendRange : upperZone_.pitchBendRange;
}

} // namespace AIMusicHardware