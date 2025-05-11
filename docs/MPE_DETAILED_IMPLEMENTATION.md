# Detailed MPE Implementation for AIMusicHardware

This document provides a comprehensive implementation plan for adding MIDI Polyphonic Expression (MPE) support to the AIMusicHardware project, based on the official MPE specification (MIDI-CI Specification M1-100-UM).

## 1. MPE Technical Requirements

### 1.1 Zone Configuration

MPE divides MIDI channels into zones, each with a Master Channel and several Member Channels:

- **Lower Zone**:
  - Master Channel: 1
  - Member Channels: 2-N (typically 2-8 or more)
  
- **Upper Zone**:
  - Master Channel: 16
  - Member Channels: 15 to 16-M (typically 15-9 or more)

### 1.2 Required Messages

#### 1.2.1 Member Channel Messages (Per-Note)

- **Note On/Off**: Note events with standard velocity
- **Pitch Bend**: For pitch dimension (X-axis)
- **CC74 (Timbre)**: For timbre dimension (Y-axis)
- **Channel Pressure**: For pressure dimension (Z-axis)

#### 1.2.2 Master Channel Messages (Global)

- **Registered Parameter Number (RPN) 0x6**: For MPE configuration
- **Program Change**: For selecting sounds
- **Control Change Messages**:
  - CC64 (Sustain Pedal)
  - CC1 (Modulation Wheel)
  - CC7 (Volume)
  - CC11 (Expression)
  - Other CCs not used for per-note expression

### 1.3 Zone Management

- The MPE receiver must respond to RPN 0x6 to configure MPE zones
- The value of RPN 0x6 sets the number of member channels in a zone

### 1.4 Default Values

- Default Pitch Bend Range: ±48 semitones (implementation-specific)
- Default Y-axis (CC74) Range: 0-127
- Default Pressure Range: 0-127

## 2. Class Structure

### 2.1 MPE Configuration & State Management

```cpp
class MpeConfiguration {
public:
    // Zone configuration
    struct Zone {
        bool active = false;
        int masterChannel = 1;
        int startMemberChannel = 2;
        int endMemberChannel = 8;
        bool ascending = true;  // Channel direction
        
        // Zone properties
        int pitchBendRange = 48;  // Default ±48 semitones
        int defaultTimbre = 64;   // Default timbre value (centered)
    };
    
    MpeConfiguration();
    
    // Zone management
    void setLowerZone(bool active, int memberChannels = 7);
    void setUpperZone(bool active, int memberChannels = 7);
    
    // Get zone information
    Zone getLowerZone() const { return lowerZone_; }
    Zone getUpperZone() const { return upperZone_; }
    
    // Determine if a channel belongs to an MPE zone
    bool isInLowerZone(int channel) const;
    bool isInUpperZone(int channel) const;
    bool isMasterChannel(int channel) const;
    bool isMemberChannel(int channel) const;
    
    // Process MPE configuration RPN
    void processRpn(int channel, int rpnMsb, int rpnLsb, int dataMsb, int dataLsb);
    
    // Set active channels for both global use and per-zone
    void setActiveChannels(const std::array<bool, 16>& activeChannels);
    std::array<bool, 16> getActiveChannels() const;
    
private:
    Zone lowerZone_;
    Zone upperZone_;
    std::array<bool, 16> activeChannels_;
};
```

### 2.2 Enhanced MIDI Router for MPE

```cpp
class MultiTimbralMidiRouter {
public:
    MultiTimbralMidiRouter(MultiTimbralEngine* engine);
    
    // MPE Configuration
    void enableMpeMode(bool enable);
    bool isMpeMode() const;
    
    void configureMpeLowerZone(bool active, int memberChannels = 7);
    void configureMpeUpperZone(bool active, int memberChannels = 7);
    
    // RPN message handling for MPE configuration
    void handleRpnMessage(int channel, int rpnMsb, int rpnLsb, int dataMsb, int dataLsb);
    
    // Message processing
    void processMidiMessage(const MidiMessage& message);
    
private:
    // MPE message handlers
    void processMpeNoteOn(const MidiMessage& message);
    void processMpeNoteOff(const MidiMessage& message);
    void processMpePitchBend(const MidiMessage& message);
    void processMpeTimbre(const MidiMessage& message);
    void processMpePressure(const MidiMessage& message);
    void processMpeControlChange(const MidiMessage& message);
    
    // Channel management
    int allocateMpeChannel(int note, int velocity, bool lowerZone);
    void releaseMpeChannel(int channel);
    
    // Note tracking
    struct NoteInfo {
        int midiNote;
        int channel;
        uint32_t timestamp;
    };
    
    std::vector<NoteInfo> activeNotes_;
    
    // Channel state tracking
    struct ChannelState {
        bool inUse = false;
        int noteNumber = -1;
        float pitchBend = 0.0f;   // -1.0 to 1.0
        float timbre = 0.5f;      // 0.0 to 1.0 (CC74)
        float pressure = 0.0f;    // 0.0 to 1.0 (Channel Pressure)
        uint32_t timestamp = 0;   // When this channel was allocated
    };
    
    std::array<ChannelState, 16> channelStates_;
    
    // Get appropriate zone for a note (for keyboard splits)
    bool isInLowerZone(int noteNumber) const;
    
    // Expression scaling utils
    float scalePitchBend(int rawValue) const;
    float scaleTimbre(int rawValue) const;
    float scalePressure(int rawValue) const;
    
    // Configuration
    MpeConfiguration mpeConfig_;
    MultiTimbralEngine* engine_;
    bool mpeMode_ = false;
    uint32_t messageCounter_ = 0;  // For timestamps
};
```

### 2.3 Enhanced Voice Manager for MPE

```cpp
class MpeAwareVoiceManager : public VoiceManager {
public:
    MpeAwareVoiceManager(int sampleRate = 44100, int maxVoices = 16);
    
    // MPE-specific note control
    void noteOnMpe(int midiNote, float velocity, int channel, 
                  float pitchBend = 0.0f, float timbre = 0.5f, float pressure = 0.0f);
    
    void noteOffMpe(int midiNote, int channel);
    
    // Expression updates
    void updatePitchBend(int channel, float normalizedValue);
    void updateTimbre(int channel, float normalizedValue);
    void updatePressure(int channel, float normalizedValue);
    
    // MPE configuration
    void setMpeMode(bool enabled);
    bool isMpeMode() const;
    
    void setPitchBendRange(int semitones, int channel = -1);  // -1 for all channels
    int getPitchBendRange(int channel = 0) const;
    
    // Voice allocation in MPE mode
    Voice* findVoiceForChannel(int channel);
    
private:
    // MPE specific state
    bool mpeMode_ = false;
    std::unordered_map<int, int> channelToVoiceMap_;  // Maps MIDI channel to voice index
    std::array<int, 16> pitchBendRanges_;  // Per-channel pitch bend ranges
    
    // Enhanced voice finding for MPE
    Voice* findVoiceForNoteMpe(int midiNote, int channel);
    Voice* findFreeVoice();
    Voice* stealVoiceForMpe(int channel);
};
```

### 2.4 Enhanced Voice Class for MPE

```cpp
class MpeAwareVoice : public Voice {
public:
    MpeAwareVoice(int sampleRate = 44100);
    
    // MPE-specific control
    void setTimbre(float value);  // 0.0 to 1.0
    float getTimbre() const;
    
    // Override base methods
    void setPitchBend(float semitones) override;
    void setPressure(float pressure) override;
    
    // Process all expression parameters
    void updateExpression(float pitchBend, float timbre, float pressure);
    
    // Generate sound with MPE expression
    float generateSample() override;
    
private:
    // MPE-specific parameters
    float timbre_ = 0.5f;  // Default centered at 0.5
    
    // Expression processing
    float processTimbreModulation() const;
    float processPressureModulation() const;
};
```

### 2.5 Extending MultiTimbralEngine

```cpp
class MpeEnabledMultiTimbralEngine : public MultiTimbralEngine {
public:
    MpeEnabledMultiTimbralEngine(int sampleRate = 44100, int maxTotalVoices = 64);
    
    // MPE configuration
    void enableMpeMode(bool enable);
    bool isMpeMode() const;
    
    void setMpeLowerZone(bool active, int memberChannels);
    void setMpeUpperZone(bool active, int memberChannels);
    
    // MPE message handling
    void noteOnWithExpression(int midiNote, float velocity, int channel,
                             float pitchBend, float timbre, float pressure);
    
    void updateExpression(int channel, float pitchBend, float timbre, float pressure);
    
    // MPE zone configuration via RPN
    void configureMpeZone(int masterChannel, int memberChannels);
    
private:
    // MPE state
    bool mpeMode_ = false;
    MpeConfiguration mpeConfig_;
    
    // Get the appropriate voice manager for a channel
    VoiceManager* getVoiceManagerForChannel(int channel);
    
    // Create an MPE-aware voice manager
    std::unique_ptr<MpeAwareVoiceManager> createMpeVoiceManager(int sampleRate, int voices);
};
```

## 3. Detailed Implementation Steps

### 3.1 MPE Message Processing Flow

```
1. MIDI Message Received
   |
   +--> Is MPE Mode Active?
         |
         +--> Yes --> Is it a Member Channel Message?
         |             |
         |             +--> Yes --> Process Per-Note Message
         |             |             |
         |             |             +--> Note On --> Allocate Channel, Track Note
         |             |             |
         |             |             +--> Note Off --> Release Channel
         |             |             |
         |             |             +--> Pitch Bend --> Update Expression
         |             |             |
         |             |             +--> CC74 --> Update Timbre
         |             |             |
         |             |             +--> Channel Pressure --> Update Pressure
         |             |
         |             +--> No --> Is it a Master Channel Message?
         |                           |
         |                           +--> Yes --> Process Global Message
         |                           |             |
         |                           |             +--> RPN 0x6 --> Configure MPE Zone
         |                           |             |
         |                           |             +--> Program Change --> Update Sound
         |                           |             |
         |                           |             +--> CCs --> Update Global Parameters
         |                           |
         |                           +--> No --> Ignore (Invalid in MPE)
         |
         +--> No --> Process Standard Multi-Timbral Message
```

### 3.2 Channel Allocation Algorithm

```cpp
int MultiTimbralMidiRouter::allocateMpeChannel(int note, int velocity, bool lowerZone) {
    const auto& zone = lowerZone ? mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
    
    if (!zone.active) {
        return -1;  // Zone not active
    }
    
    // Determine channel range
    int startChannel = zone.startMemberChannel;
    int endChannel = zone.endMemberChannel;
    bool ascending = zone.ascending;
    
    // Try to find a free channel in the zone
    std::vector<std::pair<int, uint32_t>> candidates;  // channel, timestamp
    
    for (int ch = startChannel; 
         ascending ? (ch <= endChannel) : (ch >= endChannel); 
         ascending ? ch++ : ch--) {
        
        if (!channelStates_[ch-1].inUse) {
            // Found unused channel
            channelStates_[ch-1].inUse = true;
            channelStates_[ch-1].noteNumber = note;
            channelStates_[ch-1].timestamp = messageCounter_++;
            return ch - 1;  // Return 0-based channel
        }
        
        // Store as candidate for stealing if needed
        candidates.push_back({ch - 1, channelStates_[ch-1].timestamp});
    }
    
    // No free channel, need to steal
    if (candidates.empty()) {
        return -1;  // No channels available in this zone
    }
    
    // Find oldest allocated channel
    std::sort(candidates.begin(), candidates.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    int channelToSteal = candidates[0].first;
    
    // Release and reallocate
    releaseMpeChannel(channelToSteal);
    
    channelStates_[channelToSteal].inUse = true;
    channelStates_[channelToSteal].noteNumber = note;
    channelStates_[channelToSteal].timestamp = messageCounter_++;
    
    return channelToSteal;
}

void MultiTimbralMidiRouter::releaseMpeChannel(int channel) {
    if (channel < 0 || channel >= 16) {
        return;
    }
    
    channelStates_[channel].inUse = false;
    channelStates_[channel].noteNumber = -1;
    channelStates_[channel].pitchBend = 0.0f;
    channelStates_[channel].timbre = 0.5f;
    channelStates_[channel].pressure = 0.0f;
}
```

### 3.3 RPN Processing for MPE Configuration

```cpp
void MultiTimbralMidiRouter::handleRpnMessage(int channel, int rpnMsb, int rpnLsb, 
                                             int dataMsb, int dataLsb) {
    // Check if this is MPE Configuration message (RPN 0x6)
    if (rpnMsb == 0x00 && rpnLsb == 0x06) {
        // This is MPE Configuration
        int memberChannels = dataMsb;
        
        if (channel == 0) {  // Channel 1 (0-based)
            // Configure Lower Zone
            configureMpeLowerZone(memberChannels > 0, memberChannels);
        } else if (channel == 15) {  // Channel 16 (0-based)
            // Configure Upper Zone
            configureMpeUpperZone(memberChannels > 0, memberChannels);
        }
    }
}
```

### 3.4 Expression Update Processing

```cpp
void MpeAwareVoiceManager::updatePitchBend(int channel, float normalizedValue) {
    // Find all voices assigned to this channel
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (voices_[i]->getChannel() == channel && voices_[i]->isActive()) {
            // Convert normalized value (-1.0 to 1.0) to semitones based on range
            float semitones = normalizedValue * pitchBendRanges_[channel];
            voices_[i]->setPitchBend(semitones);
        }
    }
}

void MpeAwareVoiceManager::updateTimbre(int channel, float normalizedValue) {
    // Find all voices assigned to this channel
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (voices_[i]->getChannel() == channel && voices_[i]->isActive()) {
            dynamic_cast<MpeAwareVoice*>(voices_[i].get())->setTimbre(normalizedValue);
        }
    }
}

void MpeAwareVoiceManager::updatePressure(int channel, float normalizedValue) {
    // Find all voices assigned to this channel
    for (size_t i = 0; i < voices_.size(); ++i) {
        if (voices_[i]->getChannel() == channel && voices_[i]->isActive()) {
            voices_[i]->setPressure(normalizedValue);
        }
    }
}
```

## 4. Synthesis Implementation

### 4.1 Timbre Implementation (Y-axis)

The Y-axis in MPE typically controls timbre or brightness, corresponding to MIDI CC74. We'll implement this as a filter cutoff modulation:

```cpp
float MpeAwareVoice::processTimbreModulation() const {
    // Map timbre value (0.0-1.0) to filter cutoff modulation
    // Center point (0.5) means no modulation
    float modAmount = (timbre_ - 0.5f) * 2.0f;  // Range: -1.0 to 1.0
    
    // Scale to frequency range
    // For example, modulate between 200Hz and 20kHz
    float baseFreq = 1000.0f;  // 1kHz baseline
    float modFreq;
    
    if (modAmount >= 0.0f) {
        // Upward modulation (brighter)
        modFreq = baseFreq * std::pow(20.0f, modAmount);
    } else {
        // Downward modulation (darker)
        modFreq = baseFreq * std::pow(5.0f, modAmount);
    }
    
    return modFreq;
}
```

### 4.2 Pressure Implementation (Z-axis)

Pressure (Z-axis) typically affects amplitude or other parameters:

```cpp
float MpeAwareVoice::processPressureModulation() const {
    // Map pressure to amplitude modulation
    // Can also affect other parameters like distortion or modulation depth
    
    // Example: Linear amplitude scaling
    float ampScale = 0.5f + (pressure_ * 0.5f);  // Range: 0.5 to 1.0
    
    return ampScale;
}
```

### 4.3 Complete Sample Generation with MPE

```cpp
float MpeAwareVoice::generateSample() {
    // Compute base oscillator value
    float oscillatorValue = oscillator_->generateSample();
    
    // Apply envelope
    float envelopeValue = envelope_->generateValue();
    
    // Apply pitch bend (already handled in oscillator frequency)
    
    // Apply timbre
    float filterCutoff = processTimbreModulation();
    filter_->setCutoff(filterCutoff);
    float filteredValue = filter_->process(oscillatorValue);
    
    // Apply pressure to amplitude
    float amplitudeScale = processPressureModulation();
    
    // Compute final sample
    float finalSample = filteredValue * envelopeValue * velocity_ * amplitudeScale;
    
    // Update state
    if (state_ == State::Starting && envelopeValue > 0.01f) {
        state_ = State::Playing;
    } else if (state_ == State::Released && !envelope_->isActive()) {
        state_ = State::Finished;
    }
    
    return finalSample;
}
```

## 5. Demo Implementation

### 5.1 MPE Debug Visualization

To help with development and debugging, we'll implement a visualization of MPE data:

```cpp
class MpeVisualizer {
public:
    MpeVisualizer();
    
    // Update with current MPE state
    void update(const std::array<ChannelState, 16>& channelStates);
    
    // Render to console
    void renderToConsole() const;
    
private:
    std::array<ChannelState, 16> lastStates_;
    
    // Helper to visualize expression values
    std::string getExpressionBar(float value, int width = 20) const;
};
```

### 5.2 MPE Test Generator

To test without an MPE controller, we'll implement a test generator:

```cpp
class MpeTestGenerator {
public:
    MpeTestGenerator(MultiTimbralMidiRouter* router);
    
    // Generate test sequences
    void generateChord();
    void generateArpeggio();
    void generateExpressiveGesture();
    
    // Update expression over time
    void update();
    
private:
    MultiTimbralMidiRouter* router_;
    std::vector<std::pair<int, int>> activeNotes_;  // note, channel
    
    // Generate sine-based modulation for testing
    float generateSineModulation(float time, float frequency, float min, float max) const;
};
```

## 6. Integration Plan

### 6.1 Phase 1: Core MPE Classes (2-3 days)
1. Implement `MpeConfiguration` class
2. Enhance `MultiTimbralMidiRouter` for MPE message routing
3. Add channel allocation algorithm

### 6.2 Phase 2: Voice Expression (2-3 days)
1. Extend `Voice` class with MPE expression capabilities
2. Implement `MpeAwareVoiceManager` for per-note expression
3. Add handling for all three expression dimensions

### 6.3 Phase 3: Engine Integration (1-2 days)
1. Extend `MultiTimbralEngine` for MPE support
2. Connect router, voice manager, and synthesis engine
3. Implement RPN handling for configuration

### 6.4 Phase 4: Testing & Demo (2-3 days)
1. Create comprehensive MPE testing utilities
2. Build MPE visualization tools
3. Implement MPE test generator
4. Extend demo interface for MPE configuration

## 7. Testing Strategy

### 7.1 Unit Tests

1. **Channel Allocation Tests**:
   - Test allocation in Lower Zone (ascending)
   - Test allocation in Upper Zone (descending)
   - Verify correct channel stealing when full
   - Test release and reuse of channels

2. **Expression Processing Tests**:
   - Verify pitch bend scaling works correctly
   - Test timbre control affects filter correctly
   - Confirm pressure changes amplitude appropriately
   - Ensure independence of expression between voices

3. **MPE Configuration Tests**:
   - Test RPN 0x6 processing
   - Verify zone configuration changes behavior
   - Test mixed mode operation (MPE + standard)

### 7.2 Integration Tests

1. **Full Path Tests**:
   - Process MIDI messages through router to engine
   - Verify correct voice allocation and expression
   - Test channel independence

2. **Performance Tests**:
   - Measure CPU usage with varying voice counts
   - Test rapid expression changes
   - Benchmark channel allocation/deallocation

### 7.3 User Tests

1. **Controller Tests**:
   - Test with MPE controller if available
   - Alternatively use test generator
   - Verify expressive capabilities

2. **Musical Tests**:
   - Play various musical phrases
   - Test chord playing with independent expression
   - Verify musical applications of MPE

## 8. Conclusion

This detailed implementation plan provides a comprehensive roadmap for adding MPE support to the AIMusicHardware project, aligning with the official MPE specification. The approach preserves existing multi-timbral functionality while extending it with MPE capabilities, allowing for both traditional MIDI operation and expressive performance features.

The implementation is designed to be flexible, supporting both Lower and Upper MPE zones, proper channel rotation, all three dimensions of expression, and full RPN-based configuration as required by the MPE specification.