#pragma once

#include "synthesis/wavetable/hybrid_wavetable.h"
#include "synthesis/wavetable/hybrid_wavetable_ops.h"
#include <unordered_map>
#include <list>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>

namespace AIMusicHardware {

// Lightweight LRU cache for rendered time-domain buffers keyed by CacheKey hash
class SpectralWavetableCache {
public:
    explicit SpectralWavetableCache(size_t capacity = 256)
        : capacity_(capacity) {}

    void setCapacity(size_t c) {
        std::lock_guard<std::mutex> lock(mutex_);
        capacity_ = c > 1 ? c : 1;
        evictIfNeeded();
    }

    size_t capacity() const { return capacity_; }
    size_t size() const { std::lock_guard<std::mutex> lock(mutex_); return map_.size(); }

    std::shared_ptr<WavetableBuffer> get(uint64_t keyHash) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(keyHash);
        if (it == map_.end()) { ++misses_; return nullptr; }
        // Move to front (MRU)
        lru_.splice(lru_.begin(), lru_, it->second);
        ++hits_;
        return it->second->buffer;
    }

    void put(uint64_t keyHash, std::shared_ptr<WavetableBuffer> buffer) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(keyHash);
        if (it != map_.end()) {
            it->second->buffer = std::move(buffer);
            lru_.splice(lru_.begin(), lru_, it->second);
            return;
        }
        lru_.push_front(Node{keyHash, std::move(buffer)});
        map_[keyHash] = lru_.begin();
        evictIfNeeded();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        lru_.clear();
    }

    // Telemetry
    uint64_t hits() const { return hits_.load(); }
    uint64_t misses() const { return misses_.load(); }

private:
    struct Node {
        uint64_t keyHash;
        std::shared_ptr<WavetableBuffer> buffer;
    };

    void evictIfNeeded() {
        while (lru_.size() > capacity_) {
            auto& back = lru_.back();
            map_.erase(back.keyHash);
            lru_.pop_back();
        }
    }

    size_t capacity_;
    mutable std::mutex mutex_;
    std::list<Node> lru_;
    std::unordered_map<uint64_t, std::list<Node>::iterator> map_;
    std::atomic<uint64_t> hits_{0}, misses_{0};
};

// Job specification for rendering (used by worker)
struct SpectralJobSpec {
    const SpectralTable* table = nullptr;
    float morph01 = 0.0f;
    SpectralOps ops{};
    int fftSize = 2048;
    int sampleRate = 44100;
};

// Minimal worker facade (synchronous stub by default). Async impl will be added later.
class SpectralRenderWorker {
public:
    explicit SpectralRenderWorker(std::shared_ptr<SpectralWavetableCache> cache)
        : cache_(std::move(cache)) {}

    void setAsyncEnabled(bool enabled) { asyncEnabled_ = enabled; }

    // Enqueue or synchronously render; returns cached or newly rendered buffer
    std::shared_ptr<WavetableBuffer> requestRender(const CacheKey& key, const SpectralJobSpec& spec) {
        const uint64_t h = hash(key);
        if (auto cached = cache_->get(h)) return cached;
        // Synchronous render (stub) — async path will push to a queue in future
        if (!spec.table || spec.table->frames.empty()) return nullptr;
        SpectralFrame sf = buildSpectralFrame(*spec.table, spec.morph01, spec.ops, spec.fftSize, spec.sampleRate);
        auto buf = std::make_shared<WavetableBuffer>(renderTimeDomain(sf, spec.sampleRate));
        buf->keyHash = h;
        cache_->put(h, buf);
        return buf;
    }

private:
    std::shared_ptr<SpectralWavetableCache> cache_;
    std::atomic<bool> asyncEnabled_{false};
};

} // namespace AIMusicHardware
