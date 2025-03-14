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
        
        // Get parent sequencer time signature if available, otherwise default to 4/4
        // We'll use a default minimum length of one bar, but recalculate based on notes
        double defaultBarLength = 4.0; // Default to 4 beats (1 bar at 4/4)
        
        if (notes_.empty()) {
            length_ = defaultBarLength;
            return;
        }
        
        // Start with a small value to find actual length from notes
        length_ = 0.0;
        for (const auto& note : notes_) {
            double noteEnd = note.startTime + note.duration;
            if (noteEnd > length_) {
                length_ = noteEnd;
            }
        }
        
        // If length is smaller than one bar, use a bar as minimum
        if (length_ < defaultBarLength) {
            length_ = defaultBarLength;
        }
    }
}

void Pattern::clear() {
    notes_.clear();
    
    // Default to one bar at 4/4, but this could be configurable based on time signature
    double defaultBarLength = 4.0; // Default to 4 beats (1 bar at 4/4)
    length_ = defaultBarLength;
    
    // Release memory
    notes_.shrink_to_fit();
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
            
            // Adjust duration to maintain note end, but ensure it never goes below minimum
            note.duration = std::max(gridSize * 0.5, note.duration - swingOffset);
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
    : tempo_(tempo),  // Initialize atomic<double>
      beatsPerBar_(beatsPerBar),
      playbackMode_(PlaybackMode::SinglePattern),
      songLength_(0.0),
      isPlaying_(false),
      looping_(true),
      currentPatternIndex_(0),  // Now atomic
      positionInBeats_(0.0) {
}

Sequencer::~Sequencer() {
    stop(); // Ensure all notes are turned off
}

void Sequencer::start() {
    {
        // Protect position update with mutex
        std::lock_guard<std::mutex> posLock(positionMutex_);
        positionInBeats_ = 0.0;
    }
    
    {
        // Protect activeNotes_ with its dedicated mutex
        std::lock_guard<std::mutex> notesLock(activeNotesMutex_);
        activeNotes_.clear();
    }
    
    // Atomic state change - no lock needed
    isPlaying_.store(true, std::memory_order_release);
}

void Sequencer::stop() {
    // Use std::exchange to minimize lock contention
    std::vector<ActiveNote> notesToStop;
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        // Move active notes to local vector to minimize time spent in lock
        notesToStop = std::move(activeNotes_);
        activeNotes_ = std::vector<ActiveNote>(); // Create a fresh empty vector
    }
    
    // Stop all active notes after releasing the lock
    if (noteOffCallback_) {
        for (const auto& note : notesToStop) {
            noteOffCallback_(note.pitch, note.channel);
        }
    }
    
    isPlaying_ = false;
}

void Sequencer::reset() {
    // Stop first to handle active notes
    stop();
    
    // Then reset position with proper locking
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        positionInBeats_ = 0.0;
    }
}

bool Sequencer::isPlaying() const {
    // Use explicit memory ordering for consistent reads
    return isPlaying_.load(std::memory_order_acquire);
}

void Sequencer::setTempo(double bpm) {
    // Use atomic store with explicit memory ordering - no locks needed
    tempo_.store(bpm, std::memory_order_release);
}

double Sequencer::getTempo() const {
    // Use atomic load with explicit memory ordering - no locks needed
    return tempo_.load(std::memory_order_acquire);
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
    // Check bounds under mutex
    {
        std::lock_guard<std::mutex> lock(patternMutex_);
        if (index >= patterns_.size()) {
            return;
        }
    }
    
    // Use atomic directly - no need for mutex when setting the value
    currentPatternIndex_.store(index, std::memory_order_release);
}

size_t Sequencer::getCurrentPatternIndex() const {
    // Use explicit memory ordering for better performance
    return currentPatternIndex_.load(std::memory_order_acquire);
}

void Sequencer::setPlaybackMode(PlaybackMode mode) {
    playbackMode_ = mode;
}

PlaybackMode Sequencer::getPlaybackMode() const {
    return playbackMode_;
}

void Sequencer::addPatternToSong(size_t patternIndex, double startBeat) {
    // Always lock in consistent order to prevent deadlocks:
    // patternMutex_ first, then arrangementMutex_
    std::lock_guard<std::mutex> patternLock(patternMutex_);
    std::lock_guard<std::mutex> arrangementLock(arrangementMutex_);
    
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
    songArrangement_.shrink_to_fit(); // Release memory
    songLength_ = 0.0;
}

size_t Sequencer::getNumPatternInstances() const {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    return songArrangement_.size();
}

std::optional<PatternInstance> Sequencer::getPatternInstance(size_t index) {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    if (index < songArrangement_.size()) {
        return songArrangement_[index]; // Return by value
    }
    return std::nullopt;
}

std::optional<PatternInstance> Sequencer::getPatternInstance(size_t index) const {
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    if (index < songArrangement_.size()) {
        return songArrangement_[index]; // Return by value
    }
    return std::nullopt;
}

double Sequencer::getSongLength() const {
    return songLength_;
}

void Sequencer::setLooping(bool loop) {
    // Use atomic store with explicit memory ordering
    looping_.store(loop, std::memory_order_release);
}

bool Sequencer::isLooping() const {
    // Use atomic load with explicit memory ordering
    return looping_.load(std::memory_order_acquire);
}

void Sequencer::setPositionInBeats(double positionInBeats) {
    // Stop all active notes when changing position using the more efficient approach
    std::vector<ActiveNote> notesToStop;
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        notesToStop = std::move(activeNotes_);
        activeNotes_ = std::vector<ActiveNote>();
    }
    
    // Stop notes after releasing the lock
    if (noteOffCallback_) {
        for (const auto& note : notesToStop) {
            noteOffCallback_(note.pitch, note.channel);
        }
    }
    
    // Update position and capture values for callback
    double safePosition;
    int bar, beat;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        positionInBeats_ = positionInBeats;
        safePosition = positionInBeats;  // Make a copy for the callback
        
        // Calculate bar and beat while still under lock
        bar = static_cast<int>(std::floor(positionInBeats_ / beatsPerBar_)) + 1;
        double beatInBar = fmod(positionInBeats_, static_cast<double>(beatsPerBar_));
        beat = static_cast<int>(std::floor(beatInBar)) + 1;
    }
    
    // Notify transport callback with the safe copies (outside the lock)
    if (transportCallback_) {
        transportCallback_(safePosition, bar, beat);
    }
}

double Sequencer::getPositionInBeats() const {
    std::lock_guard<std::mutex> lock(positionMutex_);
    return positionInBeats_;
}

int Sequencer::getCurrentBar() const {
    std::lock_guard<std::mutex> lock(positionMutex_);
    // More accurate bar calculation
    return static_cast<int>(std::floor(positionInBeats_ / beatsPerBar_)) + 1;
}

int Sequencer::getCurrentBeat() const {
    std::lock_guard<std::mutex> lock(positionMutex_);
    // More accurate beat calculation with consistent rounding
    double beatInBar = fmod(positionInBeats_, static_cast<double>(beatsPerBar_));
    
    // Handle the case of exactly at bar boundary
    if (fabs(beatInBar) < 1e-6) {
        return 1; // First beat of bar
    }
    
    if (fabs(beatInBar - beatsPerBar_) < 1e-6) {
        return beatsPerBar_; // Last beat of bar
    }
    
    return static_cast<int>(std::floor(beatInBar)) + 1;
}

void Sequencer::setNoteCallbacks(NoteOnCallback noteOn, NoteOffCallback noteOff) {
    noteOnCallback_ = noteOn;
    noteOffCallback_ = noteOff;
}

void Sequencer::setTransportCallback(TransportCallback callback) {
    transportCallback_ = callback;
}

void Sequencer::process(double deltaTime) {
    // Fast path exit with atomic check - no lock needed
    if (!isPlaying_.load(std::memory_order_acquire)) {
        return;
    }
    
    // Convert delta time to beats - no locks needed for tempo_
    double beatsPerSecond = tempo_ / 60.0;
    double deltaBeats = deltaTime * beatsPerSecond;
    
    // Process based on playback mode
    PlaybackMode mode = playbackMode_; // Make a local copy to avoid repeated access
    if (mode == PlaybackMode::SinglePattern) {
        processSinglePattern(deltaBeats);
    } else {
        processSongArrangement(deltaBeats);
    }
    
    // Only capture values needed for callback, minimize lock duration
    if (transportCallback_) {
        double safePosition;
        int bar, beat;
        
        // Minimize critical section - only capture necessary values
        {
            std::lock_guard<std::mutex> lock(positionMutex_);
            safePosition = positionInBeats_;
            
            // Compute derived values inside lock to ensure consistency
            bar = static_cast<int>(std::floor(safePosition / beatsPerBar_)) + 1;
            double beatInBar = fmod(safePosition, static_cast<double>(beatsPerBar_));
            beat = static_cast<int>(std::floor(beatInBar)) + 1;
        } // Lock released as soon as possible
        
        // Call callback outside the lock
        transportCallback_(safePosition, bar, beat);
    }
}

void Sequencer::processSinglePattern(double deltaBeats) {
    // Store previous and current positions
    double previousPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        previousPosition = positionInBeats_;
        positionInBeats_ += deltaBeats;
    }
    
    // Define a small epsilon for floating-point comparisons
    constexpr double EPSILON = 1e-6;
    
    // Check if we need to loop
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patterns_.empty()) {
        return;
    }
    
    // Use atomic for thread safety
    size_t patternIndex = currentPatternIndex_.load();
    if (patternIndex >= patterns_.size()) {
        return;
    }
    
    Pattern* currentPattern = patterns_[patternIndex].get();
    double patternLength = currentPattern->getLength();
    
    if (positionInBeats_ >= patternLength - EPSILON) {
        if (looping_) {
            // Loop back to beginning
            std::lock_guard<std::mutex> posLock(positionMutex_);
            // Use careful modulo to avoid precision errors
            if (fabs(positionInBeats_ - patternLength) < EPSILON) {
                positionInBeats_ = 0.0;  // Exactly at the end, reset to beginning
            } else {
                positionInBeats_ = fmod(positionInBeats_, patternLength);
            }
            previousPosition = 0.0;
        } else {
            // Stop at the end
            std::lock_guard<std::mutex> posLock(positionMutex_);
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
            
            {
                std::lock_guard<std::mutex> lock(activeNotesMutex_);
                activeNotes_.push_back(activeNote);
            }
        }
    }
    
    // Check for notes to stop in this time slice
    double currentPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        currentPosition = positionInBeats_;
    }
    
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end()) {
            if (it->endTime <= currentPosition) {
                if (noteOffCallback_) {
                    noteOffCallback_(it->pitch, it->channel);
                }
                it = activeNotes_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Sequencer::processSongArrangement(double deltaBeats) {
    // Store previous and current positions
    double previousPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        previousPosition = positionInBeats_;
        positionInBeats_ += deltaBeats;
    }
    
    // Define a small epsilon for floating-point comparisons
    constexpr double EPSILON = 1e-6;
    
    // Check if we need to loop
    if (positionInBeats_ >= songLength_ - EPSILON) {
        if (looping_ && songLength_ > EPSILON) {
            // Loop back to beginning
            std::lock_guard<std::mutex> lock(positionMutex_);
            // Use careful modulo to avoid precision errors
            if (fabs(positionInBeats_ - songLength_) < EPSILON) {
                positionInBeats_ = 0.0;  // Exactly at the end, reset to beginning
            } else {
                positionInBeats_ = fmod(positionInBeats_, songLength_);
            }
            previousPosition = 0.0;
        } else if (songLength_ > EPSILON) {
            // Stop at the end
            std::lock_guard<std::mutex> lock(positionMutex_);
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
                
                {
                    std::lock_guard<std::mutex> lock(activeNotesMutex_);
                    activeNotes_.push_back(activeNote);
                }
            }
        }
    }
    
    // Check for notes to stop in this time slice
    double currentPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        currentPosition = positionInBeats_;
    }
    
    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end()) {
            if (it->endTime <= currentPosition) {
                if (noteOffCallback_) {
                    noteOffCallback_(it->pitch, it->channel);
                }
                it = activeNotes_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::vector<PatternInstance*> Sequencer::getActivePatternInstances() {
    double currentPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        currentPosition = positionInBeats_;
    }
    
    std::lock_guard<std::mutex> lock(arrangementMutex_);
    std::vector<PatternInstance*> active;
    
    for (auto& instance : songArrangement_) {
        // Check if this pattern instance is active at current position
        if (currentPosition >= instance.startBeat && currentPosition < instance.endBeat) {
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