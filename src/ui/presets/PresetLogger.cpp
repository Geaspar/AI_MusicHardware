#include "ui/presets/PresetLogger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <unistd.h>
    #include <sys/resource.h>
#endif

namespace AIMusicHardware {

// ConsoleLogOutput implementation
ConsoleLogOutput::ConsoleLogOutput(bool colorEnabled) : colorEnabled_(colorEnabled) {}

void ConsoleLogOutput::write(const LogEntry& entry) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(consoleMutex_);
    
    if (colorEnabled_) {
        std::cout << getColorCode(entry.level);
    }
    
    std::cout << PresetLogger::defaultFormatter(entry);
    
    if (colorEnabled_) {
        std::cout << resetColor();
    }
    
    std::cout << std::endl;
}

void ConsoleLogOutput::flush() {
    std::cout.flush();
}

std::string ConsoleLogOutput::getColorCode(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:    return "\033[90m";  // Bright black
        case LogLevel::Debug:    return "\033[36m";  // Cyan
        case LogLevel::Info:     return "\033[32m";  // Green
        case LogLevel::Warning:  return "\033[33m";  // Yellow
        case LogLevel::Error:    return "\033[31m";  // Red
        case LogLevel::Critical: return "\033[35m";  // Magenta
        default:                 return "";
    }
}

std::string ConsoleLogOutput::resetColor() const {
    return "\033[0m";
}

// FileLogOutput implementation
FileLogOutput::FileLogOutput(const std::string& baseFilename, size_t maxFileSizeBytes, int maxFiles)
    : baseFilename_(baseFilename), maxFileSizeBytes_(maxFileSizeBytes), maxFiles_(maxFiles) {
    openCurrentFile();
}

FileLogOutput::~FileLogOutput() {
    if (currentFile_ && currentFile_->is_open()) {
        currentFile_->close();
    }
}

void FileLogOutput::write(const LogEntry& entry) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (!currentFile_ || !currentFile_->is_open()) {
        openCurrentFile();
    }
    
    if (currentFile_ && currentFile_->is_open()) {
        std::string formatted = PresetLogger::defaultFormatter(entry);
        *currentFile_ << formatted << std::endl;
        currentFileSize_ += formatted.length() + 1;
        
        if (currentFileSize_ > maxFileSizeBytes_) {
            rotateFile();
        }
    }
}

void FileLogOutput::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (currentFile_) {
        currentFile_->flush();
    }
}

void FileLogOutput::rotateFile() {
    if (currentFile_) {
        currentFile_->close();
    }
    
    // Rotate existing files
    for (int i = maxFiles_ - 1; i >= 1; i--) {
        std::string oldFile = getRotatedFilename(i - 1);
        std::string newFile = getRotatedFilename(i);
        
        if (std::filesystem::exists(oldFile)) {
            try {
                std::filesystem::rename(oldFile, newFile);
            } catch (const std::exception&) {
                // Ignore rotation errors
            }
        }
    }
    
    // Move current file to .1
    std::string currentName = getCurrentFilename();
    if (std::filesystem::exists(currentName)) {
        try {
            std::filesystem::rename(currentName, getRotatedFilename(0));
        } catch (const std::exception&) {
            // Ignore rotation errors
        }
    }
    
    openCurrentFile();
}

void FileLogOutput::openCurrentFile() {
    currentFile_ = std::make_unique<std::ofstream>(getCurrentFilename(), std::ios::app);
    currentFileSize_ = 0;
    
    if (currentFile_->is_open()) {
        // Get current file size
        currentFile_->seekp(0, std::ios::end);
        currentFileSize_ = static_cast<size_t>(currentFile_->tellp());
    }
}

std::string FileLogOutput::getCurrentFilename() const {
    return baseFilename_;
}

std::string FileLogOutput::getRotatedFilename(int index) const {
    return baseFilename_ + "." + std::to_string(index + 1);
}

// NetworkLogOutput implementation
NetworkLogOutput::NetworkLogOutput(const std::string& endpoint) 
    : endpoint_(endpoint), flushThread_(&NetworkLogOutput::flushWorker, this) {}

NetworkLogOutput::~NetworkLogOutput() {
    shouldStop_ = true;
    flushCondition_.notify_all();
    if (flushThread_.joinable()) {
        flushThread_.join();
    }
}

void NetworkLogOutput::write(const LogEntry& entry) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(batchMutex_);
    batch_.push_back(entry);
    
    if (static_cast<int>(batch_.size()) >= batchSize_) {
        flushCondition_.notify_one();
    }
}

void NetworkLogOutput::flush() {
    flushCondition_.notify_one();
}

void NetworkLogOutput::flushWorker() {
    while (!shouldStop_) {
        std::unique_lock<std::mutex> lock(batchMutex_);
        
        flushCondition_.wait_for(lock, flushInterval_, [this] {
            return shouldStop_ || static_cast<int>(batch_.size()) >= batchSize_;
        });
        
        if (!batch_.empty()) {
            std::vector<LogEntry> toSend = std::move(batch_);
            batch_.clear();
            lock.unlock();
            
            sendBatch(toSend);
        }
    }
}

void NetworkLogOutput::sendBatch(const std::vector<LogEntry>& entries) {
    // Implementation would depend on specific network protocol
    // For now, just a placeholder
    (void)entries; // Suppress unused parameter warning
}

// LogFilter implementation
void LogFilter::setEnabledCategories(const std::vector<LogCategory>& categories) {
    enabledCategories_ = categories;
    hasEnabledCategories_ = !categories.empty();
}

void LogFilter::setDisabledCategories(const std::vector<LogCategory>& categories) {
    disabledCategories_ = categories;
}

void LogFilter::setThreadFilter(const std::vector<std::thread::id>& threadIds) {
    filteredThreads_ = threadIds;
}

void LogFilter::setFunctionFilter(const std::vector<std::string>& functions) {
    filteredFunctions_ = functions;
}

bool LogFilter::shouldLog(const LogEntry& entry) const {
    // Check minimum level
    if (entry.level < minLevel_) {
        return false;
    }
    
    // Check enabled categories
    if (hasEnabledCategories_) {
        if (std::find(enabledCategories_.begin(), enabledCategories_.end(), entry.category) == enabledCategories_.end()) {
            return false;
        }
    }
    
    // Check disabled categories
    if (std::find(disabledCategories_.begin(), disabledCategories_.end(), entry.category) != disabledCategories_.end()) {
        return false;
    }
    
    // Check thread filter
    if (!filteredThreads_.empty()) {
        if (std::find(filteredThreads_.begin(), filteredThreads_.end(), entry.threadId) == filteredThreads_.end()) {
            return false;
        }
    }
    
    // Check function filter
    if (!filteredFunctions_.empty()) {
        if (std::find(filteredFunctions_.begin(), filteredFunctions_.end(), entry.function) == filteredFunctions_.end()) {
            return false;
        }
    }
    
    return true;
}

// PresetLogger implementation
PresetLogger::PresetLogger() : formatter_(defaultFormatter) {}

PresetLogger::~PresetLogger() {
    if (asyncEnabled_) {
        shouldStop_ = true;
        asyncCondition_.notify_all();
        if (asyncWorker_.joinable()) {
            asyncWorker_.join();
        }
    }
}

void PresetLogger::log(LogLevel level, LogCategory category, const std::string& message,
                      const std::string& function, const std::string& file, int line) {
    if (level < globalLevel_) return;
    
    LogEntry entry(level, category, message, function, file, line);
    
    if (memoryLogging_) {
        entry.memoryUsage = getCurrentMemoryUsage();
    }
    
    processLogEntry(entry);
}

void PresetLogger::log(LogLevel level, LogCategory category, const std::string& message,
                      const std::map<std::string, std::string>& metadata,
                      const std::string& function, const std::string& file, int line) {
    if (level < globalLevel_) return;
    
    LogEntry entry(level, category, message, function, file, line);
    entry.metadata = metadata;
    
    if (memoryLogging_) {
        entry.memoryUsage = getCurrentMemoryUsage();
    }
    
    processLogEntry(entry);
}

void PresetLogger::logPerformance(LogCategory category, const std::string& operation,
                                 std::chrono::microseconds duration, size_t memoryUsage,
                                 const std::string& function, const std::string& file, int line) {
    if (!performanceLogging_) return;
    
    LogEntry entry(LogLevel::Info, category, "Performance: " + operation, function, file, line);
    entry.duration = duration;
    entry.memoryUsage = memoryUsage;
    entry.metadata["operation"] = operation;
    entry.metadata["duration_us"] = std::to_string(duration.count());
    if (memoryUsage > 0) {
        entry.metadata["memory_bytes"] = std::to_string(memoryUsage);
    }
    
    processLogEntry(entry);
}

void PresetLogger::trace(const std::string& message, LogCategory category,
                        const std::string& function, const std::string& file, int line) {
    log(LogLevel::Trace, category, message, function, file, line);
}

void PresetLogger::debug(const std::string& message, LogCategory category,
                        const std::string& function, const std::string& file, int line) {
    log(LogLevel::Debug, category, message, function, file, line);
}

void PresetLogger::info(const std::string& message, LogCategory category,
                       const std::string& function, const std::string& file, int line) {
    log(LogLevel::Info, category, message, function, file, line);
}

void PresetLogger::warning(const std::string& message, LogCategory category,
                          const std::string& function, const std::string& file, int line) {
    log(LogLevel::Warning, category, message, function, file, line);
}

void PresetLogger::error(const std::string& message, LogCategory category,
                        const std::string& function, const std::string& file, int line) {
    log(LogLevel::Error, category, message, function, file, line);
}

void PresetLogger::critical(const std::string& message, LogCategory category,
                           const std::string& function, const std::string& file, int line) {
    log(LogLevel::Critical, category, message, function, file, line);
}

void PresetLogger::addOutput(std::shared_ptr<LogOutput> output) {
    outputs_.push_back(output);
}

void PresetLogger::removeOutput(std::shared_ptr<LogOutput> output) {
    outputs_.erase(std::remove(outputs_.begin(), outputs_.end(), output), outputs_.end());
}

void PresetLogger::clearOutputs() {
    outputs_.clear();
}

void PresetLogger::setFilter(const LogFilter& filter) {
    filter_ = filter;
}

void PresetLogger::setFormatter(LogFormatter formatter) {
    formatter_ = formatter;
}

void PresetLogger::setAsyncLogging(bool enabled, size_t queueSize) {
    if (enabled && !asyncEnabled_) {
        asyncEnabled_ = true;
        asyncQueueSize_ = queueSize;
        shouldStop_ = false;
        asyncWorker_ = std::thread(&PresetLogger::asyncWorkerFunction, this);
    } else if (!enabled && asyncEnabled_) {
        shouldStop_ = true;
        asyncCondition_.notify_all();
        if (asyncWorker_.joinable()) {
            asyncWorker_.join();
        }
        asyncEnabled_ = false;
    }
}

void PresetLogger::setLogLevel(LogLevel level) {
    globalLevel_ = level;
}

PresetLogger::LogStatistics PresetLogger::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void PresetLogger::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = LogStatistics{};
}

void PresetLogger::flush() {
    for (auto& output : outputs_) {
        output->flush();
    }
}

PresetLogger& PresetLogger::getInstance() {
    static PresetLogger instance;
    return instance;
}

std::string PresetLogger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARNING";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string PresetLogger::categoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::System:      return "System";
        case LogCategory::Database:    return "Database";
        case LogCategory::UI:          return "UI";
        case LogCategory::ML:          return "ML";
        case LogCategory::Performance: return "Performance";
        case LogCategory::Security:    return "Security";
        case LogCategory::Network:     return "Network";
        case LogCategory::Audio:       return "Audio";
        case LogCategory::User:        return "User";
        default:                       return "Unknown";
    }
}

LogLevel PresetLogger::stringToLevel(const std::string& level) {
    std::string upper = level;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "TRACE") return LogLevel::Trace;
    if (upper == "DEBUG") return LogLevel::Debug;
    if (upper == "INFO") return LogLevel::Info;
    if (upper == "WARNING") return LogLevel::Warning;
    if (upper == "ERROR") return LogLevel::Error;
    if (upper == "CRITICAL") return LogLevel::Critical;
    
    return LogLevel::Info; // Default
}

LogCategory PresetLogger::stringToCategory(const std::string& category) {
    std::string lower = category;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "system") return LogCategory::System;
    if (lower == "database") return LogCategory::Database;
    if (lower == "ui") return LogCategory::UI;
    if (lower == "ml") return LogCategory::ML;
    if (lower == "performance") return LogCategory::Performance;
    if (lower == "security") return LogCategory::Security;
    if (lower == "network") return LogCategory::Network;
    if (lower == "audio") return LogCategory::Audio;
    if (lower == "user") return LogCategory::User;
    
    return LogCategory::System; // Default
}

std::string PresetLogger::defaultFormatter(const LogEntry& entry) {
    std::ostringstream ss;
    
    // Timestamp
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;
    
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    // Level and category
    ss << " [" << std::setw(8) << std::left << levelToString(entry.level) << "]";
    ss << " [" << std::setw(11) << std::left << categoryToString(entry.category) << "]";
    
    // Thread ID
    ss << " [" << entry.threadId << "]";
    
    // Function and location
    if (!entry.function.empty()) {
        ss << " " << entry.function;
        if (!entry.file.empty() && entry.line > 0) {
            ss << " (" << std::filesystem::path(entry.file).filename().string() 
               << ":" << entry.line << ")";
        }
    }
    
    // Message
    ss << " - " << entry.message;
    
    // Performance metrics
    if (entry.duration.count() > 0) {
        ss << " [" << entry.duration.count() << "μs]";
    }
    if (entry.memoryUsage > 0) {
        ss << " [" << entry.memoryUsage << " bytes]";
    }
    
    // Metadata
    if (!entry.metadata.empty()) {
        ss << " {";
        bool first = true;
        for (const auto& [key, value] : entry.metadata) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << "}";
    }
    
    return ss.str();
}

void PresetLogger::processLogEntry(const LogEntry& entry) {
    if (!filter_.shouldLog(entry)) {
        return;
    }
    
    updateStatistics(entry);
    
    if (asyncEnabled_) {
        std::lock_guard<std::mutex> lock(asyncMutex_);
        if (asyncQueue_.size() < asyncQueueSize_) {
            asyncQueue_.push(entry);
            asyncCondition_.notify_one();
        } else {
            // Queue full, drop message and update statistics
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.droppedMessages++;
        }
    } else {
        writeToOutputs(entry);
    }
}

void PresetLogger::writeToOutputs(const LogEntry& entry) {
    for (auto& output : outputs_) {
        if (output && output->isEnabled()) {
            try {
                output->write(entry);
            } catch (const std::exception&) {
                // Ignore output errors to prevent logging loops
            }
        }
    }
}

void PresetLogger::asyncWorkerFunction() {
    while (!shouldStop_) {
        std::unique_lock<std::mutex> lock(asyncMutex_);
        
        asyncCondition_.wait(lock, [this] {
            return shouldStop_ || !asyncQueue_.empty();
        });
        
        while (!asyncQueue_.empty()) {
            LogEntry entry = asyncQueue_.front();
            asyncQueue_.pop();
            lock.unlock();
            
            writeToOutputs(entry);
            
            lock.lock();
        }
    }
}

void PresetLogger::updateStatistics(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    stats_.totalMessages++;
    stats_.messagesPerLevel[static_cast<int>(entry.level)]++;
    stats_.messagesPerCategory[static_cast<int>(entry.category)]++;
    stats_.lastLogTime = entry.timestamp;
    
    if (asyncEnabled_) {
        stats_.queueSize = asyncQueue_.size();
    }
}

size_t PresetLogger::getCurrentMemoryUsage() const {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert KB to bytes on Linux
    }
#endif
    return 0;
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer(const std::string& operation, LogCategory category,
                                  const std::string& function, const std::string& file, int line)
    : operation_(operation), category_(category), function_(function), file_(file), line_(line),
      startTime_(std::chrono::high_resolution_clock::now()) {
    
    if (trackMemory_) {
        startMemory_ = PresetLogger::getInstance().getCurrentMemoryUsage();
    }
}

PerformanceTimer::~PerformanceTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);
    
    size_t memoryDelta = 0;
    if (trackMemory_) {
        size_t endMemory = PresetLogger::getInstance().getCurrentMemoryUsage();
        if (endMemory > startMemory_) {
            memoryDelta = endMemory - startMemory_;
        }
    }
    
    auto& logger = PresetLogger::getInstance();
    
    // Add metadata to the performance log
    if (!metadata_.empty()) {
        logger.log(LogLevel::Info, category_, "Performance: " + operation_, metadata_,
                  function_, file_, line_);
    }
    
    logger.logPerformance(category_, operation_, duration, memoryDelta,
                         function_, file_, line_);
}

void PerformanceTimer::addMetadata(const std::string& key, const std::string& value) {
    metadata_[key] = value;
}

} // namespace AIMusicHardware