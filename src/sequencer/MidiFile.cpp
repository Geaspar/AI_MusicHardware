#include "../../include/sequencer/MidiFile.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

MidiFile::MidiFile() {
}

MidiFile::~MidiFile() {
}

bool MidiFile::exportPattern(const Pattern& pattern, const std::string& filename, double tempo) {
    std::vector<Pattern*> patterns = {const_cast<Pattern*>(&pattern)};
    return exportPatterns(patterns, filename, tempo);
}

bool MidiFile::exportPatterns(const std::vector<Pattern*>& patterns, const std::string& filename, double tempo) {
    // Open the output file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Calculate number of tracks (one for each pattern plus one for tempo track)
    uint16_t numTracks = patterns.size() + 1;
    
    // Write MIDI header
    writeHeader(file, 1, numTracks, PPQN); // Format 1 (multiple tracks)
    
    // Write the tempo track
    std::vector<uint8_t> tempoData(3);
    uint32_t microsecondsPerQuarterNote = static_cast<uint32_t>(60000000.0 / tempo);
    tempoData[0] = (microsecondsPerQuarterNote >> 16) & 0xFF;
    tempoData[1] = (microsecondsPerQuarterNote >> 8) & 0xFF;
    tempoData[2] = microsecondsPerQuarterNote & 0xFF;
    
    // Since we don't know the track length yet, we'll come back and update it
    long tempoTrackHeaderPos = file.tellp();
    writeTrackHeader(file, 0); // Placeholder value
    
    // Write tempo meta event
    writeMetaEvent(file, 0, 0x51, tempoData); // Set tempo
    
    // Write time signature (default 4/4)
    std::vector<uint8_t> timeSignatureData = {4, 2, 24, 8}; // 4/4 time signature
    writeMetaEvent(file, 0, 0x58, timeSignatureData);
    
    // Write track end
    writeTrackEnd(file);
    
    // Update tempo track length
    long currentPos = file.tellp();
    file.seekp(tempoTrackHeaderPos + 4);
    uint32_t tempoTrackLength = currentPos - (tempoTrackHeaderPos + 8);
    file.write(reinterpret_cast<const char*>(&tempoTrackLength), 4);
    file.seekp(currentPos);
    
    // Write each pattern as a separate track
    for (const auto& pattern : patterns) {
        if (!pattern) continue;
        
        // Start track position
        long trackStartPos = file.tellp();
        writeTrackHeader(file, 0); // Placeholder value
        
        // Track name meta event
        std::vector<uint8_t> trackNameData(pattern->getName().begin(), pattern->getName().end());
        writeMetaEvent(file, 0, 0x03, trackNameData);
        
        // Keep track of active notes to ensure note-offs are written
        struct ActiveNote {
            int pitch;
            int channel;
            double endTime;
        };
        std::vector<ActiveNote> activeNotes;
        
        // Collect all notes and sort by start time
        std::vector<Note> notes;
        for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
            Note* note = pattern->getNote(i);
            if (note) {
                notes.push_back(*note);
            }
        }
        
        // Sort notes by start time
        std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
            return a.startTime < b.startTime;
        });
        
        double currentTime = 0.0;
        
        // Process notes
        for (const auto& note : notes) {
            // Check for note-offs that happen before this note
            auto it = activeNotes.begin();
            while (it != activeNotes.end()) {
                if (it->endTime <= note.startTime) {
                    // Calculate delta time from current time
                    uint32_t deltaTime = static_cast<uint32_t>((it->endTime - currentTime) * PPQN);
                    
                    // Write note-off event
                    writeEvent(file, deltaTime, 0x80 | it->channel, it->pitch, 0);
                    
                    // Update current time
                    currentTime = it->endTime;
                    
                    // Remove this note from active notes
                    it = activeNotes.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Calculate delta time for note-on
            uint32_t deltaTime = static_cast<uint32_t>((note.startTime - currentTime) * PPQN);
            
            // Write note-on event
            uint8_t velocity = static_cast<uint8_t>(note.velocity * 127.0f);
            writeEvent(file, deltaTime, 0x90 | note.channel, note.pitch, velocity);
            
            // Update current time
            currentTime = note.startTime;
            
            // Add to active notes
            ActiveNote activeNote;
            activeNote.pitch = note.pitch;
            activeNote.channel = note.channel;
            activeNote.endTime = note.startTime + note.duration;
            activeNotes.push_back(activeNote);
        }
        
        // Process remaining note-offs
        for (const auto& an : activeNotes) {
            // Calculate delta time
            uint32_t deltaTime = static_cast<uint32_t>((an.endTime - currentTime) * PPQN);
            
            // Write note-off event
            writeEvent(file, deltaTime, 0x80 | an.channel, an.pitch, 0);
            
            // Update current time
            currentTime = an.endTime;
        }
        
        // Write track end
        writeTrackEnd(file);
        
        // Update track length
        long currentPos = file.tellp();
        file.seekp(trackStartPos + 4);
        uint32_t trackLength = currentPos - (trackStartPos + 8);
        file.write(reinterpret_cast<const char*>(&trackLength), 4);
        file.seekp(currentPos);
    }
    
    file.close();
    return true;
}

void MidiFile::writeHeader(std::ofstream& file, uint16_t format, uint16_t numTracks, uint16_t ticksPerQuarterNote) {
    // Write MThd chunk
    file.write("MThd", 4);
    
    // Header length (always 6)
    uint32_t headerLength = 6;
    headerLength = (headerLength << 24) | ((headerLength << 8) & 0x00FF0000) | ((headerLength >> 8) & 0x0000FF00) | (headerLength >> 24);
    file.write(reinterpret_cast<const char*>(&headerLength), 4);
    
    // Format (0 = single track, 1 = multiple tracks)
    format = (format << 8) | (format >> 8); // Swap bytes
    file.write(reinterpret_cast<const char*>(&format), 2);
    
    // Number of tracks
    numTracks = (numTracks << 8) | (numTracks >> 8); // Swap bytes
    file.write(reinterpret_cast<const char*>(&numTracks), 2);
    
    // Ticks per quarter note
    ticksPerQuarterNote = (ticksPerQuarterNote << 8) | (ticksPerQuarterNote >> 8); // Swap bytes
    file.write(reinterpret_cast<const char*>(&ticksPerQuarterNote), 2);
}

void MidiFile::writeTrackHeader(std::ofstream& file, uint32_t trackLength) {
    // Write MTrk chunk
    file.write("MTrk", 4);
    
    // Track length (will be updated later)
    trackLength = (trackLength << 24) | ((trackLength << 8) & 0x00FF0000) | ((trackLength >> 8) & 0x0000FF00) | (trackLength >> 24);
    file.write(reinterpret_cast<const char*>(&trackLength), 4);
}

void MidiFile::writeTrackEnd(std::ofstream& file) {
    // Write end of track meta event
    writeMetaEvent(file, 0, 0x2F, {});
}

void MidiFile::writeEvent(std::ofstream& file, uint32_t deltaTime, uint8_t eventType, uint8_t data1, uint8_t data2) {
    // Write variable-length delta time
    writeVarLen(file, deltaTime);
    
    // Write event
    file.write(reinterpret_cast<const char*>(&eventType), 1);
    file.write(reinterpret_cast<const char*>(&data1), 1);
    file.write(reinterpret_cast<const char*>(&data2), 1);
}

void MidiFile::writeMetaEvent(std::ofstream& file, uint32_t deltaTime, uint8_t metaType, const std::vector<uint8_t>& data) {
    // Write variable-length delta time
    writeVarLen(file, deltaTime);
    
    // Write meta event marker
    uint8_t metaEventMarker = 0xFF;
    file.write(reinterpret_cast<const char*>(&metaEventMarker), 1);
    
    // Write meta event type
    file.write(reinterpret_cast<const char*>(&metaType), 1);
    
    // Write data length
    writeVarLen(file, data.size());
    
    // Write data
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
}

void MidiFile::writeVarLen(std::ofstream& file, uint32_t value) {
    uint8_t buffer[4];
    int bufferIndex = 0;
    
    buffer[0] = value & 0x7F;
    
    while ((value >>= 7) > 0) {
        buffer[0] |= 0x80;
        buffer[++bufferIndex] = value & 0x7F;
    }
    
    while (bufferIndex >= 0) {
        file.write(reinterpret_cast<const char*>(&buffer[bufferIndex]), 1);
        bufferIndex--;
    }
}

} // namespace AIMusicHardware