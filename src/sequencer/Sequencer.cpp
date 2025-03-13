#include "../../include/sequencer/Sequencer.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace AIMusicHardware {

//-------------------------------------------------------------------------
// Pattern implementation
//-------------------------------------------------------------------------

Pattern::Pattern(const std::string& name)
    : name_(name), length_(4.0) {
}

Pattern::~Pattern() {
}

void Pattern::addNote(const Note& note) {
    notes_.push_back(note);
    
    // Update pattern length if needed
    double noteEnd = note.startTime + note.duration;
    if (noteEnd > length_) {
        length_ = noteEnd;
    }
}

void Pattern::removeNote(size_t index) {
    if (index < notes_.size()) {
        notes_.erase(notes_.begin() + index);
        
        // Recalculate length
        length_ = 4.0; // Default length (1 bar at 4/4)
        for (const auto& note : notes_) {
            double noteEnd = note.startTime + note.duration;
            if (noteEnd > length_) {
                length_ = noteEnd;
            }
        }
    }
}

void Pattern::clear() {
    notes_.clear();
    length_ = 4.0; // Reset to default length (1 bar at 4/4)
}

Note* Pattern::getNote(size_t index) {
    if (index < notes_.size()) {
        return &notes_[index];
    }
    return nullptr;
}

const Note* Pattern::getNote(size_t index) const {
    if (index < notes_.size()) {
        return &notes_[index];
    }
    return nullptr;
}

size_t Pattern::getNumNotes() const {
    return notes_.size();
}

void Pattern::setName(const std::string& name) {
    name_ = name;
}

std::string Pattern::getName() const {
    return name_;
}

void Pattern::setLength(double lengthInBeats) {
    length_ = lengthInBeats;
}

double Pattern::getLength() const {
    return length_;
}

void Pattern::quantize(double gridSize) {
    if (gridSize <= 0.0) {
        return;
    }
    
    for (auto& note : notes_) {
        // Quantize start time
        double quantizedStart = std::round(note.startTime / gridSize) * gridSize;
        
        // Adjust duration to maintain note end point
        double originalEnd = note.startTime + note.duration;
        note.startTime = quantizedStart;
        
        // Quantize end time
        double quantizedEnd = std::round(originalEnd / gridSize) * gridSize;
        note.duration = quantizedEnd - note.startTime;
        
        // Ensure minimum duration
        if (note.duration < gridSize / 2.0) {
            note.duration = gridSize;
        }
    }
    
    // Recalculate pattern length
    length_ = 4.0; // Default length
    for (const auto& note : notes_) {
        double noteEnd = note.startTime + note.duration;
        if (noteEnd > length_) {
            length_ = noteEnd;
        }
    }
}

void Pattern::applySwing(double swingAmount, double gridSize) {
    if (gridSize <= 0.0 || swingAmount <= 0.0) {
        return;
    }
    
    // Swing affects every other note in the grid
    for (auto& note : notes_) {
        // Calculate grid position
        double gridPosition = std::floor(note.startTime / gridSize);
        
        // If this is an odd-numbered grid position (off-beat)
        if (static_cast<int>(gridPosition) % 2 == 1) {
            // Apply swing
            double swingOffset = gridSize * swingAmount;
            note.startTime += swingOffset;
            
            // Adjust duration to maintain note end
            note.duration -= swingOffset;
            if (note.duration < 0.01) {
                note.duration = 0.01; // Minimum duration
            }
        }
    }
    
    // Recalculate pattern length
    length_ = 4.0; // Default length
    for (const auto& note : notes_) {
        double noteEnd = note.startTime + note.duration;
        if (noteEnd > length_) {
            length_ = noteEnd;
        }
    }
}

//-------------------------------------------------------------------------
// Sequencer implementation
//-------------------------------------------------------------------------

Sequencer::Sequencer(double tempo, int beatsPerBar)
    : tempo_(tempo),
      beatsPerBar_(beatsPerBar),
      currentPatternIndex_(0),
      playbackMode_(PlaybackMode::SinglePattern),
      songLength_(0.0),
      isPlaying_(false),
      looping_(true),
      positionInBeats_(0.0) {
}

Sequencer::~Sequencer() {
    stop(); // Ensure all notes are turned off
}

void Sequencer::start() {
    // Reset position
    positionInBeats_ = 0.0;
    activeNotes_.clear();
    isPlaying_ = true;
}

void Sequencer::stop() {
    // Stop all active notes
    if (noteOffCallback_) {
        for (const auto& note : activeNotes_) {
            noteOffCallback_(note.pitch, note.channel);
        }
    }
    
    activeNotes_.clear();
    isPlaying_ = false;
}

void Sequencer::reset() {
    stop();
    positionInBeats_ = 0.0;
}

bool Sequencer::isPlaying() const {
    return isPlaying_;
}

void Sequencer::setTempo(double bpm) {
    tempo_ = bpm;
}

double Sequencer::getTempo() const {
    return tempo_;
}

void Sequencer::addPattern(std::unique_ptr<Pattern> pattern) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    patterns_.push_back(std::move(pattern));
}

Pattern* Sequencer::getPattern(size_t index) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < patterns_.size()) {
        return patterns_[index].get();
    }
    return nullptr;
}

const Pattern* Sequencer::getPattern(size_t index) const {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < patterns_.size()) {
        return patterns_[index].get();
    }
    return nullptr;
}

size_t Sequencer::getNumPatterns() const {
    std::lock_guard<std::mutex> lock(patternMutex_);
    return patterns_.size();
}

void Sequencer::setCurrentPattern(size_t index) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < patterns_.size()) {
        currentPatternIndex_ = index;
    }
}

size_t Sequencer::getCurrentPatternIndex() const {
    return currentPatternIndex_;
}

void Sequencer::setPlaybackMode(PlaybackMode mode) {
    playbackMode_ = mode;
}

PlaybackMode Sequencer::getPlaybackMode() const {
    return playbackMode_;
}

void Sequencer::addPatternToSong(size_t patternIndex, double startBeat) {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    std::lock_guard<std::mutex> patternLock(patternMutex_);
    
    if (patternIndex >= patterns_.size()) {
        return;
    }
    
    PatternInstance instance(patternIndex, startBeat);
    instance.endBeat = startBeat + patterns_[patternIndex]->getLength();
    songArrangement_.push_back(instance);
    
    // Sort by start time
    std::sort(songArrangement_.begin(), songArrangement_.end(), 
        [](const PatternInstance& a, const PatternInstance& b) {
            return a.startBeat < b.startBeat;
        });
    
    // Update song length
    updateSongLength();
}

void Sequencer::removePatternFromSong(size_t arrangementIndex) {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    
    if (arrangementIndex < songArrangement_.size()) {
        songArrangement_.erase(songArrangement_.begin() + arrangementIndex);
        
        // Update song length
        updateSongLength();
    }
}

void Sequencer::clearSong() {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    songArrangement_.clear();
    songLength_ = 0.0;
}

size_t Sequencer::getNumPatternInstances() const {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    return songArrangement_.size();
}

PatternInstance* Sequencer::getPatternInstance(size_t index) {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    if (index < songArrangement_.size()) {
        return &songArrangement_[index];
    }
    return nullptr;
}

const PatternInstance* Sequencer::getPatternInstance(size_t index) const {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    if (index < songArrangement_.size()) {
        return &songArrangement_[index];
    }
    return nullptr;
}

double Sequencer::getSongLength() const {
    return songLength_;
}

void Sequencer::setLooping(bool loop) {
    looping_ = loop;
}

bool Sequencer::isLooping() const {
    return looping_;
}

void Sequencer::setPositionInBeats(double positionInBeats) {
    // Stop all active notes when changing position
    if (noteOffCallback_) {
        for (const auto& note : activeNotes_) {
            noteOffCallback_(note.pitch, note.channel);
        }
    }
    
    activeNotes_.clear();
    positionInBeats_ = positionInBeats;
    
    // Notify transport callback
    if (transportCallback_) {
        transportCallback_(positionInBeats_, getCurrentBar(), getCurrentBeat());
    }
}

double Sequencer::getPositionInBeats() const {
    return positionInBeats_;
}

int Sequencer::getCurrentBar() const {
    return static_cast<int>(positionInBeats_ / beatsPerBar_) + 1;
}

int Sequencer::getCurrentBeat() const {
    double beatInBar = fmod(positionInBeats_, static_cast<double>(beatsPerBar_));
    return static_cast<int>(beatInBar) + 1;
}

void Sequencer::setNoteCallbacks(NoteOnCallback noteOn, NoteOffCallback noteOff) {
    noteOnCallback_ = noteOn;
    noteOffCallback_ = noteOff;
}

void Sequencer::setTransportCallback(TransportCallback callback) {
    transportCallback_ = callback;
}

void Sequencer::process(double deltaTime) {
    if (!isPlaying_) {
        return;
    }
    
    // Convert delta time to beats
    double beatsPerSecond = tempo_ / 60.0;
    double deltaBeats = deltaTime * beatsPerSecond;
    
    // Process based on playback mode
    if (playbackMode_ == PlaybackMode::SinglePattern) {
        processSinglePattern(deltaBeats);
    } else {
        processSongArrangement(deltaBeats);
    }
    
    // Notify transport callback
    if (transportCallback_) {
        transportCallback_(positionInBeats_, getCurrentBar(), getCurrentBeat());
    }
}

void Sequencer::processSinglePattern(double deltaBeats) {
    // Store previous and current positions
    double previousPosition = positionInBeats_;
    positionInBeats_ += deltaBeats;
    
    // Check if we need to loop
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patterns_.empty()) {
        return;
    }
    
    Pattern* currentPattern = patterns_[currentPatternIndex_].get();
    double patternLength = currentPattern->getLength();
    
    if (positionInBeats_ >= patternLength) {
        if (looping_) {
            // Loop back to beginning
            positionInBeats_ = fmod(positionInBeats_, patternLength);
            previousPosition = 0.0;
        } else {
            // Stop at the end
            positionInBeats_ = patternLength;
            isPlaying_ = false;
            return;
        }
    }
    
    // Check for notes to start in this time slice
    for (size_t i = 0; i < currentPattern->getNumNotes(); ++i) {
        Note* note = currentPattern->getNote(i);
        if (!note) continue;
        
        double noteStartTime = note->startTime;
        double noteEndTime = noteStartTime + note->duration;
        
        // Note starting in this time slice
        if (noteStartTime >= previousPosition && noteStartTime < positionInBeats_) {
            if (noteOnCallback_) {
                noteOnCallback_(note->pitch, note->velocity, note->channel, note->env);
            }
            
            // Add to active notes
            ActiveNote activeNote;
            activeNote.pitch = note->pitch;
            activeNote.channel = note->channel;
            activeNote.endTime = noteEndTime;
            activeNotes_.push_back(activeNote);
        }
    }
    
    // Check for notes to stop in this time slice
    auto it = activeNotes_.begin();
    while (it != activeNotes_.end()) {
        if (it->endTime <= positionInBeats_) {
            if (noteOffCallback_) {
                noteOffCallback_(it->pitch, it->channel);
            }
            it = activeNotes_.erase(it);
        } else {
            ++it;
        }
    }
}

void Sequencer::processSongArrangement(double deltaBeats) {
    // Store previous and current positions
    double previousPosition = positionInBeats_;
    positionInBeats_ += deltaBeats;
    
    // Check if we need to loop
    if (positionInBeats_ >= songLength_) {
        if (looping_ && songLength_ > 0.0) {
            // Loop back to beginning
            positionInBeats_ = fmod(positionInBeats_, songLength_);
            previousPosition = 0.0;
        } else if (songLength_ > 0.0) {
            // Stop at the end
            positionInBeats_ = songLength_;
            isPlaying_ = false;
            return;
        }
    }
    
    // Find active pattern instances for this time slice
    std::vector<PatternInstance*> activeInstances = getActivePatternInstances();
    
    // Process each active pattern instance
    for (auto* instance : activeInstances) {
        // Get the pattern
        std::lock_guard<std::mutex> lock(patternMutex_);
        if (instance->patternIndex >= patterns_.size()) {
            continue;
        }
        
        Pattern* pattern = patterns_[instance->patternIndex].get();
        
        // Calculate local pattern position (relative to pattern start)
        double patternStart = instance->startBeat;
        double localPreviousPos = previousPosition - patternStart;
        double localCurrentPos = positionInBeats_ - patternStart;
        
        // Skip if we're before the pattern start or after pattern end
        if (localCurrentPos < 0.0 || localPreviousPos >= pattern->getLength()) {
            continue;
        }
        
        // Clamp positions to pattern boundaries
        if (localPreviousPos < 0.0) {
            localPreviousPos = 0.0;
        }
        if (localCurrentPos > pattern->getLength()) {
            localCurrentPos = pattern->getLength();
        }
        
        // Check for notes to start in this time slice
        for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
            Note* note = pattern->getNote(i);
            if (!note) continue;
            
            double noteStartTime = note->startTime;
            double noteEndTime = noteStartTime + note->duration;
            
            // Note starting in this time slice
            if (noteStartTime >= localPreviousPos && noteStartTime < localCurrentPos) {
                if (noteOnCallback_) {
                    noteOnCallback_(note->pitch, note->velocity, note->channel, note->env);
                }
                
                // Add to active notes (with global time)
                ActiveNote activeNote;
                activeNote.pitch = note->pitch;
                activeNote.channel = note->channel;
                activeNote.endTime = patternStart + noteEndTime;
                activeNotes_.push_back(activeNote);
            }
        }
    }
    
    // Check for notes to stop in this time slice
    auto it = activeNotes_.begin();
    while (it != activeNotes_.end()) {
        if (it->endTime <= positionInBeats_) {
            if (noteOffCallback_) {
                noteOffCallback_(it->pitch, it->channel);
            }
            it = activeNotes_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<PatternInstance*> Sequencer::getActivePatternInstances() {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    std::vector<PatternInstance*> active;
    
    for (auto& instance : songArrangement_) {
        // Check if this pattern instance is active at current position
        if (positionInBeats_ >= instance.startBeat && positionInBeats_ < instance.endBeat) {
            active.push_back(&instance);
        }
    }
    
    return active;
}

// Private helpers
void Sequencer::updateSongLength() {
    songLength_ = 0.0;
    
    for (const auto& instance : songArrangement_) {
        if (instance.endBeat > songLength_) {
            songLength_ = instance.endBeat;
        }
    }
}

} // namespace AIMusicHardware