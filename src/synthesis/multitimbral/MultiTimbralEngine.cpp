#include "../../../include/synthesis/multitimbral/MultiTimbralEngine.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace AIMusicHardware {

MultiTimbralEngine::MultiTimbralEngine(int sampleRate, int maxTotalVoices)
    : sampleRate_(sampleRate),
      maxTotalVoices_(std::max(16, maxTotalVoices)),
      voiceStrategy_(VoiceAllocationStrategy::Equal),
      masterVolume_(1.0f) {
    
    // Initialize all channel synthesizers
    for (int i = 0; i < 16; ++i) {
        channelSynths_[i] = std::make_unique<ChannelSynthesizer>(i, sampleRate_);
        channelActive_[i] = (i == 0); // Only channel 0 active by default
        channelVolumes_[i] = 1.0f;
        channelPans_[i] = 0.0f;
        channelPriorities_[i] = 1; // Default priority
    }
    
    // Initialize mixing buffer
    const int bufferSize = 1024 * 2; // Stereo buffer with reasonable size
    mixBuffer_.resize(bufferSize, 0.0f);
    
    // Allocate voices based on initial strategy
    allocateVoices();
}

MultiTimbralEngine::~MultiTimbralEngine() {
    // Nothing special needed here, smart pointers handle cleanup
}

bool MultiTimbralEngine::initialize() {
    bool success = true;
    
    // Initialize all channel synthesizers
    for (int i = 0; i < 16; ++i) {
        if (channelSynths_[i]) {
            success &= channelSynths_[i]->initialize();
        }
    }
    
    // Reset voice allocation based on initial settings
    allocateVoices();
    
    return success;
}

//----------------------------------------------------------------------
// MIDI Event Handling
//----------------------------------------------------------------------

void MultiTimbralEngine::noteOn(int midiNote, float velocity, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    // Handle keyboard splits
    if (performanceConfig_.splitEnabled) {
        if (midiNote < performanceConfig_.splitPoint) {
            // Use lower channel
            channel = performanceConfig_.lowerChannel;
        } else {
            // Use upper channel
            channel = performanceConfig_.upperChannel;
        }
        
        // Make sure the redirected channel is active
        if (!channelActive_[channel]) {
            return;
        }
    }
    
    // Handle layered channels
    if (performanceConfig_.layerEnabled) {
        // Play the note on all layered channels
        for (int layerChannel : performanceConfig_.layeredChannels) {
            if (isValidChannel(layerChannel) && channelActive_[layerChannel]) {
                ChannelSynthesizer* synth = channelSynths_[layerChannel].get();
                if (synth && synth->isNoteInRange(midiNote)) {
                    synth->noteOn(midiNote, velocity, layerChannel);
                }
            }
        }
        return; // We've handled the note in layers
    }
    
    // Standard note-on handling
    ChannelSynthesizer* synth = channelSynths_[channel].get();
    if (synth && synth->isNoteInRange(midiNote)) {
        synth->noteOn(midiNote, velocity, channel);
    }
}

void MultiTimbralEngine::noteOff(int midiNote, int channel) {
    if (!isValidChannel(channel)) {
        return;
    }
    
    // Handle keyboard splits
    if (performanceConfig_.splitEnabled) {
        if (midiNote < performanceConfig_.splitPoint) {
            // Use lower channel
            channel = performanceConfig_.lowerChannel;
        } else {
            // Use upper channel
            channel = performanceConfig_.upperChannel;
        }
    }
    
    // Handle layered channels
    if (performanceConfig_.layerEnabled) {
        // Release the note on all layered channels
        for (int layerChannel : performanceConfig_.layeredChannels) {
            if (isValidChannel(layerChannel) && channelActive_[layerChannel]) {
                channelSynths_[layerChannel]->noteOff(midiNote, layerChannel);
            }
        }
        return; // We've handled the note in layers
    }
    
    // Standard note-off handling
    if (channelActive_[channel]) {
        channelSynths_[channel]->noteOff(midiNote, channel);
    }
}

void MultiTimbralEngine::programChange(int program, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    ChannelSynthesizer* synth = channelSynths_[channel].get();
    if (synth && synth->receivesProgramChanges()) {
        synth->processChannelProgramChange(program);
    }
}

void MultiTimbralEngine::controlChange(int controller, int value, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    ChannelSynthesizer* synth = channelSynths_[channel].get();
    if (synth && synth->receivesControlChanges()) {
        synth->processChannelMidiCC(controller, value);
    }
}

void MultiTimbralEngine::pitchBend(float value, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    ChannelSynthesizer* synth = channelSynths_[channel].get();
    if (synth && synth->receivesPitchBend()) {
        synth->setPitchBend(value, channel);
    }
}

void MultiTimbralEngine::aftertouch(int note, float value, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    channelSynths_[channel]->setAftertouch(note, value, channel);
}

void MultiTimbralEngine::channelPressure(float value, int channel) {
    if (!isValidChannel(channel) || !channelActive_[channel]) {
        return;
    }
    
    channelSynths_[channel]->setChannelPressure(value, channel);
}

void MultiTimbralEngine::allNotesOff() {
    for (int i = 0; i < 16; ++i) {
        if (channelActive_[i] && channelSynths_[i]) {
            channelSynths_[i]->allNotesOff();
        }
    }
}

//----------------------------------------------------------------------
// Channel Management
//----------------------------------------------------------------------

void MultiTimbralEngine::setChannelActive(int channel, bool active) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        channelActive_[channel] = active;
        
        // If deactivating, stop all notes on this channel
        if (!active && channelSynths_[channel]) {
            channelSynths_[channel]->allNotesOff();
        }
        
        // Rebalance voice allocation
        allocateVoices();
    }
}

bool MultiTimbralEngine::isChannelActive(int channel) const {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        return channelActive_[channel];
    }
    return false;
}

void MultiTimbralEngine::setChannelVolume(int channel, float volume) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        channelVolumes_[channel] = std::clamp(volume, 0.0f, 1.0f);
    }
}

float MultiTimbralEngine::getChannelVolume(int channel) const {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        return channelVolumes_[channel];
    }
    return 0.0f;
}

void MultiTimbralEngine::setChannelPan(int channel, float pan) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        channelPans_[channel] = std::clamp(pan, -1.0f, 1.0f);
    }
}

float MultiTimbralEngine::getChannelPan(int channel) const {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(channelMutex_);
        return channelPans_[channel];
    }
    return 0.0f;
}

void MultiTimbralEngine::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
}

float MultiTimbralEngine::getMasterVolume() const {
    return masterVolume_;
}

//----------------------------------------------------------------------
// Voice Allocation
//----------------------------------------------------------------------

void MultiTimbralEngine::setMaxTotalVoices(int maxVoices) {
    std::lock_guard<std::mutex> lock(voiceAllocationMutex_);
    maxTotalVoices_ = std::max(16, maxVoices);
    allocateVoices();
}

int MultiTimbralEngine::getMaxTotalVoices() const {
    return maxTotalVoices_;
}

void MultiTimbralEngine::setVoiceAllocationStrategy(VoiceAllocationStrategy strategy) {
    if (voiceStrategy_ != strategy) {
        std::lock_guard<std::mutex> lock(voiceAllocationMutex_);
        voiceStrategy_ = strategy;
        allocateVoices();
    }
}

VoiceAllocationStrategy MultiTimbralEngine::getVoiceAllocationStrategy() const {
    return voiceStrategy_;
}

void MultiTimbralEngine::allocateVoices() {
    std::lock_guard<std::mutex> lock(voiceAllocationMutex_);
    
    // Count active channels
    int activeChannelCount = 0;
    for (int i = 0; i < 16; ++i) {
        if (channelActive_[i]) {
            activeChannelCount++;
        }
    }
    
    if (activeChannelCount == 0) {
        return; // No active channels
    }
    
    // Determine voice allocation based on strategy
    switch (voiceStrategy_) {
        case VoiceAllocationStrategy::Equal: {
            // Distribute voices equally among active channels
            int voicesPerChannel = maxTotalVoices_ / activeChannelCount;
            int remainingVoices = maxTotalVoices_ % activeChannelCount;
            
            for (int i = 0; i < 16; ++i) {
                if (channelActive_[i] && channelSynths_[i]) {
                    int channelVoices = voicesPerChannel;
                    if (remainingVoices > 0) {
                        channelVoices++;
                        remainingVoices--;
                    }
                    channelSynths_[i]->setVoiceCount(channelVoices);
                }
            }
            break;
        }
            
        case VoiceAllocationStrategy::PriorityBased: {
            // Allocate based on channel priorities
            int totalPriority = 0;
            for (int i = 0; i < 16; ++i) {
                if (channelActive_[i]) {
                    totalPriority += channelPriorities_[i];
                }
            }
            
            int remainingVoices = maxTotalVoices_;
            
            for (int i = 0; i < 16; ++i) {
                if (channelActive_[i] && channelSynths_[i]) {
                    // Calculate voices proportionally to priority
                    float priorityRatio = static_cast<float>(channelPriorities_[i]) / totalPriority;
                    int channelVoices = static_cast<int>(priorityRatio * maxTotalVoices_);
                    
                    // Ensure at least one voice per active channel
                    channelVoices = std::max(1, channelVoices);
                    
                    // Don't exceed remaining voices
                    channelVoices = std::min(channelVoices, remainingVoices);
                    remainingVoices -= channelVoices;
                    
                    channelSynths_[i]->setVoiceCount(channelVoices);
                }
            }
            
            // Distribute any remaining voices to highest priority channels
            if (remainingVoices > 0) {
                // Sort channels by priority
                std::vector<std::pair<int, int>> channelsByPriority; // (channel, priority)
                for (int i = 0; i < 16; ++i) {
                    if (channelActive_[i]) {
                        channelsByPriority.emplace_back(i, channelPriorities_[i]);
                    }
                }
                
                std::sort(channelsByPriority.begin(), channelsByPriority.end(),
                          [](const auto& a, const auto& b) { return a.second > b.second; });
                
                // Distribute remaining voices
                for (const auto& [channel, priority] : channelsByPriority) {
                    if (remainingVoices <= 0) break;
                    
                    if (channelSynths_[channel]) {
                        int currentVoices = channelSynths_[channel]->getVoiceCount();
                        channelSynths_[channel]->setVoiceCount(currentVoices + 1);
                        remainingVoices--;
                    }
                }
            }
            break;
        }
            
        case VoiceAllocationStrategy::Dynamic:
            // In a full implementation, this would use recent voice usage history
            // to dynamically allocate voices. For now, we'll use equal distribution.
            {
                int voicesPerChannel = maxTotalVoices_ / activeChannelCount;
                int remainingVoices = maxTotalVoices_ % activeChannelCount;
                
                for (int i = 0; i < 16; ++i) {
                    if (channelActive_[i] && channelSynths_[i]) {
                        int channelVoices = voicesPerChannel;
                        if (remainingVoices > 0) {
                            channelVoices++;
                            remainingVoices--;
                        }
                        channelSynths_[i]->setVoiceCount(channelVoices);
                    }
                }
            }
            break;
    }
}

void MultiTimbralEngine::setChannelPriority(int channel, int priority) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(voiceAllocationMutex_);
        channelPriorities_[channel] = std::max(1, priority);
        
        // Update voice allocation if using priority-based strategy
        if (voiceStrategy_ == VoiceAllocationStrategy::PriorityBased) {
            allocateVoices();
        }
        
        // Update channel synth's internal priority setting
        if (channelSynths_[channel]) {
            channelSynths_[channel]->setChannelPriority(priority);
        }
    }
}

int MultiTimbralEngine::getChannelPriority(int channel) const {
    if (isValidChannel(channel)) {
        return channelPriorities_[channel];
    }
    return 0;
}

//----------------------------------------------------------------------
// Preset Management
//----------------------------------------------------------------------

void MultiTimbralEngine::loadPreset(const class Preset& preset, int channel) {
    // Placeholder until we have access to the Preset class implementation
    // This would use the channel synthesizer's preset loading methods
    if (isValidChannel(channel) && channelSynths_[channel]) {
        // In a real implementation:
        // channelSynths_[channel]->loadPreset(preset);
    }
}

bool MultiTimbralEngine::loadPresetFromFile(const std::string& filePath, int channel) {
    // Placeholder until we have file loading implemented
    if (isValidChannel(channel) && channelSynths_[channel]) {
        // In a real implementation:
        // return channelSynths_[channel]->loadPresetFromFile(filePath);
    }
    return false;
}

std::string MultiTimbralEngine::getChannelPresetName(int channel) const {
    if (isValidChannel(channel) && channelSynths_[channel]) {
        return channelSynths_[channel]->getPresetName();
    }
    return "";
}

//----------------------------------------------------------------------
// Audio Processing
//----------------------------------------------------------------------

void MultiTimbralEngine::process(float* outputBuffer, int numFrames) {
    // Ensure buffer is large enough
    if (mixBuffer_.size() < static_cast<size_t>(numFrames * 2)) {
        mixBuffer_.resize(numFrames * 2, 0.0f);
    }
    
    // Clear output buffer
    std::fill(outputBuffer, outputBuffer + numFrames * 2, 0.0f);
    
    // Process and mix all active channels
    mixChannels(outputBuffer, numFrames);
    
    // Apply master volume
    if (masterVolume_ != 1.0f) {
        for (int i = 0; i < numFrames * 2; ++i) {
            outputBuffer[i] *= masterVolume_;
        }
    }
}

void MultiTimbralEngine::setSampleRate(int sampleRate) {
    if (sampleRate_ != sampleRate) {
        sampleRate_ = sampleRate;
        
        // Update all channel synthesizers
        for (int i = 0; i < 16; ++i) {
            if (channelSynths_[i]) {
                channelSynths_[i]->setSampleRate(sampleRate);
            }
        }
    }
}

void MultiTimbralEngine::reset() {
    // Stop all notes and reset all synthesizers
    for (int i = 0; i < 16; ++i) {
        if (channelSynths_[i]) {
            channelSynths_[i]->allNotesOff();
            channelSynths_[i]->reset();
        }
    }
}

//----------------------------------------------------------------------
// Channel Access
//----------------------------------------------------------------------

ChannelSynthesizer* MultiTimbralEngine::getChannelSynth(int channel) {
    if (isValidChannel(channel)) {
        return channelSynths_[channel].get();
    }
    return nullptr;
}

//----------------------------------------------------------------------
// Performance Modes
//----------------------------------------------------------------------

void MultiTimbralEngine::setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel) {
    if (isValidChannel(lowerChannel) && isValidChannel(upperChannel)) {
        performanceConfig_.splitEnabled = true;
        performanceConfig_.splitPoint = std::clamp(splitPoint, 0, 127);
        performanceConfig_.lowerChannel = lowerChannel;
        performanceConfig_.upperChannel = upperChannel;
        
        // Ensure both channels are active
        setChannelActive(lowerChannel, true);
        setChannelActive(upperChannel, true);
        
        // Set note ranges for visual feedback
        if (channelSynths_[lowerChannel]) {
            channelSynths_[lowerChannel]->setNoteRange(0, splitPoint - 1);
        }
        if (channelSynths_[upperChannel]) {
            channelSynths_[upperChannel]->setNoteRange(splitPoint, 127);
        }
        
        // Disable layering when using splits
        performanceConfig_.layerEnabled = false;
        performanceConfig_.layeredChannels.clear();
    }
}

void MultiTimbralEngine::setupLayeredChannels(const std::vector<int>& channels) {
    if (channels.empty()) {
        performanceConfig_.layerEnabled = false;
        performanceConfig_.layeredChannels.clear();
        return;
    }
    
    // Validate channels
    bool allValid = true;
    for (int channel : channels) {
        if (!isValidChannel(channel)) {
            allValid = false;
            break;
        }
    }
    
    if (allValid) {
        performanceConfig_.layerEnabled = true;
        performanceConfig_.layeredChannels = channels;
        
        // Activate all channels in the layer
        for (int channel : channels) {
            setChannelActive(channel, true);
        }
        
        // Disable split when using layers
        performanceConfig_.splitEnabled = false;
    }
}

void MultiTimbralEngine::clearPerformanceConfig() {
    performanceConfig_.splitEnabled = false;
    performanceConfig_.layerEnabled = false;
    performanceConfig_.layeredChannels.clear();
    
    // Reset note ranges for all channels
    for (int i = 0; i < 16; ++i) {
        if (channelSynths_[i]) {
            channelSynths_[i]->setNoteRange(0, 127);
        }
    }
}

//----------------------------------------------------------------------
// Private Methods
//----------------------------------------------------------------------

bool MultiTimbralEngine::isValidChannel(int channel) const {
    return channel >= 0 && channel < 16;
}

void MultiTimbralEngine::applyPanning(float* buffer, int numFrames, float pan) {
    // Apply equal power panning
    float leftGain = std::cos((pan + 1.0f) * M_PI / 4.0f);
    float rightGain = std::sin((pan + 1.0f) * M_PI / 4.0f);
    
    for (int i = 0; i < numFrames; ++i) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];
        
        buffer[i * 2] = left * leftGain;
        buffer[i * 2 + 1] = right * rightGain;
    }
}

void MultiTimbralEngine::mixChannels(float* outputBuffer, int numFrames) {
    // Ensure mix buffer is large enough
    if (mixBuffer_.size() < static_cast<size_t>(numFrames * 2)) {
        mixBuffer_.resize(numFrames * 2, 0.0f);
    }
    
    // Count active channels for dynamic gain adjustment
    int activeChannels = 0;
    for (int i = 0; i < 16; ++i) {
        if (channelActive_[i] && channelSynths_[i]) {
            activeChannels++;
        }
    }
    
    // Only apply gain adjustment if we have multiple active channels
    float mixGain = (activeChannels > 1) ? 1.0f / std::sqrt(static_cast<float>(activeChannels)) : 1.0f;
    
    // Process each active channel
    for (int i = 0; i < 16; ++i) {
        if (channelActive_[i] && channelSynths_[i]) {
            // Clear mix buffer
            std::fill(mixBuffer_.begin(), mixBuffer_.begin() + numFrames * 2, 0.0f);
            
            // Process this channel
            channelSynths_[i]->process(mixBuffer_.data(), numFrames);
            
            // Apply channel volume
            float volume = channelVolumes_[i] * mixGain;
            if (volume != 1.0f) {
                for (int j = 0; j < numFrames * 2; ++j) {
                    mixBuffer_[j] *= volume;
                }
            }
            
            // Apply channel panning
            if (channelPans_[i] != 0.0f) {
                applyPanning(mixBuffer_.data(), numFrames, channelPans_[i]);
            }
            
            // Mix into output buffer
            for (int j = 0; j < numFrames * 2; ++j) {
                outputBuffer[j] += mixBuffer_[j];
            }
        }
    }
}

} // namespace AIMusicHardware