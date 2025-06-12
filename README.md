# AI Music Hardware

An IoT-enabled hardware synthesizer with LLM-assisted composition capabilities, digital synthesis, effects processing, and event-based sequencing. Designed with a modular architecture where UI is optional for testing and development.

## Project Structure
- `src/` - Source code
  - `audio/` - C++ audio engine code
  - `ai/` - LLM integration code
  - `ui/` - User interface code
  - `hardware/` - Hardware interface code
  - `sequencer/` - Sequencer implementation
  - `effects/` - Audio effects processing
  - `midi/` - MIDI implementation
- `include/` - Header files
- `lib/` - External libraries
- `tests/` - Test suite
- `docs/` - Documentation
- `examples/` - Example projects

## System Architecture

The architecture follows a modular design where core functionality is independent of UI:

### Core Components (Always Present):
1. **Audio Engine**: Real-time C++ audio processing with digital synthesis
2. **Modulation System**: LFOs, envelopes, and modulation matrix (in development)
3. **Effects System**: Modular audio effects chain with reordering capabilities
4. **Event Sequencer**: Event-driven sequencer for hardware compatibility
5. **MIDI Integration**: Hardware connectivity via MIDI protocol with MPE support
6. **Multi-Timbral Engine**: Support for multiple simultaneous instruments on different MIDI channels
7. **Preset Management**: Enterprise-grade system for saving and loading synthesizer settings

### Optional Components (For Testing/Development):
8. **UI Module**: SDL2-based interface for testing and prototyping hardware layout
9. **Visualization**: Waveform, envelope, and filter visualizers to prototype hardware displays

### Hardware-Specific Components:
10. **Hardware Interface**: Physical controls and display interface
11. **IoT Integration**: Connect with other devices through MQTT protocol:
    - Discover and connect to IoT sensors and controllers
    - Map sensor data to synthesis parameters
    - Create interactive environments that respond to real-world data
    - Control synthesizer remotely via IoT devices

### Advanced Features:
12. **MPE Support**: MIDI Polyphonic Expression for expressive control with three dimensions:
    - Per-note pitch bend (X-axis)
    - Per-note timbre control (Y-axis)
    - Per-note pressure sensitivity (Z-axis)
13. **LLM Integration**:
    - Voice/text to synthesizer parameter suggestions
    - Pattern completion and suggestion for sequencer
    - User preference learning for customized assistance
    - Musical context awareness

## LLM Features

The LLM component acts as a creative copilot with these capabilities:

- Parameter suggestion based on descriptive language (e.g., "make it sound brighter" or "more aggressive")
- Pattern suggestions and completions for the sequencer
- Learning user preferences over time to offer more personalized suggestions
- Contextual music theory assistance

## Technical Implementation

1. Implement the C++ source files based on the header interfaces
2. Choose and integrate audio/MIDI libraries (RtAudio/RtMidi included in CMake)
3. Select an LLM for embedding (consider ONNX Runtime for optimized models)
4. Design and implement the hardware interface components
5. Develop the synthesizer engine with multiple oscillator types
6. Create the effects processing chain
7. Implement the pattern sequencer with MIDI I/O
8. Integrate the LLM with audio parameter mapping

## Hardware Interface Considerations

- Knobs with screens for parameter visualization
- Touch surfaces for expressive control
- LED feedback for sequencer and status
- Display for LLM interaction and suggestions
- MIDI connectivity for external devices

## Immediate Next Steps

1. **Develop a functional software prototype**
   - Create a minimal C++ audio engine implementation
   - Implement basic synthesis engine
   - Test LLM integration with an existing model

2. **Design hardware proof-of-concept**
   - Create simple controller layout with key inputs
   - Build interface for the LLM component
   - Test hardware-software communication

3. **Validate core concept**
   - Get early feedback from potential users
   - Test audio quality and latency
   - Evaluate LLM responsiveness and usefulness

4. **Secure initial funding**
   - Prepare pitch materials based on prototype
   - Explore grant opportunities for music tech
   - Consider angel investors with music industry experience

5. **Engage with prototype manufacturer**
   - Contact manufacturers with music hardware experience
   - Get preliminary quotes for prototype development
   - Discuss timeline and requirements

6. **Plan for IP protection**
   - Consider provisional patents for unique aspects
   - Secure trademarks for product name
   - Establish clear copyright for software components

7. **Develop go-to-market strategy**
   - Determine initial pricing strategy
   - Plan crowdfunding campaign structure
   - Identify key influencers in electronic music space

## Getting Started
1. Install dependencies:
   - C++ compiler with C++17 support
   - CMake 3.14 or higher
   - RtAudio library (for real-time audio)
   - RtMidi library (for MIDI support)

2. Build the project using the build script:
   ```bash
   # Make the build script executable
   chmod +x build.sh

   # Run the build script
   ./build.sh
   ```
   This will:
   - Check for dependencies and install them if possible
   - Configure the project with CMake
   - Build all test applications
   - Display available test options

3. Run the test applications:
   ```bash
   # Real-time audio test with interactive demo
   ./build/bin/TestAudio

   # Test MPE capabilities
   ./build/bin/TestMpeVoiceManager

   # Try the preset management system
   ./build/bin/SimplePresetManagerDemo

   # (Other test applications may be available based on your build)
   ```

## Key Test Applications
- **TestAudio**: Interactive demo of sound generation with different waveforms, scales, chords, and arpeggios
- **TestMpeVoiceManager**: Demo of MPE (MIDI Polyphonic Expression) with expressive per-note control
- **SimplePresetManagerDemo**: Test the preset management system for saving and loading sound settings
- **TestSequencer**: Demonstrates sequencer capabilities with pattern playback
- **TestAdaptiveSequencer**: Shows the adaptive sequencing system inspired by game audio

## Key Features

### Custom UI Framework
The UI framework provides a minimalist, hardware-oriented interface inspired by the Teenage Engineering OP-1:

- **Direct Framebuffer Access**: No dependencies on external UI frameworks
- **Music-Specific Components**: Specialized UI elements for music production
- **Hardware Integration**: Support for physical controls and touch interfaces
- **Theming System**: Consistent color palette with easy customization
- **Performance Focused**: Optimized for embedded hardware

For more information, see [UI Implementation Documentation](docs/UI_IMPLEMENTATION.md).

### Adaptive Sequencer
The AdaptiveSequencer is a state-based music sequencing system inspired by game audio middleware:

- **State-Based Architecture**: Musical states with transition rules
- **Event-Driven Design**: Trigger events for state changes and musical transitions
- **Layered Composition**: Dynamic control over musical layers and mixing
- **Hardware Integration**: Intuitive physical control over adaptive music system
- **Parameter-Driven**: Real-time control values that influence musical behavior

For more information, see [Adaptive Sequencer Documentation](docs/ADAPTIVE_SEQUENCER.md).

## IoT Integration

The IoT integration capabilities allow the AIMusicHardware to interact with external devices:

1. **MQTT Protocol**:
   - Connect to MQTT brokers for IoT device discovery and messaging
   - Support for standard MQTT features (QoS, retained messages, wildcards)

2. **Device Discovery**:
   - Automatically discover IoT devices on the network
   - Support for HomeAssistant and Homie device discovery formats

3. **Parameter Mapping**:
   - Map IoT sensor data to synthesis parameters
   - Two-way binding between hardware controls and IoT devices
   - Preset management for sensor mappings

4. **Adaptive Music Response**:
   - Trigger state changes in the adaptive sequencer based on sensor data
   - Create responsive soundscapes that evolve with the environment

To install MQTT libraries locally and enable IoT functionality:
```bash
# Install MQTT libraries locally
chmod +x tools/install_mqtt_libs.sh
./tools/install_mqtt_libs.sh

# Rebuild the project
./build.sh
```

For testing MQTT functionality:
```bash
# Basic testing with mock implementation
./build/bin/SimpleMQTTTest

# Test MQTTInterface with mock implementation
./build/bin/SimpleMQTTInterfaceTest

# Test with real MQTT broker (using mock implementation, will work with the real
# implementation once Paho libraries are properly installed)
./build/bin/RealMQTTTest [broker_host] [port] [client_id]

# Note: PahoMQTTCTest is currently disabled until Paho MQTT C library is properly installed
```

For the full IoT Configuration Manager demo:
```bash
./build/bin/IoTConfigManagerDemo
```

## Documentation
For detailed information about specific components, see the following guides:

- **Real-time Audio**: [TESTAUDIO_GUIDE.md](docs/TESTAUDIO_GUIDE.md)
- **MPE Implementation**: [MPE_VOICE_MANAGER.md](docs/MPE_VOICE_MANAGER.md)
- **Adaptive Sequencer**: [ADAPTIVE_SEQUENCER.md](docs/ADAPTIVE_SEQUENCER.md)
- **Sequencer Testing**: [SEQUENCER_TESTING_GUIDE.md](docs/SEQUENCER_TESTING_GUIDE.md)
- **UI Implementation**: [UI_IMPLEMENTATION.md](docs/UI_IMPLEMENTATION.md)
- **IoT Integration**: [IOT_INTEGRATION.md](docs/IOT_INTEGRATION.md)
- **MQTT Interface**: [MQTT_INTERFACE.md](docs/MQTT_INTERFACE.md)
- **MQTT Implementation Guide**: [MQTT_IMPLEMENTATION_GUIDE.md](docs/MQTT_IMPLEMENTATION_GUIDE.md)
- **Project Updates**: [PROJECT_UPDATES.md](docs/PROJECT_UPDATES.md)
- **Next Steps**: [next_steps.md](docs/next_steps.md)
- **Commercialization**: [commercialization_guide.md](docs/commercialization_guide.md)