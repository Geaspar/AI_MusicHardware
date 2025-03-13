# Testing Sound in AIMusicHardware

This guide provides step-by-step instructions for testing the audio capabilities of the AIMusicHardware project.

## Two Testing Options

We provide two different ways to test the sound generation capabilities:

1. **WAV File Generation** (SimpleTest - works on all systems)
   - Generates WAV files that you can play in any audio player
   - Does not require any audio libraries
   - Good for verifying basic synthesis algorithms

2. **Real-time Audio Playback** (TestAudio - requires RtAudio)
   - Interactive program that plays sounds through your speakers
   - Requires RtAudio library
   - Good for testing real-time audio capabilities

## Prerequisites

- macOS, Linux, or Windows
- CMake 3.14 or higher
- C++17 compatible compiler
- For real-time audio testing:
  - macOS: RtAudio (`brew install rtaudio`)
  - Linux: RtAudio and ALSA dev packages (`apt-get install librtaudio-dev libasound2-dev`)
  - Windows: RtAudio can be built from source

## Quick Start

The easiest way to test sound is to use the provided build script:

```bash
# Make build script executable
chmod +x build.sh

# Run the build script
./build.sh
```

The build script will attempt to find or install RtAudio and will build one or both test programs depending on what's available.

## Testing Option 1: WAV File Generation

This option works on all systems and doesn't require any audio libraries. It generates WAV files that you can play in any media player.

```bash
# Run the simple test to generate WAV files
cd build
./bin/SimpleTest
```

This will create several WAV files in the `output` directory:
- `sine_note.wav` - A single C4 note with sine wave
- `scale.wav` - A C major scale with sine wave
- `waveforms.wav` - Different waveform types (sine, square, saw, triangle, noise)
- `chord.wav` - A C major chord

You can play these files with any audio player to verify that the synthesis is working correctly.

## Testing Option 2: Real-time Audio (requires RtAudio)

If RtAudio was found during build, you can use the interactive test application:

```bash
cd build
./bin/TestAudio
```

The TestAudio application provides an interactive menu to test different sound features:

1. **Play C4 note** - Plays a single note with the sine wave oscillator
2. **Play C major scale** - Plays a sequence of notes forming the C major scale
3. **Test different waveforms** - Cycles through all available oscillator types
4. **Play chord** - Plays a C major chord with three simultaneous notes
5. **Play arpeggio pattern** - Plays a musical pattern with notes from the C major chord

## Manual Build

If you prefer to build manually:

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build
cmake --build .
```

## Troubleshooting

### No Sound Output (TestAudio)

- Check your system's audio settings and ensure the correct output device is selected
- Verify that RtAudio was found during the build process
- Try increasing the volume in your OS settings
- Check for error messages during application startup

### Cannot Play WAV Files

- Ensure the WAV files were generated successfully in the `output` directory
- Try playing the WAV files with different audio players
- Check for error messages during SimpleTest execution

### Build Errors

- Make sure your compiler supports C++17
- Check for error messages during the build process
- If building TestAudio, ensure RtAudio is properly installed

### RtAudio Installation Issues

If the automatic installation of RtAudio doesn't work, try installing it manually:

**macOS:**
```bash
brew install rtaudio
```

**Ubuntu/Debian:**
```bash
sudo apt-get install librtaudio-dev
```

## Next Steps

After verifying that basic sound generation works, you can:

1. Implement and test effects processing
2. Add MIDI controller support
3. Create a more sophisticated UI
4. Integrate the AI components

## Audio Architecture

The current audio implementation uses:

- **AudioEngine** - Handles real-time audio I/O using RtAudio
- **Synthesizer** - Generates sound with multiple oscillator types
- **Voice** - Individual sound generators with ADSR envelope

The audio callback flow:
1. OS audio system requests audio data
2. AudioEngine receives request and calls registered callback
3. Synthesizer generates audio for active voices
4. (Future) EffectProcessor applies effects to the audio
5. Audio is sent to output device