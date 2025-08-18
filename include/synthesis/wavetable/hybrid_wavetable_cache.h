#pragma once

#include "synthesis/wavetable/hybrid_wavetable.h"
#include "synthesis/wavetable/hybrid_wavetable_ops.h"
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>
#include <thread>
#include <condition_variable>
#include <deque>

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
    bool minPhase = false;
};

// Minimal worker facade (synchronous stub by default). Async impl will be added later.
class SpectralRenderWorker {
public:
    explicit SpectralRenderWorker(std::shared_ptr<SpectralWavetableCache> cache)
        : cache_(std::move(cache)) {}

    ~SpectralRenderWorker() { stop(); }

    void setAsyncEnabled(bool enabled) {
        if (enabled) start(); else stop();
        asyncEnabled_.store(enabled, std::memory_order_release);
    }

    bool isAsyncEnabled() const { return asyncEnabled_.load(std::memory_order_acquire); }
    size_t queueSize() const { std::lock_guard<std::mutex> lock(mutex_); return queue_.size(); }
    size_t inFlightCount() const { std::lock_guard<std::mutex> lock(mutex_); return inFlight_.size(); }

    // Enqueue or synchronously render; returns cached or newly rendered buffer
    std::shared_ptr<WavetableBuffer> requestRender(const CacheKey& key, const SpectralJobSpec& spec) {
        const uint64_t h = hash(key);
        if (auto cached = cache_->get(h)) return cached;
        if (asyncEnabled_.load(std::memory_order_acquire)) {
            enqueueRender(h, key, spec);
            return nullptr; // will be ready soon
        }
        // Fallback synchronous render
        if (!spec.table || spec.table->frames.empty()) return nullptr;
        SpectralFrame sf = buildSpectralFrame(*spec.table, spec.morph01, spec.ops, spec.fftSize, spec.sampleRate);
        auto buf = std::make_shared<WavetableBuffer>(renderTimeDomain(sf, spec.sampleRate, spec.minPhase));
        buf->keyHash = h;
        cache_->put(h, buf);
        return buf;
    }

    // Hint the worker to pre-render the current and neighboring morph steps
    void prewarmHints(const SpectralTable* table,
                      float morph01,
                      const SpectralOps& ops,
                      int fftSize,
                      int sampleRate,
                      bool minPhase) {
        if (!table) return;
        CacheKey k{};
        // Use pointer as table ID if string ID is not set
        k.tableIdHash = reinterpret_cast<uint64_t>(table);
        k.pitchBand = 0; // neutral for now
        k.opsHash16 = quantizeOpsHash(ops);
        k.fftSizeCode = fftSizeToCode(fftSize);
        k.quality = 0; k.sampleRateQ = static_cast<uint16_t>(sampleRate / 100);
        auto q = static_cast<int>(quantizeMorph01(morph01));
        int neighbors[3] = { q, std::max(0, q - 1), std::min(127, q + 1) };
        for (int qi : neighbors) {
            CacheKey kk = k; kk.morphQ = static_cast<uint16_t>(qi);
            SpectralJobSpec spec{ table, morph01, ops, fftSize, sampleRate, minPhase };
            enqueueRender(hash(kk), kk, spec);
        }
    }

private:
    struct Job { uint64_t keyHash; CacheKey key; SpectralJobSpec spec; };

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) return;
        running_ = true;
        workerThread_ = std::thread([this]{ this->runLoop(); });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) return;
            running_ = false;
        }
        cv_.notify_all();
        if (workerThread_.joinable()) workerThread_.join();
        std::lock_guard<std::mutex> lock2(mutex_);
        queue_.clear();
        inFlight_.clear();
    }

    void enqueueRender(uint64_t h, const CacheKey& key, const SpectralJobSpec& spec) {
        if (!asyncEnabled_.load(std::memory_order_acquire)) return;
        if (cache_->get(h)) return; // already cached
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        if (inFlight_.count(h)) return; // coalesce
        inFlight_.insert(h);
        queue_.push_back(Job{h, key, spec});
        cv_.notify_one();
    }

    void runLoop() {
        while (true) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [&]{ return !running_ || !queue_.empty(); });
                if (!running_ && queue_.empty()) break;
                job = queue_.front();
                queue_.pop_front();
            }
            // Render outside lock
            if (!cache_->get(job.keyHash) && job.spec.table && !job.spec.table->frames.empty()) {
                SpectralFrame sf = buildSpectralFrame(*job.spec.table, job.spec.morph01, job.spec.ops, job.spec.fftSize, job.spec.sampleRate);
                auto buf = std::make_shared<WavetableBuffer>(renderTimeDomain(sf, job.spec.sampleRate, false));
                buf->keyHash = job.keyHash;
                cache_->put(job.keyHash, buf);
            }
            // Mark complete
            {
                std::lock_guard<std::mutex> lock(mutex_);
                inFlight_.erase(job.keyHash);
            }
        }
    }

    std::shared_ptr<SpectralWavetableCache> cache_;
    std::atomic<bool> asyncEnabled_{false};
    std::thread workerThread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
    std::deque<Job> queue_;
    std::unordered_set<uint64_t> inFlight_;
};

} // namespace AIMusicHardware
