#pragma once

#include "MidiInterface.h"
#include "MpeConfiguration.h"
#include "MpeChannelAllocator.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <functional>
#include <array>

namespace AIMusicHardware {

// Forward declarations
class MultiTimbralEngine;

/**
 * @class MultiTimbralMidiRouter
 * @brief Specialized MIDI router for multi-timbral synthesis and MPE
 *
 * This class extends MIDI routing capabilities for multi-timbral operation,
 * supporting keyboard splits, layering, and channel filtering.
 * It also supports MPE (MIDI Polyphonic Expression) for expressive control.
 */
class MultiTimbralMidiRouter : public MidiInputCallback {
public:
    // Operation modes for the router
    enum class OperationMode {
        Standard,   // Regular multi-timbral mode
        Mpe         // MIDI Polyphonic Expression mode
    };

    // Define keyboard split configuration
    struct KeyboardSplitConfig {
        bool enabled = false;
        int splitPoint = 60;  // Middle C
        int lowerChannel = 0;
        int upperChannel = 1;
    };

    // Define layered channels configuration
    struct LayerConfig {
        bool enabled = false;
        std::vector<int> channels;
    };

    // Define velocity split configuration
    struct VelocitySplitConfig {
        bool enabled = false;
        int threshold = 64;
        int lowerChannel = 0;
        int upperChannel = 1;
    };

    /**
     * @brief Create a new MultiTimbralMidiRouter
     * @param multiTimbralEngine The multi-timbral engine to control
     */
    MultiTimbralMidiRouter(MultiTimbralEngine* multiTimbralEngine);

    /**
     * @brief Destructor
     */
    ~MultiTimbralMidiRouter() override;

    // MidiInputCallback implementation
    void handleIncomingMidiMessage(const MidiMessage& message) override;

    // Process a MIDI message (can be called directly or via callback)
    void processMidiMessage(const MidiMessage& message);

    //----------------------------------------------------------------------
    // Standard multi-timbral mode message processors
    //----------------------------------------------------------------------

    void processNoteOn(const MidiMessage& message);
    void processNoteOff(const MidiMessage& message);
    void processControlChange(const MidiMessage& message);
    void processPitchBend(const MidiMessage& message);
    void processAfterTouch(const MidiMessage& message);
    void processChannelPressure(const MidiMessage& message);
    void processProgramChange(const MidiMessage& message);
    void processAllNotesOff(const MidiMessage& message);

    //----------------------------------------------------------------------
    // MPE mode message processors
    //----------------------------------------------------------------------

    void processMpeNoteOn(const MidiMessage& message);
    void processMpeNoteOff(const MidiMessage& message);
    void processMpePitchBend(const MidiMessage& message);
    void processMpeTimbre(const MidiMessage& message);  // CC74
    void processMpePressure(const MidiMessage& message);
    void processMpeControlChange(const MidiMessage& message);
    void processMpeProgramChange(const MidiMessage& message);

    /**
     * @brief Process RPN messages for MPE configuration
     *
     * @param channel MIDI channel (0-15)
     * @param rpnMsb RPN MSB value
     * @param rpnLsb RPN LSB value
     * @param dataMsb Data MSB value
     * @param dataLsb Data LSB value
     */
    void handleRpnMessage(int channel, int rpnMsb, int rpnLsb, int dataMsb, int dataLsb);

    // MIDI device setup methods
    bool openMidiInput(int deviceIndex);
    void closeMidiInput();
    std::vector<std::string> getMidiInputDevices() const;

    //----------------------------------------------------------------------
    // Operation mode settings
    //----------------------------------------------------------------------

    /**
     * @brief Set the operation mode
     *
     * @param mode Operation mode (Standard or MPE)
     */
    void setOperationMode(OperationMode mode);

    /**
     * @brief Get the current operation mode
     *
     * @return Operation mode
     */
    OperationMode getOperationMode() const;

    /**
     * @brief Check if MPE mode is active
     *
     * @return true if in MPE mode
     */
    bool isMpeMode() const;

    //----------------------------------------------------------------------
    // MPE Configuration
    //----------------------------------------------------------------------

    /**
     * @brief Configure the MPE Lower Zone
     *
     * @param active Whether the zone is active
     * @param memberChannels Number of member channels (1-15)
     */
    void configureMpeLowerZone(bool active, int memberChannels = 7);

    /**
     * @brief Configure the MPE Upper Zone
     *
     * @param active Whether the zone is active
     * @param memberChannels Number of member channels (1-15)
     */
    void configureMpeUpperZone(bool active, int memberChannels = 7);

    /**
     * @brief Get the MPE configuration
     *
     * @return MPE configuration object
     */
    const MpeConfiguration& getMpeConfiguration() const;

    /**
     * @brief Get all MPE channel states
     *
     * @return Array of channel states
     */
    const std::array<MpeChannelAllocator::ChannelState, 16>& getMpeChannelStates() const;

    //----------------------------------------------------------------------
    // Standard multi-timbral channel management
    //----------------------------------------------------------------------

    /**
     * Enable or disable specific MIDI channels
     * @param channel MIDI channel (0-15)
     * @param enabled Whether the channel is enabled
     */
    void setChannelEnabled(int channel, bool enabled);

    /**
     * Check if a MIDI channel is enabled
     * @param channel MIDI channel (0-15)
     * @return Whether the channel is enabled
     */
    bool isChannelEnabled(int channel) const;

    /**
     * Set up a keyboard split configuration
     * @param splitPoint MIDI note number for split point
     * @param lowerChannel Channel for notes below split point
     * @param upperChannel Channel for notes at or above split point
     */
    void setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel);

    /**
     * Get the current keyboard split configuration
     * @return The current keyboard split configuration
     */
    KeyboardSplitConfig getKeyboardSplitConfig() const;

    /**
     * Set up layered channels where multiple synths play the same notes
     * @param channels Vector of channel numbers to layer
     */
    void setupLayeredChannels(const std::vector<int>& channels);

    /**
     * Get the current layer configuration
     * @return The current layer configuration
     */
    LayerConfig getLayerConfig() const;

    /**
     * Clear all performance configurations (splits, layers)
     */
    void clearPerformanceConfig();

    /**
     * Set up velocity splits where notes are routed based on velocity
     * @param threshold Velocity threshold (0-127)
     * @param lowerChannel Channel for notes below threshold
     * @param upperChannel Channel for notes at or above threshold
     */
    void setupVelocitySplit(int threshold, int lowerChannel, int upperChannel);

    /**
     * Set a program change mapping
     * @param channel MIDI channel (0-15)
     * @param sourceProgram Original program number (0-127)
     * @param targetProgram Mapped program number (0-127)
     */
    void setProgramChangeMapping(int channel, int sourceProgram, int targetProgram);

    /**
     * Clear program change mappings for a channel
     * @param channel MIDI channel (0-15)
     */
    void clearProgramChangeMappings(int channel);

    /**
     * Enable debug output for MIDI events
     * @param enable Whether to enable debugging
     */
    void setDebugMode(bool enable) { debugMode_ = enable; }

private:
    // Get effective channel for a MIDI message based on current routing configuration
    int getEffectiveChannel(int channel, int note, int velocity);

    // Checks if a channel is valid (0-15)
    bool isValidChannel(int channel) const;

    // Expression scaling utilities
    float scalePitchBend(int rawValue) const;
    float scaleTimbre(int rawValue) const;
    float scalePressure(int rawValue) const;

    // Reference to the multi-timbral engine
    MultiTimbralEngine* multiTimbralEngine_;

    // MIDI input interface
    std::unique_ptr<MidiInput> midiInput_;

    // Operation mode
    OperationMode operationMode_ = OperationMode::Standard;

    // MPE configuration and channel allocation
    MpeConfiguration mpeConfig_;
    std::unique_ptr<MpeChannelAllocator> mpeChannelAllocator_;

    // RPN handling state
    struct RpnState {
        bool active = false;
        int msb = 0;
        int lsb = 0;
    };
    std::array<RpnState, 16> rpnStates_;

    // MIDI channel enabling/filtering for standard mode
    std::array<bool, 16> channelEnabled_;

    // Performance configurations for standard mode
    KeyboardSplitConfig keySplitConfig_;
    LayerConfig layerConfig_;
    VelocitySplitConfig velocitySplitConfig_;

    // Program Change mapping
    std::map<int, std::map<int, int>> programChangeMap_; // channel -> (program -> mapped program)

    // Debug mode
    bool debugMode_ = false;

    // Thread safety
    mutable std::mutex routingMutex_;
};

} // namespace AIMusicHardware