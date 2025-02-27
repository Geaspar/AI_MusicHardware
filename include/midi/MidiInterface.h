#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>

namespace AIMusicHardware {

struct MidiMessage {
    enum class Type {
        NoteOn,
        NoteOff,
        ControlChange,
        ProgramChange,
        PitchBend,
        AfterTouch,
        ChannelPressure,
        SystemMessage
    };
    
    Type type;
    int channel;  // 0-15
    int data1;    // Note/Controller number (0-127)
    int data2;    // Velocity/Value (0-127)
    double timestamp;
    
    MidiMessage() : type(Type::NoteOn), channel(0), data1(0), data2(0), timestamp(0.0) {}
};

class MidiInputCallback {
public:
    virtual ~MidiInputCallback() = default;
    virtual void handleIncomingMidiMessage(const MidiMessage& message) = 0;
};

class MidiInput {
public:
    MidiInput();
    ~MidiInput();
    
    static std::vector<std::string> getDevices();
    bool openDevice(int deviceIndex);
    void closeDevice();
    bool isDeviceOpen() const;
    
    void setCallback(MidiInputCallback* callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class MidiOutput {
public:
    MidiOutput();
    ~MidiOutput();
    
    static std::vector<std::string> getDevices();
    bool openDevice(int deviceIndex);
    void closeDevice();
    bool isDeviceOpen() const;
    
    void sendNoteOn(int channel, int noteNumber, int velocity);
    void sendNoteOff(int channel, int noteNumber);
    void sendControlChange(int channel, int controller, int value);
    void sendProgramChange(int channel, int programNumber);
    void sendPitchBend(int channel, int value);
    
    void sendMessage(const MidiMessage& message);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

class MidiHandler : public MidiInputCallback {
public:
    using NoteOnCallback = std::function<void(int channel, int note, int velocity)>;
    using NoteOffCallback = std::function<void(int channel, int note)>;
    using ControlChangeCallback = std::function<void(int channel, int controller, int value)>;
    using GenericMidiCallback = std::function<void(const MidiMessage& message)>;
    
    MidiHandler();
    ~MidiHandler() override;
    
    void handleIncomingMidiMessage(const MidiMessage& message) override;
    
    void setNoteOnCallback(NoteOnCallback callback);
    void setNoteOffCallback(NoteOffCallback callback);
    void setControlChangeCallback(ControlChangeCallback callback);
    void setGenericCallback(GenericMidiCallback callback);
    
private:
    NoteOnCallback noteOnCallback_;
    NoteOffCallback noteOffCallback_;
    ControlChangeCallback ccCallback_;
    GenericMidiCallback genericCallback_;
};

} // namespace AIMusicHardware