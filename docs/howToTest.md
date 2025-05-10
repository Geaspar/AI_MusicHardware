# How to Test AIMusicHardware

This guide provides instructions for testing the various components of the AIMusicHardware project.

## MIDI Implementation Testing

### Installing RtMidi

Before you can test the MIDI implementation, you need to install the RtMidi library. You can use CMake to build and install it:

1. Clone the RtMidi repository:
   ```bash
   git clone https://github.com/thestk/rtmidi.git
   cd rtmidi
   ```

2. Build and install using CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. Install the library (may require sudo):
   ```bash
   sudo make install
   ```

4. On macOS, you might need to update the dynamic library path:
   ```bash
   export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
   ```

Alternatively, on macOS, you can use Homebrew to install RtMidi:

```bash
brew install rtmidi
```

### Building the TestMidi Application

Once RtMidi is installed, you can build the TestMidi application:

1. Make sure your CMake build is up to date:
   ```bash
   cd /Users/geaspar/AIMusicHardware
   mkdir -p build
   cd build
   cmake ..
   make
   ```

2. The build system should detect RtMidi and build the TestMidi application:
   ```bash
   make TestMidi
   ```

### Running the TestMidi Application

The TestMidi application allows you to test MIDI input/output functionality:

1. Connect a MIDI device to your computer (keyboard, controller, etc.)

2. Run the TestMidi application:
   ```bash
   ./bin/TestMidi
   ```

3. The application will:
   - Display a list of available MIDI input devices
   - Prompt you to select an input device
   - Show MIDI messages received from the selected device
   - Test MIDI learn functionality

4. Test interactions:
   - Play notes on your MIDI device to see the note messages
   - Move controllers to see control change messages
   - Use the pitch wheel to test pitch bend messages
   - Assign a controller to a parameter using MIDI learn

5. To exit the application, press Ctrl+C

### Troubleshooting

If you encounter issues with RtMidi:

1. Make sure your MIDI device is connected before starting the application
2. Check system MIDI settings to ensure your device is recognized
3. Verify that you have the correct permissions to access MIDI devices
4. On Linux, you might need to install ALSA development libraries:
   ```bash
   sudo apt-get install libasound2-dev
   ```
5. On macOS, ensure CoreAudio and CoreMIDI frameworks are available

## UI Components Testing

The UI component framework can be tested using the TestUI application, which provides a visual demonstration of the controls and parameter organization:

### Building the TestUI Application

1. Make sure your build environment is up to date:
   ```bash
   cd /Users/geaspar/AIMusicHardware
   mkdir -p build
   cd build
   cmake ..
   make
   ```

2. Build the TestUI application specifically:
   ```bash
   make TestUI
   ```

### Running the TestUI Application

1. Launch the TestUI application:
   ```bash
   ./bin/TestUI
   ```

2. The application provides a visual testbed with:
   - Knobs with modulation visualization
   - Parameter panels with organized controls
   - Tabbed interface for switching between parameter sections
   - MIDI learn functionality demonstration
   - Waveform display and envelopes

### Testing UI Features

1. **Parameter Controls**:
   - Click and drag knobs to change values
   - Use mouse wheel on knobs for fine adjustments
   - Double-click a knob to activate MIDI learn mode
   - Observe the visual feedback for parameter changes
   - Verify improved angle calculation for more accurate rotation mapping
   - Test step quantization for precise value settings

2. **Tab Navigation**:
   - Click on different tabs to switch between parameter sections
   - Test tab scrolling arrows if more tabs are available than can fit
   - Verify tab highlighting works correctly
   - Check that components show/hide correctly when switching tabs
   - Verify proper event delegation to components in the active tab

3. **Parameter Panels**:
   - Test grid-based layout with multiple parameters
   - Verify parameter titles and values display correctly
   - Check spacing and organization of controls
   - Test parameter grouping and visual hierarchy

4. **MIDI Learn**:
   - Enter MIDI learn mode by double-clicking on a knob
   - Send MIDI CC messages from a connected controller
   - Verify the knob shows the mapped CC number and responds to controller movements
   - Test clearing MIDI mappings
   - Check enhanced visual feedback for MIDI mapping status

5. **Modulation Visualization**:
   - Observe improved modulation visualization using line segments
   - Test changes to modulation amount
   - Verify color coding for different modulation sources
   - Check that modulation arcs update smoothly with parameter changes

6. **Performance Optimizations**:
   - Test UI responsiveness with many components
   - Observe waveform display with large sample counts
   - Verify rendering optimizations reduce CPU usage
   - Check that visibility optimizations skip rendering off-screen components

### Keyboard Shortcuts

- `Tab`: Cycle through UI components
- `Space`: Toggle selected controls (buttons, toggles)
- `Arrow Keys`: Navigate between controls
- `ESC`: Exit MIDI learn mode / Exit application

### Testing Tips

- Test on different screen sizes to verify layout adaptability
- Try different DPI settings to ensure proper scaling
- Test with both mouse and touch input if available
- Connect MIDI controllers to verify hardware integration
- Verify CPU usage remains reasonable during UI updates

## Other Tests

The project includes several other test applications:

- `SimpleTest`: Generates WAV files in the output directory
- `TestAudio`: Real-time audio playback test
- `TestSequencer`: Tests the sequencer component with real-time audio
- `TestAdaptiveSequencer`: Tests the adaptive sequencer system
- `WavetableDemo`: Demonstrates the wavetable synthesizer implementation

Run any of these tests from the build/bin directory to evaluate different aspects of the system.