#include "../../include/midi/MidiInterface.h"

namespace AIMusicHardware {

// MidiInput implementation
MidiInput::MidiInput() : callback_(nullptr) {
}

MidiInput::~MidiInput() {
}

void MidiInput::setCallback(MidiCallback* callback) {
    callback_ = callback;
}

// MidiOutput implementation
MidiOutput::MidiOutput() {
}

MidiOutput::~MidiOutput() {
}

void MidiOutput::sendNoteOn(int channel, int note, int velocity) {
    // Stub implementation - will be expanded with actual MIDI output
}

void MidiOutput::sendNoteOff(int channel, int note) {
    // Stub implementation - will be expanded with actual MIDI output
}

// MidiHandler implementation
MidiHandler::MidiHandler() {
}

MidiHandler::~MidiHandler() {
}

void MidiHandler::handleMidiMessage(const MidiMessage& message) {
    // Process MIDI message
    if (message.isNoteOn() && noteOnCallback_) {
        noteOnCallback_(message.getChannel(), message.getNote(), message.getVelocity());
    }
    else if (message.isNoteOff() && noteOffCallback_) {
        noteOffCallback_(message.getChannel(), message.getNote());
    }
}

void MidiHandler::setNoteOnCallback(NoteOnCallback callback) {
    noteOnCallback_ = callback;
}

void MidiHandler::setNoteOffCallback(NoteOffCallback callback) {
    noteOffCallback_ = callback;
}

// MidiMessage implementation
MidiMessage::MidiMessage(uint8_t status, uint8_t data1, uint8_t data2)
    : status_(status), data1_(data1), data2_(data2) {
}

bool MidiMessage::isNoteOn() const {
    return (status_ & 0xF0) == 0x90 && data2_ > 0;
}

bool MidiMessage::isNoteOff() const {
    return (status_ & 0xF0) == 0x80 || ((status_ & 0xF0) == 0x90 && data2_ == 0);
}

int MidiMessage::getChannel() const {
    return (status_ & 0x0F) + 1;
}

int MidiMessage::getNote() const {
    return data1_;
}

int MidiMessage::getVelocity() const {
    return data2_;
}

} // namespace AIMusicHardware