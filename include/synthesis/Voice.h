#pragma once

class Voice {
public:
    virtual ~Voice() = default;

    virtual void process(float* outputBuffer, int numSamples) = 0;
    virtual void noteOn(int noteNumber, float velocity) = 0;
    virtual void noteOff() = 0;
    virtual bool isPlaying() const = 0;
};