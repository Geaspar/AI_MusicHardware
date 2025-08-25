#include "../../include/effects/PingPongDelay.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware
{

PingPongDelay::PingPongDelay(int sampleRate) : Effect(sampleRate)
{
    // Initialize delay buffers (max delay time 2000ms at current sample rate)
    delayBufferL_.resize(static_cast<size_t>(2.0f * sampleRate));
    delayBufferR_.resize(static_cast<size_t>(2.0f * sampleRate));
    
    updateDelayTime();
}

PingPongDelay::~PingPongDelay()
{
}

void PingPongDelay::process(float* buffer, int numFrames)
{
    const float wet = mix_;
    const float dry = 1.0f - mix_;
    const float feedback = feedback_;
    const float width = std::clamp(width_, 0.0f, 1.0f);

    for (int i = 0; i < numFrames; ++i)
    {
        float inL = buffer[i * 2];
        float inR = buffer[i * 2 + 1];

        // Read from delay lines
        float delayedL = readFromDelay(delayBufferL_, writeIndexL_, 0.0f);
        float delayedR = readFromDelay(delayBufferR_, writeIndexR_, 0.0f);

        // Ping-pong logic (read current delayed values)
        float fbL, fbR;
        if (pingPong_)
        {
            fbL = feedback * delayedR;
            fbR = feedback * delayedL;
        }
        else
        {
            fbL = feedback * delayedL;
            fbR = feedback * delayedR;
        }

        // Apply 1-pole HP then LP on feedback path per channel
        // High-pass: y[n] = a*(y[n-1] + x[n] - x[n-1])
        // Low-pass:  y[n] = a*y[n-1] + (1-a)*x[n]
        // HP Left
        float hpOutL = hpAlpha_ * (hpStateL_ + fbL - hpPrevInL_);
        hpPrevInL_ = fbL;
        hpStateL_ = hpOutL;
        // HP Right
        float hpOutR = hpAlpha_ * (hpStateR_ + fbR - hpPrevInR_);
        hpPrevInR_ = fbR;
        hpStateR_ = hpOutR;
        // LP Left
        lpStateL_ = lpAlpha_ * lpStateL_ + (1.0f - lpAlpha_) * hpOutL;
        // LP Right
        lpStateR_ = lpAlpha_ * lpStateR_ + (1.0f - lpAlpha_) * hpOutR;

        // Write to delay lines
        delayBufferL_[writeIndexL_] = inL + lpStateL_;
        delayBufferR_[writeIndexR_] = inR + lpStateR_;

        // Stereo width processing for wet signal
        float wetL = delayedL * (1.0f - 0.5f * width) + delayedR * (0.5f * width);
        float wetR = delayedR * (1.0f - 0.5f * width) + delayedL * (0.5f * width);
        
        // Output mix
        buffer[i * 2]     = inL * dry + wetL * wet;
        buffer[i * 2 + 1] = inR * dry + wetR * wet;

        // Update write indices
        writeIndexL_ = (writeIndexL_ + 1) % delayBufferL_.size();
        writeIndexR_ = (writeIndexR_ + 1) % delayBufferR_.size();
    }
}

void PingPongDelay::setParameter(const std::string& name, float value)
{
    if (name == "mix")
    {
        mix_ = std::clamp(value, 0.0f, 1.0f);
    }
    else if (name == "time_ms")
    {
        timeMs_ = std::clamp(value, 10.0f, 2000.0f);
        updateDelayTime();
    }
    else if (name == "feedback")
    {
        feedback_ = std::clamp(value, 0.0f, 0.95f);
    }
    else if (name == "hp_freq")
    {
        hpFreq_ = std::clamp(value, 20.0f, 2000.0f);
    }
    else if (name == "lp_freq")
    {
        lpFreq_ = std::clamp(value, 1000.0f, 20000.0f);
    }
    else if (name == "ping_pong")
    {
        pingPong_ = value > 0.5f;
    }
    else if (name == "width")
    {
        width_ = std::clamp(value, 0.0f, 1.0f);
    }
    else if (name == "output_trim_db")
    {
        outputTrimDb_ = std::clamp(value, -12.0f, 12.0f);
    }
}

float PingPongDelay::getParameter(const std::string& name) const
{
    if (name == "mix") return mix_;
    if (name == "time_ms") return timeMs_;
    if (name == "feedback") return feedback_;
    if (name == "hp_freq") return hpFreq_;
    if (name == "lp_freq") return lpFreq_;
    if (name == "ping_pong") return pingPong_ ? 1.0f : 0.0f;
    if (name == "width") return width_;
    if (name == "output_trim_db") return outputTrimDb_;
    return 0.0f;
}

void PingPongDelay::updateFilterCoeffs()
{
    // Recompute coefficients from current cutoff frequencies and sample rate
    float sr = static_cast<float>(getSampleRate());
    // Avoid denorms/edge cases
    float hp = std::max(1.0f, std::min(hpFreq_, sr * 0.45f));
    float lp = std::max(1.0f, std::min(lpFreq_, sr * 0.45f));
    // 1-pole using exponential smoothing mapping
    hpAlpha_ = std::exp(-(2.0f * 3.14159265359f * hp) / sr);
    lpAlpha_ = std::exp(-(2.0f * 3.14159265359f * lp) / sr);
}

void PingPongDelay::updateDelayTime()
{
    delaySamples_ = static_cast<size_t>(timeMs_ * getSampleRate() / 1000.0f);
    if (delaySamples_ < 1) delaySamples_ = 1; // Ensure minimum delay
    // Resize buffers to exact delay length to simplify read/write indexing
    delayBufferL_.assign(delaySamples_, 0.0f);
    delayBufferR_.assign(delaySamples_, 0.0f);
    writeIndexL_ = 0;
    writeIndexR_ = 0;
}

float PingPongDelay::readFromDelay(const std::vector<float>& buffer, size_t index, float fractionalDelay)
{
    // Ring buffer read: read the sample written exactly one full delay in the past
    // With buffers sized to delaySamples_, the read index equals current write index
    (void)fractionalDelay;
    if (buffer.empty()) return 0.0f;
    size_t readIndex = index % buffer.size();
    return buffer[readIndex];
}

} // namespace AIMusicHardware
