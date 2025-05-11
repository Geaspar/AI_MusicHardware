#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include "ChannelSynthesizer.h"
#include "../../audio/Synthesizer.h"

namespace AIMusicHardware {

/**
 * Strategies for allocating voices across multiple channels.
 */
enum class VoiceAllocationStrategy {
    Equal,              // Divide voices equally among active channels
    PriorityBased,      // Allocate more voices to higher priority channels
    Dynamic             // Allocate based on recent usage patterns
};

/**
 * Core engine for managing multiple synth instances across MIDI channels.
 * This class is responsible for routing MIDI events, mixing audio, and
 * managing resources across all active channels.
 */
class MultiTimbralEngine {
public:
    /**
     * Construct a new Multi Timbral Engine
     * 
     * @param sampleRate Audio sample rate
     * @param maxTotalVoices Maximum total voices across all channels
     */
    MultiTimbralEngine(int sampleRate = 44100, int maxTotalVoices = 64);
    
    /**
     * Destructor
     */
    ~MultiTimbralEngine();
    
    /**
     * Initialize the engine and all channel synthesizers
     * 
     * @return true if initialization succeeded
     */
    bool initialize();
    
    //----------------------------------------------------------------------
    // MIDI Event Handling
    //----------------------------------------------------------------------
    
    /**
     * Trigger a note on event for the specified channel
     * 
     * @param midiNote MIDI note number (0-127)
     * @param velocity Note velocity (0.0-1.0)
     * @param channel MIDI channel (0-15)
     */
    void noteOn(int midiNote, float velocity, int channel);
    
    /**
     * Trigger a note off event for the specified channel
     * 
     * @param midiNote MIDI note number (0-127)
     * @param channel MIDI channel (0-15)
     */
    void noteOff(int midiNote, int channel);
    
    /**
     * Handle program change MIDI message
     * 
     * @param program Program number (0-127)
     * @param channel MIDI channel (0-15)
     */
    void programChange(int program, int channel);
    
    /**
     * Handle control change MIDI message
     * 
     * @param controller CC number (0-127)
     * @param value CC value (0-127)
     * @param channel MIDI channel (0-15)
     */
    void controlChange(int controller, int value, int channel);
    
    /**
     * Handle pitch bend MIDI message
     * 
     * @param value Pitch bend value (-1.0 to 1.0)
     * @param channel MIDI channel (0-15)
     */
    void pitchBend(float value, int channel);
    
    /**
     * Handle aftertouch MIDI message
     * 
     * @param note Note number (0-127)
     * @param value Pressure value (0.0-1.0)
     * @param channel MIDI channel (0-15)
     */
    void aftertouch(int note, float value, int channel);
    
    /**
     * Handle channel pressure MIDI message
     * 
     * @param value Pressure value (0.0-1.0)
     * @param channel MIDI channel (0-15)
     */
    void channelPressure(float value, int channel);
    
    /**
     * Turn off all notes on all channels
     */
    void allNotesOff();
    
    //----------------------------------------------------------------------
    // Channel Management
    //----------------------------------------------------------------------
    
    /**
     * Set a channel's active state
     * 
     * @param channel MIDI channel (0-15)
     * @param active Whether the channel should be active
     */
    void setChannelActive(int channel, bool active);
    
    /**
     * Get a channel's active state
     * 
     * @param channel MIDI channel (0-15)
     * @return true if the channel is active
     */
    bool isChannelActive(int channel) const;
    
    /**
     * Set a channel's volume
     * 
     * @param channel MIDI channel (0-15)
     * @param volume Volume level (0.0-1.0)
     */
    void setChannelVolume(int channel, float volume);
    
    /**
     * Get a channel's volume
     * 
     * @param channel MIDI channel (0-15)
     * @return The channel's volume (0.0-1.0)
     */
    float getChannelVolume(int channel) const;
    
    /**
     * Set a channel's pan position
     * 
     * @param channel MIDI channel (0-15)
     * @param pan Pan position (-1.0=left, 0.0=center, 1.0=right)
     */
    void setChannelPan(int channel, float pan);
    
    /**
     * Get a channel's pan position
     * 
     * @param channel MIDI channel (0-15)
     * @return The channel's pan position
     */
    float getChannelPan(int channel) const;
    
    /**
     * Set the master volume
     * 
     * @param volume Master volume level (0.0-1.0)
     */
    void setMasterVolume(float volume);
    
    /**
     * Get the master volume
     * 
     * @return The master volume level (0.0-1.0)
     */
    float getMasterVolume() const;
    
    //----------------------------------------------------------------------
    // Voice Allocation
    //----------------------------------------------------------------------
    
    /**
     * Set the maximum total voices across all channels
     * 
     * @param maxVoices Maximum total voices
     */
    void setMaxTotalVoices(int maxVoices);
    
    /**
     * Get the maximum total voices across all channels
     * 
     * @return The maximum total voices
     */
    int getMaxTotalVoices() const;
    
    /**
     * Set the voice allocation strategy
     * 
     * @param strategy Voice allocation strategy
     */
    void setVoiceAllocationStrategy(VoiceAllocationStrategy strategy);
    
    /**
     * Get the current voice allocation strategy
     * 
     * @return The current voice allocation strategy
     */
    VoiceAllocationStrategy getVoiceAllocationStrategy() const;
    
    /**
     * Allocate voices among active channels based on the current strategy
     */
    void allocateVoices();
    
    /**
     * Set a channel's priority for voice allocation
     * 
     * @param channel MIDI channel (0-15)
     * @param priority Priority level (higher values = higher priority)
     */
    void setChannelPriority(int channel, int priority);
    
    /**
     * Get a channel's priority for voice allocation
     * 
     * @param channel MIDI channel (0-15)
     * @return The channel's priority level
     */
    int getChannelPriority(int channel) const;
    
    //----------------------------------------------------------------------
    // Preset Management
    //----------------------------------------------------------------------
    
    /**
     * Load a preset for a specific channel
     * 
     * @param preset Preset object
     * @param channel MIDI channel (0-15)
     */
    void loadPreset(const class Preset& preset, int channel);
    
    /**
     * Load a preset from file for a specific channel
     * 
     * @param filePath Path to preset file
     * @param channel MIDI channel (0-15)
     * @return true if preset loaded successfully
     */
    bool loadPresetFromFile(const std::string& filePath, int channel);
    
    /**
     * Get currently loaded preset name for a channel
     *
     * @param channel MIDI channel (0-15)
     * @return Preset name or empty string if no preset loaded
     */
    std::string getChannelPresetName(int channel) const;
    
    //----------------------------------------------------------------------
    // Audio Processing
    //----------------------------------------------------------------------
    
    /**
     * Process audio for the next buffer
     * 
     * @param outputBuffer Output audio buffer
     * @param numFrames Number of audio frames to process
     */
    void process(float* outputBuffer, int numFrames);
    
    /**
     * Set the audio sample rate
     * 
     * @param sampleRate New sample rate in Hz
     */
    void setSampleRate(int sampleRate);
    
    /**
     * Reset all audio processing state
     */
    void reset();
    
    //----------------------------------------------------------------------
    // Channel Access
    //----------------------------------------------------------------------
    
    /**
     * Get direct access to a channel synthesizer
     * 
     * @param channel MIDI channel (0-15)
     * @return Pointer to the channel synthesizer or nullptr if invalid channel
     */
    ChannelSynthesizer* getChannelSynth(int channel);
    
    //----------------------------------------------------------------------
    // Performance Modes
    //----------------------------------------------------------------------
    
    /**
     * Set up a keyboard split configuration
     * 
     * @param splitPoint MIDI note number for split point
     * @param lowerChannel Channel for notes below split point
     * @param upperChannel Channel for notes at or above split point
     */
    void setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel);
    
    /**
     * Set up layered channels where multiple synths play the same notes
     * 
     * @param channels Vector of channel numbers to layer
     */
    void setupLayeredChannels(const std::vector<int>& channels);
    
    /**
     * Clear all performance configurations (splits, layers)
     */
    void clearPerformanceConfig();

private:
    // Validates channel number is within range
    bool isValidChannel(int channel) const;
    
    // Pan stereo audio based on position (-1.0 to 1.0)
    void applyPanning(float* buffer, int numFrames, float pan);
    
    // Mix all active channel outputs into the master buffer
    void mixChannels(float* outputBuffer, int numFrames);
    
    // Component management
    std::array<std::unique_ptr<ChannelSynthesizer>, 16> channelSynths_;
    
    // Channel state
    std::array<bool, 16> channelActive_;
    std::array<float, 16> channelVolumes_;
    std::array<float, 16> channelPans_;
    std::array<int, 16> channelPriorities_;
    
    // Performance configuration
    struct {
        bool splitEnabled = false;
        int splitPoint = 60;  // Middle C
        int lowerChannel = 0;
        int upperChannel = 1;
        
        bool layerEnabled = false;
        std::vector<int> layeredChannels;
    } performanceConfig_;
    
    // Resource management
    int sampleRate_;
    int maxTotalVoices_;
    VoiceAllocationStrategy voiceStrategy_;
    float masterVolume_;
    
    // Mixing buffers
    std::vector<float> mixBuffer_;
    
    // Thread safety
    mutable std::mutex voiceAllocationMutex_;
    mutable std::mutex channelMutex_;
};

} // namespace AIMusicHardware