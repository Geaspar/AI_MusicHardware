#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <optional>
#include <cmath> // For fabs
#include <unordered_set>

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
    float chance;       // Per-note trigger probability (0.0-1.0)
    
    Note(int p = 60, float v = 1.0f, double start = 0.0, double dur = 1.0, int ch = 0,
         float attack = 0.01f, float decay = 0.1f, float sustain = 0.7f, float release = 0.5f,
         float chanceIn = 1.0f)
        : pitch(p), velocity(v), startTime(start), duration(dur), channel(ch), 
          env(attack, decay, sustain, release), chance(chanceIn) {}
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
    // Convenience APIs for pattern workflows
    // Create a new pattern with a name and a length expressed in 16th-note steps.
    // Returns the index of the created pattern.
    size_t createPattern(const std::string& name, int steps16th);
    // Duplicate an existing pattern and append it to the pool. Returns new index.
    size_t duplicatePattern(size_t index);
    // Clear all notes in a pattern and reset to default bar length.
    void clearPattern(size_t index);
    // Rename a pattern by index.
    void renamePattern(size_t index, const std::string& newName);
    // Lookup pattern index by name.
    std::optional<size_t> getPatternIndexByName(const std::string& name) const;
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

    // Thread-safe helpers for UI/editor interactions
    // These methods take the internal pattern mutex to avoid races with the
    // audio thread while editing pattern data from the UI thread.
    void setColumnVelocity(size_t patternIndex, int column16th, float velocity);
    void addNoteToPattern(size_t patternIndex, const Note& note);
    void removeNotesAt(size_t patternIndex, int pitch, double startBeat, double epsilon = 1e-6);
    // Per-column probability (chance) setter: sets Note::chance for notes at a 16th column
    void setColumnChance(size_t patternIndex, int column16th, float chance);
    void setAllNotesChance(size_t patternIndex, float chance);
    // Step helpers for grid-style editing
    void setStep(size_t patternIndex, int step16th, int pitch, float velocity, float gate);
    void toggleStep(size_t patternIndex, int step16th, int pitch);

    // Synchronize with audio engine - allows accurate timing coordination
    void synchronizeWithAudioEngine(double audioEngineTimeInSeconds, double engineSampleRate);

    // Get precise timing information
    double getPrecisePositionInBeats() const;
    double getPreciseBeatTime() const;  // Returns time in seconds per beat

    // Resequencing MVP API (Phase A)
    enum class Timing { Immediate, OnBeat, OnBar };
    enum class ProbabilityMode { PerHitRandom, PerLoopStable };
    void defineSections(const std::vector<std::pair<std::string,double>>& sectionsInBeats);
    void jumpToSection(const std::string& name, Timing when = Timing::OnBar);
    void queueSection(const std::string& name, Timing when = Timing::OnBar); // alias to jump
    void nextSection(Timing when = Timing::OnBar);
    void prevSection(Timing when = Timing::OnBar);
    std::vector<std::string> getSectionNames() const;
    std::string getCurrentSectionName() const;
    int getBeatsPerBar() const { return beatsPerBar_; }
    std::vector<std::pair<std::string,double>> getSectionDefinitions() const { return sections_; }

    // Runtime controls
    void setPerLoopDedupeEnabled(bool enabled) { perLoopDedupeEnabled_.store(enabled, std::memory_order_relaxed); }
    void setProbabilityMode(ProbabilityMode m) { probabilityMode_ = m; }
    ProbabilityMode getProbabilityMode() const { return probabilityMode_; }
    void setWrapLongNotesAcrossLoop(bool enabled) { wrapLongNotesAcrossLoop_.store(enabled, std::memory_order_relaxed); }
    bool getWrapLongNotesAcrossLoop() const { return wrapLongNotesAcrossLoop_.load(std::memory_order_relaxed); }
    void seedRandom(uint32_t seed) { if (seed == 0) seed = 0x9E3779B9u; rngState_ = seed; }
    
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
    mutable std::mutex timingMutex_;    // Protect timing calculations

    // Callbacks
    NoteOnCallback noteOnCallback_;
    NoteOffCallback noteOffCallback_;
    TransportCallback transportCallback_;

    mutable std::mutex patternMutex_;
    mutable std::mutex arrangementMutex_;
    std::atomic<uint64_t> patternVersion_{0};

    struct ActiveNote {
        int pitch;
        int channel;
        double endTime;
        float velocity;
        Envelope env;
    };
    std::vector<ActiveNote> activeNotes_;
    mutable std::mutex activeNotesMutex_; // Protect active notes

    // Audio engine synchronization
    double audioEngineSampleRate_;
    double audioEngineTimeOffset_;
    double lastSyncTimeSeconds_;
    double beatTimeSeconds_;       // Time in seconds for one beat at current tempo
    mutable std::mutex syncMutex_; // Protect synchronization state

    // Resequencing MVP state
    std::vector<std::pair<std::string,double>> sections_; // name -> startBeat
    int currentSectionIndex_ = 0;
    struct PendingJump {
        bool active = false;
        double targetBeat = 0.0;
        Timing timing = Timing::OnBar;
    } pendingJump_;

    void scheduleJumpToBeat(double beat, Timing when);
    void servicePendingJump(double previousPosition, double currentPosition);

    // Timing correction state (moved from static in process())
    double timingAccumulatedError_ = 0.0;
    bool loopedThisCall_ = false; // set true when a loop occurs during processing

    // Per-loop deduplication of note-ons to avoid double triggers at boundaries
    std::unordered_set<uint64_t> firedThisLoop_;
    // Default OFF to avoid suppressing legitimate retriggers in edge cases.
    std::atomic<bool> perLoopDedupeEnabled_{false};
    static inline uint64_t makeDedupeKey(int col16, int pitch, int channel) {
        return (static_cast<uint64_t>(col16 & 0xFFFF) << 16) |
               (static_cast<uint64_t>(pitch & 0xFF) << 8) |
               (static_cast<uint64_t>(channel & 0xFF));
    }

    // Probability and RNG
    ProbabilityMode probabilityMode_ = ProbabilityMode::PerHitRandom;
    uint32_t rngState_ = 0x12345678u;
    inline float rngNextFloat01() {
        // xorshift32
        uint32_t x = rngState_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        rngState_ = (x == 0 ? 0x9E3779B9u : x);
        return (rngState_ & 0x00FFFFFFu) / 16777216.0f; // 24-bit mantissa
    }
    static inline uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        return x ^ (x >> 31);
    }
    inline float stableProb01(uint64_t dedupeKey) const {
        // Combine loop count with key for stability per loop
        uint64_t h = splitmix64((loopCount_ << 32) ^ dedupeKey);
        // Map to [0,1)
        return (float)((h >> 11) & 0x1fffff) / (float)0x200000; // 21 bits
    }

    std::atomic<bool> wrapLongNotesAcrossLoop_{false};
    uint64_t loopCount_ = 0; // increments on loop

    // Timing epsilon helper (beats)
    double timingEpsilonBeats() const {
        std::lock_guard<std::mutex> lock(syncMutex_);
        double bt = beatTimeSeconds_ > 0.0 ? beatTimeSeconds_ : (60.0 / std::max(1.0, tempo_.load()));
        double epsBeats = 0.00025 / std::max(1e-9, bt); // ~0.25 ms in beats
        if (epsBeats < 1e-6) epsBeats = 1e-6; // hard floor
        return epsBeats;
    }
};

} // namespace AIMusicHardware
