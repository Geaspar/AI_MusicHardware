#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <queue>

namespace AIMusicHardware {

/**
 * @brief Memory allocation tracking information
 */
struct MemoryAllocation {
    void* ptr;
    size_t size;
    std::string category;
    std::string file;
    std::string function;
    int line;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
    
    MemoryAllocation(void* p, size_t s, const std::string& cat,
                    const std::string& f, const std::string& func, int l)
        : ptr(p), size(s), category(cat), file(f), function(func), line(l),
          timestamp(std::chrono::system_clock::now()), threadId(std::this_thread::get_id()) {}
};

/**
 * @brief Memory leak detection result
 */
struct MemoryLeak {
    MemoryAllocation allocation;
    std::chrono::milliseconds age;
    bool isActive;
    
    MemoryLeak(const MemoryAllocation& alloc, std::chrono::milliseconds a, bool active)
        : allocation(alloc), age(a), isActive(active) {}
};

/**
 * @brief Memory pool for efficient allocation of same-sized objects
 */
template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t blockSize = 64);
    ~MemoryPool();
    
    T* allocate();
    void deallocate(T* ptr);
    
    size_t getBlockSize() const { return blockSize_; }
    size_t getAllocatedCount() const { return allocatedCount_; }
    size_t getAvailableCount() const { return freeList_.size(); }
    size_t getTotalMemoryUsage() const;
    
    void preallocate(size_t count);
    void shrink();
    
private:
    size_t blockSize_;
    std::vector<std::unique_ptr<T[]>> blocks_;
    std::queue<T*> freeList_;
    std::atomic<size_t> allocatedCount_{0};
    mutable std::mutex poolMutex_;
    
    void expandPool();
};

/**
 * @brief Object cache with LRU eviction policy
 */
template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t maxSize, size_t maxMemory = 0);
    
    void put(const Key& key, const Value& value);
    bool get(const Key& key, Value& value);
    bool contains(const Key& key) const;
    void remove(const Key& key);
    void clear();
    
    size_t size() const { return cache_.size(); }
    size_t maxSize() const { return maxSize_; }
    size_t getMemoryUsage() const { return currentMemoryUsage_; }
    
    void setMaxSize(size_t maxSize);
    void setMaxMemory(size_t maxMemory);
    
    // Statistics
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        double hitRate = 0.0;
    };
    CacheStats getStatistics() const;
    void resetStatistics();

private:
    struct CacheNode {
        Key key;
        Value value;
        std::chrono::system_clock::time_point lastAccess;
        size_t memorySize;
        
        std::shared_ptr<CacheNode> prev;
        std::shared_ptr<CacheNode> next;
        
        CacheNode(const Key& k, const Value& v, size_t size = 0)
            : key(k), value(v), lastAccess(std::chrono::system_clock::now()), 
              memorySize(size) {}
    };
    
    size_t maxSize_;
    size_t maxMemory_;
    std::atomic<size_t> currentMemoryUsage_{0};
    
    std::unordered_map<Key, std::shared_ptr<CacheNode>> cache_;
    std::shared_ptr<CacheNode> head_;
    std::shared_ptr<CacheNode> tail_;
    mutable std::mutex cacheMutex_;
    
    // Statistics
    mutable std::atomic<size_t> hits_{0};
    mutable std::atomic<size_t> misses_{0};
    mutable std::atomic<size_t> evictions_{0};
    
    void moveToHead(std::shared_ptr<CacheNode> node);
    void removeNode(std::shared_ptr<CacheNode> node);
    void evictLRU();
    size_t calculateMemorySize(const Value& value) const;
};

/**
 * @brief Smart pointer with automatic memory tracking
 */
template<typename T>
class TrackedPtr {
public:
    TrackedPtr() = default;
    explicit TrackedPtr(T* ptr, const std::string& category = "default");
    TrackedPtr(const TrackedPtr& other);
    TrackedPtr(TrackedPtr&& other) noexcept;
    ~TrackedPtr();
    
    TrackedPtr& operator=(const TrackedPtr& other);
    TrackedPtr& operator=(TrackedPtr&& other) noexcept;
    
    T* get() const { return ptr_.get(); }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_.get(); }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    void reset(T* ptr = nullptr);
    T* release();
    
    const std::string& getCategory() const { return category_; }

private:
    std::unique_ptr<T> ptr_;
    std::string category_;
    
    void trackAllocation();
    void trackDeallocation();
};

/**
 * @brief Memory usage monitor for different categories
 */
class MemoryUsageMonitor {
public:
    MemoryUsageMonitor();
    ~MemoryUsageMonitor();
    
    // Memory tracking
    void trackAllocation(void* ptr, size_t size, const std::string& category,
                        const std::string& file = "", const std::string& function = "", int line = 0);
    void trackDeallocation(void* ptr);
    
    // Statistics
    struct CategoryStats {
        size_t totalAllocations = 0;
        size_t totalDeallocations = 0;
        size_t currentAllocations = 0;
        size_t peakAllocations = 0;
        size_t totalBytes = 0;
        size_t currentBytes = 0;
        size_t peakBytes = 0;
        double averageAllocationSize = 0.0;
    };
    
    CategoryStats getCategoryStats(const std::string& category) const;
    std::map<std::string, CategoryStats> getAllCategoryStats() const;
    
    size_t getTotalMemoryUsage() const;
    size_t getPeakMemoryUsage() const;
    size_t getCurrentAllocationCount() const;
    
    // Leak detection
    std::vector<MemoryLeak> detectLeaks(std::chrono::minutes ageThreshold = std::chrono::minutes(10)) const;
    void reportLeaks(const std::vector<MemoryLeak>& leaks) const;
    
    // Configuration
    void setLeakDetectionEnabled(bool enabled) { leakDetectionEnabled_ = enabled; }
    void setStackTraceEnabled(bool enabled) { stackTraceEnabled_ = enabled; }
    void setMaxTrackedAllocations(size_t maxAllocations);
    
    // Monitoring callbacks
    using MemoryCallback = std::function<void(const std::string&, size_t, size_t)>;
    void setAllocationCallback(MemoryCallback callback) { allocationCallback_ = callback; }
    void setDeallocationCallback(MemoryCallback callback) { deallocationCallback_ = callback; }
    void setLeakCallback(std::function<void(const std::vector<MemoryLeak>&)> callback);
    
    // Automatic monitoring
    void startPeriodicMonitoring(std::chrono::seconds interval = std::chrono::seconds(30));
    void stopPeriodicMonitoring();

private:
    mutable std::mutex allocationsMutex_;
    std::unordered_map<void*, MemoryAllocation> activeAllocations_;
    std::map<std::string, CategoryStats> categoryStats_;
    
    std::atomic<size_t> totalMemoryUsage_{0};
    std::atomic<size_t> peakMemoryUsage_{0};
    std::atomic<size_t> maxTrackedAllocations_{100000};
    
    bool leakDetectionEnabled_ = true;
    bool stackTraceEnabled_ = false;
    
    // Callbacks
    MemoryCallback allocationCallback_;
    MemoryCallback deallocationCallback_;
    std::function<void(const std::vector<MemoryLeak>&)> leakCallback_;
    
    // Periodic monitoring
    std::atomic<bool> monitoringEnabled_{false};
    std::thread monitoringThread_;
    
    void updateCategoryStats(const std::string& category, size_t size, bool isAllocation);
    void periodicMonitoringLoop();
    std::string captureStackTrace() const;
};

/**
 * @brief Garbage collector for automatic memory management
 */
class GarbageCollector {
public:
    GarbageCollector();
    ~GarbageCollector();
    
    // Object registration
    template<typename T>
    void registerObject(std::shared_ptr<T> obj, const std::string& category = "default");
    
    template<typename T>
    void unregisterObject(std::shared_ptr<T> obj);
    
    // Collection policies
    enum class CollectionPolicy {
        Immediate,    // Collect immediately when objects are unreferenced
        Periodic,     // Collect periodically
        Manual,       // Collect only when explicitly requested
        Adaptive      // Adapt collection frequency based on memory pressure
    };
    
    void setCollectionPolicy(CollectionPolicy policy);
    void setCollectionInterval(std::chrono::seconds interval);
    void setMemoryThreshold(size_t bytes);
    
    // Manual collection
    size_t collect();
    size_t forceCollect();
    
    // Statistics
    struct GCStats {
        size_t totalCollections = 0;
        size_t objectsCollected = 0;
        size_t bytesFreed = 0;
        std::chrono::milliseconds totalCollectionTime{0};
        std::chrono::system_clock::time_point lastCollection;
    };
    
    GCStats getStatistics() const;
    void resetStatistics();
    
    // Configuration
    void start();
    void stop();
    bool isRunning() const { return running_; }

private:
    struct GCObject {
        std::weak_ptr<void> weakRef;
        std::string category;
        size_t estimatedSize;
        std::chrono::system_clock::time_point registrationTime;
    };
    
    std::vector<GCObject> registeredObjects_;
    mutable std::mutex objectsMutex_;
    
    CollectionPolicy policy_ = CollectionPolicy::Periodic;
    std::chrono::seconds collectionInterval_{60}; // 1 minute
    size_t memoryThreshold_ = 100 * 1024 * 1024; // 100MB
    
    std::atomic<bool> running_{false};
    std::thread collectionThread_;
    
    GCStats stats_;
    mutable std::mutex statsMutex_;
    
    void collectionLoop();
    size_t performCollection();
    bool shouldCollect() const;
    size_t estimateObjectSize(const std::type_info& typeInfo) const;
};

/**
 * @brief Comprehensive memory management system
 */
class PresetMemoryManager {
public:
    PresetMemoryManager();
    ~PresetMemoryManager();
    
    // Memory pools
    template<typename T>
    MemoryPool<T>& getPool();
    
    // Caches
    template<typename Key, typename Value>
    LRUCache<Key, Value>& getCache(const std::string& name, size_t maxSize = 1000);
    
    // Memory monitoring
    MemoryUsageMonitor& getMonitor() { return monitor_; }
    const MemoryUsageMonitor& getMonitor() const { return monitor_; }
    
    // Garbage collection
    GarbageCollector& getGarbageCollector() { return gc_; }
    const GarbageCollector& getGarbageCollector() const { return gc_; }
    
    // Factory methods for tracked objects
    template<typename T, typename... Args>
    TrackedPtr<T> createTracked(const std::string& category, Args&&... args);
    
    template<typename T>
    std::shared_ptr<T> createShared(const std::string& category = "default");
    
    // Memory optimization
    void optimizeMemoryUsage();
    void clearCaches();
    void shrinkPools();
    void triggerGarbageCollection();
    
    // Statistics and reporting
    struct MemoryReport {
        size_t totalMemoryUsage = 0;
        size_t peakMemoryUsage = 0;
        size_t activeAllocations = 0;
        
        std::map<std::string, MemoryUsageMonitor::CategoryStats> categoryStats;
        std::map<std::string, size_t> poolUsage;
        std::map<std::string, size_t> cacheUsage;
        
        GarbageCollector::GCStats gcStats;
        std::vector<MemoryLeak> detectedLeaks;
        
        std::chrono::system_clock::time_point generatedAt;
    };
    
    MemoryReport generateReport() const;
    std::string formatReport(const MemoryReport& report) const;
    
    // Configuration
    void configure(const std::map<std::string, std::string>& config);
    void setMemoryLimit(size_t bytes);
    void setLeakDetectionInterval(std::chrono::minutes interval);
    void enableDetailedTracking(bool enabled);
    
    // Singleton access
    static PresetMemoryManager& getInstance();

private:
    MemoryUsageMonitor monitor_;
    GarbageCollector gc_;
    
    // Memory pools for common objects
    std::map<std::type_index, std::unique_ptr<void, void(*)(void*)>> pools_;
    mutable std::mutex poolsMutex_;
    
    // Named caches
    std::map<std::string, std::unique_ptr<void, void(*)(void*)>> caches_;
    mutable std::mutex cachesMutex_;
    
    // Configuration
    size_t memoryLimit_ = 0; // 0 = no limit
    bool detailedTracking_ = false;
    std::chrono::minutes leakDetectionInterval_{5};
    
    // Memory pressure monitoring
    std::atomic<bool> memoryPressureMonitoring_{false};
    std::thread memoryPressureThread_;
    
    void initializeDefaultPools();
    void monitorMemoryPressure();
    void handleMemoryPressure();
    
    template<typename T>
    std::type_index getTypeIndex() const { return std::type_index(typeid(T)); }
};

/**
 * @brief Memory management utilities and macros
 */
namespace MemoryUtils {
    /**
     * @brief Get size of an object in memory (including vtable, padding, etc.)
     */
    template<typename T>
    size_t getObjectSize(const T& obj);
    
    /**
     * @brief Estimate memory usage of standard containers
     */
    template<typename Container>
    size_t estimateContainerMemory(const Container& container);
    
    /**
     * @brief Get current process memory usage
     */
    size_t getCurrentProcessMemory();
    
    /**
     * @brief Get available system memory
     */
    size_t getAvailableSystemMemory();
    
    /**
     * @brief Check if system is under memory pressure
     */
    bool isMemoryPressureHigh();
}

/**
 * @brief Macros for convenient memory management
 */
#define TRACK_MEMORY_ALLOCATION(ptr, size, category) \
    PresetMemoryManager::getInstance().getMonitor().trackAllocation(ptr, size, category, __FILE__, __FUNCTION__, __LINE__)

#define TRACK_MEMORY_DEALLOCATION(ptr) \
    PresetMemoryManager::getInstance().getMonitor().trackDeallocation(ptr)

#define CREATE_TRACKED(Type, category, ...) \
    PresetMemoryManager::getInstance().createTracked<Type>(category, ##__VA_ARGS__)

#define CREATE_SHARED(Type, category) \
    PresetMemoryManager::getInstance().createShared<Type>(category)

#define GET_MEMORY_POOL(Type) \
    PresetMemoryManager::getInstance().getPool<Type>()

#define GET_CACHE(KeyType, ValueType, name, maxSize) \
    PresetMemoryManager::getInstance().getCache<KeyType, ValueType>(name, maxSize)

} // namespace AIMusicHardware