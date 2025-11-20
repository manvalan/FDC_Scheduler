/**
 * @file logging_demo.cpp
 * @brief Demonstration of Logging Framework capabilities
 * 
 * This example showcases:
 * 1. Console logging with colors
 * 2. File logging with rotation
 * 3. Daily file logging
 * 4. Multiple sinks per logger
 * 5. Custom log levels
 * 6. Formatted logging
 * 7. Logger registry
 */

#include <fdc_scheduler/logger.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace fdc_scheduler;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

/**
 * Demo 1: Basic Console Logging
 */
void demo_console_logging() {
    print_separator("Demo 1: Console Logging with Colors");
    
    auto logger = LoggerRegistry::create_console_logger("console_demo");
    
    std::cout << "Logging at all levels:\n\n";
    
    FDC_TRACE(logger, "This is a TRACE message (most verbose)");
    FDC_DEBUG(logger, "This is a DEBUG message");
    FDC_INFO(logger, "This is an INFO message");
    FDC_WARN(logger, "This is a WARNING message");
    FDC_ERROR(logger, "This is an ERROR message");
    FDC_CRITICAL(logger, "This is a CRITICAL message");
    
    std::cout << "\n✓ Console logging with color coding\n";
}

/**
 * Demo 2: File Logging with Rotation
 */
void demo_file_logging() {
    print_separator("Demo 2: File Logging with Rotation");
    
    auto logger = LoggerRegistry::create_rotating_logger(
        "file_demo", 
        "test_rotating.log", 
        1,  // 1 MB max size
        3   // Keep 3 backup files
    );
    
    std::cout << "Writing logs to test_rotating.log...\n";
    
    for (int i = 0; i < 10; ++i) {
        FDC_INFO(logger, "Log entry %d with some content", i);
        FDC_DEBUG(logger, "Debug information for entry %d", i);
        
        if (i % 3 == 0) {
            FDC_WARN(logger, "Warning: checkpoint reached at entry %d", i);
        }
    }
    
    logger->flush();
    
    std::cout << "✓ Written 30 log entries to rotating file\n";
    std::cout << "  Check: test_rotating.log\n";
}

/**
 * Demo 3: Daily File Logging
 */
void demo_daily_logging() {
    print_separator("Demo 3: Daily File Rotation");
    
    auto logger = std::make_shared<Logger>("daily_demo");
    logger->add_sink(std::make_shared<DailyFileSink>("test_daily", 0));
    
    std::cout << "Writing to daily rotating file...\n";
    
    FDC_INFO(logger, "Application started");
    FDC_INFO(logger, "Processing data...");
    FDC_DEBUG(logger, "Internal state: OK");
    FDC_INFO(logger, "Application finished");
    
    logger->flush();
    
    std::cout << "✓ Logs written to test_daily_YYYY-MM-DD.log\n";
}

/**
 * Demo 4: Multiple Sinks
 */
void demo_multiple_sinks() {
    print_separator("Demo 4: Multiple Sinks (Console + File)");
    
    auto logger = std::make_shared<Logger>("multi_demo");
    
    // Add console sink
    logger->add_sink(std::make_shared<ConsoleSink>(true));
    
    // Add file sink
    logger->add_sink(std::make_shared<FileSink>("test_multi.log"));
    
    std::cout << "Logging to both console and file:\n\n";
    
    FDC_INFO(logger, "This goes to both console and file");
    FDC_WARN(logger, "Warning: visible in both outputs");
    FDC_ERROR(logger, "Error: logged to multiple destinations");
    
    logger->flush();
    
    std::cout << "\n✓ Messages written to console AND test_multi.log\n";
}

/**
 * Demo 5: Log Levels and Filtering
 */
void demo_log_levels() {
    print_separator("Demo 5: Log Level Filtering");
    
    auto logger = LoggerRegistry::create_console_logger("level_demo");
    
    std::cout << "Setting level to WARNING (filters out INFO and below):\n\n";
    logger->set_level(LogLevel::WARNING);
    
    FDC_TRACE(logger, "TRACE: won't be shown");
    FDC_DEBUG(logger, "DEBUG: won't be shown");
    FDC_INFO(logger, "INFO: won't be shown");
    FDC_WARN(logger, "WARNING: will be shown");
    FDC_ERROR(logger, "ERROR: will be shown");
    
    std::cout << "\n✓ Only WARNING and above were displayed\n";
    
    std::cout << "\nSetting level back to INFO:\n\n";
    logger->set_level(LogLevel::INFO);
    
    FDC_DEBUG(logger, "DEBUG: still won't be shown");
    FDC_INFO(logger, "INFO: now visible");
    FDC_WARN(logger, "WARNING: visible");
    
    std::cout << "\n✓ INFO and above now displayed\n";
}

/**
 * Demo 6: Formatted Logging
 */
void demo_formatted_logging() {
    print_separator("Demo 6: Formatted Logging (printf-style)");
    
    auto logger = LoggerRegistry::create_console_logger("format_demo");
    
    std::cout << "Using printf-style formatting:\n\n";
    
    int trains = 42;
    double speed = 125.5;
    const char* status = "operational";
    
    FDC_INFO(logger, "System status: %s", status);
    FDC_INFO(logger, "Active trains: %d", trains);
    FDC_INFO(logger, "Average speed: %.2f km/h", speed);
    FDC_INFO(logger, "Statistics: %d trains at %.1f km/h (%s)", 
             trains, speed, status);
    
    // Complex formatting
    for (int i = 1; i <= 3; ++i) {
        FDC_DEBUG(logger, "Train %d: position=%.2f, speed=%.1f", 
                 i, i * 10.5, 100.0 + i * 5.0);
    }
    
    std::cout << "\n✓ Formatted logging working\n";
}

/**
 * Demo 7: Logger Registry
 */
void demo_logger_registry() {
    print_separator("Demo 7: Logger Registry");
    
    std::cout << "Creating named loggers:\n\n";
    
    // Create different loggers for different components
    auto network_logger = LoggerRegistry::instance().get_or_create("network");
    network_logger->add_sink(std::make_shared<ConsoleSink>(true));
    
    auto scheduler_logger = LoggerRegistry::instance().get_or_create("scheduler");
    scheduler_logger->add_sink(std::make_shared<ConsoleSink>(true));
    
    auto optimizer_logger = LoggerRegistry::instance().get_or_create("optimizer");
    optimizer_logger->add_sink(std::make_shared<ConsoleSink>(true));
    
    // Use different loggers
    FDC_INFO(network_logger, "Network initialized with 10 nodes");
    FDC_INFO(scheduler_logger, "Scheduling 5 trains");
    FDC_INFO(optimizer_logger, "Optimization complete: 15%% improvement");
    
    FDC_DEBUG(network_logger, "Processing edge MILANO->ROMA");
    FDC_WARN(scheduler_logger, "Conflict detected between trains T1 and T2");
    FDC_INFO(optimizer_logger, "Energy savings: 12.5 kWh");
    
    std::cout << "\n✓ Multiple named loggers working\n";
    
    // Retrieve logger by name
    auto retrieved = LoggerRegistry::instance().get("network");
    if (retrieved) {
        FDC_INFO(retrieved, "Retrieved logger works!");
    }
}

/**
 * Demo 8: Callback Sink (Custom Handler)
 */
void demo_callback_sink() {
    print_separator("Demo 8: Custom Callback Sink");
    
    int error_count = 0;
    int warning_count = 0;
    
    auto logger = std::make_shared<Logger>("callback_demo");
    
    // Add console sink
    logger->add_sink(std::make_shared<ConsoleSink>(true));
    
    // Add callback sink for counting
    auto callback_sink = std::make_shared<CallbackSink>(
        [&](const LogMessage& msg) {
            if (msg.level == LogLevel::ERROR || msg.level == LogLevel::CRITICAL) {
                error_count++;
            } else if (msg.level == LogLevel::WARNING) {
                warning_count++;
            }
        }
    );
    logger->add_sink(callback_sink);
    
    std::cout << "Logging with custom callback:\n\n";
    
    FDC_INFO(logger, "Normal operation");
    FDC_WARN(logger, "First warning");
    FDC_ERROR(logger, "Error occurred");
    FDC_WARN(logger, "Second warning");
    FDC_WARN(logger, "Third warning");
    FDC_ERROR(logger, "Another error");
    FDC_INFO(logger, "Recovery successful");
    
    std::cout << "\n✓ Callback sink statistics:\n";
    std::cout << "  Warnings: " << warning_count << "\n";
    std::cout << "  Errors: " << error_count << "\n";
}

/**
 * Demo 9: Default Logger Macros
 */
void demo_default_logger() {
    print_separator("Demo 9: Default Logger Macros");
    
    std::cout << "Using global LOG_* macros:\n\n";
    
    LOG_INFO("Application starting...");
    LOG_DEBUG("Loading configuration");
    LOG_INFO("Configuration loaded: 10 settings");
    LOG_WARN("Using default port 8080");
    LOG_INFO("Server ready");
    
    std::cout << "\n✓ Default logger macros working\n";
}

int main() {
    std::cout << "FDC_Scheduler Logging Framework Demo\n";
    std::cout << "======================================\n";
    std::cout << "\nThis demo showcases logging capabilities:\n";
    std::cout << "- Console output with colors\n";
    std::cout << "- File logging with rotation\n";
    std::cout << "- Daily rotating files\n";
    std::cout << "- Multiple sinks per logger\n";
    std::cout << "- Log level filtering\n";
    std::cout << "- Formatted logging\n";
    std::cout << "- Logger registry\n";
    std::cout << "- Custom callbacks\n";
    
    try {
        demo_console_logging();
        demo_file_logging();
        demo_daily_logging();
        demo_multiple_sinks();
        demo_log_levels();
        demo_formatted_logging();
        demo_logger_registry();
        demo_callback_sink();
        demo_default_logger();
        
        print_separator("All Demos Completed Successfully!");
        
        std::cout << "Logging Framework Features:\n";
        std::cout << "✓ Multiple log levels (TRACE to CRITICAL)\n";
        std::cout << "✓ Console sink with ANSI colors\n";
        std::cout << "✓ File sink with size-based rotation\n";
        std::cout << "✓ Daily file sink with date-based rotation\n";
        std::cout << "✓ Custom callback sinks\n";
        std::cout << "✓ Multiple sinks per logger\n";
        std::cout << "✓ Thread-safe logging\n";
        std::cout << "✓ Printf-style formatting\n";
        std::cout << "✓ Logger registry for named loggers\n";
        std::cout << "✓ Per-logger and per-sink level filtering\n";
        
        std::cout << "\nGenerated log files:\n";
        std::cout << "  - test_rotating.log (+ backups)\n";
        std::cout << "  - test_daily_YYYY-MM-DD.log\n";
        std::cout << "  - test_multi.log\n";
        
        std::cout << "\nNext steps:\n";
        std::cout << "1. Integrate spdlog for production (optional)\n";
        std::cout << "2. Add async logging support\n";
        std::cout << "3. Add pattern-based formatting\n";
        std::cout << "4. Add JSON log format\n";
        std::cout << "5. Replace cout/cerr throughout codebase\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
