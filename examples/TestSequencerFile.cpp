#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdint>

#include "../include/sequencer/Sequencer.h"

using namespace AIMusicHardware;

// Constants for WAV output
const int SAMPLE_RATE = 44100;
const int NUM_CHANNELS = 2;
const int BITS_PER_SAMPLE = 16;

// Write a simple WAV file - minimal implementation
void writeWavFile(const std::string& filename, const std::vector<float>& audioData, int sampleRate, int numChannels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    // Convert float audio data to 16-bit PCM
    std::vector<int16_t> pcmData(audioData.size());
    for (size_t i = 0; i < audioData.size(); ++i) {
        // Scale and clamp
        float sample = audioData[i] * 32767.0f;
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        pcmData[i] = static_cast<int16_t>(sample);
    }

    // Calculate data size
    uint32_t dataChunkSize = static_cast<uint32_t>(pcmData.size() * sizeof(int16_t));
    uint32_t fileSize = 36 + dataChunkSize;  // File size - 8 bytes for RIFF header

    // WAV header structure
    #pragma pack(push, 1)
    struct WavHeader {
        // RIFF chunk
        char        riffChunkId[4] = {'R', 'I', 'F', 'F'};
        uint32_t    riffChunkSize; // File size - 8
        char        riffFormat[4] = {'W', 'A', 'V', 'E'};
        
        // fmt sub-chunk
        char        fmtChunkId[4] = {'f', 'm', 't', ' '};
        uint32_t    fmtChunkSize = 16;
        uint16_t    audioFormat = 1; // PCM
        uint16_t    numChannels;
        uint32_t    sampleRate;
        uint32_t    byteRate;
        uint16_t    blockAlign;
        uint16_t    bitsPerSample = BITS_PER_SAMPLE;
        
        // data sub-chunk
        char        dataChunkId[4] = {'d', 'a', 't', 'a'};
        uint32_t    dataChunkSize;
    };
    #pragma pack(pop)

    // Prepare WAV header
    WavHeader header;
    header.riffChunkSize = fileSize;
    header.numChannels = static_cast<uint16_t>(numChannels);
    header.sampleRate = static_cast<uint32_t>(sampleRate);
    header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.dataChunkSize = dataChunkSize;

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write audio data
    file.write(reinterpret_cast<const char*>(pcmData.data()), dataChunkSize);
    
    file.close();
    std::cout << "WAV file written: " << filename << std::endl;
}

// Simple synthesizer for note generation
class SimpleOscillator {
public:
    // Generate a sine wave note
    static float sineWave(float phase) {
        return std::sin(phase * 2.0f * M_PI);
    }
    
    // Convert MIDI note to frequency
    static float midiNoteToFrequency(int midiNote) {
        return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    }
    
    // Generate audio for a list of notes
    static std::vector<float> renderNotes(const std::vector<Note>& notes, double lengthInBeats, double tempo) {
        double beatsPerSecond = tempo / 60.0;
        double totalTimeSeconds = lengthInBeats / beatsPerSecond;
        int totalSamples = static_cast<int>(totalTimeSeconds * SAMPLE_RATE);
        
        // Initialize buffer with zeros
        std::vector<float> buffer(totalSamples * NUM_CHANNELS, 0.0f);
        
        // Process each note
        for (const auto& note : notes) {
            // Calculate start and end samples
            int startSample = static_cast<int>(note.startTime / beatsPerSecond * SAMPLE_RATE);
            int endSample = static_cast<int>((note.startTime + note.duration) / beatsPerSecond * SAMPLE_RATE);
            
            if (endSample > totalSamples) {
                endSample = totalSamples;
            }
            
            // Generate audio for this note
            float frequency = midiNoteToFrequency(note.pitch);
            float phase = 0.0f;
            float phaseIncrement = frequency / SAMPLE_RATE;
            
            for (int i = startSample; i < endSample; ++i) {
                if (i < 0 || i >= totalSamples) continue;
                
                // Generate sine wave sample
                float sample = sineWave(phase) * note.velocity;
                
                // Apply a simple envelope
                float envelopePosition = static_cast<float>(i - startSample) / (endSample - startSample);
                float envelope = 1.0f;
                
                // Attack (first 10%)
                if (envelopePosition < 0.1f) {
                    envelope = envelopePosition / 0.1f;
                }
                // Release (last 20%)
                else if (envelopePosition > 0.8f) {
                    envelope = (1.0f - envelopePosition) / 0.2f;
                }
                
                // Apply envelope
                sample *= envelope;
                
                // Mix into output buffer
                buffer[i * NUM_CHANNELS] += sample;
                buffer[i * NUM_CHANNELS + 1] += sample;
                
                // Update phase
                phase += phaseIncrement;
                if (phase >= 1.0f) {
                    phase -= 1.0f;
                }
            }
        }
        
        // Normalize to prevent clipping
        float maxAmplitude = 0.0f;
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (std::abs(buffer[i]) > maxAmplitude) {
                maxAmplitude = std::abs(buffer[i]);
            }
        }
        
        if (maxAmplitude > 1.0f) {
            for (size_t i = 0; i < buffer.size(); ++i) {
                buffer[i] /= maxAmplitude;
            }
        }
        
        return buffer;
    }
};

// Helper function to create a simple pattern
std::unique_ptr<Pattern> createSimplePattern() {
    auto pattern = std::make_unique<Pattern>("Simple Pattern");
    
    // Add a simple C major scale
    // C4, D4, E4, F4, G4, A4, B4, C5
    int notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
    
    for (int i = 0; i < 8; ++i) {
        // Each note is placed at a specific beat position
        // with a duration of 0.5 beats
        Note note(notes[i], 0.8f, i * 0.5, 0.4, 0);
        pattern->addNote(note);
    }
    
    return pattern;
}

// Helper function to create a chord pattern
std::unique_ptr<Pattern> createChordPattern() {
    auto pattern = std::make_unique<Pattern>("Chord Pattern");
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 0.0, 1.0, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 0.0, 1.0, 0));  // E4
    pattern->addNote(Note(67, 0.7f, 0.0, 1.0, 0));  // G4
    
    // F Major chord (F, A, C)
    pattern->addNote(Note(65, 0.7f, 1.0, 1.0, 0));  // F4
    pattern->addNote(Note(69, 0.7f, 1.0, 1.0, 0));  // A4
    pattern->addNote(Note(72, 0.7f, 1.0, 1.0, 0));  // C5
    
    // G Major chord (G, B, D)
    pattern->addNote(Note(67, 0.7f, 2.0, 1.0, 0));  // G4
    pattern->addNote(Note(71, 0.7f, 2.0, 1.0, 0));  // B4
    pattern->addNote(Note(74, 0.7f, 2.0, 1.0, 0));  // D5
    
    // C Major chord (C, E, G)
    pattern->addNote(Note(60, 0.7f, 3.0, 1.0, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 3.0, 1.0, 0));  // E4
    pattern->addNote(Note(67, 0.7f, 3.0, 1.0, 0));  // G4
    
    return pattern;
}

// Helper function to create an arpeggio pattern
std::unique_ptr<Pattern> createArpeggioPattern() {
    auto pattern = std::make_unique<Pattern>("Arpeggio Pattern");
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 0.0, 0.25, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 0.25, 0.25, 0)); // E4
    pattern->addNote(Note(67, 0.7f, 0.5, 0.25, 0));  // G4
    pattern->addNote(Note(72, 0.7f, 0.75, 0.25, 0)); // C5
    
    // F Major arpeggio (F, A, C, F)
    pattern->addNote(Note(65, 0.7f, 1.0, 0.25, 0));  // F4
    pattern->addNote(Note(69, 0.7f, 1.25, 0.25, 0)); // A4
    pattern->addNote(Note(72, 0.7f, 1.5, 0.25, 0));  // C5
    pattern->addNote(Note(77, 0.7f, 1.75, 0.25, 0)); // F5
    
    // G Major arpeggio (G, B, D, G)
    pattern->addNote(Note(67, 0.7f, 2.0, 0.25, 0));  // G4
    pattern->addNote(Note(71, 0.7f, 2.25, 0.25, 0)); // B4
    pattern->addNote(Note(74, 0.7f, 2.5, 0.25, 0));  // D5
    pattern->addNote(Note(79, 0.7f, 2.75, 0.25, 0)); // G5
    
    // C Major arpeggio (C, E, G, C)
    pattern->addNote(Note(60, 0.7f, 3.0, 0.25, 0));  // C4
    pattern->addNote(Note(64, 0.7f, 3.25, 0.25, 0)); // E4
    pattern->addNote(Note(67, 0.7f, 3.5, 0.25, 0));  // G4
    pattern->addNote(Note(72, 0.7f, 3.75, 0.25, 0)); // C5
    
    return pattern;
}

// Collect all notes from a pattern
std::vector<Note> collectNotesFromPattern(const Pattern* pattern) {
    std::vector<Note> allNotes;
    
    for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
        const Note* note = pattern->getNote(i);
        if (note) {
            allNotes.push_back(*note);
        }
    }
    
    return allNotes;
}

int main() {
    std::cout << "===== Sequencer File Test =====" << std::endl;
    
    // Create directory for output files
    system("mkdir -p output");
    
    // Create patterns
    auto simplePattern = createSimplePattern();
    auto chordPattern = createChordPattern();
    auto arpeggioPattern = createArpeggioPattern();
    
    // Generate audio for each pattern
    double tempo = 120.0;  // BPM
    
    // Simple pattern
    {
        std::vector<Note> notes = collectNotesFromPattern(simplePattern.get());
        std::vector<float> audio = SimpleOscillator::renderNotes(notes, simplePattern->getLength(), tempo);
        writeWavFile("output/sequencer_scale.wav", audio, SAMPLE_RATE, NUM_CHANNELS);
    }
    
    // Chord pattern
    {
        std::vector<Note> notes = collectNotesFromPattern(chordPattern.get());
        std::vector<float> audio = SimpleOscillator::renderNotes(notes, chordPattern->getLength(), tempo);
        writeWavFile("output/sequencer_chords.wav", audio, SAMPLE_RATE, NUM_CHANNELS);
    }
    
    // Arpeggio pattern
    {
        std::vector<Note> notes = collectNotesFromPattern(arpeggioPattern.get());
        std::vector<float> audio = SimpleOscillator::renderNotes(notes, arpeggioPattern->getLength(), tempo);
        writeWavFile("output/sequencer_arpeggio.wav", audio, SAMPLE_RATE, NUM_CHANNELS);
    }
    
    std::cout << "All patterns have been rendered to WAV files in the output directory." << std::endl;
    return 0;
}