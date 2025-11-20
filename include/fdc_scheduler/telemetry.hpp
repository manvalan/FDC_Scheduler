/**
 * @file telemetry.hpp
 * @brief Telemetry and monitoring system with Prometheus metrics
 * 
 * Features:
 * - Prometheus metrics (counter, gauge, histogram, summary)
 * - Health check system with detailed status
 * - Resource monitoring (CPU, memory, disk)
 * - Custom metrics registration
 * - Metrics exposition in Prometheus format
 * - Performance tracking with histograms
 * - Automatic metric labeling
 * 
 * @author FDC Scheduler Team
 * @version 2.0.0
 */

#ifndef FDC_SCHEDULER_TELEMETRY_HPP
#define FDC_SCHEDULER_TELEMETRY_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>

namespace fdc_scheduler {

// =============================================================================
// Metric Types
// =============================================================================

/**
 * @brief Labels for metrics (key-value pairs)
 */
using Labels = std::map<std::string, std::string>;

/**
 * @brief Metric types supported by Prometheus
 */
enum class MetricType {
    COUNTER,    ///< Monotonically increasing counter
    GAUGE,      ///< Value that can go up and down
    HISTOGRAM,  ///< Distribution of values with buckets
    SUMMARY     ///< Distribution with quantiles
};

/**
 * @brief Health status levels
 */
enum class HealthStatus {
    HEALTHY,    ///< System is operating normally
    DEGRADED,   ///< System is operational but with issues
    UNHEALTHY   ///< System has critical issues
};

// =============================================================================
// Metric Base Classes
// =============================================================================

/**
 * @brief Base class for all metrics
 */
class Metric {
public:
    virtual ~Metric() = default;
    
    /**
     * @brief Get metric name
     */
    virtual std::string name() const = 0;
    
    /**
     * @brief Get metric type
     */
    virtual MetricType type() const = 0;
    
    /**
     * @brief Get metric help text
     */
    virtual std::string help() const = 0;
    
    /**
     * @brief Serialize metric to Prometheus format
     */
    virtual std::string serialize() const = 0;
};

/**
 * @brief Counter metric (monotonically increasing)
 * 
 * Example: Total requests processed, errors encountered
 */
class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& help);
    
    /**
     * @brief Increment counter by 1
     */
    void increment();
    
    /**
     * @brief Increment counter by value
     */
    void increment(double value);
    
    /**
     * @brief Increment counter with labels
     */
    void increment(const Labels& labels, double value = 1.0);
    
    /**
     * @brief Get current value
     */
    double value() const;
    
    /**
     * @brief Get value for specific labels
     */
    double value(const Labels& labels) const;
    
    // Metric interface
    std::string name() const override { return name_; }
    MetricType type() const override { return MetricType::COUNTER; }
    std::string help() const override { return help_; }
    std::string serialize() const override;
    
private:
    std::string name_;
    std::string help_;
    mutable std::mutex mutex_;
    double value_ = 0.0;
    std::map<Labels, double> labeled_values_;
};

/**
 * @brief Gauge metric (can go up and down)
 * 
 * Example: Current memory usage, active connections
 */
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& help);
    
    /**
     * @brief Set gauge value
     */
    void set(double value);
    
    /**
     * @brief Set gauge value with labels
     */
    void set(const Labels& labels, double value);
    
    /**
     * @brief Increment gauge
     */
    void increment(double value = 1.0);
    
    /**
     * @brief Decrement gauge
     */
    void decrement(double value = 1.0);
    
    /**
     * @brief Get current value
     */
    double value() const;
    
    /**
     * @brief Get value for specific labels
     */
    double value(const Labels& labels) const;
    
    // Metric interface
    std::string name() const override { return name_; }
    MetricType type() const override { return MetricType::GAUGE; }
    std::string help() const override { return help_; }
    std::string serialize() const override;
    
private:
    std::string name_;
    std::string help_;
    mutable std::mutex mutex_;
    double value_ = 0.0;
    std::map<Labels, double> labeled_values_;
};

/**
 * @brief Histogram metric (distribution with buckets)
 * 
 * Example: Request duration, response size
 */
class Histogram : public Metric {
public:
    /**
     * @brief Create histogram with default buckets
     * Default: 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10
     */
    Histogram(const std::string& name, const std::string& help);
    
    /**
     * @brief Create histogram with custom buckets
     */
    Histogram(const std::string& name, const std::string& help,
              const std::vector<double>& buckets);
    
    /**
     * @brief Observe a value
     */
    void observe(double value);
    
    /**
     * @brief Observe a value with labels
     */
    void observe(const Labels& labels, double value);
    
    /**
     * @brief Get bucket counts
     */
    std::map<double, uint64_t> buckets() const;
    
    /**
     * @brief Get total count
     */
    uint64_t count() const;
    
    /**
     * @brief Get sum of all observations
     */
    double sum() const;
    
    // Metric interface
    std::string name() const override { return name_; }
    MetricType type() const override { return MetricType::HISTOGRAM; }
    std::string help() const override { return help_; }
    std::string serialize() const override;
    
private:
    std::string name_;
    std::string help_;
    std::vector<double> buckets_;
    mutable std::mutex mutex_;
    
    struct HistogramData {
        std::map<double, uint64_t> bucket_counts;
        uint64_t count = 0;
        double sum = 0.0;
    };
    
    HistogramData data_;
    std::map<Labels, HistogramData> labeled_data_;
};

// =============================================================================
// Health Check System
// =============================================================================

/**
 * @brief Health check result
 */
struct HealthCheck {
    std::string name;                           ///< Component name
    HealthStatus status;                        ///< Health status
    std::string message;                        ///< Status message
    std::map<std::string, std::string> details; ///< Additional details
    std::chrono::system_clock::time_point timestamp; ///< Check time
};

/**
 * @brief Health check function type
 */
using HealthCheckFunction = std::function<HealthCheck()>;

/**
 * @brief System health monitor
 */
class HealthMonitor {
public:
    /**
     * @brief Register a health check
     */
    void register_check(const std::string& name, HealthCheckFunction check);
    
    /**
     * @brief Remove a health check
     */
    void unregister_check(const std::string& name);
    
    /**
     * @brief Run all health checks
     */
    std::vector<HealthCheck> check_all();
    
    /**
     * @brief Run specific health check
     */
    HealthCheck check(const std::string& name);
    
    /**
     * @brief Get overall system status
     */
    HealthStatus overall_status();
    
    /**
     * @brief Get health report as JSON
     */
    std::string to_json() const;
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, HealthCheckFunction> checks_;
    std::map<std::string, HealthCheck> last_results_;
};

// =============================================================================
// Resource Monitor
// =============================================================================

/**
 * @brief System resource information
 */
struct ResourceInfo {
    double cpu_percent = 0.0;          ///< CPU usage (0-100)
    uint64_t memory_used_bytes = 0;    ///< Memory used in bytes
    uint64_t memory_total_bytes = 0;   ///< Total memory in bytes
    double memory_percent = 0.0;       ///< Memory usage (0-100)
    uint64_t disk_used_bytes = 0;      ///< Disk used in bytes
    uint64_t disk_total_bytes = 0;     ///< Total disk in bytes
    double disk_percent = 0.0;         ///< Disk usage (0-100)
    uint32_t open_files = 0;           ///< Number of open file descriptors
    uint32_t thread_count = 0;         ///< Number of threads
    std::chrono::system_clock::time_point timestamp; ///< Sample time
};

/**
 * @brief System resource monitor
 */
class ResourceMonitor {
public:
    /**
     * @brief Get current resource usage
     */
    ResourceInfo get_usage();
    
    /**
     * @brief Start automatic monitoring
     * @param interval_ms Monitoring interval in milliseconds
     */
    void start_monitoring(int interval_ms = 5000);
    
    /**
     * @brief Stop automatic monitoring
     */
    void stop_monitoring();
    
    /**
     * @brief Check if monitoring is active
     */
    bool is_monitoring() const { return monitoring_.load(); }
    
    /**
     * @brief Get last resource snapshot
     */
    ResourceInfo last_snapshot() const;
    
private:
    std::atomic<bool> monitoring_{false};
    mutable std::mutex mutex_;
    ResourceInfo last_snapshot_;
    
    double get_cpu_usage();
    uint64_t get_memory_usage();
    uint64_t get_total_memory();
    uint64_t get_disk_usage();
    uint64_t get_total_disk();
    uint32_t get_open_files();
    uint32_t get_thread_count();
};

// =============================================================================
// Telemetry Manager
// =============================================================================

/**
 * @brief Central telemetry and monitoring system
 */
class TelemetryManager {
public:
    /**
     * @brief Get singleton instance
     */
    static TelemetryManager& instance();
    
    /**
     * @brief Register a counter metric
     */
    std::shared_ptr<Counter> register_counter(const std::string& name,
                                              const std::string& help);
    
    /**
     * @brief Register a gauge metric
     */
    std::shared_ptr<Gauge> register_gauge(const std::string& name,
                                          const std::string& help);
    
    /**
     * @brief Register a histogram metric
     */
    std::shared_ptr<Histogram> register_histogram(const std::string& name,
                                                   const std::string& help,
                                                   const std::vector<double>& buckets = {});
    
    /**
     * @brief Get registered metric by name
     */
    std::shared_ptr<Metric> get_metric(const std::string& name);
    
    /**
     * @brief Remove metric
     */
    void unregister_metric(const std::string& name);
    
    /**
     * @brief Export all metrics in Prometheus format
     */
    std::string export_metrics() const;
    
    /**
     * @brief Get health monitor
     */
    HealthMonitor& health() { return health_monitor_; }
    
    /**
     * @brief Get resource monitor
     */
    ResourceMonitor& resources() { return resource_monitor_; }
    
    /**
     * @brief Clear all metrics
     */
    void clear();
    
private:
    TelemetryManager() = default;
    ~TelemetryManager() = default;
    TelemetryManager(const TelemetryManager&) = delete;
    TelemetryManager& operator=(const TelemetryManager&) = delete;
    
    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Metric>> metrics_;
    HealthMonitor health_monitor_;
    ResourceMonitor resource_monitor_;
};

// =============================================================================
// RAII Performance Timer
// =============================================================================

/**
 * @brief RAII timer for automatic histogram observation
 * 
 * Usage:
 * @code
 * auto timer = Timer(histogram);
 * // ... code to measure ...
 * // Duration automatically recorded on destruction
 * @endcode
 */
class Timer {
public:
    /**
     * @brief Start timer for histogram
     */
    explicit Timer(std::shared_ptr<Histogram> histogram);
    
    /**
     * @brief Start timer for histogram with labels
     */
    Timer(std::shared_ptr<Histogram> histogram, const Labels& labels);
    
    /**
     * @brief Stop timer and record duration
     */
    ~Timer();
    
    /**
     * @brief Get elapsed time in seconds
     */
    double elapsed() const;
    
private:
    std::shared_ptr<Histogram> histogram_;
    Labels labels_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace fdc_scheduler

#endif // FDC_SCHEDULER_TELEMETRY_HPP
