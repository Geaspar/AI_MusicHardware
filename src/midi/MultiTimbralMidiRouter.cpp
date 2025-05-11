#include "../../include/midi/MultiTimbralMidiRouter.h"
#include "../../include/synthesis/multitimbral/MultiTimbralEngine.h"
#include <algorithm>
#include <iostream>

namespace AIMusicHardware {

MultiTimbralMidiRouter::MultiTimbralMidiRouter(MultiTimbralEngine* multiTimbralEngine)
    : multiTimbralEngine_(multiTimbralEngine),
      midiInput_(std::make_unique<MidiInput>()),
      operationMode_(OperationMode::Standard),
      mpeConfig_(),
      mpeChannelAllocator_(std::make_unique<MpeChannelAllocator>(mpeConfig_)) {
    
    // Initialize all channels as enabled by default
    channelEnabled_.fill(true);
    
    // Initialize RPN states
    for (auto& state : rpnStates_) {
        state.active = false;
        state.msb = 0;
        state.lsb = 0;
    }
}

MultiTimbralMidiRouter::~MultiTimbralMidiRouter() {
    // Ensure MIDI devices are closed properly
    closeMidiInput();
}

void MultiTimbralMidiRouter::handleIncomingMidiMessage(const MidiMessage& message) {
    processMidiMessage(message);
}

void MultiTimbralMidiRouter::processMidiMessage(const MidiMessage& message) {
    if (!multiTimbralEngine_) {
        return;
    }
    
    // Choose processing path based on operation mode
    if (operationMode_ == OperationMode::Mpe) {
        // MPE mode processing
        switch (message.type) {
            case MidiMessage::Type::NoteOn:
                processMpeNoteOn(message);
                break;
                
            case MidiMessage::Type::NoteOff:
                processMpeNoteOff(message);
                break;
                
            case MidiMessage::Type::ControlChange:
                // Special CC handling for MPE
                if (message.data1 == 74) {  // CC 74 = Timbre (Y-axis)
                    processMpeTimbre(message);
                } else {
                    processMpeControlChange(message);
                }
                break;
                
            case MidiMessage::Type::PitchBend:
                processMpePitchBend(message);
                break;
                
            case MidiMessage::Type::ChannelPressure:
                processMpePressure(message);
                break;
                
            case MidiMessage::Type::ProgramChange:
                processMpeProgramChange(message);
                break;
                
            default:
                // Other message types not handled
                break;
        }
    } else {
        // Standard multi-timbral mode processing
        switch (message.type) {
            case MidiMessage::Type::NoteOn:
                processNoteOn(message);
                break;
                
            case MidiMessage::Type::NoteOff:
                processNoteOff(message);
                break;
                
            case MidiMessage::Type::ControlChange:
                processControlChange(message);
                break;
                
            case MidiMessage::Type::PitchBend:
                processPitchBend(message);
                break;
                
            case MidiMessage::Type::AfterTouch:
                processAfterTouch(message);
                break;
                
            case MidiMessage::Type::ChannelPressure:
                processChannelPressure(message);
                break;
                
            case MidiMessage::Type::ProgramChange:
                processProgramChange(message);
                break;
                
            default:
                // Other message types not handled
                break;
        }
    }
}

//----------------------------------------------------------------------
// Standard Multi-timbral Message Processors
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::processNoteOn(const MidiMessage& message) {
    int channel = message.channel;
    int note = message.data1;
    int velocity = message.data2;
    
    // Skip if velocity is 0 (Note Off) or channel is disabled
    if (velocity == 0 || !isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: NoteOn - Ch: " << (channel + 1) 
                  << ", Note: " << note << ", Vel: " << velocity << std::endl;
    }
    
    // Handle layered mode
    if (layerConfig_.enabled) {
        for (int layerChannel : layerConfig_.channels) {
            if (isChannelEnabled(layerChannel)) {
                multiTimbralEngine_->noteOn(note, velocity / 127.0f, layerChannel);
                
                if (debugMode_) {
                    std::cout << "  Layer Routing: Ch: " << (layerChannel + 1) << std::endl;
                }
            }
        }
        return;
    }
    
    // Handle keyboard split and velocity split
    int effectiveChannel = getEffectiveChannel(channel, note, velocity);
    
    if (effectiveChannel != channel && debugMode_) {
        std::cout << "  Routed to Ch: " << (effectiveChannel + 1) << std::endl;
    }
    
    // Pass to engine with normalized velocity
    multiTimbralEngine_->noteOn(note, velocity / 127.0f, effectiveChannel);
}

void MultiTimbralMidiRouter::processNoteOff(const MidiMessage& message) {
    int channel = message.channel;
    int note = message.data1;
    
    if (debugMode_) {
        std::cout << "MIDI IN: NoteOff - Ch: " << (channel + 1) 
                  << ", Note: " << note << std::endl;
    }
    
    // Handle layered mode
    if (layerConfig_.enabled) {
        for (int layerChannel : layerConfig_.channels) {
            if (isChannelEnabled(layerChannel)) {
                multiTimbralEngine_->noteOff(note, layerChannel);
                
                if (debugMode_) {
                    std::cout << "  Layer Routing: Ch: " << (layerChannel + 1) << std::endl;
                }
            }
        }
        return;
    }
    
    // Handle keyboard split
    int effectiveChannel = getEffectiveChannel(channel, note, 64);
    
    if (effectiveChannel != channel && debugMode_) {
        std::cout << "  Routed to Ch: " << (effectiveChannel + 1) << std::endl;
    }
    
    multiTimbralEngine_->noteOff(note, effectiveChannel);
}

void MultiTimbralMidiRouter::processControlChange(const MidiMessage& message) {
    int channel = message.channel;
    int controller = message.data1;
    int value = message.data2;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: CC - Ch: " << (channel + 1) 
                  << ", CC: " << controller << ", Value: " << value << std::endl;
    }
    
    // Check for RPN messages
    if (controller == 101) {  // RPN MSB
        rpnStates_[channel].active = true;
        rpnStates_[channel].msb = value;
        return;
    } else if (controller == 100) {  // RPN LSB
        rpnStates_[channel].active = true;
        rpnStates_[channel].lsb = value;
        return;
    } else if (controller == 6 && rpnStates_[channel].active) {  // Data Entry MSB
        // Process RPN
        handleRpnMessage(channel, rpnStates_[channel].msb, rpnStates_[channel].lsb, value, 0);
        return;
    } else if (controller == 38 && rpnStates_[channel].active) {  // Data Entry LSB
        // We don't use LSB here, but we could for more precision
        return;
    } else if (controller == 127) {  // RPN Null - deactivate RPN
        rpnStates_[channel].active = false;
        return;
    }
    
    // Always route controller messages to the original channel
    // This keeps channel-specific controllers working correctly
    multiTimbralEngine_->controlChange(controller, value, channel);
}

void MultiTimbralMidiRouter::processPitchBend(const MidiMessage& message) {
    int channel = message.channel;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    // Combine LSB and MSB for full 14-bit resolution
    int combined = message.data1 | (message.data2 << 7);
    
    // Convert from 0-16383 to -1.0 to +1.0
    float normalizedValue = (combined / 8192.0f) - 1.0f;
    
    if (debugMode_) {
        std::cout << "MIDI IN: Pitch Bend - Ch: " << (channel + 1) 
                  << ", Value: " << normalizedValue << std::endl;
    }
    
    multiTimbralEngine_->pitchBend(normalizedValue, channel);
}

void MultiTimbralMidiRouter::processAfterTouch(const MidiMessage& message) {
    int channel = message.channel;
    int note = message.data1;
    int pressure = message.data2;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: Aftertouch - Ch: " << (channel + 1) 
                  << ", Note: " << note << ", Pressure: " << pressure << std::endl;
    }
    
    multiTimbralEngine_->aftertouch(note, pressure / 127.0f, channel);
}

void MultiTimbralMidiRouter::processChannelPressure(const MidiMessage& message) {
    int channel = message.channel;
    int pressure = message.data1;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: Channel Pressure - Ch: " << (channel + 1) 
                  << ", Pressure: " << pressure << std::endl;
    }
    
    multiTimbralEngine_->channelPressure(pressure / 127.0f, channel);
}

void MultiTimbralMidiRouter::processProgramChange(const MidiMessage& message) {
    int channel = message.channel;
    int program = message.data1;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: Program Change - Ch: " << (channel + 1) 
                  << ", Program: " << program << std::endl;
    }
    
    // Check if this program change is remapped
    std::lock_guard<std::mutex> lock(routingMutex_);
    auto channelIt = programChangeMap_.find(channel);
    if (channelIt != programChangeMap_.end()) {
        auto programIt = channelIt->second.find(program);
        if (programIt != channelIt->second.end()) {
            int mappedProgram = programIt->second;
            
            if (debugMode_) {
                std::cout << "  Mapped to Program: " << mappedProgram << std::endl;
            }
            
            multiTimbralEngine_->programChange(mappedProgram, channel);
            return;
        }
    }
    
    // No mapping found, use original
    multiTimbralEngine_->programChange(program, channel);
}

void MultiTimbralMidiRouter::processAllNotesOff(const MidiMessage& message) {
    int channel = message.channel;
    
    if (!isChannelEnabled(channel)) {
        return;
    }
    
    if (debugMode_) {
        std::cout << "MIDI IN: All Notes Off - Ch: " << (channel + 1) << std::endl;
    }
    
    // In layered mode, send to all layered channels
    if (layerConfig_.enabled) {
        for (int layerChannel : layerConfig_.channels) {
            if (isChannelEnabled(layerChannel)) {
                multiTimbralEngine_->allNotesOff();
            }
        }
        return;
    }
    
    // Otherwise, just send to the original channel
    multiTimbralEngine_->allNotesOff();
}

//----------------------------------------------------------------------
// MPE-specific Message Processors
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::processMpeNoteOn(const MidiMessage& message) {
    int channel = message.channel;
    int note = message.data1;
    int velocity = message.data2;
    
    if (velocity == 0) {
        // Note On with velocity 0 is equivalent to Note Off
        processMpeNoteOff(message);
        return;
    }
    
    if (debugMode_) {
        std::cout << "MPE IN: NoteOn - Ch: " << (channel + 1) 
                  << ", Note: " << note << ", Vel: " << velocity << std::endl;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel notes are handled directly
        multiTimbralEngine_->noteOn(note, velocity / 127.0f, channel);
        
        if (debugMode_) {
            std::cout << "  Master Channel: Direct routing" << std::endl;
        }
        return;
    }
    
    // Determine the zone for this channel
    bool isLowerZone = mpeConfig_.isInLowerZone(channel);
    
    // For member channels, allocate a proper channel
    int allocatedChannel = mpeChannelAllocator_->allocateChannel(note, velocity, isLowerZone);
    
    if (allocatedChannel >= 0) {
        // Use the allocated channel
        if (debugMode_) {
            std::cout << "  Allocated channel: " << (allocatedChannel + 1) << std::endl;
        }
        
        // Send to engine with expression values
        multiTimbralEngine_->noteOn(note, velocity / 127.0f, allocatedChannel);
    } else {
        // Fallback to direct routing if allocation failed
        if (debugMode_) {
            std::cout << "  Channel allocation failed, using direct routing" << std::endl;
        }
        
        multiTimbralEngine_->noteOn(note, velocity / 127.0f, channel);
    }
}

void MultiTimbralMidiRouter::processMpeNoteOff(const MidiMessage& message) {
    int channel = message.channel;
    int note = message.data1;
    
    if (debugMode_) {
        std::cout << "MPE IN: NoteOff - Ch: " << (channel + 1) 
                  << ", Note: " << note << std::endl;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel notes are handled directly
        multiTimbralEngine_->noteOff(note, channel);
        
        if (debugMode_) {
            std::cout << "  Master Channel: Direct routing" << std::endl;
        }
        return;
    }
    
    // Find the allocated channel for this note
    int allocatedChannel = mpeChannelAllocator_->getChannelForNote(note);
    
    if (allocatedChannel >= 0) {
        // Use the allocated channel
        if (debugMode_) {
            std::cout << "  Found allocated channel: " << (allocatedChannel + 1) << std::endl;
        }
        
        // Send to engine
        multiTimbralEngine_->noteOff(note, allocatedChannel);
        
        // Release the channel
        mpeChannelAllocator_->releaseChannel(allocatedChannel);
    } else {
        // Fallback to direct routing if no allocation found
        if (debugMode_) {
            std::cout << "  No channel allocation found, using direct routing" << std::endl;
        }
        
        multiTimbralEngine_->noteOff(note, channel);
    }
}

void MultiTimbralMidiRouter::processMpePitchBend(const MidiMessage& message) {
    int channel = message.channel;
    
    // Combine LSB and MSB for full 14-bit resolution
    int combined = message.data1 | (message.data2 << 7);
    
    // Convert from 0-16383 to -1.0 to +1.0
    float normalizedValue = (combined / 8192.0f) - 1.0f;
    
    if (debugMode_) {
        std::cout << "MPE IN: Pitch Bend - Ch: " << (channel + 1) 
                  << ", Value: " << normalizedValue << std::endl;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel pitch bend affects all notes in the zone
        bool isLowerZone = (channel == 0);  // Channel 1 (0-based) is Lower Zone
        const MpeConfiguration::Zone& zone = isLowerZone ? 
            mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
        
        // Apply to all channels in this zone
        if (isLowerZone) {
            for (int ch = zone.startMemberChannel - 1; ch <= zone.endMemberChannel - 1; ch++) {
                multiTimbralEngine_->pitchBend(normalizedValue, ch);
            }
        } else {
            for (int ch = zone.startMemberChannel - 1; ch >= zone.endMemberChannel - 1; ch--) {
                multiTimbralEngine_->pitchBend(normalizedValue, ch);
            }
        }
        
        // Also apply to the master channel itself
        multiTimbralEngine_->pitchBend(normalizedValue, channel);
    } else {
        // Member channel pitch bend affects only this channel
        
        // Update expression for this channel
        auto& state = mpeChannelAllocator_->getChannelState(channel);
        state.pitchBend = normalizedValue;
        
        // Send to engine
        multiTimbralEngine_->pitchBend(normalizedValue, channel);
    }
}

void MultiTimbralMidiRouter::processMpeTimbre(const MidiMessage& message) {
    int channel = message.channel;
    int value = message.data2;
    
    // Normalize timbre value (0-127) to 0.0-1.0
    float normalizedValue = value / 127.0f;
    
    if (debugMode_) {
        std::cout << "MPE IN: Timbre (CC74) - Ch: " << (channel + 1) 
                  << ", Value: " << normalizedValue << std::endl;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel timbre affects all notes in the zone
        bool isLowerZone = (channel == 0);  // Channel 1 (0-based) is Lower Zone
        const MpeConfiguration::Zone& zone = isLowerZone ? 
            mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
        
        // Apply to all channels in this zone
        if (isLowerZone) {
            for (int ch = zone.startMemberChannel - 1; ch <= zone.endMemberChannel - 1; ch++) {
                // This would be a special method in our enhanced engine
                // For now, we use CC as a fallback
                multiTimbralEngine_->controlChange(74, value, ch);
            }
        } else {
            for (int ch = zone.startMemberChannel - 1; ch >= zone.endMemberChannel - 1; ch--) {
                multiTimbralEngine_->controlChange(74, value, ch);
            }
        }
        
        // Also apply to the master channel itself
        multiTimbralEngine_->controlChange(74, value, channel);
    } else {
        // Member channel timbre affects only this channel
        
        // Update expression for this channel
        auto& state = mpeChannelAllocator_->getChannelState(channel);
        state.timbre = normalizedValue;
        
        // Send to engine
        multiTimbralEngine_->controlChange(74, value, channel);
    }
}

void MultiTimbralMidiRouter::processMpePressure(const MidiMessage& message) {
    int channel = message.channel;
    int pressure = message.data1;
    
    // Normalize pressure value (0-127) to 0.0-1.0
    float normalizedValue = pressure / 127.0f;
    
    if (debugMode_) {
        std::cout << "MPE IN: Channel Pressure - Ch: " << (channel + 1) 
                  << ", Value: " << normalizedValue << std::endl;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel pressure affects all notes in the zone
        bool isLowerZone = (channel == 0);  // Channel 1 (0-based) is Lower Zone
        const MpeConfiguration::Zone& zone = isLowerZone ? 
            mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
        
        // Apply to all channels in this zone
        if (isLowerZone) {
            for (int ch = zone.startMemberChannel - 1; ch <= zone.endMemberChannel - 1; ch++) {
                multiTimbralEngine_->channelPressure(normalizedValue, ch);
            }
        } else {
            for (int ch = zone.startMemberChannel - 1; ch >= zone.endMemberChannel - 1; ch--) {
                multiTimbralEngine_->channelPressure(normalizedValue, ch);
            }
        }
        
        // Also apply to the master channel itself
        multiTimbralEngine_->channelPressure(normalizedValue, channel);
    } else {
        // Member channel pressure affects only this channel
        
        // Update expression for this channel
        auto& state = mpeChannelAllocator_->getChannelState(channel);
        state.pressure = normalizedValue;
        
        // Send to engine
        multiTimbralEngine_->channelPressure(normalizedValue, channel);
    }
}

void MultiTimbralMidiRouter::processMpeControlChange(const MidiMessage& message) {
    int channel = message.channel;
    int controller = message.data1;
    int value = message.data2;
    
    if (debugMode_) {
        std::cout << "MPE IN: CC - Ch: " << (channel + 1) 
                  << ", CC: " << controller << ", Value: " << value << std::endl;
    }
    
    // Check for RPN messages
    if (controller == 101) {  // RPN MSB
        rpnStates_[channel].active = true;
        rpnStates_[channel].msb = value;
        return;
    } else if (controller == 100) {  // RPN LSB
        rpnStates_[channel].active = true;
        rpnStates_[channel].lsb = value;
        return;
    } else if (controller == 6 && rpnStates_[channel].active) {  // Data Entry MSB
        // Process RPN
        handleRpnMessage(channel, rpnStates_[channel].msb, rpnStates_[channel].lsb, value, 0);
        return;
    } else if (controller == 38 && rpnStates_[channel].active) {  // Data Entry LSB
        // We don't use LSB here, but we could for more precision
        return;
    } else if (controller == 127) {  // RPN Null - deactivate RPN
        rpnStates_[channel].active = false;
        return;
    }
    
    // Check if this is a master channel or member channel
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel CCs affect all notes in the zone
        bool isLowerZone = (channel == 0);  // Channel 1 (0-based) is Lower Zone
        const MpeConfiguration::Zone& zone = isLowerZone ? 
            mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
        
        // Apply to all channels in this zone
        if (isLowerZone) {
            for (int ch = zone.startMemberChannel - 1; ch <= zone.endMemberChannel - 1; ch++) {
                multiTimbralEngine_->controlChange(controller, value, ch);
            }
        } else {
            for (int ch = zone.startMemberChannel - 1; ch >= zone.endMemberChannel - 1; ch--) {
                multiTimbralEngine_->controlChange(controller, value, ch);
            }
        }
        
        // Also apply to the master channel itself
        multiTimbralEngine_->controlChange(controller, value, channel);
    } else {
        // Member channel CCs affect only this channel
        multiTimbralEngine_->controlChange(controller, value, channel);
    }
}

void MultiTimbralMidiRouter::processMpeProgramChange(const MidiMessage& message) {
    int channel = message.channel;
    int program = message.data1;
    
    if (debugMode_) {
        std::cout << "MPE IN: Program Change - Ch: " << (channel + 1) 
                  << ", Program: " << program << std::endl;
    }
    
    // In MPE, program changes should only be sent on master channels
    if (mpeConfig_.isMasterChannel(channel)) {
        // Master channel program change affects all notes in the zone
        bool isLowerZone = (channel == 0);  // Channel 1 (0-based) is Lower Zone
        const MpeConfiguration::Zone& zone = isLowerZone ? 
            mpeConfig_.getLowerZone() : mpeConfig_.getUpperZone();
        
        // Apply to all channels in this zone
        if (isLowerZone) {
            for (int ch = zone.startMemberChannel - 1; ch <= zone.endMemberChannel - 1; ch++) {
                multiTimbralEngine_->programChange(program, ch);
            }
        } else {
            for (int ch = zone.startMemberChannel - 1; ch >= zone.endMemberChannel - 1; ch--) {
                multiTimbralEngine_->programChange(program, ch);
            }
        }
        
        // Also apply to the master channel itself
        multiTimbralEngine_->programChange(program, channel);
    } else {
        // Member channel program change affects only this channel
        multiTimbralEngine_->programChange(program, channel);
    }
}

//----------------------------------------------------------------------
// RPN Message Handling
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::handleRpnMessage(int channel, int rpnMsb, int rpnLsb, 
                                            int dataMsb, int dataLsb) {
    // Check if this is MPE Configuration message (RPN 0x6)
    if (rpnMsb == 0x00 && rpnLsb == 0x06) {
        // This is MPE Configuration
        int memberChannels = dataMsb;
        
        if (debugMode_) {
            std::cout << "MPE Configuration RPN - Ch: " << (channel + 1) 
                      << ", Member Channels: " << memberChannels << std::endl;
        }
        
        if (channel == 0) {  // Channel 1 (0-based)
            // Configure Lower Zone
            configureMpeLowerZone(memberChannels > 0, memberChannels);
        } else if (channel == 15) {  // Channel 16 (0-based)
            // Configure Upper Zone
            configureMpeUpperZone(memberChannels > 0, memberChannels);
        }
    }
}

//----------------------------------------------------------------------
// MIDI Device Setup
//----------------------------------------------------------------------

bool MultiTimbralMidiRouter::openMidiInput(int deviceIndex) {
    if (midiInput_->openDevice(deviceIndex)) {
        midiInput_->setCallback(this);
        return true;
    }
    return false;
}

void MultiTimbralMidiRouter::closeMidiInput() {
    midiInput_->closeDevice();
}

std::vector<std::string> MultiTimbralMidiRouter::getMidiInputDevices() const {
    return midiInput_->getDevices();
}

//----------------------------------------------------------------------
// Operation Mode Control
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::setOperationMode(OperationMode mode) {
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    if (operationMode_ != mode) {
        operationMode_ = mode;
        
        if (mode == OperationMode::Mpe) {
            // Reset MPE channel allocator when entering MPE mode
            mpeChannelAllocator_->reset();
        }
    }
}

MultiTimbralMidiRouter::OperationMode MultiTimbralMidiRouter::getOperationMode() const {
    std::lock_guard<std::mutex> lock(routingMutex_);
    return operationMode_;
}

bool MultiTimbralMidiRouter::isMpeMode() const {
    std::lock_guard<std::mutex> lock(routingMutex_);
    return operationMode_ == OperationMode::Mpe;
}

//----------------------------------------------------------------------
// MPE Configuration
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::configureMpeLowerZone(bool active, int memberChannels) {
    mpeConfig_.setLowerZone(active, memberChannels);
    
    if (active) {
        // Automatically switch to MPE mode when activating a zone
        setOperationMode(OperationMode::Mpe);
    } else if (!mpeConfig_.isActive()) {
        // Switch back to standard mode if no zones are active
        setOperationMode(OperationMode::Standard);
    }
}

void MultiTimbralMidiRouter::configureMpeUpperZone(bool active, int memberChannels) {
    mpeConfig_.setUpperZone(active, memberChannels);
    
    if (active) {
        // Automatically switch to MPE mode when activating a zone
        setOperationMode(OperationMode::Mpe);
    } else if (!mpeConfig_.isActive()) {
        // Switch back to standard mode if no zones are active
        setOperationMode(OperationMode::Standard);
    }
}

const MpeConfiguration& MultiTimbralMidiRouter::getMpeConfiguration() const {
    return mpeConfig_;
}

const std::array<MpeChannelAllocator::ChannelState, 16>& MultiTimbralMidiRouter::getMpeChannelStates() const {
    return mpeChannelAllocator_->getAllChannelStates();
}

//----------------------------------------------------------------------
// Standard Multi-timbral Channel Management
//----------------------------------------------------------------------

void MultiTimbralMidiRouter::setChannelEnabled(int channel, bool enabled) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(routingMutex_);
        channelEnabled_[channel] = enabled;
    }
}

bool MultiTimbralMidiRouter::isChannelEnabled(int channel) const {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(routingMutex_);
        return channelEnabled_[channel];
    }
    return false;
}

void MultiTimbralMidiRouter::setupKeyboardSplit(int splitPoint, int lowerChannel, int upperChannel) {
    if (!isValidChannel(lowerChannel) || !isValidChannel(upperChannel)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    keySplitConfig_.enabled = true;
    keySplitConfig_.splitPoint = std::clamp(splitPoint, 0, 127);
    keySplitConfig_.lowerChannel = lowerChannel;
    keySplitConfig_.upperChannel = upperChannel;
    
    // Disable layering when using splits
    layerConfig_.enabled = false;
    velocitySplitConfig_.enabled = false;
}

MultiTimbralMidiRouter::KeyboardSplitConfig MultiTimbralMidiRouter::getKeyboardSplitConfig() const {
    std::lock_guard<std::mutex> lock(routingMutex_);
    return keySplitConfig_;
}

void MultiTimbralMidiRouter::setupLayeredChannels(const std::vector<int>& channels) {
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    // Validate all channels
    for (int channel : channels) {
        if (!isValidChannel(channel)) {
            return;
        }
    }
    
    layerConfig_.enabled = !channels.empty();
    layerConfig_.channels = channels;
    
    // Disable splits when using layers
    keySplitConfig_.enabled = false;
    velocitySplitConfig_.enabled = false;
}

MultiTimbralMidiRouter::LayerConfig MultiTimbralMidiRouter::getLayerConfig() const {
    std::lock_guard<std::mutex> lock(routingMutex_);
    return layerConfig_;
}

void MultiTimbralMidiRouter::clearPerformanceConfig() {
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    keySplitConfig_.enabled = false;
    layerConfig_.enabled = false;
    velocitySplitConfig_.enabled = false;
}

void MultiTimbralMidiRouter::setupVelocitySplit(int threshold, int lowerChannel, int upperChannel) {
    if (!isValidChannel(lowerChannel) || !isValidChannel(upperChannel)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    velocitySplitConfig_.enabled = true;
    velocitySplitConfig_.threshold = std::clamp(threshold, 1, 127);
    velocitySplitConfig_.lowerChannel = lowerChannel;
    velocitySplitConfig_.upperChannel = upperChannel;
    
    // Disable other routing modes
    keySplitConfig_.enabled = false;
    layerConfig_.enabled = false;
}

void MultiTimbralMidiRouter::setProgramChangeMapping(int channel, int sourceProgram, int targetProgram) {
    if (!isValidChannel(channel) || sourceProgram < 0 || sourceProgram > 127 || 
        targetProgram < 0 || targetProgram > 127) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(routingMutex_);
    programChangeMap_[channel][sourceProgram] = targetProgram;
}

void MultiTimbralMidiRouter::clearProgramChangeMappings(int channel) {
    if (isValidChannel(channel)) {
        std::lock_guard<std::mutex> lock(routingMutex_);
        programChangeMap_.erase(channel);
    }
}

//----------------------------------------------------------------------
// Private Methods
//----------------------------------------------------------------------

int MultiTimbralMidiRouter::getEffectiveChannel(int channel, int note, int velocity) {
    std::lock_guard<std::mutex> lock(routingMutex_);
    
    // Start with the original channel
    int effectiveChannel = channel;
    
    // Apply keyboard split routing if enabled
    if (keySplitConfig_.enabled) {
        if (note < keySplitConfig_.splitPoint) {
            effectiveChannel = keySplitConfig_.lowerChannel;
        } else {
            effectiveChannel = keySplitConfig_.upperChannel;
        }
    }
    
    // Apply velocity split routing if enabled
    if (velocitySplitConfig_.enabled) {
        if (velocity < velocitySplitConfig_.threshold) {
            effectiveChannel = velocitySplitConfig_.lowerChannel;
        } else {
            effectiveChannel = velocitySplitConfig_.upperChannel;
        }
    }
    
    // Check if the resulting channel is enabled
    if (!channelEnabled_[effectiveChannel]) {
        // If disabled, fall back to the original channel
        return channel;
    }
    
    return effectiveChannel;
}

bool MultiTimbralMidiRouter::isValidChannel(int channel) const {
    return channel >= 0 && channel < 16;
}

// Expression scaling utilities

float MultiTimbralMidiRouter::scalePitchBend(int rawValue) const {
    // Scale 14-bit pitch bend (0-16383) to -1.0 to 1.0
    return (rawValue / 8192.0f) - 1.0f;
}

float MultiTimbralMidiRouter::scaleTimbre(int rawValue) const {
    // Scale 7-bit CC value (0-127) to 0.0 to 1.0
    return rawValue / 127.0f;
}

float MultiTimbralMidiRouter::scalePressure(int rawValue) const {
    // Scale 7-bit pressure value (0-127) to 0.0 to 1.0
    return rawValue / 127.0f;
}

} // namespace AIMusicHardware