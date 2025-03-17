# UI Testing Instructions

## Overview

This document provides instructions for testing the custom UI framework implemented for the AI Music Hardware project. The UI framework is a minimalist, hardware-oriented system inspired by the Teenage Engineering OP-1, designed for efficient direct framebuffer manipulation.

## Building the Test Program

The test program (`TestUI`) requires SDL2 to visualize the UI components. Follow these steps to build and run it:

1. First, make sure SDL2 is installed:
   - **macOS**: `brew install sdl2`
   - **Ubuntu/Debian**: `sudo apt-get install libsdl2-dev`
   - **Windows**: Use the SDL2 development libraries from [SDL's website](https://www.libsdl.org/)

2. Build the project with CMake:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make TestUI
   ```

3. Run the test program:
   ```bash
   ./bin/TestUI
   ```

## What You'll See

The test program opens a window demonstrating various UI components from our custom framework:

1. **Title Bar**: Simple header area showing the application name
2. **Navigation Buttons**: Three buttons for different sections
3. **Waveform Display**: Shows a sine wave with grid lines
4. **Envelope Editor**: ADSR envelope visualization with draggable handles
5. **Sequencer Grid**: Pattern editor grid with an animated playhead
6. **Parameter Knob**: Rotary control for parameter adjustment

## Interacting with the UI

While the current test program is a visual demonstration only, the actual framework supports these interactions:

- **Mouse clicks** for buttons and grid cells
- **Mouse drag** for envelope handles and knobs
- **Arrow keys** for navigation in grid-based components
- **Scroll wheel** for simulating rotary encoders
- **Press Escape** to exit the application

## Implementation Details

The test version uses direct SDL2 rendering to visualize what the full UI framework renders. In the actual implementation:

- The UI components render to a DisplayManager framebuffer
- Components respond to input events from hardware or touch
- The UI system manages component hierarchies and screen transitions
- Theme-based styling provides consistent colors across the UI

## Next Steps

1. **Font Integration**: Add bitmap fonts for text rendering
2. **Hardware Integration**: Connect with actual hardware controls
3. **Real-Time Audio Visualization**: Connect waveform display to audio engine
4. **Touch Input Calibration**: Fine-tune touch response for hardware
5. **Screen Transitions**: Implement animations between screens

## Troubleshooting

If you encounter build issues:

1. Ensure SDL2 is properly installed and in your path
2. Check that CMake is finding the SDL2 libraries
3. For linker errors, verify that the SDL2 development libraries are correctly set up

On macOS, you may need to specify the SDL2 path if it's not automatically found:
```bash
cmake -DSDL2_DIR=/usr/local/lib/cmake/SDL2 ..
```

## Learning from the Test

The test program demonstrates several key aspects of the UI design:

1. **Minimalist Aesthetic**: Clean, functional design prioritizing usability
2. **Music-Focused Elements**: Components specifically designed for music production
3. **Visual Hierarchy**: Clear distinction between different types of controls
4. **Color Coding**: Consistent color scheme for different elements
5. **Spatial Organization**: Logical grouping of related components