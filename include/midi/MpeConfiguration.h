#pragma once

#include <array>
#include <vector>
#include <mutex>

namespace AIMusicHardware {

/**
 * @class MpeConfiguration
 * @brief Handles MPE zone configuration and state management
 *
 * This class implements MIDI Polyphonic Expression (MPE) configuration
 * according to the official MPE specification, supporting both Lower and
 * Upper zones with appropriate channel management.
 */
class MpeConfiguration {
public:
    /**
     * @struct Zone
     * @brief Stores configuration for an MPE zone
     */
    struct Zone {
        bool active = false;
        int masterChannel = 1;             // 1 for Lower Zone, 16 for Upper Zone
        int startMemberChannel = 2;        // First member channel (2 for Lower, 15 for Upper)
        int endMemberChannel = 8;          // Last member channel (varies based on allocation)
        bool ascending = true;             // Channel direction (true for Lower, false for Upper)
        
        // Zone properties
        int pitchBendRange = 48;           // Default ±48 semitones
        int defaultTimbre = 64;            // Default timbre value (centered)
    };
    
    /**
     * @brief Constructor initializes default zone settings
     */
    MpeConfiguration();
    
    /**
     * @brief Configure the Lower MPE zone
     *
     * @param active Whether the zone is active
     * @param memberChannels Number of member channels (1-15)
     */
    void setLowerZone(bool active, int memberChannels = 7);
    
    /**
     * @brief Configure the Upper MPE zone
     *
     * @param active Whether the zone is active
     * @param memberChannels Number of member channels (1-15)
     */
    void setUpperZone(bool active, int memberChannels = 7);
    
    /**
     * @brief Get the configuration for the Lower MPE zone
     * 
     * @return Zone configuration
     */
    Zone getLowerZone() const;
    
    /**
     * @brief Get the configuration for the Upper MPE zone
     * 
     * @return Zone configuration
     */
    Zone getUpperZone() const;
    
    /**
     * @brief Check if MPE mode is currently active
     * 
     * @return true if either zone is active
     */
    bool isActive() const;
    
    /**
     * @brief Check if a channel belongs to the Lower MPE zone
     * 
     * @param channel MIDI channel (0-15)
     * @return true if the channel is in the Lower zone
     */
    bool isInLowerZone(int channel) const;
    
    /**
     * @brief Check if a channel belongs to the Upper MPE zone
     * 
     * @param channel MIDI channel (0-15)
     * @return true if the channel is in the Upper zone
     */
    bool isInUpperZone(int channel) const;
    
    /**
     * @brief Check if a channel is a Master channel
     * 
     * @param channel MIDI channel (0-15)
     * @return true if the channel is a Master channel (1 or 16)
     */
    bool isMasterChannel(int channel) const;
    
    /**
     * @brief Check if a channel is a Member channel
     * 
     * @param channel MIDI channel (0-15)
     * @return true if the channel is a Member channel
     */
    bool isMemberChannel(int channel) const;
    
    /**
     * @brief Get the zone that contains a specific channel
     * 
     * @param channel MIDI channel (0-15)
     * @return Pointer to the zone, or nullptr if none
     */
    const Zone* getZoneForChannel(int channel) const;
    
    /**
     * @brief Process an MPE configuration RPN message
     * 
     * Handles RPN 0x6 for MPE zone configuration
     * 
     * @param channel MIDI channel (0-15)
     * @param rpnMsb MSB of the RPN (should be 0 for MPE config)
     * @param rpnLsb LSB of the RPN (should be 6 for MPE config)
     * @param dataMsb MSB of the data value (number of member channels)
     * @param dataLsb LSB of the data value (typically 0)
     */
    void processRpn(int channel, int rpnMsb, int rpnLsb, int dataMsb, int dataLsb);
    
    /**
     * @brief Set the active channels mask
     * 
     * @param activeChannels Array of 16 booleans for channel activation
     */
    void setActiveChannels(const std::array<bool, 16>& activeChannels);
    
    /**
     * @brief Get the active channels mask
     * 
     * @return Array of 16 booleans for channel activation
     */
    std::array<bool, 16> getActiveChannels() const;
    
    /**
     * @brief Set the pitch bend range for a zone
     * 
     * @param semitones Pitch bend range in semitones
     * @param isLowerZone true for Lower zone, false for Upper zone
     */
    void setPitchBendRange(int semitones, bool isLowerZone);
    
    /**
     * @brief Get the pitch bend range for a zone
     * 
     * @param isLowerZone true for Lower zone, false for Upper zone
     * @return Pitch bend range in semitones
     */
    int getPitchBendRange(bool isLowerZone) const;

private:
    Zone lowerZone_;
    Zone upperZone_;
    std::array<bool, 16> activeChannels_;
    
    mutable std::mutex configMutex_;
};

} // namespace AIMusicHardware