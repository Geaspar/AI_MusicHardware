#pragma once

#include <string>
#include <vector>
#include <fstream>
#include "Sequencer.h"

namespace AIMusicHardware {

// Simple class for MIDI file export
class MidiFile {
public:
    MidiFile();
    ~MidiFile();
    
    // Export a pattern to a MIDI file
    bool exportPattern(const Pattern& pattern, const std::string& filename, double tempo = 120.0);
    
    // Export multiple patterns as separate tracks in a MIDI file
    bool exportPatterns(const std::vector<Pattern*>& patterns, const std::string& filename, double tempo = 120.0);
    
private:
    // MIDI file writing utilities
    void writeHeader(std::ofstream& file, uint16_t format, uint16_t numTracks, uint16_t ticksPerQuarterNote);
    void writeTrackHeader(std::ofstream& file, uint32_t trackLength);
    void writeTrackEnd(std::ofstream& file);
    void writeEvent(std::ofstream& file, uint32_t deltaTime, uint8_t eventType, uint8_t data1, uint8_t data2);
    void writeMetaEvent(std::ofstream& file, uint32_t deltaTime, uint8_t metaType, const std::vector<uint8_t>& data);
    void writeVarLen(std::ofstream& file, uint32_t value);
    
    // Constants for MIDI file format
    static constexpr uint16_t PPQN = 480; // Pulses (ticks) per quarter note
};

} // namespace AIMusicHardware