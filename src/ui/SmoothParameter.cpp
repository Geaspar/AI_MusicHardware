#include "ui/SmoothParameter.h"
#include <cmath>

SmoothParameter::SmoothParameter(float initial_value)
    : target_value_(initial_value)
    , current_value_(initial_value)
    , smoothing_factor_(kDefaultSmoothingFactor)
    , linear_threshold_(kDefaultLinearThreshold)
    , cached_target_(initial_value) {
}

SmoothParameter::SmoothParameter(const SmoothParameter& other)
    : target_value_(other.target_value_.load())
    , current_value_(other.current_value_)
    , smoothing_factor_(other.smoothing_factor_)
    , linear_threshold_(other.linear_threshold_)
    , cached_target_(other.cached_target_) {
}

SmoothParameter::SmoothParameter(SmoothParameter&& other) noexcept
    : target_value_(other.target_value_.load())
    , current_value_(other.current_value_)
    , smoothing_factor_(other.smoothing_factor_)
    , linear_threshold_(other.linear_threshold_)
    , cached_target_(other.cached_target_) {
}

SmoothParameter& SmoothParameter::operator=(const SmoothParameter& other) {
    if (this != &other) {
        target_value_.store(other.target_value_.load());
        current_value_ = other.current_value_;
        smoothing_factor_ = other.smoothing_factor_;
        linear_threshold_ = other.linear_threshold_;
        cached_target_ = other.cached_target_;
    }
    return *this;
}

SmoothParameter& SmoothParameter::operator=(SmoothParameter&& other) noexcept {
    if (this != &other) {
        target_value_.store(other.target_value_.load());
        current_value_ = other.current_value_;
        smoothing_factor_ = other.smoothing_factor_;
        linear_threshold_ = other.linear_threshold_;
        cached_target_ = other.cached_target_;
    }
    return *this;
}

void SmoothParameter::setTarget(float value) {
    // Clamp to valid range
    value = std::clamp(value, 0.0f, 1.0f);
    target_value_.store(value, std::memory_order_relaxed);
}

float SmoothParameter::getCurrentValue() const {
    return current_value_;
}

float SmoothParameter::process() {
    // Cache target to avoid repeated atomic reads
    cached_target_ = target_value_.load(std::memory_order_relaxed);
    
    float difference = cached_target_ - current_value_;
    
    // Use linear interpolation when very close to target (avoids infinite smoothing)
    if (std::abs(difference) < linear_threshold_) {
        current_value_ = cached_target_;
    } else {
        // Exponential smoothing: current += difference * (1 - smoothing_factor)
        current_value_ += difference * (1.0f - smoothing_factor_);
    }
    
    return current_value_;
}

void SmoothParameter::processBuffer(float* output, int num_samples) {
    if (!output) return;
    
    // Cache target value to avoid atomic reads in loop
    cached_target_ = target_value_.load(std::memory_order_relaxed);
    
    for (int i = 0; i < num_samples; ++i) {
        float difference = cached_target_ - current_value_;
        
        if (std::abs(difference) < linear_threshold_) {
            current_value_ = cached_target_;
        } else {
            current_value_ += difference * (1.0f - smoothing_factor_);
        }
        
        output[i] = current_value_;
    }
}

void SmoothParameter::setSmoothingFactor(float factor) {
    smoothing_factor_ = std::clamp(factor, kMinSmoothingFactor, kMaxSmoothingFactor);
}

void SmoothParameter::setLinearThreshold(float threshold) {
    linear_threshold_ = std::max(threshold, 0.0f);
}

bool SmoothParameter::isSmoothing() const {
    float target = target_value_.load(std::memory_order_relaxed);
    return std::abs(target - current_value_) > linear_threshold_;
}

void SmoothParameter::setImmediate(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    current_value_ = value;
    cached_target_ = value;
    target_value_.store(value, std::memory_order_relaxed);
}

void SmoothParameter::reset(float value) {
    setImmediate(value);
}