# Multi-Timbral Synthesizer Demo Guide

This document explains how to build and run the Multi-Timbral synthesizer demo, including how to set up MIDI keyboard support.

## What is Multi-Timbral Synthesis?

Multi-timbral synthesis allows you to play multiple different sounds simultaneously, each on a separate MIDI channel. This is analogous to having multiple synthesizers in one device, which enables:

- Playing different instruments on different MIDI channels
- Creating keyboard splits with different sounds in different regions of the keyboard
- Layering multiple sounds for rich, complex textures
- Integration with DAWs for multi-track sequencing

## Building the Demo

The Multi-Timbral demo is included in the project's build system. To build it:

1. Make sure you have all prerequisites installed (CMake, RtAudio, RtMidi)
2. From the project root directory, run:

```bash
mkdir -p build && cd build
cmake ..
make
```

This will build all project components, including the `MultiTimbralDemo` executable.

## Running the Demo

To run the demo:

```bash
cd build/bin
./MultiTimbralDemo
```

### Demo Controls

The demo features an interactive menu system that allows you to:

- View and configure active channels
- Set up keyboard splits and layers
- Adjust voice allocation strategies
- Configure channel volumes and panning
- Play test notes on each channel

## Connecting a MIDI Keyboard

The demo supports external MIDI controllers for a more expressive experience:

### Prerequisites

1. Make sure you have a MIDI keyboard or controller connected to your computer
2. Verify that RtMidi was successfully detected during build (check CMake output)

### MIDI Setup

1. Connect your MIDI controller before starting the demo
2. The demo automatically connects to the first available MIDI input port
3. MIDI activity (notes, controllers, etc.) will be displayed in the terminal with color-coding by channel

### MIDI Features

The demo supports these MIDI message types:

- **Note On/Off**: Play notes on the synthesizer (routed based on current settings)
- **Control Change**: Adjust parameters
- **Program Change**: Change presets on each channel
- **Pitch Bend**: For expressive pitch modulation
- **Aftertouch**: For additional expression
- **Channel Pressure**: For global pressure control

## Performance Modes

### Keyboard Splits

Keyboard splits allow you to play different sounds in different regions of your keyboard:

1. From the main menu, select "Setup keyboard split"
2. Enter the split point (MIDI note number, e.g., 60 for middle C)
3. Specify which channels to use for lower and upper regions
4. Notes will now be automatically routed to the appropriate channel based on pitch

### Layered Sounds

Layering lets you play multiple sounds simultaneously with each key press:

1. From the main menu, select "Setup layered channels"
2. Enter the channel numbers you want to layer, separated by commas (e.g., "1,2,3")
3. Now every note will trigger sounds on all the specified channels

## Troubleshooting

### No MIDI Input

If your MIDI controller isn't recognized:

1. Ensure your MIDI device is connected and powered on before starting the demo
2. Verify that your OS recognizes the MIDI device
3. Check that RtMidi was detected during build
4. Try running with different MIDI ports if multiple are available

### No Audio Output

If you don't hear any sound:

1. Ensure your audio output device is properly connected and unmuted
2. Verify that RtAudio was detected during build
3. Check the system volume settings
4. Try playing test notes from the menu to verify audio output

### Performance Issues

If you experience audio glitches or high CPU usage:

1. Reduce the number of active channels
2. Lower the voice count
3. Disable complex effects
4. Ensure your computer isn't running other CPU-intensive applications

## Advanced Usage

### Custom Preset Loading

The demo loads basic presets by default. For more advanced sounds:

1. Create custom presets in your user presets directory
2. Use Program Change messages to select them
3. Or use keyboard shortcuts to load them directly

### DAW Integration

To use with a DAW:

1. Set up your DAW to send MIDI on multiple channels
2. Configure the demo to listen to those channels
3. Route MIDI from your DAW to the demo application

## Next Steps

After trying the Multi-Timbral demo, consider:

1. Exploring the source code to understand the implementation
2. Building your own multi-timbral patches
3. Extending the synthesizer with new sounds and effects
4. Contributing to the project by adding new features

Enjoy creating with multiple sounds simultaneously!