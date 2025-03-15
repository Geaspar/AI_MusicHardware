#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <optional>
#include <cmath> // For fabs

namespace AIMusicHardware {

struct Envelope {
    float attack;       // Attack time in seconds
    float decay;        // Decay time in seconds
    float sustain;      // Sustain level (0.0-1.0)
    float release;      // Release time in seconds
    
    Envelope(float a = 0.01f, float d = 0.1f, float s = 0.7f, float r = 0.5f)
        : attack(a), decay(d), sustain(s), release(r) {}
};

struct Note {
    int pitch;          // MIDI note number (0-127)
    float velocity;     // Note velocity (0.0-1.0)
    double startTime;   // Start time in beats
    double duration;    // Duration in beats
    int channel;        // MIDI channel (0-15)
    Envelope env;       // ADSR envelope parameters
    
    Note(int p = 60, float v = 1.0f, double start = 0.0, double dur = 1.0, int ch = 0,
         float attack = 0.01f, float decay = 0.1f, float sustain = 0.7f, float release = 0.5f)
        : pitch(p), velocity(v), startTime(start), duration(dur), channel(ch), 
          env(attack, decay, sustain, release) {}
};

class Pattern {
public:
    Pattern(const std::string& name = "");
    ~Pattern();
    
    void addNote(const Note& note);
    void removeNote(size_t index);
    void clear();
    
    Note* getNote(size_t index);
    const Note* getNote(size_t index) const;
    size_t getNumNotes() const;
    
    void setName(const std::string& name);
    std::string getName() const;
    
    void setLength(double lengthInBeats);
    double getLength() const;
    
    // Quantize notes to a grid
    void quantize(double gridSize);
    
    // Apply swing/groove
    void applySwing(double swingAmount, double gridSize = 0.25);
    
private:
    std::string name_;
    double length_;
    std::vector<Note> notes_;
};

// Used for song arrangement
struct PatternInstance {
    size_t patternIndex;     // Index of the pattern in the patterns_ vector
    double startBeat;        // Start position in the song (in beats)
    double endBeat;          // End position in the song (in beats)
    
    PatternInstance(size_t index = 0, double start = 0.0)
        : patternIndex(index), startBeat(start), endBeat(0.0) {}
};

enum class PlaybackMode {
    SinglePattern,   // Play a single pattern (traditional mode)
    Song             // Play a sequence of patterns (song arrangement)
};

class Sequencer {
public:
    using NoteOnCallback = std::function<void(int pitch, float velocity, int channel, const Envelope& env)>;
    using NoteOffCallback = std::function<void(int pitch, int channel)>;
    using TransportCallback = std::function<void(double positionInBeats, int bar, int beat)>;
    
    Sequencer(double tempo = 120.0, int beatsPerBar = 4);
    ~Sequencer();
    
    bool initialize();
    void start();
    void stop();
    void reset();
    bool isPlaying() const;
    
    void setTempo(double bpm);
    double getTempo() const;
    
    // Pattern management
    void addPattern(std::unique_ptr<Pattern> pattern);
    Pattern* getPattern(size_t index);
    const Pattern* getPattern(size_t index) const;
    size_t getNumPatterns() const;
    
    void setCurrentPattern(size_t index);
    size_t getCurrentPatternIndex() const;
    
    // Song arrangement
    void setPlaybackMode(PlaybackMode mode);
    PlaybackMode getPlaybackMode() const;
    
    void addPatternToSong(size_t patternIndex, double startBeat);
    void removePatternFromSong(size_t arrangementIndex);
    void clearSong();
    
    size_t getNumPatternInstances() const;
    
    // Return optional instead of raw pointers for safety
    std::optional<PatternInstance> getPatternInstance(size_t index);
    std::optional<PatternInstance> getPatternInstance(size_t index) const;
    
    double getSongLength() const;
    
    // Transport controls
    void setLooping(bool loop);
    bool isLooping() const;
    
    void setPositionInBeats(double positionInBeats);
    double getPositionInBeats() const;
    
    int getCurrentBar() const;
    int getCurrentBeat() const;
    
    // Callbacks
    void setNoteCallbacks(NoteOnCallback noteOn, NoteOffCallback noteOff);
    void setTransportCallback(TransportCallback callback);
    
    // Call this at regular intervals from the audio thread
    void process(double sampleTime);
    
private:
    // Pattern processing method for audio thread
    void processSinglePattern(double deltaBeats);
    void processSongArrangement(double deltaBeats);
    
    // Find the pattern instances that should be playing at the current position
    std::vector<PatternInstance*> getActivePatternInstances();
    
    // Update song length based on pattern instances
    void updateSongLength();
    
    std::atomic<double> tempo_;  // Make tempo atomic for lock-free access
    int beatsPerBar_;
    std::vector<std::unique_ptr<Pattern>> patterns_;
    
    // Song arrangement
    PlaybackMode playbackMode_;
    std::vector<PatternInstance> songArrangement_;
    double songLength_;
    
    std::atomic<bool> isPlaying_;
    std::atomic<bool> looping_;
    std::atomic<size_t> currentPatternIndex_;  // Make thread-safe
    double positionInBeats_;
    mutable std::mutex positionMutex_;  // Protect position access
    
    // Callbacks
    NoteOnCallback noteOnCallback_;
    NoteOffCallback noteOffCallback_;
    TransportCallback transportCallback_;
    
    mutable std::mutex patternMutex_;
    mutable std::mutex arrangementMutex_;
    
    struct ActiveNote {
        int pitch;
        int channel;
        double endTime;
    };
    std::vector<ActiveNote> activeNotes_;
    mutable std::mutex activeNotesMutex_; // Protect active notes
};

} // namespace AIMusicHardware