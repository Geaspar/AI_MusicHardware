#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <queue>

namespace AIMusicHardware {

/**
 * @brief Performance metric types
 */
enum class MetricType {
    Counter,      // Incrementing value (e.g., number of operations)
    Gauge,        // Current value (e.g., memory usage)
    Histogram,    // Distribution of values (e.g., response times)
    Timer         // Duration measurements
};

/**
 * @brief Performance metric data point
 */
struct MetricDataPoint {
    std::chrono::system_clock::time_point timestamp;
    double value;
    std::map<std::string, std::string> tags;
    
    MetricDataPoint(double val, const std::map<std::string, std::string>& t = {})
        : timestamp(std::chrono::system_clock::now()), value(val), tags(t) {}
};

/**
 * @brief Performance metric with statistical analysis
 */
class PerformanceMetric {
public:
    PerformanceMetric(const std::string& name, MetricType type, 
                     const std::string& description = "", const std::string& unit = "");
    
    // Data recording
    void record(double value, const std::map<std::string, std::string>& tags = {});
    void increment(double delta = 1.0, const std::map<std::string, std::string>& tags = {});
    void set(double value, const std::map<std::string, std::string>& tags = {});
    
    // Statistics
    struct Statistics {
        double count = 0;
        double sum = 0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        double mean = 0;
        double variance = 0;
        double stddev = 0;
        
        // Percentiles for histogram metrics
        double p50 = 0;  // Median
        double p90 = 0;
        double p95 = 0;
        double p99 = 0;
        
        std::chrono::system_clock::time_point lastUpdate;
    };
    
    Statistics getStatistics() const;
    Statistics getStatistics(const std::chrono::system_clock::time_point& since) const;
    
    // Data access
    std::vector<MetricDataPoint> getDataPoints(size_t maxPoints = 1000) const;
    std::vector<MetricDataPoint> getDataPoints(
        const std::chrono::system_clock::time_point& start,
        const std::chrono::system_clock::time_point& end) const;
    
    // Configuration
    void setRetentionPeriod(std::chrono::seconds period);
    void setMaxDataPoints(size_t maxPoints);
    void enableHistogram(const std::vector<double>& buckets = {});
    
    // Metadata
    const std::string& getName() const { return name_; }
    MetricType getType() const { return type_; }
    const std::string& getDescription() const { return description_; }
    const std::string& getUnit() const { return unit_; }

private:
    std::string name_;
    MetricType type_;
    std::string description_;
    std::string unit_;
    
    mutable std::mutex dataMutex_;
    std::vector<MetricDataPoint> dataPoints_;
    size_t maxDataPoints_ = 10000;
    std::chrono::seconds retentionPeriod_{3600}; // 1 hour
    
    // Histogram configuration
    bool histogramEnabled_ = false;
    std::vector<double> histogramBuckets_;
    std::map<double, size_t> histogramCounts_;
    
    // Internal methods
    void cleanup();
    void updateHistogram(double value);
    double calculatePercentile(double percentile) const;
};

/**
 * @brief Timer metric for automatic duration measurement
 */
class TimerMetric {
public:
    TimerMetric(PerformanceMetric& metric, const std::map<std::string, std::string>& tags = {});
    ~TimerMetric();
    
    void addTag(const std::string& key, const std::string& value);
    void stop();
    std::chrono::microseconds getDuration() const;

private:
    PerformanceMetric& metric_;
    std::map<std::string, std::string> tags_;
    std::chrono::high_resolution_clock::time_point startTime_;
    bool stopped_ = false;
};

/**
 * @brief System resource monitor
 */
class SystemResourceMonitor {
public:
    struct ResourceSnapshot {
        double cpuUsagePercent = 0.0;
        size_t memoryUsageBytes = 0;
        size_t memoryAvailableBytes = 0;
        double memoryUsagePercent = 0.0;
        size_t diskUsageBytes = 0;
        size_t diskAvailableBytes = 0;
        double diskUsagePercent = 0.0;
        int threadCount = 0;
        int handleCount = 0;
        std::chrono::system_clock::time_point timestamp;
    };
    
    SystemResourceMonitor();
    ~SystemResourceMonitor();
    
    void start();
    void stop();
    
    ResourceSnapshot getCurrentSnapshot() const;
    std::vector<ResourceSnapshot> getHistory(size_t maxSnapshots = 100) const;
    
    void setMonitoringInterval(std::chrono::milliseconds interval);
    void setHistorySize(size_t maxSnapshots);
    
    // Callbacks for resource threshold alerts
    using AlertCallback = std::function<void(const std::string&, double, double)>;
    void setMemoryUsageAlert(double thresholdPercent, AlertCallback callback);
    void setCpuUsageAlert(double thresholdPercent, AlertCallback callback);
    void setDiskUsageAlert(double thresholdPercent, AlertCallback callback);

private:
    std::atomic<bool> monitoring_{false};
    std::thread monitorThread_;
    std::chrono::milliseconds monitoringInterval_{1000}; // 1 second
    
    mutable std::mutex historyMutex_;
    std::vector<ResourceSnapshot> history_;
    size_t maxHistorySize_ = 1000;
    
    // Alert thresholds and callbacks
    double memoryThreshold_ = 90.0;
    double cpuThreshold_ = 90.0;
    double diskThreshold_ = 90.0;
    AlertCallback memoryAlertCallback_;
    AlertCallback cpuAlertCallback_;
    AlertCallback diskAlertCallback_;
    
    void monitoringLoop();
    ResourceSnapshot captureSnapshot() const;
    void checkAlerts(const ResourceSnapshot& snapshot);
    
    // Platform-specific resource gathering
    double getCpuUsage() const;
    size_t getMemoryUsage() const;
    size_t getAvailableMemory() const;
    size_t getDiskUsage(const std::string& path = "/") const;
    size_t getAvailableDisk(const std::string& path = "/") const;
    int getThreadCount() const;
    int getHandleCount() const;
};

/**
 * @brief Performance alert system
 */
class PerformanceAlertSystem {
public:
    enum class AlertType {
        Threshold,      // Value exceeds threshold
        Anomaly,        // Statistical anomaly detected
        Trend,          // Negative trend detected
        RateLimit       // Rate of change exceeds limit
    };
    
    struct Alert {
        std::string metricName;
        AlertType type;
        double threshold;
        double actualValue;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
        std::map<std::string, std::string> metadata;
    };
    
    using AlertCallback = std::function<void(const Alert&)>;
    
    PerformanceAlertSystem();
    ~PerformanceAlertSystem();
    
    // Alert configuration
    void addThresholdAlert(const std::string& metricName, double threshold, 
                          const std::string& message = "");
    void addAnomalyAlert(const std::string& metricName, double sensitivityScore = 2.0);
    void addTrendAlert(const std::string& metricName, double trendThreshold = -0.1);
    void addRateLimitAlert(const std::string& metricName, double maxRatePerSecond);
    
    void removeAlert(const std::string& metricName, AlertType type);
    void clearAlerts(const std::string& metricName = "");
    
    // Alert handling
    void setAlertCallback(AlertCallback callback);
    void checkMetric(const PerformanceMetric& metric);
    void checkAllMetrics(const std::vector<std::shared_ptr<PerformanceMetric>>& metrics);
    
    // Alert history
    std::vector<Alert> getRecentAlerts(size_t maxAlerts = 100) const;
    void clearAlertHistory();
    
    // Configuration
    void setAlertCooldown(std::chrono::seconds cooldown);
    void setMaxAlertHistory(size_t maxAlerts);

private:
    struct AlertRule {
        AlertType type;
        double threshold;
        std::string message;
        std::chrono::system_clock::time_point lastTriggered;
        
        // Anomaly detection state
        std::vector<double> historicalValues;
        double runningMean = 0.0;
        double runningVariance = 0.0;
        size_t sampleCount = 0;
        
        // Trend detection state
        std::vector<std::pair<std::chrono::system_clock::time_point, double>> trendData;
        
        // Rate limiting state
        std::chrono::system_clock::time_point lastRateCheck;
        double lastValue = 0.0;
    };
    
    std::map<std::string, std::vector<AlertRule>> alertRules_;
    mutable std::mutex alertMutex_;
    
    std::vector<Alert> alertHistory_;
    size_t maxAlertHistory_ = 1000;
    std::chrono::seconds alertCooldown_{60}; // 1 minute
    
    AlertCallback alertCallback_;
    
    // Internal methods
    void triggerAlert(const Alert& alert);
    bool shouldTriggerAlert(const std::string& metricName, AlertType type) const;
    bool checkThreshold(const PerformanceMetric& metric, AlertRule& rule);
    bool checkAnomaly(const PerformanceMetric& metric, AlertRule& rule);
    bool checkTrend(const PerformanceMetric& metric, AlertRule& rule);
    bool checkRateLimit(const PerformanceMetric& metric, AlertRule& rule);
    
    double calculateZScore(double value, double mean, double variance) const;
    double calculateTrendSlope(const std::vector<std::pair<std::chrono::system_clock::time_point, double>>& data) const;
};

/**
 * @brief Comprehensive performance monitoring system
 */
class PresetPerformanceMonitor {
public:
    PresetPerformanceMonitor();
    ~PresetPerformanceMonitor();
    
    // Metric management
    std::shared_ptr<PerformanceMetric> createMetric(const std::string& name, MetricType type,
                                                   const std::string& description = "",
                                                   const std::string& unit = "");
    
    std::shared_ptr<PerformanceMetric> getMetric(const std::string& name) const;
    std::vector<std::shared_ptr<PerformanceMetric>> getAllMetrics() const;
    void removeMetric(const std::string& name);
    
    // Timer utilities
    TimerMetric startTimer(const std::string& metricName, 
                          const std::map<std::string, std::string>& tags = {});
    
    // Built-in metrics for preset operations
    void recordDatabaseOperation(const std::string& operation, std::chrono::microseconds duration,
                               bool success = true);
    void recordUIOperation(const std::string& operation, std::chrono::microseconds duration);
    void recordMLOperation(const std::string& operation, std::chrono::microseconds duration,
                          size_t dataSize = 0);
    void recordMemoryUsage(size_t bytes);
    void recordCacheHit(const std::string& cacheType);
    void recordCacheMiss(const std::string& cacheType);
    
    // System monitoring
    SystemResourceMonitor& getSystemMonitor() { return systemMonitor_; }
    const SystemResourceMonitor& getSystemMonitor() const { return systemMonitor_; }
    
    // Alert system
    PerformanceAlertSystem& getAlertSystem() { return alertSystem_; }
    const PerformanceAlertSystem& getAlertSystem() const { return alertSystem_; }
    
    // Reporting
    struct PerformanceReport {
        std::chrono::system_clock::time_point generatedAt;
        std::chrono::system_clock::time_point periodStart;
        std::chrono::system_clock::time_point periodEnd;
        
        std::map<std::string, PerformanceMetric::Statistics> metricStats;
        SystemResourceMonitor::ResourceSnapshot currentResources;
        std::vector<PerformanceAlertSystem::Alert> recentAlerts;
        
        // Preset-specific metrics
        double averageLoadTime = 0.0;
        double averageSearchTime = 0.0;
        double averageMLAnalysisTime = 0.0;
        size_t totalOperations = 0;
        double cacheHitRate = 0.0;
        double errorRate = 0.0;
    };
    
    PerformanceReport generateReport(std::chrono::hours period = std::chrono::hours(1)) const;
    std::string formatReport(const PerformanceReport& report) const;
    
    // Configuration
    void enableAutoReporting(std::chrono::minutes interval = std::chrono::minutes(15));
    void disableAutoReporting();
    void setReportCallback(std::function<void(const PerformanceReport&)> callback);
    
    // Data export
    void exportMetrics(const std::string& filename, const std::string& format = "json") const;
    void exportReport(const PerformanceReport& report, const std::string& filename,
                     const std::string& format = "json") const;
    
    // Singleton access
    static PresetPerformanceMonitor& getInstance();

private:
    std::map<std::string, std::shared_ptr<PerformanceMetric>> metrics_;
    mutable std::mutex metricsMutex_;
    
    SystemResourceMonitor systemMonitor_;
    PerformanceAlertSystem alertSystem_;
    
    // Auto reporting
    std::atomic<bool> autoReportingEnabled_{false};
    std::thread autoReportingThread_;
    std::chrono::minutes reportingInterval_{15};
    std::function<void(const PerformanceReport&)> reportCallback_;
    
    // Built-in metrics
    void initializeBuiltinMetrics();
    void autoReportingLoop();
    
    // Utility methods
    double calculateCacheHitRate(const std::string& cacheType) const;
    double calculateErrorRate() const;
};

/**
 * @brief RAII performance measurement utility
 */
class ScopedPerformanceMeasurement {
public:
    ScopedPerformanceMeasurement(const std::string& metricName,
                               const std::map<std::string, std::string>& tags = {});
    ~ScopedPerformanceMeasurement();
    
    void addTag(const std::string& key, const std::string& value);
    void addMetadata(const std::string& key, const std::string& value);
    
private:
    std::string metricName_;
    std::map<std::string, std::string> tags_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * @brief Convenient macros for performance monitoring
 */
#define MONITOR_PERFORMANCE(metricName) \
    ScopedPerformanceMeasurement _perf_monitor(metricName)

#define MONITOR_PERFORMANCE_WITH_TAGS(metricName, tags) \
    ScopedPerformanceMeasurement _perf_monitor(metricName, tags)

#define MONITOR_DATABASE_OP(operation) \
    auto _db_timer = PresetPerformanceMonitor::getInstance().startTimer("database." operation)

#define MONITOR_UI_OP(operation) \
    auto _ui_timer = PresetPerformanceMonitor::getInstance().startTimer("ui." operation)

#define MONITOR_ML_OP(operation) \
    auto _ml_timer = PresetPerformanceMonitor::getInstance().startTimer("ml." operation)

} // namespace AIMusicHardware