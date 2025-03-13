#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdint>

// Simple audio synthesis test that writes to a WAV file
// This bypasses RtAudio to test just the synthesis logic

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
    const uint32_t dataChunkSize = static_cast<uint32_t>(pcmData.size() * sizeof(int16_t));
    const uint32_t fileSize = 36 + dataChunkSize;  // File size - 8 bytes for RIFF header

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

// Oscillator types
enum class OscillatorType {
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
};

// Generate a single sample for a given oscillator type
float generateSample(OscillatorType type, float phase) {
    switch (type) {
        case OscillatorType::Sine:
            return std::sin(phase * 2.0f * M_PI);
            
        case OscillatorType::Square:
            return (phase < 0.5f) ? 1.0f : -1.0f;
            
        case OscillatorType::Saw:
            return 2.0f * phase - 1.0f;
            
        case OscillatorType::Triangle:
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            
        case OscillatorType::Noise:
            return 2.0f * (static_cast<float>(rand()) / RAND_MAX) - 1.0f;
            
        default:
            return 0.0f;
    }
}

// Generate a note with the given oscillator type, frequency, and duration
std::vector<float> generateNote(OscillatorType type, float frequency, float duration, float amplitude = 0.5f) {
    int numSamples = static_cast<int>(SAMPLE_RATE * duration);
    std::vector<float> audio(numSamples * NUM_CHANNELS);
    
    float phase = 0.0f;
    float phaseIncrement = frequency / SAMPLE_RATE;
    
    // Simple ADSR envelope parameters (in seconds)
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.2f;
    
    // Convert times to samples
    int attackSamples = static_cast<int>(attack * SAMPLE_RATE);
    int decaySamples = static_cast<int>(decay * SAMPLE_RATE);
    int sustainSamples = static_cast<int>((duration - attack - decay - release) * SAMPLE_RATE);
    int releaseSamples = static_cast<int>(release * SAMPLE_RATE);
    
    for (int i = 0; i < numSamples; ++i) {
        // Generate oscillator sample
        float sample = generateSample(type, phase);
        
        // Update phase
        phase += phaseIncrement;
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }
        
        // Apply envelope
        float envelope;
        if (i < attackSamples) {
            // Attack
            envelope = static_cast<float>(i) / attackSamples;
        } else if (i < attackSamples + decaySamples) {
            // Decay
            float decayPhase = static_cast<float>(i - attackSamples) / decaySamples;
            envelope = 1.0f - (1.0f - sustain) * decayPhase;
        } else if (i < attackSamples + decaySamples + sustainSamples) {
            // Sustain
            envelope = sustain;
        } else {
            // Release
            float releasePhase = static_cast<float>(i - attackSamples - decaySamples - sustainSamples) / releaseSamples;
            envelope = sustain * (1.0f - releasePhase);
        }
        
        // Apply envelope and amplitude
        sample *= envelope * amplitude;
        
        // Add to both channels (stereo)
        audio[i * NUM_CHANNELS] = sample;
        audio[i * NUM_CHANNELS + 1] = sample;
    }
    
    return audio;
}

// Generate a scale with the given oscillator type
std::vector<float> generateScale(OscillatorType type) {
    std::vector<float> audio;
    
    // C major scale frequencies: C4, D4, E4, F4, G4, A4, B4, C5
    float notes[] = {261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f};
    
    for (float note : notes) {
        std::vector<float> noteAudio = generateNote(type, note, 0.3f, 0.3f);
        audio.insert(audio.end(), noteAudio.begin(), noteAudio.end());
        
        // Add a small gap between notes
        std::vector<float> gap(SAMPLE_RATE * 0.1 * NUM_CHANNELS, 0.0f);
        audio.insert(audio.end(), gap.begin(), gap.end());
    }
    
    return audio;
}

// Generate a test file with different waveforms
std::vector<float> generateWaveformTest() {
    std::vector<float> audio;
    
    OscillatorType types[] = {
        OscillatorType::Sine,
        OscillatorType::Square,
        OscillatorType::Saw,
        OscillatorType::Triangle,
        OscillatorType::Noise
    };
    
    const char* names[] = {
        "Sine",
        "Square",
        "Saw",
        "Triangle",
        "Noise"
    };
    
    // Test each oscillator type with a C4 note
    for (int i = 0; i < 5; ++i) {
        std::cout << "Generating " << names[i] << " wave..." << std::endl;
        
        std::vector<float> noteAudio = generateNote(types[i], 261.63f, 1.0f, 0.3f);
        audio.insert(audio.end(), noteAudio.begin(), noteAudio.end());
        
        // Add a small gap between waveforms
        std::vector<float> gap(SAMPLE_RATE * 0.5 * NUM_CHANNELS, 0.0f);
        audio.insert(audio.end(), gap.begin(), gap.end());
    }
    
    return audio;
}

// Generate a C major chord
std::vector<float> generateChord() {
    // C major chord: C4, E4, G4
    float frequencies[] = {261.63f, 329.63f, 392.00f};
    float duration = 2.0f;
    int numSamples = static_cast<int>(SAMPLE_RATE * duration);
    std::vector<float> audio(numSamples * NUM_CHANNELS, 0.0f);
    
    // Generate each note
    for (float frequency : frequencies) {
        float phase = 0.0f;
        float phaseIncrement = frequency / SAMPLE_RATE;
        
        for (int i = 0; i < numSamples; ++i) {
            // Generate sine wave
            float sample = std::sin(phase * 2.0f * M_PI) * 0.2f;  // Lower amplitude for chord
            
            // Apply simple envelope
            float envelope = 1.0f;
            if (i < SAMPLE_RATE * 0.01f) {  // 10ms attack
                envelope = static_cast<float>(i) / (SAMPLE_RATE * 0.01f);
            } else if (i > numSamples - SAMPLE_RATE * 0.5f) {  // 500ms release
                envelope = (numSamples - i) / (SAMPLE_RATE * 0.5f);
            }
            
            // Add to existing samples
            audio[i * NUM_CHANNELS] += sample * envelope;
            audio[i * NUM_CHANNELS + 1] += sample * envelope;
            
            // Update phase
            phase += phaseIncrement;
            if (phase >= 1.0f) {
                phase -= 1.0f;
            }
        }
    }
    
    return audio;
}

int main() {
    std::cout << "AI Music Hardware - Simple Audio Synthesis Test" << std::endl;
    std::cout << "Generating test audio files..." << std::endl;
    
    // Create output directory if it doesn't exist
    system("mkdir -p output");
    
    // Test 1: Single note (C4) with sine wave
    std::vector<float> sineNote = generateNote(OscillatorType::Sine, 261.63f, 2.0f);
    writeWavFile("output/sine_note.wav", sineNote, SAMPLE_RATE, NUM_CHANNELS);
    
    // Test 2: C major scale with sine wave
    std::vector<float> scale = generateScale(OscillatorType::Sine);
    writeWavFile("output/scale.wav", scale, SAMPLE_RATE, NUM_CHANNELS);
    
    // Test 3: Different waveforms
    std::vector<float> waveforms = generateWaveformTest();
    writeWavFile("output/waveforms.wav", waveforms, SAMPLE_RATE, NUM_CHANNELS);
    
    // Test 4: C major chord
    std::vector<float> chord = generateChord();
    writeWavFile("output/chord.wav", chord, SAMPLE_RATE, NUM_CHANNELS);
    
    std::cout << "All tests completed!" << std::endl;
    std::cout << "WAV files have been written to the 'output' directory." << std::endl;
    
    return 0;
}