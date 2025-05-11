#pragma once

#include "MidiInterface.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <functional>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;

/**
 * @class MidiManager
 * @brief Manages MIDI connections and message routing for synthesizer control
 * 
 * This class handles MIDI device connectivity, message processing, parameter
 * mapping, and integration with the synthesizer engine. It supports MIDI learn
 * functionality and handles various MIDI control messages.
 */
class MidiManager : public MidiInputCallback {
public:
    // MIDI message main type constants (status byte masks)
    enum MidiMainType {
        kNoteOff = 0x80,
        kNoteOn = 0x90,
        kAftertouch = 0xA0,
        kController = 0xB0,
        kProgramChange = 0xC0,
        kChannelPressure = 0xD0,
        kPitchWheel = 0xE0,
        kSystem = 0xF0
    };

    // MIDI controller numbers for common controllers
    enum MidiController {
        kModWheel = 1,
        kBreathController = 2,
        kFootController = 4,
        kPortamentoTime = 5,
        kDataEntryMSB = 6,
        kVolume = 7,
        kBalance = 8,
        kPan = 10,
        kExpression = 11,
        kSustainPedal = 64,
        kPortamentoSwitch = 65,
        kSostenutoPedal = 66,
        kSoftPedal = 67,
        kLegatoSwitch = 68,
        kAllSoundOff = 120,
        kResetAllControllers = 121,
        kAllNotesOff = 123
    };

    using MidiParameterMap = std::map<int, std::map<int, std::string>>;  // channel -> (cc -> parameter)

    /**
     * @brief Listener interface for MIDI-triggered events
     */
    class Listener {
    public:
        virtual ~Listener() = default;
        
        // Called when a parameter value is changed via MIDI
        virtual void parameterChangedViaMidi(const std::string& paramId, float value) = 0;
        
        // Called for specific common controllers
        virtual void pitchBendChanged(int channel, float value) = 0;
        virtual void modWheelChanged(int channel, float value) = 0;
        virtual void afterTouchChanged(int channel, float value) = 0;
    };

    /**
     * @brief Create a new MIDIManager
     * @param synthesizer The synthesizer to control
     * @param listener Optional listener for MIDI-triggered events
     */
    MidiManager(Synthesizer* synthesizer, Listener* listener = nullptr);
    ~MidiManager() override;

    /**
     * Set a listener for MIDI-triggered events
     * @param listener The listener to set (can be nullptr to remove)
     */
    void setListener(Listener* listener) { listener_ = listener; }

    // MidiInputCallback implementation
    void handleIncomingMidiMessage(const MidiMessage& message) override;

    // Process a MIDI message (can be called directly or via callback)
    void processMidiMessage(const MidiMessage& message, int samplePosition = 0);

    // Parameter MIDI learn functionality
    void armMidiLearn(const std::string& paramId);
    void cancelMidiLearn();
    void clearMidiLearn(const std::string& paramId);
    bool isMidiMapped(const std::string& paramId) const;

    // Setup methods
    bool openMidiInput(int deviceIndex);
    void closeMidiInput();
    std::vector<std::string> getMidiInputDevices() const;
    
    bool openMidiOutput(int deviceIndex);
    void closeMidiOutput();
    std::vector<std::string> getMidiOutputDevices() const;

    // MIDI parameter mapping
    MidiParameterMap getMidiMappings() const;
    void setMidiMappings(const MidiParameterMap& mappings);
    
    // Message type processors
    void processNoteOn(const MidiMessage& message, int samplePosition);
    void processNoteOff(const MidiMessage& message, int samplePosition);
    void processControlChange(const MidiMessage& message, int samplePosition);
    void processPitchBend(const MidiMessage& message, int samplePosition);
    void processAfterTouch(const MidiMessage& message, int samplePosition);
    void processChannelPressure(const MidiMessage& message, int samplePosition);
    void processAllNotesOff(const MidiMessage& message, int samplePosition);
    void processSustain(const MidiMessage& message, int samplePosition);

public:
    // Different scaling types for MIDI to parameter conversion
    enum class ParameterScaling {
        Linear,     // Linear mapping (default)
        Logarithmic, // Logarithmic mapping (good for frequencies)
        Exponential, // Exponential mapping (good for times)
        Stepped      // Stepped mapping (for discrete values)
    };
    
    // Parameter mapping information
    struct ParameterMapping {
        std::string paramId;
        ParameterScaling scaling = ParameterScaling::Linear;
        float min = 0.0f;  // Minimum parameter value
        float max = 1.0f;  // Maximum parameter value
        int steps = 0;     // For stepped parameters, number of discrete steps (0 = continuous)
    };
    
private:
    // Maps a MIDI controller value (0-127) to a parameter value
    float midiValueToParameter(int value) const;
    float midiValueToParameter(int value, ParameterScaling scaling, 
                              float min, float max, int steps) const;
    
    // Maps a parameter value to a MIDI controller value (0-127)
    int parameterToMidiValue(float value) const;
    int parameterToMidiValue(float value, ParameterScaling scaling, 
                            float min, float max, int steps) const;
    
    // Handles parameter mapping for learning and updates
    void updateMappedParameter(int channel, int controller, int value);

    Synthesizer* synthesizer_;
    Listener* listener_;
    
    std::unique_ptr<MidiInput> midiInput_;
    std::unique_ptr<MidiOutput> midiOutput_;
    
    // Parameter being learned
    std::string learnParamId_;
    
    // MIDI parameter mappings: channel -> (controller -> parameter ID)
    MidiParameterMap midiMappings_;
    
    // Thread safety
    mutable std::mutex mappingMutex_;
    mutable std::mutex learnMutex_;
};

} // namespace AIMusicHardware