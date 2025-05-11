# MPE Implementation Plan for AIMusicHardware

This document outlines the plan for implementing MIDI Polyphonic Expression (MPE) support in the AIMusicHardware project, complementing the existing multi-timbral functionality.

## 1. Architecture Overview

We'll extend our current architecture to support both traditional multi-timbral operation and MPE operation, with the ability to switch between them or use them simultaneously in split configurations.

### Key Components to Modify:

1. **MultiTimbralMidiRouter**: Add MPE routing capabilities
2. **MultiTimbralEngine**: Support both multi-timbral and MPE synthesis modes
3. **VoiceManager**: Enhance to handle per-voice expression parameters
4. **ChannelSynthesizer**: Add MPE mode where all channels share the same parameters except for expression

## 2. MultiTimbralMidiRouter Enhancements

```cpp
class MultiTimbralMidiRouter {
public:
    // MPE Zone configuration
    struct MpeZoneConfig {
        bool active = false;
        int masterChannel = 1;  // 1 for Lower Zone, 16 for Upper Zone
        int startMemberChannel = 2;  // First member channel (2 for Lower, 15 for Upper)
        int endMemberChannel = 8;    // Last member channel (varies based on allocation)
        bool ascending = true;       // Channel direction (true for Lower, false for Upper)
    };
    
    // Configure MPE zones
    void setLowerZone(bool active, int numChannels = 7);  // Channels 2-8 by default
    void setUpperZone(bool active, int numChannels = 7);  // Channels 15-9 by default
    bool isInMpeMode() const;
    
    // Get zone information
    MpeZoneConfig getLowerZoneConfig() const;
    MpeZoneConfig getUpperZoneConfig() const;
    
private:
    // MPE configuration
    MpeZoneConfig lowerZone_;
    MpeZoneConfig upperZone_;
    
    // MPE channel management
    struct MpeChannelState {
        bool inUse = false;
        int noteNumber = -1;
        uint8_t noteId = 0;  // For tracking specific note instances
        float pitchBend = 0.0f;
        float timbre = 0.0f;
        float pressure = 0.0f;
    };
    
    std::array<MpeChannelState, 16> channelStates_;
    
    // Channel allocation for MPE
    int findFreeMpeChannel(const MpeZoneConfig& zone);
    void releaseChannel(int channel);
    int getZoneForNote(int noteNumber) const;  // Determines which zone handles this note
    
    // MPE message handlers
    void processMpeNoteOn(const MidiMessage& message);
    void processMpeNoteOff(const MidiMessage& message);
    void processMpePitchBend(const MidiMessage& message);
    void processMpeTimbre(const MidiMessage& message);  // CC74
    void processMpePressure(const MidiMessage& message);
    void processMpeGlobalControl(const MidiMessage& message);  // Master channel messages
};
```

## 3. Voice Manager Enhancements

```cpp
class VoiceManager {
public:
    // Set MPE mode for voice management
    void setMpeMode(bool enabled);
    bool isMpeMode() const;
    
    // Enhanced note control for MPE
    void noteOnWithExpression(int midiNote, float velocity, 
                             int channel,
                             float pitchBend = 0.0f,
                             float timbre = 0.5f,
                             float pressure = 0.0f);
    
    // Expression updates (called when expression changes for a note)
    void updateNotePitchBend(int channel, float pitchBend);
    void updateNoteTimbre(int channel, float timbre);
    void updateNotePressure(int channel, float pressure);
    
private:
    bool mpeMode_ = false;
    
    // Mapping of active notes to their MIDI channels in MPE mode
    std::map<int, int> noteToChannelMap_;  // midiNote -> channel
    
    // Per-voice expression data in MPE mode
    struct VoiceExpressionData {
        float pitchBend = 0.0f;  // -1.0 to 1.0
        float timbre = 0.5f;     // 0.0 to 1.0 (CC74)
        float pressure = 0.0f;   // 0.0 to 1.0 (Channel Pressure)
    };
    
    std::vector<VoiceExpressionData> voiceExpressionData_;  // Parallels the voices_ vector
    
    // Enhanced voice processing for MPE
    void processMpeVoice(Voice* voice, const VoiceExpressionData& expressionData);
};
```

## 4. MultiTimbralEngine Enhancements

```cpp
class MultiTimbralEngine {
public:
    // MPE configuration
    void setMpeMode(bool enabled);
    bool isMpeMode() const;
    
    void setMpeLowerZone(bool active, int numChannels = 7);
    void setMpeUpperZone(bool active, int numChannels = 7);
    
    // Support for MPE expression controls
    void updateExpression(int channel, float pitchBend, float timbre, float pressure);
    
    // Hybrid mode where some channels are MPE and others are standard
    void setChannelMpeMode(int channel, bool mpeMode);
    bool isChannelMpeMode(int channel) const;
    
private:
    bool mpeMode_ = false;
    
    // Configuration for active MPE zones
    bool mpeLowerZoneActive_ = false;
    int mpeLowerZoneChannels_ = 7;  // How many member channels in lower zone
    bool mpeUpperZoneActive_ = false;
    int mpeUpperZoneChannels_ = 7;  // How many member channels in upper zone
    
    // Channel modes
    std::array<bool, 16> channelMpeMode_; // Whether each channel is in MPE mode
};
```

## 5. Voice Class Enhancements

```cpp
class Voice {
public:
    // Enhanced expression controls for MPE
    void setPitchBendContinuous(float semitones);  // Direct bend amount
    void setTimbre(float value);  // CC74 (0.0 to 1.0)
    void setPressure(float value);  // Channel pressure (0.0 to 1.0)
    
    // Get current expression values
    float getPitchBendContinuous() const;
    float getTimbre() const;
    float getPressure() const;
    
private:
    // MPE-specific expression parameters
    float timbre_ = 0.5f;  // Centered at 0.5
    
    // Current continuous pitch bend in semitones (versus the step value)
    float pitchBendSemitonesContinuous_ = 0.0f;
};
```

## 6. Implementation Phases

### Phase 1: MPE Channel Management
- Implement channel allocation and tracking in MultiTimbralMidiRouter
- Add zone configuration for Lower and Upper zones
- Set up routing of MPE messages to the appropriate channels

### Phase 2: Voice Manager MPE Support
- Extend VoiceManager to handle per-voice expression
- Implement note-to-channel mapping
- Add support for continuous expression updates

### Phase 3: Voice Expression Implementation
- Enhance Voice class with full expression parameters
- Add support for timbre (CC74) and smooth pitch bends
- Ensure expression parameters are properly processed in synthesis

### Phase 4: Engine Integration
- Connect the router to the enhanced voice manager
- Set up MPE mode switching
- Ensure compatibility with existing multi-timbral functionality

### Phase 5: Demo and Testing
- Create an MPE demo application
- Test with MPE controllers or simulation
- Verify smooth interaction between traditional multi-timbral and MPE modes

## 7. Demo Interface

We'll extend the MultiTimbralDemo application with MPE features:

```cpp
// In MultiTimbralDemo.cpp, add:
void setupMpeMode(MultiTimbralEngine* engine, MultiTimbralMidiRouter* router) {
    std::cout << "Setup MPE Configuration" << std::endl;
    std::cout << "1. Enable Lower Zone only (channels 1 + 2-8)" << std::endl;
    std::cout << "2. Enable Upper Zone only (channels 16 + 15-9)" << std::endl;
    std::cout << "3. Enable both zones (Split keyboard)" << std::endl;
    std::cout << "4. Disable MPE mode" << std::endl;
    
    // Handle user input and configure MPE zones
    // ...
}
```

## 8. Testing Strategies

1. **Channel Rotation Test**:
   - Verify notes are properly assigned to channels
   - Check channel reuse after notes are released
   - Test with rapid note sequences to stress the allocation system

2. **Expression Continuity Test**:
   - Ensure pitch bend is smooth and per-note
   - Check that timbre and pressure controls modify only the intended notes
   - Test with simultaneous expression changes on multiple notes

3. **Zone Interaction Test**:
   - Verify proper handling of both Lower and Upper zones
   - Test split keyboard configuration
   - Ensure master channel messages are properly applied

4. **Performance Test**:
   - Check CPU usage with multiple expressive voices
   - Verify real-time response to rapid expression changes
   - Test polyphony limits with heavy expression modulation