#pragma once

#include <vector>
#include <array>
#include <memory>
#include "../sequencer/Sequencer.h" // Include for Envelope struct

namespace AIMusicHardware {

enum class OscillatorType {
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
};

class Voice {
public:
    Voice();
    ~Voice();

    void setFrequency(float frequency);
    void setOscillatorType(OscillatorType type);
    void setAmplitude(float amplitude);
    void noteOn(int midiNote, float velocity);
    void noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env);
    void noteOff();
    bool isActive() const;
    
    float generateSample();
    
private:
    float frequency_;
    float amplitude_;
    float phase_;
    OscillatorType oscType_;
    bool isActive_;
    float envelope_;
    
    // Envelope parameters
    float attack_;
    float decay_;
    float sustain_;
    float release_;
    float envelopeValue_;
    enum class EnvelopeStage { Idle, Attack, Decay, Sustain, Release };
    EnvelopeStage envelopeStage_;
};

class Synthesizer {
public:
    Synthesizer(int sampleRate = 44100);
    ~Synthesizer();
    
    void setSampleRate(int sampleRate);
    void noteOn(int midiNote, float velocity);
    void noteOn(int midiNote, float velocity, const AIMusicHardware::Envelope& env);
    void noteOff(int midiNote);
    void allNotesOff();
    
    void setOscillatorType(OscillatorType type);
    
    void process(float* outputBuffer, int numFrames);
    
private:
    int sampleRate_;
    std::vector<std::unique_ptr<Voice>> voices_;
    static constexpr int kMaxVoices = 16;
    OscillatorType currentOscType_;
};

} // namespace AIMusicHardware