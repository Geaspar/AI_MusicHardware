#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../../audio/Synthesizer.h"

namespace AIMusicHardware {

/**
 * Channel-specific parameter container for multi-timbral operation
 */
struct ChannelParameters {
    // Basic channel settings
    int channelNumber = 0;
    std::string name = "Channel";
    int polyphony = 8;
    int priority = 1;
    
    // Performance settings
    bool monophonic = false;
    int portamentoTime = 0;
    int transposition = 0;
    int fineTuning = 0;
    int noteRangeLow = 0;
    int noteRangeHigh = 127;
    
    // Mixing settings
    float volume = 1.0f;
    float pan = 0.0f;
    
    // MIDI settings
    bool receivesProgramChange = true;
    bool receivesControllers = true;
    bool receivesPitchBend = true;
    std::unordered_map<int, bool> ccResponseMap;  // Which CCs this channel responds to
    
    // Preset identification
    int programNumber = 0;
    std::string presetName = "Default";
    
    // Compare parameters
    bool operator==(const ChannelParameters& other) const;
    bool operator!=(const ChannelParameters& other) const;
};

/**
 * Channel-specific synthesizer for multi-timbral operation.
 * Extends the base Synthesizer with channel-specific functionality.
 */
class ChannelSynthesizer : public Synthesizer {
public:
    /**
     * Construct a new Channel Synthesizer
     * 
     * @param channel MIDI channel number (0-15)
     * @param sampleRate Audio sample rate
     */
    ChannelSynthesizer(int channel, int sampleRate = 44100);
    
    /**
     * Destructor
     */
    ~ChannelSynthesizer() override;
    
    /**
     * Get the channel number
     * 
     * @return Channel number (0-15)
     */
    int getChannel() const;
    
    /**
     * Set the channel number
     * 
     * @param channel Channel number (0-15)
     */
    void setChannel(int channel);
    
    /**
     * Set the note range this channel responds to
     * 
     * @param low Lowest MIDI note number
     * @param high Highest MIDI note number
     */
    void setNoteRange(int low, int high);
    
    /**
     * Check if a MIDI note is within this channel's range
     * 
     * @param midiNote MIDI note number to check
     * @return true if the note is in range
     */
    bool isNoteInRange(int midiNote) const;
    
    /**
     * Set the channel's display name
     * 
     * @param name Display name for the channel
     */
    void setName(const std::string& name);
    
    /**
     * Get the channel's display name
     *
     * @return Display name
     */
    std::string getName() const override;
    
    /**
     * Set whether this channel should operate in monophonic mode
     * 
     * @param mono true for monophonic, false for polyphonic
     */
    void setMonophonic(bool mono);
    
    /**
     * Check if this channel is in monophonic mode
     * 
     * @return true if monophonic
     */
    bool isMonophonic() const;
    
    /**
     * Set the portamento time for monophonic mode
     * 
     * @param timeMs Portamento time in milliseconds
     */
    void setPortamentoTime(int timeMs);
    
    /**
     * Get the portamento time
     * 
     * @return Portamento time in milliseconds
     */
    int getPortamentoTime() const;
    
    /**
     * Set the channel's transposition in semitones
     * 
     * @param semitones Transposition in semitones (+/- 48)
     */
    void setTransposition(int semitones);
    
    /**
     * Get the channel's transposition
     * 
     * @return Transposition in semitones
     */
    int getTransposition() const;
    
    /**
     * Set the channel's fine tuning in cents
     * 
     * @param cents Fine tuning in cents (+/- 100)
     */
    void setFineTuning(int cents);
    
    /**
     * Get the channel's fine tuning
     * 
     * @return Fine tuning in cents
     */
    int getFineTuning() const;
    
    /**
     * Set the channel's priority for voice allocation
     * 
     * @param priority Priority level (higher = more important)
     */
    void setChannelPriority(int priority);
    
    /**
     * Get the channel's priority
     * 
     * @return Priority level
     */
    int getChannelPriority() const;
    
    /**
     * Process a MIDI program change for this channel
     * 
     * @param program Program number (0-127)
     */
    void processChannelProgramChange(int program);
    
    /**
     * Process a MIDI CC message for this channel
     * 
     * @param controller CC number (0-127)
     * @param value CC value (0-127)
     */
    void processChannelMidiCC(int controller, int value);
    
    /**
     * Configure whether this channel responds to program changes
     * 
     * @param enable true to enable program changes
     */
    void enableProgramChanges(bool enable);
    
    /**
     * Check if this channel responds to program changes
     * 
     * @return true if program changes are enabled
     */
    bool receivesProgramChanges() const;
    
    /**
     * Configure whether this channel responds to control changes
     * 
     * @param enable true to enable control changes
     */
    void enableControlChanges(bool enable);
    
    /**
     * Check if this channel responds to control changes
     * 
     * @return true if control changes are enabled
     */
    bool receivesControlChanges() const;
    
    /**
     * Configure whether this channel responds to pitch bend
     * 
     * @param enable true to enable pitch bend
     */
    void enablePitchBend(bool enable);
    
    /**
     * Check if this channel responds to pitch bend
     * 
     * @return true if pitch bend is enabled
     */
    bool receivesPitchBend() const;
    
    /**
     * Set the program number for this channel
     * 
     * @param program Program number (0-127)
     */
    void setProgramNumber(int program);
    
    /**
     * Get the current program number
     * 
     * @return Program number
     */
    int getProgramNumber() const;
    
    /**
     * Set the preset name for this channel
     * 
     * @param name Preset name
     */
    void setPresetName(const std::string& name);
    
    /**
     * Get the current preset name
     * 
     * @return Preset name
     */
    std::string getPresetName() const;
    
    /**
     * Load channel parameters
     * 
     * @param params Channel parameters
     */
    void loadChannelParameters(const ChannelParameters& params);
    
    /**
     * Save channel parameters
     * 
     * @return Channel parameters
     */
    ChannelParameters saveChannelParameters() const;
    
    /**
     * Override base class process method to apply channel-specific processing
     * 
     * @param buffer Audio buffer
     * @param numFrames Number of frames to process
     */
    void process(float* buffer, int numFrames) override;

    // Override note handling methods for channel-specific processing
    void noteOn(int midiNote, float velocity, int channel = 0) override;
    void noteOff(int midiNote, int channel = 0) override;
    void allNotesOff(int channel = -1) override;

private:
    // Helper to apply transposition to a MIDI note
    int applyTransposition(int midiNote) const;
    
    // Channel specific state
    int channelNumber_;
    std::string name_;
    
    // Performance settings
    bool monophonic_;
    int portamentoTime_;
    int transposition_;
    int fineTuning_;
    int noteRangeLow_;
    int noteRangeHigh_;
    
    // Voice management
    int channelPriority_;
    
    // MIDI response flags
    bool receivesProgramChange_;
    bool receivesControllers_;
    bool receivesPitchBend_;
    
    // Preset information
    int programNumber_;
    std::string presetName_;
    
    // Monophonic mode tracking
    int lastPlayedNote_;
    float lastNoteVelocity_;
    std::vector<int> heldNotes_;
};

} // namespace AIMusicHardware