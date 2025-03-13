# AIMusicHardware Project TODOs

## Audio Engine Improvements
- Fix note-off timing and handling - currently cutting off sequences prematurely
- Implement proper voice allocation and tracking to fix polyphony issues
- Fine-tune the envelope processing to ensure smoother release stages
- Consider adding a proper voice manager to track which voices are playing which notes

## Potential Enhancement Paths

1. **Pattern Editing Interface**
   - Create a simple console-based pattern editor
   - Allow real-time addition, removal, and editing of notes
   - Support step sequencing and live input modes

2. **Effect Processing**
   - Add audio effects (reverb, delay, filter, etc.)
   - Create an effect chain system with configurable parameters
   - Implement per-pattern or per-channel effect routing

3. **MIDI Import/Export**
   - Enhance existing MIDI capabilities to import external MIDI files
   - Implement MIDI file parsing and conversion to internal pattern format
   - Add MIDI file export with full song arrangement support

4. **AI-Assisted Pattern Generation**
   - Implement simple algorithms for melodic/rhythmic pattern generation
   - Add options for user-defined constraints (scale, chord progression, etc.)
   - Create variation generator for existing patterns

5. **Programmable Modulation**
   - Add LFOs (Low-Frequency Oscillators) for parameter modulation
   - Implement envelope generators for dynamic control
   - Create modulation matrix for routing modulators to parameters