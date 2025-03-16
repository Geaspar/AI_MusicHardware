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

> **Note:** TestSequencerAdvanced is a simulation-only application that demonstrates sequencer functionality visually through the console. It does not produce actual audio output. For audio output, use TestSequencerFile to generate WAV files or TestAudio for real-time audio.

This is an interactive application that demonstrates all the advanced sequencer features. The recent improvements make it more stable and user-friendly with better input handling, thread safety, and error management.

### Building and Running

1. Build the program:
   ```bash
   # From the project root directory
   mkdir -p build
   cd build
   cmake ..
   make TestSequencerAdvanced
   ```

2. Run the program:
   ```bash
   ./bin/TestSequencerAdvanced
   ```

3. You'll see the initial output with pre-loaded patterns and the main menu will appear.

### Step-by-Step Usage Guide

When you start TestSequencerAdvanced, the program:
1. Creates a sequencer at 120 BPM in 4/4 time
2. Loads five predefined patterns:
   - C Major Scale
   - C-F-G-C Chord Progression
   - C-F-G-C Arpeggios
   - Bass Pattern
   - Basic Drum Pattern
3. Presents the main menu

#### Main Menu Navigation

The main menu offers these options:
1. **Manage Patterns**
2. **Edit Song Arrangement**
3. **Export to MIDI**
4. **Apply Quantization/Swing**
5. **Playback Test**
0. **Exit**

Navigate by entering the number corresponding to your choice. The program validates your input to ensure it's within range.

#### 1. Pattern Management

In this section you can:

1. **View Pattern Details**: Select a pattern to see its:
   - Name and length
   - Number of notes
   - Each note's pitch, start time, duration, velocity, and channel
   
2. **Create New Patterns**:
   - Choose "Create New Pattern" from the menu
   - Enter a name for your pattern
   - The new (empty) pattern is added to the pattern list
   
3. **Return to Main Menu**:
   - Select "Exit" (option 0)

Example workflow:
```
1. Select "Manage Patterns"
2. Choose "C Major Scale" (option 1)
3. View the pattern details
4. Press Enter to continue
5. Select "Create New Pattern" (last option)
6. Enter a name like "My Custom Pattern"
7. Select "Exit" (option 0) to return to the main menu
```

#### 2. Song Arrangement

This section allows you to:

1. **View Current Arrangement**:
   - Shows all pattern instances in the song
   - Displays pattern name, start beat, and end beat
   - Uses pagination for large arrangements (5 items per page)
   
2. **Add Pattern to Song**:
   - Select a pattern from the list
   - Enter a start position in beats
   - The pattern is added to the arrangement
   
3. **Remove Pattern from Song**:
   - View the arrangement
   - Enter the index of the pattern instance to remove
   
4. **Clear Song Arrangement**:
   - Removes all patterns from the arrangement
   
5. **Set Playback Mode**:
   - Toggle between Single Pattern and Song Arrangement modes

Example workflow:
```
1. Select "Edit Song Arrangement"
2. Choose "Add Pattern to Song"
3. Select "C Major Scale" (option 1)
4. Enter start position "0.0"
5. Choose "Add Pattern to Song" again
6. Select "C-F-G-C Progression" (option 2)
7. Enter start position "4.0"
8. Choose "View Current Arrangement" to see your song
9. Select "Exit" to return to the main menu
```

#### 3. MIDI Export

Options in this section:

1. **Export Current Pattern**:
   - Saves the current pattern as "output_pattern.mid"
   
2. **Export All Patterns as Separate Tracks**:
   - Saves all patterns as "output_all_patterns.mid"
   
3. **Export Song Arrangement** (not fully implemented)

Example workflow:
```
1. Select "Export to MIDI"
2. Choose "Export All Patterns as Separate Tracks"
3. Verify the export message
4. Select "Exit" to return to the main menu
```

#### 4. Quantization and Swing

This section allows you to:

1. **Quantize a Pattern**:
   - Select a pattern
   - Enter grid size (e.g., 0.25 for 16th notes)
   - Notes are aligned to the nearest grid position
   
2. **Apply Swing**:
   - Select a pattern
   - Enter swing amount (0.0-0.5, where 0.33 is typical)
   - Enter grid size
   - This adds a "groove" feel to the rhythm

Example workflow:
```
1. Select "Apply Quantization/Swing"
2. Choose "Apply Swing"
3. Select a pattern (e.g., "C Major Scale")
4. Enter swing amount "0.33"
5. Enter grid size "0.25"
6. Verify the confirmation message
7. Return to the main menu
```

#### 5. Playback Test

This simulates playback of your patterns or song (no actual audio output):

1. Displays current mode (Single Pattern or Song Arrangement)
2. Shows pattern name or song length
3. Press Enter to start playback
4. The position indicator shows the current playback position
5. Note On/Off events are displayed as they occur in the console (visual feedback only)
6. Press Enter again to stop playback

**Important Note:** This is a visual simulation only - no actual sound will be produced. The TestSequencerAdvanced application is designed to test the sequencer's functionality without audio output. To hear actual audio:
- Use TestSequencerFile to generate WAV files that you can listen to
- Use TestAudio for real-time audio playback

Example workflow:
```
1. Select "Playback Test"
2. Press Enter to start playback
3. Watch the transport position and note events in the console
4. Press Enter to stop playback
5. Return to the main menu
```

### Troubleshooting

If you encounter issues:

1. **Input Problems**:
   - The program now handles invalid input gracefully
   - If asked to enter a number, you'll be prompted to try again if input is invalid

2. **Display Issues**:
   - For large arrangements, the display uses pagination
   - Press Enter to see more items when prompted

3. **Playback Performance**:
   - Playback is simulated and might show different timing behavior than real-time systems
   - Thread safety has been improved to prevent crashes during callback execution

4. **Build Issues**:
   - Make sure you have built the program with the latest code
   - If building fails, check for missing dependencies

### Advanced Usage Tips

1. **Creating Complex Songs**:
   - Add multiple patterns with strategic start positions
   - Use the "View Current Arrangement" to verify timing
   - Test with "Playback Test" to hear the result

2. **Effective Swing Application**:
   - Start with 0.33 swing amount on 16th note grid (0.25)
   - For subtle swing, use values around 0.2
   - For stronger swing, try values up to 0.45

3. **Exiting the Program**:
   - Select "Exit" (option 0) from the main menu
   - The program will clean up resources properly before exiting

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

## Running TestAdaptiveSequencer

The AdaptiveSequencer test application demonstrates state-based music sequencing with dynamic transitions and parameter control.

### Building and Running

1. Build the program:
   ```bash
   # From the project root directory
   mkdir -p build
   cd build
   cmake ..
   make TestAdaptiveSequencer
   ```

2. Run the program:
   ```bash
   ./bin/TestAdaptiveSequencer
   ```

3. You'll see a welcome message and instructions for controlling the adaptive sequencer.

### Interactive Controls

The application provides a keyboard interface to control the adaptive sequencer parameters:

1. **Intensity Parameter Control**:
   - Press `1` - Set intensity to 0.0 (lowest)
   - Press `2` - Set intensity to 0.25
   - Press `3` - Set intensity to 0.5
   - Press `4` - Set intensity to 0.75
   - Press `5` - Set intensity to 1.0 (highest)

2. **State Control**:
   - Press `a` - Force transition to "ambient" state (low intensity)
   - Press `e` - Force transition to "energetic" state (high intensity)

3. **Mood Parameter Control**:
   - Press `m` - Decrease mood parameter by 0.1
   - Press `M` (shift+m) - Increase mood parameter by 0.1

4. **Playback Control**:
   - Press `p` - Pause/resume playback
   - Press `q` - Quit the application

### Automatic Transitions

The test demonstrates automatic transitions based on parameter values:

1. **Ambient to Energetic**: When intensity increases above 0.7, the sequencer will automatically transition from ambient to energetic state
2. **Energetic to Ambient**: When intensity decreases below 0.3, the sequencer will automatically transition back to ambient state

### Musical States 

The test includes two musical states with distinct characteristics:

1. **Ambient State (Low Intensity)**:
   - Slower tempo (80 BPM)
   - Three layers: bass, pad, and melody
   - Two mix snapshots: "full" and "minimal"
   - Sparser rhythmic patterns

2. **Energetic State (High Intensity)**:
   - Faster tempo (120 BPM)
   - Three layers: bass, lead, and drums
   - Two mix snapshots: "full" and "drums_only"
   - Denser rhythmic patterns

### Testing Suggestions

1. **Test Parameter-Driven Transitions**:
   - Start with intensity at 0.0 (press `1`)
   - Gradually increase intensity (press `2`, `3`, `4`, `5`)
   - Observe the automatic transition to energetic state
   - Gradually decrease intensity (press `4`, `3`, `2`, `1`)
   - Observe the automatic transition back to ambient state

2. **Test Forced State Changes**:
   - Press `a` to force ambient state
   - Press `e` to force energetic state
   - Try modifying intensity while in each state

3. **Test Mood Parameter**:
   - Adjust the mood parameter up and down using `m` and `M`
   - In a complete implementation, this would affect elements like timbre or effects

4. **Test Playback Control**:
   - Use `p` to pause and resume playback
   - Ensure the system resumes in the same state

## Next Steps

After testing, consider these potential enhancements:
- MIDI file import functionality
- Integration with AI components for pattern generation
- Development of a graphical user interface for pattern editing
- Real-time MIDI input/output capabilities
- Hardware controller mapping for physical control of the adaptive sequencer
