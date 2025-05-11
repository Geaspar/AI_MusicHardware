# TestAudio User Guide

This guide explains how to use the real-time audio test application (`TestAudio`) for the AIMusicHardware project.

## Introduction

`TestAudio` is an interactive application that demonstrates the real-time audio capabilities of the AIMusicHardware framework. It allows you to test various sound generation features in real-time through your computer's audio output.

## Prerequisites

- A built version of the `TestAudio` application
- RtAudio installed on your system
- Working audio output device (speakers or headphones)

## Running TestAudio

1. **Build the application**:
   ```bash
   # Using the build script
   ./build.sh
   
   # Or manually
   mkdir -p build && cd build
   cmake ..
   cmake --build .
   ```

2. **Run the application**:
   ```bash
   ./build/bin/TestAudio
   ```

3. **Follow the on-screen menu**:
   The application will present a menu with several test options.

## Available Tests

The `TestAudio` application offers the following test options:

### 1. Play C4 note (Sine wave)
Plays a middle C note (MIDI note 60) for 2 seconds using a sine wave oscillator.

### 2. Play C major scale
Plays the C major scale (C4, D4, E4, F4, G4, A4, B4, C5) using a sine wave oscillator.

### 3. Test different waveforms
Demonstrates different oscillator types by playing the same note with each waveform type:
- Square wave
- Saw wave
- Triangle wave
- Sine wave
- Noise

### 4. Play chord (C major)
Plays a C major chord (C4, E4, G4) for 2 seconds using a sine wave oscillator.

### 5. Play arpeggio pattern
Plays an arpeggio pattern based on the C major chord.

### 0. Exit
Exits the application.

## Keyboard Input

For each test, the application will wait for your input. Simply type the number of the test you want to run and press Enter. The application will execute the test and then return to the menu.

## MPE Demo Integration

To experience the MPE capabilities (MIDI Polyphonic Expression), we recommend using the `TestMpeVoiceManager` application, which specifically demonstrates the expressive capabilities of our MPE implementation.

## Troubleshooting

If you experience issues with `TestAudio`:

1. **No sound output:**
   - Check your system volume settings
   - Verify the correct audio output device is selected in your system settings
   - Ensure no other application is using the audio device exclusively
   - Try restarting the application

2. **Distorted audio:**
   - Lower the system volume
   - Check for conflicts with other audio applications
   - Ensure your audio device supports the default sample rate (44.1kHz)

3. **Application crashes:**
   - Check the console output for error messages
   - Verify RtAudio was properly installed
   - Ensure your audio hardware is functioning properly

4. **Menu navigation issues:**
   - Ensure you're entering only the numbers shown in the menu
   - Press Enter after typing your selection

## Advanced Usage

For developers looking to extend the `TestAudio` application:

1. The application is in `examples/TestAudio.cpp`
2. To add new tests, simply add a new case to the switch statement in the main loop
3. You can modify parameters like note duration, velocity, and oscillator types
4. Advanced testing with effects can be added by incorporating the EffectProcessor

## Next Steps

After exploring the basic audio capabilities with `TestAudio`, you may want to try:

1. The `TestMpeVoiceManager` application for expressive MPE capabilities
2. The `SimplePresetManagerDemo` for preset management
3. Creating custom sound designs by modifying the application