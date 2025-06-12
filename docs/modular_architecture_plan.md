# Modular Architecture Plan for IoT Hardware Synthesizer

## Overview
This document outlines the architectural approach for developing an IoT hardware synthesizer with a modular, testable design. The UI is treated as an optional testing/debugging layer that informs the hardware interface design but is not required for core functionality.

## Core Design Principles

### 1. **UI as a Thin Layer**
- Keep UI code completely separate from core logic
- All UI components should be optional and removable
- UI serves as a prototype for hardware display design

### 2. **Hardware-First API**
- Design all interfaces as if they're hardware controls
- Use parameter IDs that map directly to hardware knobs/buttons
- Event-driven communication suitable for IoT messaging

### 3. **Event-Driven Architecture**
- Everything communicates through events
- Perfect for IoT integration (MQTT events)
- Natural mapping to hardware interrupts

### 4. **Testable Components**
- Each system can run headless for unit testing
- Minimal dependencies between modules
- Clear interfaces for mocking/stubbing

## Proposed Architecture

```
main_modular.cpp - Core synthesizer with pluggable interfaces
├── Audio Engine (always present)
├── MIDI System (always present)
├── Modulation System (always present)
├── Event Sequencer (always present)
├── UI Module (optional - for testing)
└── IoT Module (optional - can test without hardware)
```

## Implementation Structure

```cpp
// Core synthesizer functionality - no UI dependencies
class SynthesizerCore {
protected:
    AudioEngine audio;
    ModulationMatrix modulation;
    EventSequencer sequencer;
    MidiProcessor midi;
    
public:
    // Abstract interfaces for external control
    virtual void onParameterChange(ParamID param, float value) = 0;
    virtual void onMidiEvent(const MidiMessage& msg) = 0;
    virtual void onSequencerEvent(const Event& evt) = 0;
    virtual void onModulationEvent(const ModulationEvent& evt) = 0;
    
    // Core processing
    void processAudio(float* buffer, size_t frames);
    void processEvents();
};

// Testable main application with optional modules
class TestableMainApp : public SynthesizerCore {
private:
    // Optional modules
    std::unique_ptr<UIModule> ui;      // Only when USE_UI defined
    std::unique_ptr<IoTModule> iot;    // Only when USE_IOT defined
    std::unique_ptr<HardwareInterface> hw; // For actual hardware
    
public:
    // Can run headless for testing
    void run(bool enableUI = true, bool enableIoT = true);
    
    // Route events to appropriate handlers
    void onParameterChange(ParamID param, float value) override;
    void onMidiEvent(const MidiMessage& msg) override;
    void onSequencerEvent(const Event& evt) override;
};
```

## Module Interfaces

### UI Module (Optional)
```cpp
class UIModule {
public:
    // Initialize with screen dimensions
    void initialize(int width, int height);
    
    // Update display
    void render();
    
    // Handle input
    void processInput();
    
    // Bind to synthesizer parameters
    void bindParameter(ParamID id, UIWidget* widget);
};
```

### IoT Module (Optional)
```cpp
class IoTModule {
public:
    // Connect to MQTT broker
    void connect(const std::string& broker);
    
    // Subscribe to control topics
    void subscribeToControls();
    
    // Publish state updates
    void publishState(const SynthState& state);
    
    // Handle incoming messages
    void onMessage(const MqttMessage& msg);
};
```

### Hardware Interface
```cpp
class HardwareInterface {
public:
    // Initialize hardware peripherals
    void initialize();
    
    // Read hardware controls (knobs, buttons)
    void pollControls();
    
    // Update hardware displays (LEDs, screen)
    void updateDisplays();
    
    // Handle hardware interrupts
    void onInterrupt(int pin);
};
```

## Benefits for Hardware Development

1. **Isolation**: Core synthesizer logic has no UI dependencies
2. **Testing**: Can test synthesis without any display
3. **Flexibility**: Easy to swap between UI testing and hardware testing
4. **Event Mapping**: Natural progression from UI events → hardware events
5. **Performance**: No UI overhead in production hardware build

## Development Phases

### Phase 1: Core Implementation (Current)
- Implement modulation system without UI
- Test using console output and unit tests
- Ensure all core features work headless

### Phase 2: UI Testing Layer
- Add minimal UI for parameter visualization
- Use UI to prototype hardware interface layout
- Validate user experience before hardware build

### Phase 3: IoT Integration
- Connect modulation parameters to MQTT
- Test remote control capabilities
- Implement event synchronization

### Phase 4: Hardware Deployment
- Remove UI module
- Map UI controls to hardware inputs
- Deploy screen design based on UI prototype

## Migration Strategy

1. **Current State**: Use AIMusicHardwareIntegrated for development
2. **Extract Core**: Identify and isolate UI-specific code
3. **Build Modular**: Create new main with pluggable architecture
4. **Test Headless**: Verify all features work without UI
5. **Add Optional UI**: Re-implement UI as optional module

## Testing Strategy

### Unit Tests (No UI Required)
- Test each synthesizer component in isolation
- Verify modulation routing
- Validate event sequencing
- Check parameter ranges and scaling

### Integration Tests (Optional UI)
- Test parameter changes through UI
- Verify visual feedback
- Validate user workflows
- Test MIDI input handling

### Hardware Simulation
- Mock hardware interface for testing
- Simulate button presses and knob turns
- Test interrupt handling
- Verify timing constraints

## Conclusion

This modular approach ensures that the core synthesizer functionality remains independent of any UI implementation, making it perfect for deployment on IoT hardware while still allowing convenient testing and debugging during development.