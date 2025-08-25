#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace AIMusicHardware {

// Minimal phase-1 Sensor Matrix: lane conditioning + live values
class SensorMatrix {
public:
    enum class Mode {
        AbsoluteUnipolar, // 0..1
        AbsoluteBipolar   // -1..1
    };

    struct LaneConfig {
        bool enabled = true;
        Mode mode = Mode::AbsoluteUnipolar;
        float deadband = 0.0f;       // ignore changes smaller than this (normalized domain)
        float smoothingAlpha = 0.1f; // one-pole smoothing (0..1]
        float scale = 1.0f;          // post-conditioned scale
        float offset = 0.0f;         // post-conditioned offset
        bool invert = false;         // invert after normalization
        std::string publishName;     // optional alias for UI
    };

    struct LaneState {
        float raw = 0.0f;       // most recent raw sample
        float conditioned = 0.0f; // smoothed/mapped value in lane domain
        bool initialized = false;
    };

    explicit SensorMatrix(size_t laneCount = 8) {
        lanes_.resize(laneCount);
        configs_.resize(laneCount);
    }

    size_t laneCount() const { return lanes_.size(); }

    // Configure a lane
    void setConfig(size_t i, const LaneConfig& cfg) {
        if (i >= configs_.size()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        configs_[i] = cfg;
    }
    LaneConfig getConfig(size_t i) const {
        if (i >= configs_.size()) return LaneConfig{};
        std::lock_guard<std::mutex> lock(mutex_);
        return configs_[i];
    }

    // Feed a raw value into a lane (caller supplies raw in appropriate domain).
    // For unipolar mode, expect ~0..1; for bipolar, ~-1..1. Values will be clamped.
    void setRaw(size_t i, float value) {
        if (i >= lanes_.size()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& cfg = configs_[i];
        auto& st = lanes_[i];
        if (!cfg.enabled) return;

        float v = clampToMode(value, cfg.mode);
        // Deadband (on raw delta)
        if (st.initialized) {
            float delta = std::abs(v - st.raw);
            if (delta < cfg.deadband) {
                // no update
            } else {
                st.raw = v;
            }
        } else {
            st.raw = v;
            st.conditioned = v;
            st.initialized = true;
        }

        // Map/invert then smooth toward target
        float mapped = cfg.invert ? invert(v, cfg.mode) : v;
        mapped = mapped * cfg.scale + cfg.offset;

        // One-pole smoothing
        float a = std::clamp(cfg.smoothingAlpha, 0.0f, 1.0f);
        st.conditioned = st.conditioned + a * (mapped - st.conditioned);
        // Clamp after processing
        st.conditioned = clampToMode(st.conditioned, cfg.mode);
    }

    // Get current conditioned value for lane i
    float get(size_t i) const {
        if (i >= lanes_.size()) return 0.0f;
        std::lock_guard<std::mutex> lock(mutex_);
        return lanes_[i].conditioned;
    }

    // Helper: set human-readable name for a lane (used by UI/tooling)
    void setPublishName(size_t i, const std::string& name) {
        if (i >= configs_.size()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        configs_[i].publishName = name;
    }
    std::string getPublishName(size_t i) const {
        if (i >= configs_.size()) return {};
        std::lock_guard<std::mutex> lock(mutex_);
        return configs_[i].publishName;
    }

private:
    static inline float clampToMode(float v, Mode m) {
        if (m == Mode::AbsoluteUnipolar) return std::clamp(v, 0.0f, 1.0f);
        return std::clamp(v, -1.0f, 1.0f);
    }
    static inline float invert(float v, Mode m) {
        if (m == Mode::AbsoluteUnipolar) return 1.0f - v;
        return -v;
    }

    mutable std::mutex mutex_;
    std::vector<LaneConfig> configs_;
    std::vector<LaneState> lanes_;
};

} // namespace AIMusicHardware

