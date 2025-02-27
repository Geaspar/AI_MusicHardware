#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

namespace AIMusicHardware {

struct Note {
    int pitch;          // MIDI note number (0-127)
    float velocity;     // Note velocity (0.0-1.0)
    double startTime;   // Start time in beats
    double duration;    // Duration in beats
    int channel;        // MIDI channel (0-15)
    
    Note(int p = 60, float v = 1.0f, double start = 0.0, double dur = 1.0, int ch = 0)
        : pitch(p), velocity(v), startTime(start), duration(dur), channel(ch) {}
};

class Pattern {
public:
    Pattern(const std::string& name = "");
    ~Pattern();
    
    void addNote(const Note& note);
    void removeNote(size_t index);
    void clear();
    
    Note* getNote(size_t index);
    size_t getNumNotes() const;
    
    void setName(const std::string& name);
    std::string getName() const;
    
    void setLength(double lengthInBeats);
    double getLength() const;
    
private:
    std::string name_;
    double length_;
    std::vector<Note> notes_;
};

class Sequencer {
public:
    using NoteOnCallback = std::function<void(int pitch, float velocity, int channel)>;
    using NoteOffCallback = std::function<void(int pitch, int channel)>;
    
    Sequencer(double tempo = 120.0, int beatsPerBar = 4);
    ~Sequencer();
    
    void start();
    void stop();
    void reset();
    bool isPlaying() const;
    
    void setTempo(double bpm);
    double getTempo() const;
    
    void addPattern(std::unique_ptr<Pattern> pattern);
    Pattern* getPattern(size_t index);
    size_t getNumPatterns() const;
    
    void setCurrentPattern(size_t index);
    size_t getCurrentPatternIndex() const;
    
    void setLooping(bool loop);
    bool isLooping() const;
    
    // Transport position
    void setPositionInBeats(double positionInBeats);
    double getPositionInBeats() const;
    
    void setNoteCallbacks(NoteOnCallback noteOn, NoteOffCallback noteOff);
    
    // Call this at regular intervals from the audio thread
    void process(double sampleTime);
    
private:
    double tempo_;
    int beatsPerBar_;
    std::vector<std::unique_ptr<Pattern>> patterns_;
    size_t currentPatternIndex_;
    
    std::atomic<bool> isPlaying_;
    std::atomic<bool> looping_;
    std::atomic<double> positionInBeats_;
    
    NoteOnCallback noteOnCallback_;
    NoteOffCallback noteOffCallback_;
    
    std::mutex patternMutex_;
    
    struct ActiveNote {
        int pitch;
        int channel;
        double endTime;
    };
    std::vector<ActiveNote> activeNotes_;
};

} // namespace AIMusicHardware