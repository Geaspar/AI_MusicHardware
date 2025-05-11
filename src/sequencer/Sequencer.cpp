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
      positionInBeats_(0.0),
      audioEngineSampleRate_(44100.0),  // Default sample rate
      audioEngineTimeOffset_(0.0),
      lastSyncTimeSeconds_(0.0),
      beatTimeSeconds_(60.0 / tempo) {  // Initialize beat time based on tempo
}

Sequencer::~Sequencer() {
    stop(); // Ensure all notes are turned off
}

bool Sequencer::initialize() {
    try {
        // Clear any existing state
        {
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            activeNotes_.clear();
        }
        
        // Reset position
        {
            std::lock_guard<std::mutex> lock(positionMutex_);
            positionInBeats_ = 0.0;
        }
        
        // Create default empty pattern if needed
        {
            std::lock_guard<std::mutex> lock(patternMutex_);
            if (patterns_.empty()) {
                patterns_.push_back(std::make_unique<Pattern>("Default Pattern"));
                if (!patterns_.back()) {
                    return false;
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        // Handle any exceptions during initialization
        return false;
    }
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
    // Use atomic store with explicit memory ordering
    tempo_.store(bpm, std::memory_order_release);

    // Update beat time when tempo changes
    double newBeatTime = 60.0 / bpm;

    {
        std::lock_guard<std::mutex> lock(syncMutex_);
        beatTimeSeconds_ = newBeatTime;
    }
}

double Sequencer::getTempo() const {
    // Use atomic load with explicit memory ordering
    return tempo_.load(std::memory_order_acquire);
}

void Sequencer::synchronizeWithAudioEngine(double audioEngineTimeInSeconds, double engineSampleRate) {
    std::lock_guard<std::mutex> lock(syncMutex_);

    // Store the sample rate for future calculations
    audioEngineSampleRate_ = engineSampleRate;

    // Compute the time offset between sequencer and audio engine
    double currentTempo = tempo_.load(std::memory_order_acquire);
    double currentPositionInBeats;

    {
        std::lock_guard<std::mutex> posLock(positionMutex_);
        currentPositionInBeats = positionInBeats_;
    }

    // Calculate the beat time in seconds
    beatTimeSeconds_ = 60.0 / currentTempo;

    // Calculate what time the audio engine thinks beat 0 occurred
    audioEngineTimeOffset_ = audioEngineTimeInSeconds - (currentPositionInBeats * beatTimeSeconds_);

    // Store the sync time for drift compensation
    lastSyncTimeSeconds_ = audioEngineTimeInSeconds;
}

double Sequencer::getPrecisePositionInBeats() const {
    std::lock_guard<std::mutex> lock(syncMutex_);
    std::lock_guard<std::mutex> posLock(positionMutex_);
    return positionInBeats_;
}

double Sequencer::getPreciseBeatTime() const {
    std::lock_guard<std::mutex> lock(syncMutex_);
    return beatTimeSeconds_;
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

    // Get synchronized timing information
    double currentBeatTime;
    double audioTimeOffset;
    double sampleRate;

    {
        std::lock_guard<std::mutex> lock(syncMutex_);
        currentBeatTime = beatTimeSeconds_;
        audioTimeOffset = audioEngineTimeOffset_;
        sampleRate = audioEngineSampleRate_;
    }

    // Calculate beats per second from the current tempo
    const double beatsPerSecond = 1.0 / currentBeatTime;

    // Convert delta time to beats with high precision using synchronized timing
    double deltaBeats = deltaTime * beatsPerSecond;

    // Apply timing correction to compensate for drift
    static double accumulatedError = 0.0;
    double adjustedDeltaBeats = deltaBeats;

    {
        std::lock_guard<std::mutex> lock(timingMutex_);

        // Use high-precision quantization for timing correction
        // Quantize to 1/960th note precision (standard MIDI tick resolution)
        const double quantizationFactor = 960.0;
        double quantizedDeltaBeats = std::round(deltaBeats * quantizationFactor) / quantizationFactor;

        // Accumulate tiny errors for future correction
        accumulatedError += deltaBeats - quantizedDeltaBeats;

        // When accumulated error gets large enough, apply the correction
        // Use a threshold that corresponds to less than 1ms at typical tempos
        if (std::abs(accumulatedError) >= 0.0005) {
            adjustedDeltaBeats = quantizedDeltaBeats + accumulatedError;
            accumulatedError = 0.0;
        } else {
            adjustedDeltaBeats = quantizedDeltaBeats;
        }
    }

    // Use synchronized audio timing if available
    if (std::abs(audioTimeOffset) > 0.0001) {
        // Check if we need to adjust for drift between audio engine and sequencer
        // This keeps the sequencer locked to the audio engine's timing
        std::lock_guard<std::mutex> lock(syncMutex_);

        // Calculate the time elapsed since last sync in the audio engine's timeline
        double currentAudioTime = lastSyncTimeSeconds_ + deltaTime;
        lastSyncTimeSeconds_ = currentAudioTime;

        // Calculate what the beat position should be based on the audio engine's timing
        double expectedPositionInBeats = (currentAudioTime - audioTimeOffset) / currentBeatTime;

        // Get the current beat position
        double currentPositionInBeats;
        {
            std::lock_guard<std::mutex> posLock(positionMutex_);
            currentPositionInBeats = positionInBeats_;
        }

        // If the difference is significant, adjust the delta to converge
        double positionDifference = expectedPositionInBeats - currentPositionInBeats;
        if (std::abs(positionDifference) > 0.001) {
            // Adjust by moving up to 10% of the way toward the correct position each frame
            // This prevents sudden jumps while still converging to the right timing
            double correctionFactor = 0.1;
            double correction = positionDifference * correctionFactor;

            // Add the correction to the delta beats
            adjustedDeltaBeats += correction;
        }
    }

    // Process based on playback mode
    PlaybackMode mode = playbackMode_; // Make a local copy to avoid repeated access
    if (mode == PlaybackMode::SinglePattern) {
        processSinglePattern(adjustedDeltaBeats);
    } else {
        processSongArrangement(adjustedDeltaBeats);
    }

    // Prepare transport information for callback
    if (transportCallback_) {
        double safePosition;
        int bar, beat, ticksPerBeat = 960; // High-resolution MIDI ticks per beat
        int tick;

        // Minimize critical section - only capture necessary values
        {
            std::lock_guard<std::mutex> lock(positionMutex_);
            safePosition = positionInBeats_;

            // Compute derived values inside lock to ensure consistency
            bar = static_cast<int>(std::floor(safePosition / beatsPerBar_)) + 1;

            // Calculate beat with higher precision
            double beatInBar = std::fmod(safePosition, static_cast<double>(beatsPerBar_));
            if (beatInBar < 0.0) beatInBar += beatsPerBar_; // Handle negative modulo correctly

            beat = static_cast<int>(std::floor(beatInBar)) + 1;

            // Calculate tick with high precision for smoother timing display
            double tickPosition = std::fmod(safePosition * ticksPerBeat, ticksPerBeat);
            tick = static_cast<int>(std::round(tickPosition));
        } // Lock released as soon as possible

        // Call callback outside the lock
        transportCallback_(safePosition, bar, beat);
    }
}

void Sequencer::processSinglePattern(double deltaBeats) {
    // Store previous and current positions
    double previousPosition;
    double currentPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        previousPosition = positionInBeats_;
        positionInBeats_ += deltaBeats;
        currentPosition = positionInBeats_;
    }

    // Define a small epsilon for floating-point comparisons with better precision
    constexpr double EPSILON = 1e-9; // Smaller epsilon for more precise comparisons

    // Check if we need to loop
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patterns_.empty()) {
        return;
    }

    // Use atomic for thread safety
    size_t patternIndex = currentPatternIndex_.load(std::memory_order_acquire);
    if (patternIndex >= patterns_.size()) {
        return;
    }

    Pattern* currentPattern = patterns_[patternIndex].get();
    double patternLength = currentPattern->getLength();

    // Check for pattern end/loop condition with higher precision
    if (currentPosition >= patternLength - EPSILON) {
        if (looping_.load(std::memory_order_acquire)) {
            // Loop back to beginning with more accurate reset
            std::lock_guard<std::mutex> posLock(positionMutex_);
            // Use precise comparison and correctly handle the modulo
            if (std::abs(currentPosition - patternLength) < EPSILON) {
                positionInBeats_ = 0.0;  // Exactly at the end, reset to beginning
            } else {
                // Precise modulo calculation that avoids accumulating errors
                double numPatterns = std::floor(currentPosition / patternLength);
                positionInBeats_ = currentPosition - (numPatterns * patternLength);

                // Avoid floating point precision issues with extremely small values
                if (std::abs(positionInBeats_) < EPSILON) {
                    positionInBeats_ = 0.0;
                } else if (std::abs(positionInBeats_ - patternLength) < EPSILON) {
                    positionInBeats_ = 0.0;
                }
            }
            // Update with the corrected position after looping
            currentPosition = positionInBeats_;
            // At the loop point, we're starting from the beginning
            previousPosition = 0.0;
        } else {
            // Stop at the end with precise positioning
            std::lock_guard<std::mutex> posLock(positionMutex_);
            positionInBeats_ = patternLength;
            currentPosition = patternLength;
            isPlaying_ = false;
            return;
        }
    }

    // Create a local vector to store notes that need to be started in this frame
    // This minimizes the time we hold locks while calling callbacks
    struct NoteToStart {
        int pitch;
        float velocity;
        int channel;
        Envelope env;
        double endTime;
    };
    std::vector<NoteToStart> notesToStart;

    // Check for notes to start in this time slice with improved timing precision
    for (size_t i = 0; i < currentPattern->getNumNotes(); ++i) {
        Note* note = currentPattern->getNote(i);
        if (!note) continue;

        double noteStartTime = note->startTime;
        double noteEndTime = noteStartTime + note->duration;

        // Note starting in this time slice - use a more precise check
        // Handle edge cases like notes exactly at frame boundaries
        bool noteStartsInFrame = (noteStartTime >= previousPosition - EPSILON) &&
                                 (noteStartTime < currentPosition + EPSILON);

        // Special case for looping - handle notes at the start of the pattern
        if (previousPosition > currentPosition && noteStartTime < currentPosition) {
            noteStartsInFrame = true;
        }

        if (noteStartsInFrame) {
            // Add to queue of notes to start
            NoteToStart noteStart;
            noteStart.pitch = note->pitch;
            noteStart.velocity = note->velocity;
            noteStart.channel = note->channel;
            noteStart.env = note->env;
            noteStart.endTime = noteEndTime;
            notesToStart.push_back(noteStart);
        }
    }

    // Start notes outside of pattern lock
    for (const auto& noteToStart : notesToStart) {
        if (noteOnCallback_) {
            noteOnCallback_(noteToStart.pitch, noteToStart.velocity, noteToStart.channel, noteToStart.env);
        }

        // Add to active notes
        ActiveNote activeNote;
        activeNote.pitch = noteToStart.pitch;
        activeNote.channel = noteToStart.channel;
        activeNote.endTime = noteToStart.endTime;

        {
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            activeNotes_.push_back(activeNote);
        }
    }

    // Queue notes to stop in this time slice to avoid callbacks within locks
    std::vector<ActiveNote> notesToStop;

    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end()) {
            // More accurate comparison for note end times
            // A note should end if its end time is in this frame
            // or if we looped and the note end time was beyond the loop point
            bool noteEndsInFrame = (it->endTime <= currentPosition + EPSILON);

            // Special case for looping - if we wrapped around, also check notes that should have ended
            if (previousPosition > currentPosition && it->endTime > previousPosition) {
                noteEndsInFrame = true;
            }

            if (noteEndsInFrame) {
                notesToStop.push_back(*it);
                it = activeNotes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Call note off callbacks outside the lock
    for (const auto& noteToStop : notesToStop) {
        if (noteOffCallback_) {
            noteOffCallback_(noteToStop.pitch, noteToStop.channel);
        }
    }
}

void Sequencer::processSongArrangement(double deltaBeats) {
    // Store previous and current positions
    double previousPosition;
    double currentPosition;
    {
        std::lock_guard<std::mutex> lock(positionMutex_);
        previousPosition = positionInBeats_;
        positionInBeats_ += deltaBeats;
        currentPosition = positionInBeats_;
    }

    // Define a small epsilon for floating-point comparisons with better precision
    constexpr double EPSILON = 1e-9; // Smaller epsilon for more precise comparisons

    // Check if we need to loop
    if (currentPosition >= songLength_ - EPSILON) {
        if (looping_.load(std::memory_order_acquire) && songLength_ > EPSILON) {
            // Loop back to beginning with more accurate reset
            std::lock_guard<std::mutex> lock(positionMutex_);

            // Use precise comparison and correctly handle the modulo
            if (std::abs(currentPosition - songLength_) < EPSILON) {
                positionInBeats_ = 0.0;  // Exactly at the end, reset to beginning
            } else {
                // Precise modulo calculation that avoids accumulating errors
                double numLoops = std::floor(currentPosition / songLength_);
                positionInBeats_ = currentPosition - (numLoops * songLength_);

                // Avoid floating point precision issues with extremely small values
                if (std::abs(positionInBeats_) < EPSILON) {
                    positionInBeats_ = 0.0;
                } else if (std::abs(positionInBeats_ - songLength_) < EPSILON) {
                    positionInBeats_ = 0.0;
                }
            }

            // Update with the corrected position after looping
            currentPosition = positionInBeats_;
            // At the loop point, we're starting from the beginning
            previousPosition = 0.0;
        } else if (songLength_ > EPSILON) {
            // Stop at the end with precise positioning
            std::lock_guard<std::mutex> lock(positionMutex_);
            positionInBeats_ = songLength_;
            currentPosition = songLength_;
            isPlaying_ = false;
            return;
        }
    }

    // Create data structures to collect notes to process
    struct NoteToStart {
        int pitch;
        float velocity;
        int channel;
        Envelope env;
        double endTime;
    };
    std::vector<NoteToStart> notesToStart;

    // Find active pattern instances for this time slice
    std::vector<PatternInstance*> activeInstances = getActivePatternInstances();

    // Process each active pattern instance
    for (auto* instance : activeInstances) {
        // Get the pattern with proper locking
        std::shared_ptr<Pattern> patternPtr;
        {
            std::lock_guard<std::mutex> lock(patternMutex_);
            if (instance->patternIndex >= patterns_.size()) {
                continue;
            }

            // Use a shared_ptr to keep the pattern alive while processing
            patternPtr = std::shared_ptr<Pattern>(patterns_[instance->patternIndex].get(),
                                                 [](Pattern*) {}); // Non-owning deleter
        }

        // Skip if pattern is invalid
        if (!patternPtr) {
            continue;
        }

        Pattern* pattern = patternPtr.get();

        // Calculate local pattern position (relative to pattern start) with higher precision
        double patternStart = instance->startBeat;
        double localPreviousPos = previousPosition - patternStart;
        double localCurrentPos = currentPosition - patternStart;

        // Skip if we're completely outside the pattern boundaries
        // Use a more robust check that handles corner cases
        if ((localCurrentPos < -EPSILON) || (localPreviousPos >= pattern->getLength() + EPSILON)) {
            continue;
        }

        // Clamp positions to pattern boundaries with improved precision
        if (localPreviousPos < 0.0) {
            localPreviousPos = 0.0;
        }
        if (localCurrentPos > pattern->getLength()) {
            localCurrentPos = pattern->getLength();
        }

        // Check for notes to start in this time slice with improved precision
        for (size_t i = 0; i < pattern->getNumNotes(); ++i) {
            Note* note = pattern->getNote(i);
            if (!note) continue;

            double noteStartTime = note->startTime;
            double noteEndTime = noteStartTime + note->duration;

            // Note starting in this time slice - use a more precise check
            // Handle edge cases like notes exactly at frame boundaries
            bool noteStartsInFrame = (noteStartTime >= localPreviousPos - EPSILON) &&
                                     (noteStartTime < localCurrentPos + EPSILON);

            // Handle song loop case - notes at pattern start after song loop
            if (previousPosition > currentPosition &&
                ((patternStart <= currentPosition + EPSILON) &&
                 (patternStart + noteStartTime < currentPosition + EPSILON))) {
                noteStartsInFrame = true;
            }

            if (noteStartsInFrame) {
                // Add to queue of notes to start
                NoteToStart noteStart;
                noteStart.pitch = note->pitch;
                noteStart.velocity = note->velocity;
                noteStart.channel = note->channel;
                noteStart.env = note->env;

                // The end time is in global song time, not pattern-local time
                noteStart.endTime = patternStart + noteEndTime;

                // Ensure end time is properly bounded by song loop if applicable
                if (looping_.load(std::memory_order_acquire) &&
                    songLength_ > EPSILON &&
                    noteStart.endTime > songLength_) {
                    // Handle note that extends beyond the song loop point
                    // This is a design choice - could also wrap the end time
                    // but here we just clamp it to the end of the song
                    noteStart.endTime = songLength_;
                }

                notesToStart.push_back(noteStart);
            }
        }
    }

    // Process notes to start outside of pattern lock
    for (const auto& noteToStart : notesToStart) {
        if (noteOnCallback_) {
            noteOnCallback_(noteToStart.pitch, noteToStart.velocity, noteToStart.channel, noteToStart.env);
        }

        // Add to active notes
        ActiveNote activeNote;
        activeNote.pitch = noteToStart.pitch;
        activeNote.channel = noteToStart.channel;
        activeNote.endTime = noteToStart.endTime;

        {
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            activeNotes_.push_back(activeNote);
        }
    }

    // Queue notes to stop in this time slice to avoid callbacks within locks
    std::vector<ActiveNote> notesToStop;

    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end()) {
            // More accurate comparison for note end times
            // A note should end if its end time is in this frame
            bool noteEndsInFrame = (it->endTime <= currentPosition + EPSILON);

            // Special case for song looping - if we wrapped around, also check notes that would have ended
            if (previousPosition > currentPosition && it->endTime > previousPosition) {
                noteEndsInFrame = true;
            }

            if (noteEndsInFrame) {
                notesToStop.push_back(*it);
                it = activeNotes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Call note off callbacks outside the lock
    for (const auto& noteToStop : notesToStop) {
        if (noteOffCallback_) {
            noteOffCallback_(noteToStop.pitch, noteToStop.channel);
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