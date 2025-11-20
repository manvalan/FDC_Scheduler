#include "fdc_scheduler/logger.hpp"
#include <iostream>
#include <ctime>
#include <thread>
#include <cstring>

namespace fdc_scheduler {

// ============================================================================
// LogMessage Implementation
// ============================================================================

std::string LogMessage::level_string() const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

std::string LogMessage::format() const {
    std::ostringstream oss;
    
    // Timestamp
    auto time_t_now = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    char time_buf[100];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t_now));
    
    oss << "[" << time_buf << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
    
    // Level
    oss << "[" << std::setw(5) << level_string() << "] ";
    
    // Logger name
    if (!logger_name.empty()) {
        oss << "[" << logger_name << "] ";
    }
    
    // Message
    oss << message;
    
    // Source location (optional, can be controlled by a flag)
    // oss << " (" << file << ":" << line << ")";
    
    return oss.str();
}

// ============================================================================
// ConsoleSink Implementation
// ============================================================================

ConsoleSink::ConsoleSink(bool use_colors) : use_colors_(use_colors) {
}

std::string ConsoleSink::get_color_code(LogLevel level) const {
    if (!use_colors_) return "";
    
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";      // White
        case LogLevel::DEBUG: return "\033[36m";      // Cyan
        case LogLevel::INFO: return "\033[32m";       // Green
        case LogLevel::WARNING: return "\033[33m";    // Yellow
        case LogLevel::ERROR: return "\033[31m";      // Red
        case LogLevel::CRITICAL: return "\033[35m";   // Magenta
        default: return "";
    }
}

std::string ConsoleSink::get_reset_code() const {
    return use_colors_ ? "\033[0m" : "";
}

void ConsoleSink::log(const LogMessage& msg) {
    if (!should_log(msg.level)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stream = (msg.level >= LogLevel::ERROR) ? std::cerr : std::cout;
    
    stream << get_color_code(msg.level) 
           << msg.format() 
           << get_reset_code() 
           << std::endl;
}

void ConsoleSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout.flush();
    std::cerr.flush();
}

// ============================================================================
// FileSink Implementation
// ============================================================================

FileSink::FileSink(const std::string& filename, size_t max_size_mb, size_t max_files)
    : base_filename_(filename)
    , max_size_bytes_(max_size_mb * 1024 * 1024)
    , max_files_(max_files)
    , current_size_(0) {
    open_file();
}

FileSink::~FileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

void FileSink::open_file() {
    file_.open(base_filename_, std::ios::app);
    if (file_.is_open()) {
        file_.seekp(0, std::ios::end);
        current_size_ = file_.tellp();
    }
}

void FileSink::rotate_files() {
    file_.close();
    
    // Delete oldest file
    if (max_files_ > 0) {
        std::string oldest = base_filename_ + "." + std::to_string(max_files_);
        std::remove(oldest.c_str());
    }
    
    // Rotate existing files
    for (int i = max_files_ - 1; i >= 1; --i) {
        std::string old_name = base_filename_ + "." + std::to_string(i);
        std::string new_name = base_filename_ + "." + std::to_string(i + 1);
        std::rename(old_name.c_str(), new_name.c_str());
    }
    
    // Rotate current file
    std::string rotated = base_filename_ + ".1";
    std::rename(base_filename_.c_str(), rotated.c_str());
    
    // Open new file
    current_size_ = 0;
    open_file();
}

void FileSink::log(const LogMessage& msg) {
    if (!should_log(msg.level)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!file_.is_open()) {
        open_file();
    }
    
    std::string formatted = msg.format() + "\n";
    
    // Check if rotation needed
    if (current_size_ + formatted.size() > max_size_bytes_) {
        rotate_files();
    }
    
    file_ << formatted;
    current_size_ += formatted.size();
}

void FileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

// ============================================================================
// DailyFileSink Implementation
// ============================================================================

DailyFileSink::DailyFileSink(const std::string& base_filename, int rotation_hour)
    : base_filename_(base_filename)
    , rotation_hour_(rotation_hour) {
    current_filename_ = get_filename();
    file_.open(current_filename_, std::ios::app);
}

DailyFileSink::~DailyFileSink() {
    if (file_.is_open()) {
        file_.close();
    }
}

std::string DailyFileSink::get_filename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    char date_buf[20];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", std::localtime(&time_t_now));
    
    return base_filename_ + "_" + date_buf + ".log";
}

void DailyFileSink::check_rotation() {
    std::string new_filename = get_filename();
    
    if (new_filename != current_filename_) {
        if (file_.is_open()) {
            file_.close();
        }
        
        current_filename_ = new_filename;
        file_.open(current_filename_, std::ios::app);
    }
}

void DailyFileSink::log(const LogMessage& msg) {
    if (!should_log(msg.level)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    check_rotation();
    
    if (file_.is_open()) {
        file_ << msg.format() << "\n";
    }
}

void DailyFileSink::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

// ============================================================================
// CallbackSink Implementation
// ============================================================================

CallbackSink::CallbackSink(Callback callback) : callback_(callback) {
}

void CallbackSink::log(const LogMessage& msg) {
    if (!should_log(msg.level)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_) {
        callback_(msg);
    }
}

void CallbackSink::flush() {
    // Nothing to flush for callback
}

// ============================================================================
// Logger Implementation
// ============================================================================

Logger::Logger(const std::string& name) 
    : name_(name)
    , level_(LogLevel::INFO) {
}

void Logger::add_sink(std::shared_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(sink);
}

void Logger::clear_sinks() {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.clear();
}

void Logger::log(LogLevel level, const std::string& message,
                const char* file, int line, const char* function) {
    if (level < level_) return;
    
    LogMessage msg;
    msg.level = level;
    msg.logger_name = name_;
    msg.message = message;
    msg.file = file ? file : "";
    msg.line = line;
    msg.function = function ? function : "";
    msg.timestamp = std::chrono::system_clock::now();
    msg.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        sink->log(msg);
    }
}

void Logger::trace(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::INFO, message, file, line, function);
}

void Logger::warning(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::WARNING, message, file, line, function);
}

void Logger::error(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::ERROR, message, file, line, function);
}

void Logger::critical(const std::string& message, const char* file, int line, const char* function) {
    log(LogLevel::CRITICAL, message, file, line, function);
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::get_level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        sink->flush();
    }
}

// ============================================================================
// LoggerRegistry Implementation
// ============================================================================

LoggerRegistry& LoggerRegistry::instance() {
    static LoggerRegistry registry;
    return registry;
}

LoggerRegistry::LoggerRegistry() {
    // Create default logger
    default_logger_ = create_console_logger("default");
}

std::shared_ptr<Logger> LoggerRegistry::get(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::shared_ptr<Logger> LoggerRegistry::get_or_create(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return it->second;
    }
    
    auto logger = std::make_shared<Logger>(name);
    loggers_[name] = logger;
    return logger;
}

void LoggerRegistry::set_default_logger(std::shared_ptr<Logger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    default_logger_ = logger;
}

std::shared_ptr<Logger> LoggerRegistry::default_logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    return default_logger_;
}

void LoggerRegistry::set_global_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, logger] : loggers_) {
        logger->set_level(level);
    }
    if (default_logger_) {
        default_logger_->set_level(level);
    }
}

void LoggerRegistry::flush_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, logger] : loggers_) {
        logger->flush();
    }
    if (default_logger_) {
        default_logger_->flush();
    }
}

std::shared_ptr<Logger> LoggerRegistry::create_console_logger(const std::string& name) {
    auto logger = std::make_shared<Logger>(name);
    logger->add_sink(std::make_shared<ConsoleSink>(true));
    return logger;
}

std::shared_ptr<Logger> LoggerRegistry::create_file_logger(const std::string& name,
                                                           const std::string& filename) {
    auto logger = std::make_shared<Logger>(name);
    logger->add_sink(std::make_shared<FileSink>(filename));
    return logger;
}

std::shared_ptr<Logger> LoggerRegistry::create_rotating_logger(const std::string& name,
                                                               const std::string& filename,
                                                               size_t max_size_mb,
                                                               size_t max_files) {
    auto logger = std::make_shared<Logger>(name);
    logger->add_sink(std::make_shared<FileSink>(filename, max_size_mb, max_files));
    return logger;
}

} // namespace fdc_scheduler
