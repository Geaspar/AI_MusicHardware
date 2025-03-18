#include "../../../include/synthesis/wavetable/wavetable.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace AIMusicHardware {

constexpr float PI = 3.14159265358979323846f;

// WaveFrame implementation
WaveFrame::WaveFrame(int size)
    : data_(size, 0.0f) {
}

WaveFrame::~WaveFrame() {
}

void WaveFrame::initSine() {
    const int size = static_cast<int>(data_.size());
    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        data_[i] = std::sin(phase * 2.0f * PI);
    }
}

void WaveFrame::initSaw() {
    const int size = static_cast<int>(data_.size());
    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        data_[i] = 2.0f * phase - 1.0f;
    }
}

void WaveFrame::initSquare() {
    const int size = static_cast<int>(data_.size());
    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        data_[i] = (phase < 0.5f) ? 1.0f : -1.0f;
    }
}

void WaveFrame::initTriangle() {
    const int size = static_cast<int>(data_.size());
    for (int i = 0; i < size; ++i) {
        float phase = static_cast<float>(i) / size;
        data_[i] = (phase < 0.5f) ? 
            (4.0f * phase - 1.0f) : 
            (3.0f - 4.0f * phase);
    }
}

void WaveFrame::initNoise() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (int i = 0; i < static_cast<int>(data_.size()); ++i) {
        data_[i] = dist(gen);
    }
}

void WaveFrame::setData(const float* data, int size) {
    if (size != static_cast<int>(data_.size())) {
        data_.resize(size);
    }
    
    if (data && size > 0) {
        std::copy(data, data + size, data_.begin());
    }
}

float WaveFrame::getSample(float phase) const {
    // Ensure phase is in range 0-1
    phase = phase - std::floor(phase);
    
    // Convert phase to sample index with interpolation
    const int size = static_cast<int>(data_.size());
    const float indexFloat = phase * size;
    
    // Get the two samples to interpolate between
    const int index1 = static_cast<int>(indexFloat) % size;
    const int index2 = (index1 + 1) % size;
    
    // Calculate interpolation factor
    const float frac = indexFloat - std::floor(indexFloat);
    
    // Linear interpolation between the two samples
    return data_[index1] * (1.0f - frac) + data_[index2] * frac;
}

// Wavetable implementation
Wavetable::Wavetable(int frameSize, int numFrames) {
    // Create empty frames
    for (int i = 0; i < numFrames; ++i) {
        frames_.push_back(std::make_unique<WaveFrame>(frameSize));
    }
}

Wavetable::~Wavetable() {
}

void Wavetable::addFrame(std::unique_ptr<WaveFrame> frame) {
    if (frame) {
        frames_.push_back(std::move(frame));
    }
}

void Wavetable::setFrame(int index, std::unique_ptr<WaveFrame> frame) {
    if (frame && index >= 0 && index < getNumFrames()) {
        frames_[index] = std::move(frame);
    }
}

WaveFrame* Wavetable::getFrame(int index) {
    if (index >= 0 && index < getNumFrames()) {
        return frames_[index].get();
    }
    return nullptr;
}

void Wavetable::initBasicWaveforms(int numFrames) {
    frames_.clear();
    
    const int waveformCount = 5; // sine, saw, square, triangle, noise
    
    // Default to 5 frames if not specified
    if (numFrames <= 0) {
        numFrames = 5;
    }
    
    for (int i = 0; i < numFrames; ++i) {
        auto frame = std::make_unique<WaveFrame>(2048);
        
        // Create basic waveforms
        if (i == 0) {
            frame->initSine();
        } else if (i == 1) {
            frame->initSaw();
        } else if (i == 2) {
            frame->initSquare();
        } else if (i == 3) {
            frame->initTriangle();
        } else if (i == 4) {
            frame->initNoise();
        } else {
            // For extra frames, create a mix based on position
            frame->initSine(); // Default
        }
        
        frames_.push_back(std::move(frame));
    }
}

float Wavetable::getSample(float framePosition, float phase) const {
    if (frames_.empty()) {
        return 0.0f;
    }
    
    // Ensure frame position is in range 0-1
    framePosition = std::clamp(framePosition, 0.0f, 1.0f);
    
    // Map frame position to frame index with interpolation
    const float frameIndexFloat = framePosition * (frames_.size() - 1);
    const int frameIndex1 = static_cast<int>(frameIndexFloat);
    const int frameIndex2 = std::min(frameIndex1 + 1, static_cast<int>(frames_.size()) - 1);
    const float frameFrac = frameIndexFloat - frameIndex1;
    
    // Get samples from both frames
    const float sample1 = frames_[frameIndex1]->getSample(phase);
    const float sample2 = frames_[frameIndex2]->getSample(phase);
    
    // Interpolate between the two frame samples
    return sample1 * (1.0f - frameFrac) + sample2 * frameFrac;
}

// WavetableOscillator implementation
WavetableOscillator::WavetableOscillator(int sampleRate)
    : wavetable_(nullptr),
      frequency_(440.0f),
      phase_(0.0f),
      framePosition_(0.0f),
      sampleRate_(sampleRate),
      oversample_(true),
      oversamplingFactor_(2) {
}

WavetableOscillator::~WavetableOscillator() {
}

void WavetableOscillator::setWavetable(std::shared_ptr<Wavetable> wavetable) {
    wavetable_ = wavetable;
}

void WavetableOscillator::setFrequency(float frequency) {
    frequency_ = std::max(0.0f, frequency);
}

void WavetableOscillator::setFramePosition(float position) {
    framePosition_ = std::clamp(position, 0.0f, 1.0f);
}

void WavetableOscillator::setPhase(float phase) {
    phase_ = phase - std::floor(phase);
}

void WavetableOscillator::resetPhase() {
    phase_ = 0.0f;
}

float WavetableOscillator::generateSample() {
    if (!wavetable_) {
        return 0.0f;
    }
    
    float sample = 0.0f;
    
    if (oversample_) {
        // Oversample to reduce aliasing
        for (int i = 0; i < oversamplingFactor_; ++i) {
            // Get sample from the wavetable
            sample += wavetable_->getSample(framePosition_, phase_) / oversamplingFactor_;
            
            // Increment phase for next oversample
            phase_ += frequency_ / (sampleRate_ * oversamplingFactor_);
            if (phase_ >= 1.0f) {
                phase_ -= 1.0f;
            }
        }
    } else {
        // No oversampling
        sample = wavetable_->getSample(framePosition_, phase_);
        
        // Increment phase
        phase_ += frequency_ / sampleRate_;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }
    }
    
    return sample;
}

void WavetableOscillator::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
}

} // namespace AIMusicHardware