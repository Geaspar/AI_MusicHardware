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
    // Default sections A/B/C at 0, 4, 8 beats
    sections_.push_back({"A", 0.0});
    sections_.push_back({"B", static_cast<double>(beatsPerBar_) });
    sections_.push_back({"C", static_cast<double>(2 * beatsPerBar_) });
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
    // Reset per-loop dedupe
    firedThisLoop_.clear();
    loopedThisCall_ = false;
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

// Resequencing MVP API
void Sequencer::defineSections(const std::vector<std::pair<std::string,double>>& sectionsInBeats) {
    sections_ = sectionsInBeats;
    if (sections_.empty()) {
        sections_.push_back({"A", 0.0});
    }
    currentSectionIndex_ = 0;
}

void Sequencer::scheduleJumpToBeat(double beat, Timing when) {
    if (when == Timing::Immediate) {
        setPositionInBeats(beat);
        // Update current section index to closest
        int best = 0; double bestDiff = 1e9;
        for (int i=0;i<(int)sections_.size();++i) {
            double d = std::abs(sections_[i].second - beat);
            if (d < bestDiff) { bestDiff = d; best = i; }
        }
        currentSectionIndex_ = best;
    } else {
        pendingJump_.active = true;
        pendingJump_.targetBeat = beat;
        pendingJump_.timing = when;
    }
}

void Sequencer::servicePendingJump(double previousPosition, double currentPosition) {
    if (!pendingJump_.active) return;
    if (pendingJump_.timing == Timing::OnBar) {
        int prevBar = static_cast<int>(std::floor(previousPosition / beatsPerBar_));
        int currBar = static_cast<int>(std::floor(currentPosition / beatsPerBar_));
        if (currBar != prevBar) {
            setPositionInBeats(pendingJump_.targetBeat);
            pendingJump_.active = false;
        }
    } else if (pendingJump_.timing == Timing::OnBeat) {
        int prevBeat = static_cast<int>(std::floor(previousPosition));
        int currBeat = static_cast<int>(std::floor(currentPosition));
        if (currBeat != prevBeat) {
            setPositionInBeats(pendingJump_.targetBeat);
            pendingJump_.active = false;
        }
    }
}

void Sequencer::jumpToSection(const std::string& name, Timing when) {
    for (size_t i = 0; i < sections_.size(); ++i) {
        if (sections_[i].first == name) {
            currentSectionIndex_ = static_cast<int>(i);
            scheduleJumpToBeat(sections_[i].second, when);
            return;
        }
    }
}

void Sequencer::queueSection(const std::string& name, Timing when) {
    jumpToSection(name, when);
}

void Sequencer::nextSection(Timing when) {
    if (sections_.empty()) return;
    currentSectionIndex_ = (currentSectionIndex_ + 1) % static_cast<int>(sections_.size());
    scheduleJumpToBeat(sections_[currentSectionIndex_].second, when);
}

void Sequencer::prevSection(Timing when) {
    if (sections_.empty()) return;
    currentSectionIndex_ = (currentSectionIndex_ - 1);
    if (currentSectionIndex_ < 0) currentSectionIndex_ = static_cast<int>(sections_.size()) - 1;
    scheduleJumpToBeat(sections_[currentSectionIndex_].second, when);
}

std::vector<std::string> Sequencer::getSectionNames() const {
    std::vector<std::string> names;
    for (const auto& s : sections_) names.push_back(s.first);
    return names;
}

std::string Sequencer::getCurrentSectionName() const {
    if (sections_.empty()) return std::string();
    int idx = currentSectionIndex_;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(sections_.size())) idx = static_cast<int>(sections_.size()) - 1;
    return sections_[idx].first;
}

void Sequencer::addPattern(std::unique_ptr<Pattern> pattern) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    patterns_.push_back(std::move(pattern));
}

size_t Sequencer::createPattern(const std::string& name, int steps16th) {
    if (steps16th < 1) steps16th = 16; // default 1 bar @ 4/4
    double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
    double lenBeats = static_cast<double>(steps16th) * stepBeats;
    auto p = std::make_unique<Pattern>(name);
    p->setLength(lenBeats);
    std::lock_guard<std::mutex> lock(patternMutex_);
    patterns_.push_back(std::move(p));
    return patterns_.size() - 1;
}

size_t Sequencer::duplicatePattern(size_t index) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index >= patterns_.size()) return patterns_.size();
    const Pattern* src = patterns_[index].get();
    auto copy = std::make_unique<Pattern>(src->getName() + " Copy");
    copy->setLength(src->getLength());
    for (size_t i = 0; i < src->getNumNotes(); ++i) {
        if (const Note* n = src->getNote(i)) {
            copy->addNote(*n);
        }
    }
    patterns_.push_back(std::move(copy));
    return patterns_.size() - 1;
}

void Sequencer::clearPattern(size_t index) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < patterns_.size() && patterns_[index]) {
        patterns_[index]->clear();
        ++patternVersion_;
    }
}

void Sequencer::renamePattern(size_t index, const std::string& newName) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (index < patterns_.size() && patterns_[index]) {
        patterns_[index]->setName(newName);
    }
}

std::optional<size_t> Sequencer::getPatternIndexByName(const std::string& name) const {
    std::lock_guard<std::mutex> lock(patternMutex_);
    for (size_t i = 0; i < patterns_.size(); ++i) {
        if (patterns_[i] && patterns_[i]->getName() == name) return i;
    }
    return std::nullopt;
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
    // Reset per-loop dedupe keys since we performed an explicit jump
    firedThisLoop_.clear();
    
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

//------------------------------------------------------------------------------
// Thread-safe editor helpers
//------------------------------------------------------------------------------

void Sequencer::setColumnVelocity(size_t patternIndex, int column16th, float velocity) {
    // Clamp velocity 0..1 for safety
    if (velocity < 0.0f) velocity = 0.0f;
    if (velocity > 1.0f) velocity = 1.0f;

    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patternIndex >= patterns_.size()) return;
    Pattern* p = patterns_[patternIndex].get();
    if (!p) return;

    const double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
    const size_t n = p->getNumNotes();
    const double startBeat = static_cast<double>(column16th) * stepBeats;
    constexpr double EPS = 1e-6;
    for (size_t i = 0; i < n; ++i) {
        if (Note* note = p->getNote(i)) {
            if (std::abs(note->startTime - startBeat) < EPS) {
                note->velocity = velocity;
            }
        }
    }
    ++patternVersion_;
}

void Sequencer::addNoteToPattern(size_t patternIndex, const Note& note) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patternIndex >= patterns_.size()) return;
    Pattern* p = patterns_[patternIndex].get();
    if (!p) return;
    p->addNote(note);
    ++patternVersion_;
}

void Sequencer::removeNotesAt(size_t patternIndex, int pitch, double startBeat, double epsilon) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patternIndex >= patterns_.size()) return;
    Pattern* p = patterns_[patternIndex].get();
    if (!p) return;
    // Erase any matching notes
    for (size_t i = 0; i < p->getNumNotes(); ) {
        Note* n = p->getNote(i);
        if (n && n->pitch == pitch && std::abs(n->startTime - startBeat) < epsilon) {
            p->removeNote(i);
        } else {
            ++i;
        }
    }
    ++patternVersion_;
}

void Sequencer::setColumnChance(size_t patternIndex, int column16th, float chance) {
    if (chance < 0.0f) chance = 0.0f;
    if (chance > 1.0f) chance = 1.0f;
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patternIndex >= patterns_.size()) return;
    Pattern* p = patterns_[patternIndex].get();
    if (!p) return;
    const double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
    const double startBeat = static_cast<double>(column16th) * stepBeats;
    constexpr double EPS = 1e-6;
    const size_t n = p->getNumNotes();
    for (size_t i = 0; i < n; ++i) {
        if (Note* note = p->getNote(i)) {
            if (std::abs(note->startTime - startBeat) < EPS) {
                note->chance = chance;
            }
        }
    }
    ++patternVersion_;
}

void Sequencer::setAllNotesChance(size_t patternIndex, float chance) {
    if (chance < 0.0f) chance = 0.0f; if (chance > 1.0f) chance = 1.0f;
    std::lock_guard<std::mutex> lock(patternMutex_);
    if (patternIndex >= patterns_.size()) return;
    Pattern* p = patterns_[patternIndex].get();
    if (!p) return;
    const size_t n = p->getNumNotes();
    for (size_t i = 0; i < n; ++i) {
        if (Note* no = p->getNote(i)) no->chance = chance;
    }
    ++patternVersion_;
}

void Sequencer::setStep(size_t patternIndex, int step16th, int pitch, float velocity, float gate) {
    if (step16th < 0) return;
    if (velocity < 0.0f) velocity = 0.0f; if (velocity > 1.0f) velocity = 1.0f;
    if (gate <= 0.0f) gate = 1.0f; if (gate > 4.0f) gate = 4.0f; // allow up to 4x step length (ties)
    double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
    double startBeat = static_cast<double>(step16th) * stepBeats;
    double dur = gate * stepBeats;
    // Remove any existing note at this cell (same pitch and start)
    removeNotesAt(patternIndex, pitch, startBeat, 1e-6);
    // Add new
    Note n;
    n.pitch = pitch; n.velocity = velocity; n.channel = 0; n.startTime = startBeat; n.duration = dur; n.env = Envelope(); n.chance = 1.0f;
    addNoteToPattern(patternIndex, n);
    ++patternVersion_;
}

void Sequencer::toggleStep(size_t patternIndex, int step16th, int pitch) {
    if (step16th < 0) return;
    double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
    double startBeat = static_cast<double>(step16th) * stepBeats;
    // Check if note exists at this start/pitch
    bool exists = false;
    {
        std::lock_guard<std::mutex> lock(patternMutex_);
        if (patternIndex < patterns_.size()) {
            Pattern* p = patterns_[patternIndex].get();
            for (size_t i = 0; i < p->getNumNotes(); ++i) {
                Note* no = p->getNote(i);
                if (!no) continue;
                if (no->pitch == pitch && std::fabs(no->startTime - startBeat) <= 1e-6) { exists = true; break; }
            }
        }
    }
    if (exists) {
        removeNotesAt(patternIndex, pitch, startBeat, 1e-6);
    } else {
        setStep(patternIndex, step16th, pitch, 0.8f, 1.0f);
    }
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

    // Simplified timing correction - just use the delta beats directly
    // The complex quantization was causing timing issues
    double adjustedDeltaBeats = deltaBeats;

    // Simplified audio synchronization - just use basic timing without complex drift correction
    // The complex drift correction was causing timing instability

    // Process based on playback mode
    PlaybackMode mode = playbackMode_; // Make a local copy to avoid repeated access
    if (mode == PlaybackMode::SinglePattern) {
        processSinglePattern(adjustedDeltaBeats);
    } else {
        processSongArrangement(adjustedDeltaBeats);
    }

    // Reset loop flag for next call
    if (loopedThisCall_) {
        loopedThisCall_ = false;
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

    // Service any pending resequencing at musical boundaries
    servicePendingJump(previousPosition, currentPosition);

    // Define epsilon (in beats) based on beat time (~0.25ms) with a hard floor
    const double EPSILON = timingEpsilonBeats();

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
            // Before looping, stop ALL active notes to ensure clean loop boundaries
            // Optionally collect those that should wrap across the loop point
            struct NoteToStart { int pitch; float velocity; int channel; Envelope env; double endTime; };
            std::vector<NoteToStart> wrappedRestarts;
            {
                std::lock_guard<std::mutex> notesLock(activeNotesMutex_);
                if (wrapLongNotesAcrossLoop_.load(std::memory_order_relaxed)) {
                    for (const auto& an : activeNotes_) {
                        if (an.endTime > patternLength) {
                            NoteToStart ns; ns.pitch = an.pitch; ns.velocity = an.velocity; ns.channel = an.channel; ns.env = an.env;
                            ns.endTime = an.endTime - patternLength;
                            wrappedRestarts.push_back(ns);
                        }
                    }
                }
                for (const auto& activeNote : activeNotes_) {
                    if (noteOffCallback_) noteOffCallback_(activeNote.pitch, activeNote.channel);
                }
                activeNotes_.clear();
            }
            
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
            // Set previousPosition to the pattern length to avoid triggering notes that were
            // already processed in the previous loop iteration
            previousPosition = patternLength;
            // Mark that we looped and reset per-loop state
            loopedThisCall_ = true;
            firedThisLoop_.clear();
            ++loopCount_;

            // If we have wrapped restarts, schedule them now at position 0
            if (!wrappedRestarts.empty()) {
                for (const auto& ns : wrappedRestarts) {
                    if (noteOnCallback_) noteOnCallback_(ns.pitch, ns.velocity, ns.channel, ns.env);
                    ActiveNote an; an.pitch = ns.pitch; an.channel = ns.channel; an.endTime = ns.endTime; an.velocity = ns.velocity; an.env = ns.env;
                    std::lock_guard<std::mutex> notesLock(activeNotesMutex_);
                    activeNotes_.push_back(an);
                }
            }
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
    notesToStart.reserve(static_cast<size_t>(std::max(16.0, std::ceil(currentPattern->getNumNotes() * 0.25))));

    // Check for notes to start in this time slice with improved timing precision
    for (size_t i = 0; i < currentPattern->getNumNotes(); ++i) {
        Note* note = currentPattern->getNote(i);
        if (!note) continue;

        double noteStartTime = note->startTime;
        double noteEndTime = noteStartTime + note->duration;

        // Note starting in this time slice - half-open window [prev-EPS, current)
        bool noteStartsInFrame = (noteStartTime >= previousPosition - EPSILON) &&
                                 (noteStartTime < currentPosition);

        // Looping special-case: union of [prev-EPS, end) and [0, current)
        if (previousPosition > currentPosition) {
            bool inFirst = (noteStartTime >= previousPosition - EPSILON);
            bool inSecond = (noteStartTime < currentPosition);
            noteStartsInFrame = inFirst || inSecond;
        }

        if (noteStartsInFrame) {
#ifdef SEQ_DEBUG_PRINT
            std::cout << "[SEQDBG] start note pitch=" << note->pitch
                      << " vel=" << note->velocity
                      << " start=" << noteStartTime
                      << " prevPos=" << previousPosition
                      << " curPos=" << currentPosition
                      << " len=" << note->duration
                      << std::endl;
#endif
            // Compute dynamic 16th column for dedupe/prob modes
            const double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
            const int cols = std::max(1, (int) std::llround(currentPattern->getLength() / stepBeats));
            int col = (int) std::llround(std::fmod(noteStartTime, currentPattern->getLength()) / stepBeats + 1e-7);
            if (col < 0) col = 0; if (cols > 0) col = col % cols;
            const uint64_t dedupeKey = makeDedupeKey(col, note->pitch, note->channel);

            // Gate by per-note chance (probability), with selectable mode
            float prob = std::clamp(note->chance, 0.0f, 1.0f);
            if (prob < 1.0f) {
                float r = (probabilityMode_ == ProbabilityMode::PerLoopStable)
                          ? stableProb01(dedupeKey)
                          : rngNextFloat01();
                if (r > prob) continue; // skip triggering this note this time
            }
            // Per-loop dedupe: avoid double triggers at boundaries
            if (perLoopDedupeEnabled_.load(std::memory_order_relaxed)) {
                if (firedThisLoop_.find(dedupeKey) != firedThisLoop_.end()) {
                    continue;
                }
                firedThisLoop_.insert(dedupeKey);
            }

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
        activeNote.velocity = noteToStart.velocity;
        activeNote.env = noteToStart.env;

        {
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            activeNotes_.push_back(activeNote);
        }
    }

    // Queue notes to stop in this time slice to avoid callbacks within locks
    std::vector<ActiveNote> notesToStop;
    notesToStop.reserve(activeNotes_.size());

    {
        std::lock_guard<std::mutex> lock(activeNotesMutex_);
        auto it = activeNotes_.begin();
        while (it != activeNotes_.end()) {
            // More accurate comparison for note end times
            // A note should end if its end time is in this frame
            // or if we looped and the note end time was beyond the loop point
            bool noteEndsInFrame = (it->endTime <= currentPosition + EPSILON);

            // Special case for looping - if we wrapped around, check notes that:
            // 1. Should have ended in the previous position range
            // 2. Extend beyond the pattern length (these must be stopped at loop boundary)
            if (previousPosition > currentPosition) {
                // We've looped - stop any notes that extend beyond pattern length
                if (it->endTime >= patternLength) {
                    noteEndsInFrame = true;
                }
                // Also stop notes that should have ended in the gap
                else if (it->endTime > previousPosition) {
                    noteEndsInFrame = true;
                }
            }

            if (noteEndsInFrame) {
#ifdef SEQ_DEBUG_PRINT
                std::cout << "[SEQDBG] stop note pitch=" << it->pitch
                          << " endTime=" << it->endTime
                          << " prevPos=" << previousPosition
                          << " curPos=" << currentPosition
                          << std::endl;
#endif
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

    // Service any pending resequencing at musical boundaries
    servicePendingJump(previousPosition, currentPosition);

    // Define epsilon (in beats) based on beat time (~0.25ms) with a hard floor
    const double EPSILON = timingEpsilonBeats();

    // Check if we need to loop
    if (currentPosition >= songLength_ - EPSILON) {
        if (looping_.load(std::memory_order_acquire) && songLength_ > EPSILON) {
            // Prepare to wrap long notes if enabled
            struct NoteToStart { int pitch; float velocity; int channel; Envelope env; double endTime; };
            std::vector<NoteToStart> wrappedRestarts;
            // Before looping, stop ALL active notes to ensure clean loop boundaries
            // Optionally collect those that should wrap across the loop point
            {
                std::lock_guard<std::mutex> notesLock(activeNotesMutex_);
                if (wrapLongNotesAcrossLoop_.load(std::memory_order_relaxed)) {
                    for (const auto& an : activeNotes_) {
                        if (an.endTime > songLength_) {
                            NoteToStart ns; ns.pitch = an.pitch; ns.velocity = an.velocity; ns.channel = an.channel; ns.env = an.env;
                            ns.endTime = an.endTime - songLength_;
                            wrappedRestarts.push_back(ns);
                        }
                    }
                }
                for (const auto& activeNote : activeNotes_) {
                    if (noteOffCallback_) noteOffCallback_(activeNote.pitch, activeNote.channel);
                }
                activeNotes_.clear();
            }
            
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
            // Set previousPosition to the song length to avoid triggering notes that were
            // already processed in the previous loop iteration
            previousPosition = songLength_;
            // Signal to the outer process() that a loop occurred so it can reset timing accumulator
            loopedThisCall_ = true;
            firedThisLoop_.clear();
            ++loopCount_;

            // If we have wrapped restarts, schedule them now at position 0
            if (!wrappedRestarts.empty()) {
                for (const auto& ns : wrappedRestarts) {
                    if (noteOnCallback_) noteOnCallback_(ns.pitch, ns.velocity, ns.channel, ns.env);
                    ActiveNote an; an.pitch = ns.pitch; an.channel = ns.channel; an.endTime = ns.endTime; an.velocity = ns.velocity; an.env = ns.env;
                    std::lock_guard<std::mutex> notesLock(activeNotesMutex_);
                    activeNotes_.push_back(an);
                }
            }
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
    struct NoteToStart { int pitch; float velocity; int channel; Envelope env; double endTime; };
    std::vector<NoteToStart> notesToStart;
    notesToStart.reserve(64);

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

            // Compute global start time of this note within the song timeline
            const double globalStart = patternStart + noteStartTime;

            // Half-open window: [previousPosition - EPS, currentPosition)
            bool noteStartsInFrame = (globalStart >= previousPosition - EPSILON) &&
                                     (globalStart < currentPosition);

            // Looping special-case: if we wrapped this frame, take union of
            // [previousPosition - EPS, songLength) U [0, currentPosition)
            if (previousPosition > currentPosition) {
                const bool inFirst = (globalStart >= previousPosition - EPSILON);
                const bool inSecond = (globalStart < currentPosition);
                noteStartsInFrame = inFirst || inSecond;
            }

            if (noteStartsInFrame) {
                // Probability gating
                float prob = std::clamp(note->chance, 0.0f, 1.0f);
                if (prob < 1.0f) {
                    // Compute per-step dedupe key for stable probability if needed
                    const double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
                    const int cols = std::max(1, (int) std::llround(pattern->getLength() / stepBeats));
                    int col = (int) std::llround(std::fmod(noteStartTime, pattern->getLength()) / stepBeats + 1e-7);
                    if (col < 0) col = 0; if (cols > 0) col = col % cols;
                    const uint64_t dedupeKey = makeDedupeKey(col, note->pitch, note->channel);
                    float r = (probabilityMode_ == ProbabilityMode::PerLoopStable)
                                ? stableProb01(dedupeKey)
                                : rngNextFloat01();
                    if (r > prob) continue;
                }
                // Per-loop dedupe (optional)
                if (perLoopDedupeEnabled_.load(std::memory_order_relaxed)) {
                    const double stepBeats = static_cast<double>(beatsPerBar_) / 16.0;
                    const int cols = std::max(1, (int) std::llround(pattern->getLength() / stepBeats));
                    int col = (int) std::llround(std::fmod(noteStartTime, pattern->getLength()) / stepBeats + 1e-7);
                    if (col < 0) col = 0; if (cols > 0) col = col % cols;
                    const uint64_t dedupeKey = makeDedupeKey(col, note->pitch, note->channel);
                    if (firedThisLoop_.find(dedupeKey) != firedThisLoop_.end()) {
                        continue;
                    }
                    firedThisLoop_.insert(dedupeKey);
                }
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
        activeNote.velocity = noteToStart.velocity;
        activeNote.env = noteToStart.env;

        {
            std::lock_guard<std::mutex> lock(activeNotesMutex_);
            activeNotes_.push_back(activeNote);
        }
    }

    // Queue notes to stop in this time slice to avoid callbacks within locks
    std::vector<ActiveNote> notesToStop;
    notesToStop.reserve(activeNotes_.size());

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
