# Sequencer Testing Guide

⏺ To test the sequencer, you have two main options:

  1. TestSequencerFile - This program generates WAV audio files from sequencer patterns without requiring real-time audio. It creates three
  example patterns (scale, chord progression, and arpeggio) and renders them to WAV files in an "output" directory.
  2. TestSequencerAdvanced - This is an interactive console application that lets you explore all the advanced sequencer features including:
    - Pattern management (viewing and creating patterns)
    - Song arrangement (chaining patterns into a sequence)
    - MIDI file export
    - Quantization and swing features
    - Simulated playback

  To build and run these tests:

  1. Create a build directory if it doesn't exist:
  mkdir -p build
  2. Navigate to the build directory:
  cd build
  3. Run CMake to configure the project:
  cmake ..
  4. Build the test programs:
  make TestSequencerFile TestSequencerAdvanced
  5. Run the file-based test:
  ./bin/TestSequencerFile
  5. This will create WAV files in the "output" directory that you can listen to.
  6. Run the advanced features test:
  ./bin/TestSequencerAdvanced
  6. This will launch an interactive console menu where you can explore all the sequencer features.

  The TestSequencerAdvanced application is particularly useful as it demonstrates all the sequencer capabilities through an interactive menu,
   allowing you to create patterns, build song arrangements, apply quantization and swing, and export to MIDI files.

This document provides instructions on how to test the sequencer component of the AIMusicHardware project.

## Available Test Programs

The project includes two main test programs for the sequencer:

1. **TestSequencerFile** - Generates WAV audio files from patterns without requiring real-time audio
2. **TestSequencerAdvanced** - Interactive console application with comprehensive sequencer features

## Building the Test Programs

Follow these steps to build the test programs:

1. Create a build directory if it doesn't exist:
   ```bash
   mkdir -p build
   ```

2. Navigate to the build directory:
   ```bash
   cd build
   ```

3. Run CMake to configure the project:
   ```bash
   cmake ..
   ```

4. Build the test programs:
   ```bash
   make TestSequencerFile TestSequencerAdvanced
   ```

## Running TestSequencerFile

This program generates WAV audio files from pre-defined patterns.

1. Run the program:
   ```bash
   ./bin/TestSequencerFile
   ```

2. The program will:
   - Create an "output" directory if it doesn't exist
   - Generate three WAV files:
     - `sequencer_scale.wav` - A C major scale pattern
     - `sequencer_chords.wav` - A C-F-G-C chord progression
     - `sequencer_arpeggio.wav` - Arpeggiated versions of the same chord progression

3. You can listen to these WAV files using any audio player to hear the sequencer output.

## Running TestSequencerAdvanced

This is an interactive application that demonstrates all the advanced sequencer features.

1. Run the program:
   ```bash
   ./bin/TestSequencerAdvanced
   ```

2. You'll see the main menu with the following options:
   - **Manage Patterns** - View and create note patterns
   - **Edit Song Arrangement** - Chain patterns together into a song
   - **Export to MIDI** - Save patterns or songs as MIDI files
   - **Apply Quantization/Swing** - Apply timing adjustments to patterns
   - **Playback Test** - Simulate pattern or song playback

### Pattern Management

In the Pattern Management menu, you can:
- View details of existing patterns (notes, timing, velocities)
- Create new patterns
- (Note: The full pattern editing functionality is not implemented in this demo)

### Song Arrangement

The Song Arrangement menu allows you to:
- View the current arrangement
- Add patterns to the song at specific positions
- Remove patterns from the arrangement
- Clear the entire arrangement
- Switch between Single Pattern and Song Arrangement playback modes

### MIDI Export

The MIDI Export menu provides options to:
- Export the current pattern as a MIDI file
- Export all patterns as separate tracks in a MIDI file
- (Note: Full song arrangement export would require extending the MidiFile class)

### Quantization and Swing

This menu allows you to:
- Quantize pattern notes to a timing grid (e.g., 16th notes)
- Apply swing feel to patterns with adjustable intensity

### Playback Test

The Playback Test simulates sequencer playback:
- Displays transport information (position, bar, beat)
- Shows note on/off events as they occur
- Works in both Single Pattern and Song Arrangement modes

## Sequencer Features Overview

Through these test programs, you can explore the following sequencer capabilities:

1. **Note Management**
   - MIDI notes with velocity, duration, start time, and channel
   - Pattern organization with multiple patterns

2. **Playback Features**
   - Beat-based timing and position tracking
   - Accurate note triggering with start/end time handling
   - Looping support with tempo control

3. **Advanced Features**
   - Quantization to snap notes to a grid
   - Swing/groove for more natural rhythmic feel
   - Song arrangement system for pattern sequencing
   - MIDI file export

## Next Steps

After testing, consider these potential enhancements:
- MIDI file import functionality
- Integration with AI components for pattern generation
- Development of a graphical user interface for pattern editing
- Real-time MIDI input/output capabilities
