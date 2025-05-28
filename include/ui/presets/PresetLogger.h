#pragma once

#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>
#include <fstream>
#include <chrono>
#include <map>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace AIMusicHardware {

/**
 * @brief Log levels for different types of messages
 */
enum class LogLevel {
    Trace = 0,    // Extremely detailed debug information
    Debug = 1,    // Debug information
    Info = 2,     // General information
    Warning = 3,  // Warning messages
    Error = 4,    // Error messages
    Critical = 5  // Critical system errors
};

/**
 * @brief Log categories for organizing log messages
 */
enum class LogCategory {
    System,       // System-level operations
    Database,     // Database operations
    UI,          // User interface operations
    ML,          // Machine learning operations
    Performance, // Performance monitoring
    Security,    // Security-related events
    Network,     // Network/IoT operations
    Audio,       // Audio processing
    User         // User actions
};

/**
 * @brief Structured log entry with metadata
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string function;
    std::string file;
    int line = 0;
    std::thread::id threadId;
    
    // Additional metadata
    std::map<std::string, std::string> metadata;
    
    // Performance metrics (optional)
    std::chrono::microseconds duration{0};
    size_t memoryUsage = 0;
    
    LogEntry(LogLevel lvl, LogCategory cat, const std::string& msg,
             const std::string& func = "", const std::string& fil = "", int ln = 0)
        : timestamp(std::chrono::system_clock::now()), level(lvl), category(cat),
          message(msg), function(func), file(fil), line(ln), 
          threadId(std::this_thread::get_id()) {}
};

/**
 * @brief Log output destination interface
 */
class LogOutput {
public:
    virtual ~LogOutput() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

/**
 * @brief Console log output
 */
class ConsoleLogOutput : public LogOutput {
public:
    ConsoleLogOutput(bool colorEnabled = true);
    void write(const LogEntry& entry) override;
    void flush() override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    
    void setColorEnabled(bool enabled) { colorEnabled_ = enabled; }
    
private:
    bool enabled_ = true;
    bool colorEnabled_ = true;
    std::mutex consoleMutex_;
    
    std::string getColorCode(LogLevel level) const;
    std::string resetColor() const;
};

/**
 * @brief File log output with rotation support
 */
class FileLogOutput : public LogOutput {
public:
    explicit FileLogOutput(const std::string& baseFilename, 
                          size_t maxFileSizeBytes = 10 * 1024 * 1024,  // 10MB
                          int maxFiles = 5);
    ~FileLogOutput();
    
    void write(const LogEntry& entry) override;
    void flush() override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    
    void setMaxFileSize(size_t bytes) { maxFileSizeBytes_ = bytes; }
    void setMaxFiles(int count) { maxFiles_ = count; }
    
private:
    bool enabled_ = true;
    std::string baseFilename_;
    size_t maxFileSizeBytes_;
    int maxFiles_;
    std::unique_ptr<std::ofstream> currentFile_;
    size_t currentFileSize_ = 0;
    std::mutex fileMutex_;
    
    void rotateFile();
    void openCurrentFile();
    std::string getCurrentFilename() const;
    std::string getRotatedFilename(int index) const;
};

/**
 * @brief Network log output for remote logging
 */
class NetworkLogOutput : public LogOutput {
public:
    explicit NetworkLogOutput(const std::string& endpoint);
    ~NetworkLogOutput();
    
    void write(const LogEntry& entry) override;
    void flush() override;
    bool isEnabled() const override { return enabled_; }
    void setEnabled(bool enabled) override { enabled_ = enabled; }
    
    void setEndpoint(const std::string& endpoint) { endpoint_ = endpoint; }
    void setBatchSize(int size) { batchSize_ = size; }
    void setFlushInterval(std::chrono::milliseconds interval) { flushInterval_ = interval; }
    
private:
    bool enabled_ = true;
    std::string endpoint_;
    int batchSize_ = 100;
    std::chrono::milliseconds flushInterval_{1000};
    
    std::vector<LogEntry> batch_;
    std::mutex batchMutex_;
    std::thread flushThread_;
    std::atomic<bool> shouldStop_{false};
    std::condition_variable flushCondition_;
    
    void flushWorker();
    void sendBatch(const std::vector<LogEntry>& entries);
};

/**
 * @brief Log filter for selective logging
 */
class LogFilter {
public:
    LogFilter() = default;
    
    void setMinLevel(LogLevel level) { minLevel_ = level; }
    void setEnabledCategories(const std::vector<LogCategory>& categories);
    void setDisabledCategories(const std::vector<LogCategory>& categories);
    void setThreadFilter(const std::vector<std::thread::id>& threadIds);
    void setFunctionFilter(const std::vector<std::string>& functions);
    
    bool shouldLog(const LogEntry& entry) const;
    
private:
    LogLevel minLevel_ = LogLevel::Info;
    std::vector<LogCategory> enabledCategories_;
    std::vector<LogCategory> disabledCategories_;
    std::vector<std::thread::id> filteredThreads_;
    std::vector<std::string> filteredFunctions_;
    bool hasEnabledCategories_ = false;
};

/**
 * @brief Performance-optimized production logging system
 */
class PresetLogger {
public:
    using LogFormatter = std::function<std::string(const LogEntry&)>;
    
    /**
     * @brief Constructor
     */
    PresetLogger();
    
    /**
     * @brief Destructor
     */
    ~PresetLogger();
    
    // Core logging methods
    
    /**
     * @brief Log a message with specified level and category
     */
    void log(LogLevel level, LogCategory category, const std::string& message,
             const std::string& function = "", const std::string& file = "", int line = 0);
    
    /**
     * @brief Log with additional metadata
     */
    void log(LogLevel level, LogCategory category, const std::string& message,
             const std::map<std::string, std::string>& metadata,
             const std::string& function = "", const std::string& file = "", int line = 0);
    
    /**
     * @brief Log with performance metrics
     */
    void logPerformance(LogCategory category, const std::string& operation,
                       std::chrono::microseconds duration, size_t memoryUsage = 0,
                       const std::string& function = "", const std::string& file = "", int line = 0);
    
    // Convenience methods for different log levels
    
    void trace(const std::string& message, LogCategory category = LogCategory::System,
               const std::string& function = "", const std::string& file = "", int line = 0);
    
    void debug(const std::string& message, LogCategory category = LogCategory::System,
               const std::string& function = "", const std::string& file = "", int line = 0);
    
    void info(const std::string& message, LogCategory category = LogCategory::System,
              const std::string& function = "", const std::string& file = "", int line = 0);
    
    void warning(const std::string& message, LogCategory category = LogCategory::System,
                 const std::string& function = "", const std::string& file = "", int line = 0);
    
    void error(const std::string& message, LogCategory category = LogCategory::System,
               const std::string& function = "", const std::string& file = "", int line = 0);
    
    void critical(const std::string& message, LogCategory category = LogCategory::System,
                  const std::string& function = "", const std::string& file = "", int line = 0);
    
    // Output management
    
    /**
     * @brief Add log output destination
     */
    void addOutput(std::shared_ptr<LogOutput> output);
    
    /**
     * @brief Remove log output destination
     */
    void removeOutput(std::shared_ptr<LogOutput> output);
    
    /**
     * @brief Clear all output destinations
     */
    void clearOutputs();
    
    // Filtering
    
    /**
     * @brief Set log filter
     */
    void setFilter(const LogFilter& filter);
    
    /**
     * @brief Get current filter
     */
    const LogFilter& getFilter() const { return filter_; }
    
    // Configuration
    
    /**
     * @brief Set custom log formatter
     */
    void setFormatter(LogFormatter formatter);
    
    /**
     * @brief Enable/disable asynchronous logging
     */
    void setAsyncLogging(bool enabled, size_t queueSize = 1000);
    
    /**
     * @brief Set global log level
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief Enable/disable automatic performance logging
     */
    void setPerformanceLogging(bool enabled) { performanceLogging_ = enabled; }
    
    /**
     * @brief Enable/disable automatic memory usage logging
     */
    void setMemoryLogging(bool enabled) { memoryLogging_ = enabled; }
    
    // Statistics and monitoring
    
    struct LogStatistics {
        int totalMessages = 0;
        int messagesPerLevel[6] = {0}; // One for each LogLevel
        int messagesPerCategory[9] = {0}; // One for each LogCategory
        int droppedMessages = 0;
        size_t queueSize = 0;
        std::chrono::milliseconds averageProcessingTime{0};
        std::chrono::system_clock::time_point lastLogTime;
    };
    
    /**
     * @brief Get logging statistics
     */
    LogStatistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Force flush all outputs
     */
    void flush();
    
    /**
     * @brief Get singleton instance
     */
    static PresetLogger& getInstance();
    
    // Utility methods
    
    /**
     * @brief Convert log level to string
     */
    static std::string levelToString(LogLevel level);
    
    /**
     * @brief Convert category to string
     */
    static std::string categoryToString(LogCategory category);
    
    /**
     * @brief Parse log level from string
     */
    static LogLevel stringToLevel(const std::string& level);
    
    /**
     * @brief Parse category from string
     */
    static LogCategory stringToCategory(const std::string& category);
    
    /**
     * @brief Default log formatter
     */
    static std::string defaultFormatter(const LogEntry& entry);

private:
    std::vector<std::shared_ptr<LogOutput>> outputs_;
    LogFilter filter_;
    LogFormatter formatter_;
    LogLevel globalLevel_ = LogLevel::Info;
    
    // Asynchronous logging
    bool asyncEnabled_ = false;
    size_t asyncQueueSize_ = 1000;
    std::queue<LogEntry> asyncQueue_;
    std::mutex asyncMutex_;
    std::condition_variable asyncCondition_;
    std::thread asyncWorker_;
    std::atomic<bool> shouldStop_{false};
    
    // Performance monitoring
    bool performanceLogging_ = true;
    bool memoryLogging_ = false;
    
    // Statistics
    mutable std::mutex statsMutex_;
    mutable LogStatistics stats_;
    
    // Internal methods
    void processLogEntry(const LogEntry& entry);
    void writeToOutputs(const LogEntry& entry);
    void asyncWorkerFunction();
    void updateStatistics(const LogEntry& entry);
    size_t getCurrentMemoryUsage() const;
};

/**
 * @brief RAII class for automatic performance logging
 */
class PerformanceTimer {
public:
    PerformanceTimer(const std::string& operation, LogCategory category = LogCategory::Performance,
                    const std::string& function = "", const std::string& file = "", int line = 0);
    ~PerformanceTimer();
    
    void addMetadata(const std::string& key, const std::string& value);
    void setMemoryTracking(bool enabled) { trackMemory_ = enabled; }
    
private:
    std::string operation_;
    LogCategory category_;
    std::string function_;
    std::string file_;
    int line_;
    std::chrono::high_resolution_clock::time_point startTime_;
    size_t startMemory_;
    bool trackMemory_ = false;
    std::map<std::string, std::string> metadata_;
};

/**
 * @brief Convenient macros for logging with automatic context
 */
#define LOG_TRACE(message, category) \
    PresetLogger::getInstance().trace(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_DEBUG(message, category) \
    PresetLogger::getInstance().debug(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_INFO(message, category) \
    PresetLogger::getInstance().info(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_WARNING(message, category) \
    PresetLogger::getInstance().warning(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_ERROR(message, category) \
    PresetLogger::getInstance().error(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_CRITICAL(message, category) \
    PresetLogger::getInstance().critical(message, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_PERFORMANCE(operation, category) \
    PerformanceTimer _perfTimer(operation, category, __FUNCTION__, __FILE__, __LINE__)

#define LOG_PERFORMANCE_WITH_MEMORY(operation, category) \
    PerformanceTimer _perfTimer(operation, category, __FUNCTION__, __FILE__, __LINE__); \
    _perfTimer.setMemoryTracking(true)

} // namespace AIMusicHardware