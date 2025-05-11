# MIDI Keyboard Integration Guide

This guide explains how to set up and use your MIDI keyboard with the AIMusicHardware synthesizer.

## Overview

The MIDI keyboard integration allows you to play the synthesizer in real-time using an external MIDI controller or keyboard. The system connects three main components:

1. **MIDI Manager** - Handles MIDI input from your keyboard
2. **Synthesizer** - Generates audio based on MIDI input
3. **Audio Engine** - Delivers the audio to your speakers/headphones

## Prerequisites

- A MIDI keyboard or controller connected to your computer
- C++ compiler with C++17 support
- CMake 3.14 or higher
- RtAudio and RtMidi libraries installed

## Building the Demo

1. First, update your CMakeLists.txt to include the new MidiKeyboardDemo example. Add the following under the section where other examples are defined:

```cmake
# MIDI Keyboard Demo
add_executable(MidiKeyboardDemo examples/MidiKeyboardDemo.cpp)
target_link_libraries(MidiKeyboardDemo AIMusicCore)
```

2. Build the project:

```bash
mkdir -p build
cd build
cmake ..
make MidiKeyboardDemo
```

## Running the Demo

1. Make sure your MIDI keyboard is connected to your computer
2. Run the demo:

```bash
./bin/MidiKeyboardDemo
```

3. The application will:
   - Initialize the audio engine
   - List available MIDI input devices
   - Prompt you to select your MIDI keyboard
   - Allow you to play the synthesizer with your MIDI keyboard

## Using the Demo

Once the demo is running:

1. Play notes on your MIDI keyboard to trigger the synthesizer
2. Use the computer keyboard numbers 1-5 to change oscillator types:
   - 1: Sine wave
   - 2: Square wave
   - 3: Saw wave
   - 4: Triangle wave
   - 5: Noise

3. The synthesizer will respond to the following MIDI messages from your keyboard:
   - Note On/Off
   - Pitch Bend
   - Modulation Wheel (CC 1)
   - Aftertouch/Channel Pressure
   - Sustain Pedal (CC 64)

4. Press Ctrl+C to exit the application

## How It Works

The demo application connects several components:

1. **AudioEngine** - Manages real-time audio output
2. **Synthesizer** - Generates audio based on active notes
3. **MidiManager** - Processes incoming MIDI messages
4. **MidiListener** - Handles MIDI events like pitch bend and aftertouch

The basic flow is:

1. MIDI keyboard sends MIDI messages
2. MidiManager receives these messages and converts them to synthesizer commands
3. Synthesizer generates audio samples based on the active notes and parameters
4. AudioEngine delivers these samples to your audio output device

## Extending the Demo

You can extend this demo by:

1. Adding more synthesizer parameters controlled via MIDI CCs
2. Implementing MIDI learn functionality
3. Adding effects processing (reverb, delay, etc.)
4. Creating a simple UI with parameter visualization
5. Saving/loading presets

## Troubleshooting

Common issues and solutions:

1. **No audio output**
   - Check your system audio settings
   - Try different audio buffer sizes
   - Ensure AudioEngine initializes successfully

2. **MIDI keyboard not detected**
   - Make sure your MIDI device is connected before starting the application
   - Check if your MIDI device requires special drivers
   - Try a different USB port

3. **High latency**
   - Decrease the audio buffer size (at the cost of potential audio glitches)
   - Check for CPU-intensive processes that might be running simultaneously

4. **Crashes or errors**
   - Check for proper initialization of all components
   - Ensure thread safety in audio callback
   - Verify correct resource cleanup on shutdown

## Next Steps

After getting the basic MIDI keyboard integration working, consider these improvements:

1. Implement a more sophisticated voice management system for polyphony
2. Add support for MIDI Program Change messages to switch presets
3. Implement MIDI mapping for all synthesizer parameters
4. Add support for MPE (MIDI Polyphonic Expression) if your controller supports it
5. Create a visual feedback system to show which notes are currently playing
6. Integrate with the existing UI framework

## Architecture Deep Dive

For developers who want to understand the implementation details:

### MidiManager

The MidiManager class acts as a bridge between the MIDI input and the synthesizer. It:

1. Handles MIDI device connections and enumeration
2. Processes incoming MIDI messages
3. Maps MIDI controllers to synthesizer parameters
4. Provides MIDI learn functionality

### Audio Callback

The audio callback is the heart of the real-time audio processing:

```cpp
audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
    // Process synthesizer directly
    synthesizer->process(outputBuffer, numFrames);
});
```

This function is called by the audio driver whenever it needs more audio samples. The synthesizer generates the audio based on the current state of active notes and parameters.

### Event Handling

MIDI events and audio processing happen on different threads:

1. The main thread handles user input and MIDI device selection
2. The MIDI input thread processes incoming MIDI messages
3. The audio thread calls the audio callback to generate samples

Proper synchronization is crucial to prevent race conditions and ensure stable audio output.

## Reference Implementation

The demo is partially based on the Vital synthesizer's MIDI implementation, which provides a reference for professional-grade MIDI handling in a software synthesizer.