# Multi-Timbral Architecture for AIMusicHardware

*May 11, 2025*

This document outlines the design and implementation of multi-timbral support for the AIMusicHardware project, allowing multiple simultaneous instruments with different sounds on separate MIDI channels.

## 1. Overview

Multi-timbral capability lets our synthesizer play different sounds on different MIDI channels simultaneously - a fundamental feature for professional synthesizers. This architecture enables advanced performance setups including:

- Playing different sounds with multiple controllers
- Creating layered/split keyboard configurations
- Building complex arrangements with separate instruments
- Providing DAW integration with multi-track control

## 2. Architecture Design

![Multi-Timbral Architecture Overview](https://via.placeholder.com/800x500.png?text=Multi-Timbral+Architecture+Diagram)

### 2.1 Core Components

#### 2.1.1 MultiTimbralEngine

This central class manages multiple synthesizer instances, routing MIDI data and mixing audio:

```cpp
class MultiTimbralEngine {
private:
    // Array of synthesizers, one per MIDI channel
    std::array<std::unique_ptr<ChannelSynthesizer>, 16> channelSynths_;
    
    // Channel active status
    std::array<bool, 16> channelActive_;
    
    // Output mixing settings
    float masterVolume_;
    std::array<float, 16> channelVolumes_;
    std::array<float, 16> channelPans_;
    
    // Resource management
    int maxTotalVoices_;
    int sampleRate_;
    VoiceAllocationStrategy voiceStrategy_;
    
    // Audio buffer for mixing
    std::vector<float> mixBuffer_;
    
    // Voice allocation mutexes
    std::mutex voiceAllocationMutex_;
    
public:
    MultiTimbralEngine(int sampleRate = 44100, int maxTotalVoices = 64);
    ~MultiTimbralEngine();
    
    // MIDI event handling
    void noteOn(int midiNote, float velocity, int channel);
    void noteOff(int midiNote, int channel);
    void programChange(int program, int channel);
    void controlChange(int controller, int value, int channel);
    void pitchBend(float value, int channel);
    void aftertouch(int note, float value, int channel);
    void channelPressure(float value, int channel);
    
    // Channel management
    void setChannelActive(int channel, bool active);
    bool isChannelActive(int channel) const;
    void setChannelVolume(int channel, float volume);
    void setChannelPan(int channel, float pan);
    
    // Voice allocation
    void setMaxTotalVoices(int maxVoices);
    void allocateVoices(); // Distributes available voices among active channels
    
    // Preset management
    void loadPreset(const Preset& preset, int channel);
    void loadPresetFromFile(const std::string& filePath, int channel);
    
    // Master audio processing
    void process(float* outputBuffer, int numFrames);
    void setSampleRate(int sampleRate);
    
    // Access to individual channel synthesizers
    ChannelSynthesizer* getChannelSynth(int channel);
    
    // Additional performance modes
    void setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel);
    void setupLayeredChannels(const std::vector<int>& channels);
};
```

#### 2.1.2 ChannelSynthesizer

Extended from our existing Synthesizer, this class maintains channel-specific state:

```cpp
class ChannelSynthesizer : public Synthesizer {
private:
    int channelNumber_;
    
    // Channel-specific settings
    int channelPriority_;
    bool receivesProgramChange_;
    int transposition_;
    int noteRangeLow_;
    int noteRangeHigh_;
    
    // Performance memory
    std::vector<int> lastPlayedNotes_; // For mono/legato modes

public:
    ChannelSynthesizer(int channel, int sampleRate = 44100);
    
    // Channel-specific settings
    void setChannel(int channel);
    int getChannel() const;
    
    // Note range limiting (for splits)
    void setNoteRange(int low, int high);
    bool isNoteInRange(int midiNote) const;
    
    // Parameter management
    void loadChannelParameters(const ChannelParameters& params);
    ChannelParameters saveChannelParameters() const;
    
    // Override base class methods to handle channel-specific behavior
    void processChannelMidiCC(int controller, int value);
    void processChannelProgramChange(int program);
    
    // Specialized voice allocation
    void setChannelPriority(int priority);
    int getChannelPriority() const;
    
    // Process audio for just this channel
    void process(float* buffer, int numFrames);
};
```

#### 2.1.3 ChannelParameters

A container for channel-specific parameters:

```cpp
struct ChannelParameters {
    // Basic channel settings
    int channelNumber;
    std::string name;
    int polyphony;
    int priority;
    
    // Performance settings
    bool monophonic;
    int portamentoTime;
    int transposition;
    int fineTuning;
    int noteRangeLow;
    int noteRangeHigh;
    
    // Mixing settings
    float volume;
    float pan;
    
    // MIDI settings
    bool receivesProgramChange;
    bool receivesControllers;
    bool receivesPitchBend;
    std::unordered_map<int, bool> ccResponseMap; // Which CCs this channel responds to
    
    // Preset identification
    int programNumber;
    std::string presetName;
    
    // Save/load methods
    void toJson(json& j) const;
    void fromJson(const json& j);
};
```

#### 2.1.4 Enhanced Voice Management

Our current `VoiceManager` already has per-channel tracking, but needs extensions:

```cpp
class VoiceManager {
    // Existing implementation...
    
    // New/modified methods:
    
    // Voice allocation with global voice count and priority management
    void setChannelPriority(int channel, int priority);
    int getVoiceCountForChannel(int channel) const;
    void setMaxVoicesForChannel(int channel, int maxVoices);
    int reallocateVoices(const std::vector<std::pair<int, int>>& channelPriorities);
    
    // For voice stealing across multiple channels
    enum class GlobalStealStrategy {
        LowestPriorityChannel,
        EqualDistribution,
        DynamicAllocation
    };
    
    void setGlobalStealStrategy(GlobalStealStrategy strategy);
};
```

### 2.2 MIDI Manager Extensions

The MidiManager needs updates to support multi-timbral operation:

```cpp
class MidiManager {
    // Existing implementation...
    
private:
    // New members:
    MultiTimbralEngine* engine_;
    std::array<bool, 16> channelMask_; // For filtering specific channels
    std::unordered_map<int, std::vector<int>> keyboardSplits_; // Maps note ranges to channels
    
public:
    // New/modified methods:
    
    // Channel routing
    void setChannelMask(int channel, bool enabled);
    void setMultiTimbralEngine(MultiTimbralEngine* engine);
    
    // Specialized routing
    void setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel);
    void setupNoteSplit(const std::vector<std::pair<int, int>>& rangesToChannels);
    void setupVelocitySplit(int velocityThreshold, int lowerChannel, int upperChannel);
    
    // Program changes
    void sendProgramChange(int program, int channel);
    void mapProgramChangeTable(int channel, const std::vector<int>& programMap);
};
```

### 2.3 User Interface for Multi-Timbral Control

The UI needs specialized components to manage multi-timbral settings:

```cpp
class MultiTimbralPanel : public UIComponent {
private:
    MultiTimbralEngine* engine_;
    std::vector<ChannelStripComponent*> channelStrips_;
    TabView* channelDetailView_;
    
public:
    MultiTimbralPanel(MultiTimbralEngine* engine);
    
    // UI methods for managing channels
    void onChannelSelectClick(int channel);
    void onChannelMuteToggle(int channel, bool muted);
    void onChannelVolumeChange(int channel, float volume);
    void onChannelPresetClick(int channel);
    
    // Override rendering methods
    void render(DisplayManager* display) override;
    bool onInputEvent(const InputEvent& event) override;
};

class ChannelStripComponent : public UIComponent {
private:
    int channel_;
    bool active_;
    float volume_;
    float pan_;
    std::string presetName_;
    
public:
    ChannelStripComponent(int channel);
    
    // UI state
    void setActive(bool active);
    void setVolume(float volume);
    void setPan(float pan);
    void setPresetName(const std::string& name);
    
    // Callbacks
    std::function<void(int,bool)> onActiveChanged;
    std::function<void(int,float)> onVolumeChanged;
    std::function<void(int,float)> onPanChanged;
    std::function<void(int)> onPresetSelectClick;
    
    // Override rendering methods
    void render(DisplayManager* display) override;
    bool onInputEvent(const InputEvent& event) override;
};
```

## 3. Implementation Plan

### 3.1 Phase 1: Core Engine (1-2 weeks)

1. Create `MultiTimbralEngine` class
   - Implement basic channel management
   - Set up MIDI routing infrastructure
   - Add audio mixing foundations

2. Implement `ChannelSynthesizer` class
   - Extend current Synthesizer for channel awareness
   - Add channel-specific parameter handling
   - Create preset management per channel

3. Update `VoiceManager` for multi-channel voice allocation
   - Implement voice stealing across channels
   - Add prioritization mechanism
   - Configure voice limits per channel

### 3.2 Phase 2: MIDI Implementation (1 week)

1. Update `MidiManager` for multi-channel routing
   - Implement channel filtering
   - Add program change handling
   - Create interface for keyboard splits

2. Create specialized MIDI routing modes
   - Implement keyboard splits (by note range)
   - Add velocity switching
   - Build layering support

3. Implement Program Change handling
   - Add preset switching per channel
   - Create program map tables
   - Support Bank Select messages

### 3.3 Phase 3: User Interface (1-2 weeks)

1. Design and implement mixer interface
   - Create channel strip components
   - Build master section
   - Add visual feedback for active notes

2. Add channel parameter editing
   - Create per-channel preset browser
   - Build note range editor for splits
   - Implement MIDI routing configuration UI

3. Develop performance view
   - Create keyboard visualization with split/layer indication
   - Add quick channel mute/solo controls
   - Build preset quick-selection interface

### 3.4 Phase 4: Integration and Testing (1 week)

1. Integrate with existing codebase
   - Update main application to use MultiTimbralEngine
   - Connect UI components to engine
   - Restructure audio pipeline

2. Performance optimization
   - Implement efficient voice allocation
   - Add voice recycling across channels
   - Optimize audio mixing

3. Testing plan
   - Test with multiple MIDI controllers
   - Validate DAW integration
   - Verify resource usage

## 4. Technical Considerations

### 4.1 Thread Safety

Multi-timbral operation introduces additional concurrency concerns:

1. **Voice Allocation Across Channels**
   - Global voice allocation must be thread-safe
   - Need mutex protection for cross-channel operations
   - Implement atomic operations for voice count tracking

2. **Parameter Changes**
   - Channel-specific parameters need proper synchronization
   - Use lock-free operations where possible for real-time safety
   - Implement wait-free reads for critical audio path

3. **Audio Mixing**
   - Ensure lock-free audio processing path
   - Use double-buffering for parameter updates
   - Implement thread-safe resource management

### 4.2 Resource Management

Efficient resource allocation is critical:

1. **Memory Usage**
   - Share resources (wavetables, samples) where possible
   - Implement lazy loading for inactive channels
   - Create resource pooling for efficient memory use

2. **CPU Considerations**
   - Only process active channels
   - Implement voice prioritization based on activity
   - Add dynamic voice allocation based on CPU usage

3. **Voice Allocation Strategies**
   - Equal distribution: Divide voices evenly across active channels
   - Priority-based: Allocate more voices to important channels
   - Dynamic: Allocate based on actual usage patterns

### 4.3 Real-Time Considerations

Performance is critical for real-time audio:

1. **Buffer Management**
   - Optimize buffer reuse to minimize allocations
   - Pre-allocate mixing buffers for all channels
   - Use aligned memory for SIMD optimizations

2. **Parameter Smoothing**
   - Implement per-channel parameter smoothing
   - Ensure glitch-free transitions between presets
   - Add ramping for volume/pan changes

3. **Latency Management**
   - Minimize processing overhead for multi-channel mixing
   - Use efficient lock-free algorithms for parameter updates
   - Implement predictive processing for critical paths

## 5. API Usage Examples

### 5.1 Basic Multi-Timbral Setup

```cpp
// Initialize the multi-timbral engine
MultiTimbralEngine engine(44100, 64);  // 64 total voices

// Load different presets for different channels
engine.loadPresetFromFile("presets/bass.json", 0);
engine.loadPresetFromFile("presets/lead.json", 1);
engine.loadPresetFromFile("presets/pad.json", 2);

// Set channel volumes and pans
engine.setChannelVolume(0, 0.8f);
engine.setChannelPan(0, -0.3f);  // Bass slightly to the left
engine.setChannelVolume(1, 1.0f);
engine.setChannelPan(1, 0.0f);   // Lead centered
engine.setChannelVolume(2, 0.6f);
engine.setChannelPan(2, 0.3f);   // Pad slightly to the right

// Connect to audio engine
audioEngine.setCallback([&engine](float* buffer, int numFrames) {
    engine.process(buffer, numFrames);
});

// MIDI handling
midiInterface.setNoteOnCallback([&engine](int note, int velocity, int channel) {
    engine.noteOn(note, velocity / 127.0f, channel);
});
```

### 5.2 Keyboard Split Configuration

```cpp
// Setup a keyboard split at note 60 (C4)
// Channel 0 for lower notes, Channel 1 for higher notes
engine.setupKeyboardSplit(60, 0, 1);

// Load appropriate presets
engine.loadPresetFromFile("presets/bass.json", 0);
engine.loadPresetFromFile("presets/lead.json", 1);

// Alternative approach with MIDI manager
midiManager.setupKeyboardSplit(60, 0, 1);

// Process incoming MIDI message with the manager
midiManager.processMidiMessage(midiMessage);  // Routes automatically
```

### 5.3 Channel Parameter Adjustments

```cpp
// Get access to a specific channel synthesizer
ChannelSynthesizer* channelSynth = engine.getChannelSynth(0);

// Adjust channel-specific parameters
channelSynth->setPolyphony(8);  // Limit to 8 voices
channelSynth->setTransposition(12);  // Transpose up an octave
channelSynth->setNoteRange(36, 72);  // C2 to C5 range

// Save channel settings
ChannelParameters params = channelSynth->saveChannelParameters();
params.toJson(jsonObject);
```

## 6. Future Enhancements

1. **Dynamic Voice Allocation**
   - Machine learning-based voice prediction
   - Adaptive voice distribution based on playing style
   - Predictive note allocation for complex passages

2. **Advanced Routing**
   - Effect sends/returns per channel
   - Channel audio routing to specific outputs
   - MIDI processing chains per channel

3. **Performance Modes**
   - Scene management with multiple multi-timbral configurations
   - Live performance snapshots
   - Song mode with automatic preset changes

4. **Integration with Adaptive Sequencer**
   - Multi-channel pattern sequencing
   - Per-channel adaptive parameters
   - Dynamic orchestration based on intensity

## 7. Conclusion

This multi-timbral architecture enables professional-level synthesizer functionality comparable to high-end hardware and software instruments. The design balances flexibility, performance, and resource efficiency while building on our existing codebase structure. 

Implementation will proceed in phases, starting with the core engine and voice management systems, followed by MIDI routing and UI components. The result will be a fully-featured multi-timbral synthesizer capable of complex sound layering, keyboard splits, and DAW integration.