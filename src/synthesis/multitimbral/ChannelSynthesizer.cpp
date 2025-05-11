#include "../../../include/synthesis/multitimbral/ChannelSynthesizer.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

//----------------------------------------------------------------------
// ChannelParameters Implementation
//----------------------------------------------------------------------

bool ChannelParameters::operator==(const ChannelParameters& other) const {
    return channelNumber == other.channelNumber &&
           name == other.name &&
           polyphony == other.polyphony &&
           priority == other.priority &&
           monophonic == other.monophonic &&
           portamentoTime == other.portamentoTime &&
           transposition == other.transposition &&
           fineTuning == other.fineTuning &&
           noteRangeLow == other.noteRangeLow &&
           noteRangeHigh == other.noteRangeHigh &&
           volume == other.volume &&
           pan == other.pan &&
           receivesProgramChange == other.receivesProgramChange &&
           receivesControllers == other.receivesControllers &&
           receivesPitchBend == other.receivesPitchBend &&
           ccResponseMap == other.ccResponseMap &&
           programNumber == other.programNumber &&
           presetName == other.presetName;
}

bool ChannelParameters::operator!=(const ChannelParameters& other) const {
    return !(*this == other);
}

//----------------------------------------------------------------------
// ChannelSynthesizer Implementation
//----------------------------------------------------------------------

ChannelSynthesizer::ChannelSynthesizer(int channel, int sampleRate)
    : Synthesizer(sampleRate),
      channelNumber_(std::clamp(channel, 0, 15)),
      name_("Channel " + std::to_string(channelNumber_ + 1)),
      monophonic_(false),
      portamentoTime_(0),
      transposition_(0),
      fineTuning_(0),
      noteRangeLow_(0),
      noteRangeHigh_(127),
      channelPriority_(1),
      receivesProgramChange_(true),
      receivesControllers_(true),
      receivesPitchBend_(true),
      programNumber_(0),
      presetName_("Default"),
      lastPlayedNote_(-1),
      lastNoteVelocity_(0.0f) {
}

ChannelSynthesizer::~ChannelSynthesizer() {
    // Base class destructor will handle cleanup
}

int ChannelSynthesizer::getChannel() const {
    return channelNumber_;
}

void ChannelSynthesizer::setChannel(int channel) {
    channelNumber_ = std::clamp(channel, 0, 15);
    name_ = "Channel " + std::to_string(channelNumber_ + 1);
}

void ChannelSynthesizer::setNoteRange(int low, int high) {
    noteRangeLow_ = std::clamp(low, 0, 127);
    noteRangeHigh_ = std::clamp(high, 0, 127);
    
    // Ensure low is actually lower than high
    if (noteRangeLow_ > noteRangeHigh_) {
        std::swap(noteRangeLow_, noteRangeHigh_);
    }
}

bool ChannelSynthesizer::isNoteInRange(int midiNote) const {
    return midiNote >= noteRangeLow_ && midiNote <= noteRangeHigh_;
}

void ChannelSynthesizer::setName(const std::string& name) {
    name_ = name;
}

std::string ChannelSynthesizer::getName() const {
    return name_;
}

void ChannelSynthesizer::setMonophonic(bool mono) {
    monophonic_ = mono;
    
    // If switching to monophonic, release any active notes except the last played one
    if (mono && lastPlayedNote_ >= 0) {
        for (int note : heldNotes_) {
            if (note != lastPlayedNote_) {
                Synthesizer::noteOff(note, channelNumber_);
            }
        }
        
        // Keep only the last played note in the held notes list
        heldNotes_.clear();
        heldNotes_.push_back(lastPlayedNote_);
    }
}

bool ChannelSynthesizer::isMonophonic() const {
    return monophonic_;
}

void ChannelSynthesizer::setPortamentoTime(int timeMs) {
    portamentoTime_ = std::max(0, timeMs);
    
    // In a full implementation, we would apply this to the voice manager
    // or modulation system to implement pitch sliding between notes
}

int ChannelSynthesizer::getPortamentoTime() const {
    return portamentoTime_;
}

void ChannelSynthesizer::setTransposition(int semitones) {
    // Limit to +/- 48 semitones (4 octaves)
    transposition_ = std::clamp(semitones, -48, 48);
}

int ChannelSynthesizer::getTransposition() const {
    return transposition_;
}

void ChannelSynthesizer::setFineTuning(int cents) {
    // Limit to +/- 100 cents (1 semitone)
    fineTuning_ = std::clamp(cents, -100, 100);
}

int ChannelSynthesizer::getFineTuning() const {
    return fineTuning_;
}

void ChannelSynthesizer::setChannelPriority(int priority) {
    channelPriority_ = std::max(1, priority);
}

int ChannelSynthesizer::getChannelPriority() const {
    return channelPriority_;
}

void ChannelSynthesizer::processChannelProgramChange(int program) {
    if (!receivesProgramChange_) {
        return;
    }
    
    programNumber_ = std::clamp(program, 0, 127);
    
    // In a full implementation, this would load a preset corresponding
    // to the program number from the preset library
    // For now, just update the preset name
    presetName_ = "Program " + std::to_string(programNumber_);
}

void ChannelSynthesizer::processChannelMidiCC(int controller, int value) {
    if (!receivesControllers_) {
        return;
    }
    
    // Handle standard MIDI CCs
    switch (controller) {
        case 7: // Volume
            // Transform 0-127 to 0.0-1.0
            setParameter("masterVolume", value / 127.0f);
            break;
            
        case 10: // Pan
            // Transform 0-127 to -1.0-1.0
            setParameter("masterPan", (value / 64.0f) - 1.0f);
            break;
            
        case 5: // Portamento time
            portamentoTime_ = static_cast<int>(value * 20.0f); // 0-2540ms range
            break;
            
        case 65: // Portamento on/off
            monophonic_ = (value >= 64);
            break;
            
        case 126: // Mono mode on
            setMonophonic(true);
            break;
            
        case 127: // Poly mode on
            setMonophonic(false);
            break;
            
        default:
            // Pass other controllers to the base synthesizer
            // Transform 0-127 to 0.0-1.0 for parameter values
            Synthesizer::setParameter("cc" + std::to_string(controller), value / 127.0f);
            break;
    }
}

void ChannelSynthesizer::enableProgramChanges(bool enable) {
    receivesProgramChange_ = enable;
}

bool ChannelSynthesizer::receivesProgramChanges() const {
    return receivesProgramChange_;
}

void ChannelSynthesizer::enableControlChanges(bool enable) {
    receivesControllers_ = enable;
}

bool ChannelSynthesizer::receivesControlChanges() const {
    return receivesControllers_;
}

void ChannelSynthesizer::enablePitchBend(bool enable) {
    receivesPitchBend_ = enable;
}

bool ChannelSynthesizer::receivesPitchBend() const {
    return receivesPitchBend_;
}

void ChannelSynthesizer::setProgramNumber(int program) {
    programNumber_ = std::clamp(program, 0, 127);
}

int ChannelSynthesizer::getProgramNumber() const {
    return programNumber_;
}

void ChannelSynthesizer::setPresetName(const std::string& name) {
    presetName_ = name;
}

std::string ChannelSynthesizer::getPresetName() const {
    return presetName_;
}

void ChannelSynthesizer::loadChannelParameters(const ChannelParameters& params) {
    // Load channel-specific parameters
    setChannel(params.channelNumber);
    setName(params.name);
    setVoiceCount(params.polyphony);
    setChannelPriority(params.priority);
    
    setMonophonic(params.monophonic);
    setPortamentoTime(params.portamentoTime);
    setTransposition(params.transposition);
    setFineTuning(params.fineTuning);
    setNoteRange(params.noteRangeLow, params.noteRangeHigh);
    
    // Set MIDI response flags
    enableProgramChanges(params.receivesProgramChange);
    enableControlChanges(params.receivesControllers);
    enablePitchBend(params.receivesPitchBend);
    
    // Set preset information
    setProgramNumber(params.programNumber);
    setPresetName(params.presetName);
    
    // In a full implementation, we would also load the synth parameters 
    // from the preset data
}

ChannelParameters ChannelSynthesizer::saveChannelParameters() const {
    ChannelParameters params;
    
    // Copy channel-specific parameters
    params.channelNumber = channelNumber_;
    params.name = name_;
    params.polyphony = getVoiceCount();
    params.priority = channelPriority_;
    
    params.monophonic = monophonic_;
    params.portamentoTime = portamentoTime_;
    params.transposition = transposition_;
    params.fineTuning = fineTuning_;
    params.noteRangeLow = noteRangeLow_;
    params.noteRangeHigh = noteRangeHigh_;
    
    // Copy MIDI response flags
    params.receivesProgramChange = receivesProgramChange_;
    params.receivesControllers = receivesControllers_;
    params.receivesPitchBend = receivesPitchBend_;
    
    // Copy preset information
    params.programNumber = programNumber_;
    params.presetName = presetName_;
    
    // In a full implementation, we would also save the synth parameters
    
    return params;
}

void ChannelSynthesizer::process(float* buffer, int numFrames) {
    // Apply channel-specific processing before passing to base class
    
    // In a full implementation, we might apply channel-specific effects
    // or processing here, such as EQ, compression, etc.
    
    // Call base class implementation
    Synthesizer::process(buffer, numFrames);
}

// Override base class noteOn to implement channel-specific behavior
void ChannelSynthesizer::noteOn(int midiNote, float velocity, int channel) {
    // Apply transposition
    int transposedNote = applyTransposition(midiNote);
    
    // Check if note is in range
    if (!isNoteInRange(midiNote)) {
        return;
    }
    
    // Handle monophonic mode
    if (monophonic_) {
        // If we already have a note playing, trigger legato behavior
        if (!heldNotes_.empty()) {
            // For legato, release previous note but keep it in held notes list
            Synthesizer::noteOff(lastPlayedNote_, channelNumber_);
        }
        
        // Add to held notes
        if (std::find(heldNotes_.begin(), heldNotes_.end(), midiNote) == heldNotes_.end()) {
            heldNotes_.push_back(midiNote);
        }
        
        // Update last played note
        lastPlayedNote_ = midiNote;
        lastNoteVelocity_ = velocity;
        
        // Trigger the note
        Synthesizer::noteOn(transposedNote, velocity, channelNumber_);
    } else {
        // Standard polyphonic behavior
        Synthesizer::noteOn(transposedNote, velocity, channelNumber_);
        
        // Keep track of held notes for sustain pedal
        if (std::find(heldNotes_.begin(), heldNotes_.end(), midiNote) == heldNotes_.end()) {
            heldNotes_.push_back(midiNote);
        }
        
        // Update last played note
        lastPlayedNote_ = midiNote;
        lastNoteVelocity_ = velocity;
    }
}

// Override base class noteOff to implement channel-specific behavior
void ChannelSynthesizer::noteOff(int midiNote, int channel) {
    // Apply transposition
    int transposedNote = applyTransposition(midiNote);
    
    // Handle monophonic mode
    if (monophonic_) {
        // Remove from held notes
        auto it = std::find(heldNotes_.begin(), heldNotes_.end(), midiNote);
        if (it != heldNotes_.end()) {
            heldNotes_.erase(it);
        }
        
        // If this was the last played note
        if (midiNote == lastPlayedNote_) {
            // If there are still held notes, play the most recent one
            if (!heldNotes_.empty()) {
                int noteToResume = heldNotes_.back();
                lastPlayedNote_ = noteToResume;
                
                // Trigger note on for the next note in the stack
                int transposedResumeNote = applyTransposition(noteToResume);
                Synthesizer::noteOn(transposedResumeNote, lastNoteVelocity_, channelNumber_);
            } else {
                // No more held notes, release the last one
                Synthesizer::noteOff(transposedNote, channelNumber_);
                lastPlayedNote_ = -1;
            }
        }
    } else {
        // Standard polyphonic behavior
        Synthesizer::noteOff(transposedNote, channelNumber_);
        
        // Remove from held notes
        auto it = std::find(heldNotes_.begin(), heldNotes_.end(), midiNote);
        if (it != heldNotes_.end()) {
            heldNotes_.erase(it);
        }
        
        // Update last played note if this was it
        if (midiNote == lastPlayedNote_ && !heldNotes_.empty()) {
            lastPlayedNote_ = heldNotes_.back();
        } else if (heldNotes_.empty()) {
            lastPlayedNote_ = -1;
        }
    }
}

// Override allNotesOff to implement channel-specific behavior
void ChannelSynthesizer::allNotesOff(int channel) {
    Synthesizer::allNotesOff(channelNumber_);
    heldNotes_.clear();
    lastPlayedNote_ = -1;
}

int ChannelSynthesizer::applyTransposition(int midiNote) const {
    // Apply semitone transposition
    int transposedNote = midiNote + transposition_;
    
    // Apply fine tuning (cents)
    // Note: This would require pitch adjustment in the voice manager
    // For now we just apply semitone transposition
    
    // Clamp to valid MIDI note range
    return std::clamp(transposedNote, 0, 127);
}

} // namespace AIMusicHardware