# AI Music Hardware

An electronic musical instrument with LLM-assisted composition capabilities, digital synthesis, effects processing, and sequencing.

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

The architecture consists of the following core components:

1. **Audio Engine**: Real-time C++ audio processing with digital synthesis
2. **Effects System**: Modular audio effects chain
3. **Sequencer**: Pattern-based sequencer with timing and note events
4. **MIDI Integration**: Hardware connectivity via MIDI protocol
5. **Hardware Interface**: Physical controls and display interface
6. **LLM Integration**:
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

## Next Steps

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

## Getting Started
1. Install dependencies
2. Build the project with CMake
3. Run examples