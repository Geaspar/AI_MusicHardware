#pragma once

#include <atomic>
#include <cmath>
#include <algorithm>

/**
 * @brief Professional-grade parameter smoothing class inspired by Vital's approach
 * 
 * Provides exponential smoothing with linear interpolation fallback to eliminate
 * zipper noise and provide natural parameter transitions. Thread-safe for use
 * between UI and audio threads.
 */
class SmoothParameter {
public:
    /**
     * @brief Construct a new SmoothParameter with default settings
     * @param initial_value Starting value for the parameter (0.0-1.0)
     */
    explicit SmoothParameter(float initial_value = 0.0f);
    
    /**
     * @brief Copy constructor
     */
    SmoothParameter(const SmoothParameter& other);
    
    /**
     * @brief Move constructor
     */
    SmoothParameter(SmoothParameter&& other) noexcept;
    
    /**
     * @brief Copy assignment operator
     */
    SmoothParameter& operator=(const SmoothParameter& other);
    
    /**
     * @brief Move assignment operator
     */
    SmoothParameter& operator=(SmoothParameter&& other) noexcept;
    
    /**
     * @brief Set the target value for smoothing
     * @param value Target value (0.0-1.0), thread-safe
     */
    void setTarget(float value);
    
    /**
     * @brief Get the current smoothed value
     * @return Current value after smoothing
     */
    float getCurrentValue() const;
    
    /**
     * @brief Process one sample of smoothing
     * @return The smoothed value for this sample
     */
    float process();
    
    /**
     * @brief Process an entire audio buffer with smoothing
     * @param output Buffer to write smoothed values to
     * @param num_samples Number of samples to process
     */
    void processBuffer(float* output, int num_samples);
    
    /**
     * @brief Set the smoothing factor (0.0 = no smoothing, 0.99 = heavy smoothing)
     * @param factor Smoothing factor (0.0-0.99)
     */
    void setSmoothingFactor(float factor);
    
    /**
     * @brief Set the linear interpolation threshold
     * @param threshold When difference is below this, snap to target
     */
    void setLinearThreshold(float threshold);
    
    /**
     * @brief Check if the parameter is currently smoothing to target
     * @return true if actively smoothing, false if at target
     */
    bool isSmoothing() const;
    
    /**
     * @brief Immediately set current value without smoothing
     * @param value Value to set immediately
     */
    void setImmediate(float value);
    
    /**
     * @brief Reset the parameter to a specific value
     * @param value Value to reset to
     */
    void reset(float value = 0.0f);

private:
    std::atomic<float> target_value_;
    float current_value_;
    float smoothing_factor_;
    float linear_threshold_;
    
    // Performance optimization: avoid atomic reads in tight loops
    float cached_target_;
    
    static constexpr float kDefaultSmoothingFactor = 0.95f;
    static constexpr float kDefaultLinearThreshold = 0.001f;
    static constexpr float kMinSmoothingFactor = 0.0f;
    static constexpr float kMaxSmoothingFactor = 0.99f;
};