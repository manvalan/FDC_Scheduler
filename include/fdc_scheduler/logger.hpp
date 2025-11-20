#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <sstream>
#include <chrono>
#include <fstream>
#include <mutex>
#include <iomanip>

namespace fdc_scheduler {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

/**
 * @brief Log message structure
 */
struct LogMessage {
    LogLevel level;
    std::string logger_name;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    size_t thread_id;
    
    std::string format() const;
    std::string level_string() const;
};

/**
 * @brief Abstract sink interface for log output
 */
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void log(const LogMessage& msg) = 0;
    virtual void flush() = 0;
    
    void set_level(LogLevel level) { level_ = level; }
    LogLevel get_level() const { return level_; }
    
    bool should_log(LogLevel level) const {
        return level >= level_;
    }
    
protected:
    LogLevel level_ = LogLevel::TRACE;
};

/**
 * @brief Console sink (stdout/stderr)
 */
class ConsoleSink : public LogSink {
public:
    ConsoleSink(bool use_colors = true);
    
    void log(const LogMessage& msg) override;
    void flush() override;
    
private:
    bool use_colors_;
    mutable std::mutex mutex_;
    
    std::string get_color_code(LogLevel level) const;
    std::string get_reset_code() const;
};

/**
 * @brief File sink with rotation support
 */
class FileSink : public LogSink {
public:
    FileSink(const std::string& filename, size_t max_size_mb = 10, size_t max_files = 3);
    ~FileSink() override;
    
    void log(const LogMessage& msg) override;
    void flush() override;
    
private:
    std::string base_filename_;
    size_t max_size_bytes_;
    size_t max_files_;
    size_t current_size_;
    std::ofstream file_;
    mutable std::mutex mutex_;
    
    void rotate_files();
    void open_file();
};

/**
 * @brief Rotating file sink (daily rotation)
 */
class DailyFileSink : public LogSink {
public:
    DailyFileSink(const std::string& base_filename, int rotation_hour = 0);
    ~DailyFileSink() override;
    
    void log(const LogMessage& msg) override;
    void flush() override;
    
private:
    std::string base_filename_;
    int rotation_hour_;
    std::string current_filename_;
    std::ofstream file_;
    mutable std::mutex mutex_;
    
    std::string get_filename() const;
    void check_rotation();
};

/**
 * @brief Callback sink for custom handling
 */
class CallbackSink : public LogSink {
public:
    using Callback = std::function<void(const LogMessage&)>;
    
    CallbackSink(Callback callback);
    
    void log(const LogMessage& msg) override;
    void flush() override;
    
private:
    Callback callback_;
    mutable std::mutex mutex_;
};

/**
 * @brief Logger class
 */
class Logger {
public:
    Logger(const std::string& name);
    
    // Add sinks
    void add_sink(std::shared_ptr<LogSink> sink);
    void clear_sinks();
    
    // Logging methods
    void log(LogLevel level, const std::string& message, 
             const char* file, int line, const char* function);
    
    void trace(const std::string& message, const char* file, int line, const char* function);
    void debug(const std::string& message, const char* file, int line, const char* function);
    void info(const std::string& message, const char* file, int line, const char* function);
    void warning(const std::string& message, const char* file, int line, const char* function);
    void error(const std::string& message, const char* file, int line, const char* function);
    void critical(const std::string& message, const char* file, int line, const char* function);
    
    // Formatted logging
    template<typename... Args>
    void trace(const char* file, int line, const char* function, 
               const char* fmt, Args&&... args) {
        log(LogLevel::TRACE, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    template<typename... Args>
    void debug(const char* file, int line, const char* function,
               const char* fmt, Args&&... args) {
        log(LogLevel::DEBUG, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    template<typename... Args>
    void info(const char* file, int line, const char* function,
              const char* fmt, Args&&... args) {
        log(LogLevel::INFO, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    template<typename... Args>
    void warning(const char* file, int line, const char* function,
                 const char* fmt, Args&&... args) {
        log(LogLevel::WARNING, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    template<typename... Args>
    void error(const char* file, int line, const char* function,
               const char* fmt, Args&&... args) {
        log(LogLevel::ERROR, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    template<typename... Args>
    void critical(const char* file, int line, const char* function,
                  const char* fmt, Args&&... args) {
        log(LogLevel::CRITICAL, format(fmt, std::forward<Args>(args)...), file, line, function);
    }
    
    // Level control
    void set_level(LogLevel level);
    LogLevel get_level() const;
    
    // Flush all sinks
    void flush();
    
    const std::string& name() const { return name_; }
    
private:
    std::string name_;
    LogLevel level_;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    mutable std::mutex mutex_;
    
    template<typename... Args>
    std::string format(const char* fmt, Args&&... args) {
        size_t size = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, fmt, std::forward<Args>(args)...);
        return std::string(buf.get(), buf.get() + size - 1);
    }
};

/**
 * @brief Global logger registry
 */
class LoggerRegistry {
public:
    static LoggerRegistry& instance();
    
    std::shared_ptr<Logger> get(const std::string& name);
    std::shared_ptr<Logger> get_or_create(const std::string& name);
    
    void set_default_logger(std::shared_ptr<Logger> logger);
    std::shared_ptr<Logger> default_logger();
    
    void set_global_level(LogLevel level);
    void flush_all();
    
    // Create common logger configurations
    static std::shared_ptr<Logger> create_console_logger(const std::string& name);
    static std::shared_ptr<Logger> create_file_logger(const std::string& name, 
                                                       const std::string& filename);
    static std::shared_ptr<Logger> create_rotating_logger(const std::string& name,
                                                          const std::string& filename,
                                                          size_t max_size_mb = 10,
                                                          size_t max_files = 3);
    
private:
    LoggerRegistry();
    
    std::map<std::string, std::shared_ptr<Logger>> loggers_;
    std::shared_ptr<Logger> default_logger_;
    mutable std::mutex mutex_;
};

// Convenience macros
#define FDC_TRACE(logger, ...) (logger)->trace(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define FDC_DEBUG(logger, ...) (logger)->debug(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define FDC_INFO(logger, ...)  (logger)->info(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define FDC_WARN(logger, ...)  (logger)->warning(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define FDC_ERROR(logger, ...) (logger)->error(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define FDC_CRITICAL(logger, ...) (logger)->critical(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// Default logger macros
#define LOG_TRACE(...) FDC_TRACE(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)
#define LOG_DEBUG(...) FDC_DEBUG(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)
#define LOG_INFO(...)  FDC_INFO(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)
#define LOG_WARN(...)  FDC_WARN(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)
#define LOG_ERROR(...) FDC_ERROR(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)
#define LOG_CRITICAL(...) FDC_CRITICAL(fdc_scheduler::LoggerRegistry::instance().default_logger(), __VA_ARGS__)

} // namespace fdc_scheduler
