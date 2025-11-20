/**
 * @file telemetry.cpp
 * @brief Telemetry and monitoring implementation
 */

#include <fdc_scheduler/telemetry.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <cmath>

// Platform-specific includes
#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#include <unistd.h>
#elif __linux__
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>
#endif

namespace fdc_scheduler {

// =============================================================================
// Helper Functions
// =============================================================================

namespace {

/**
 * @brief Escape string for Prometheus format
 */
std::string escape_prometheus(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if (c == '\\' || c == '"' || c == '\n') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

/**
 * @brief Format labels for Prometheus
 */
std::string format_labels(const Labels& labels) {
    if (labels.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << escape_prometheus(value) << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

/**
 * @brief Format timestamp for Prometheus
 */
std::string format_timestamp(const std::chrono::system_clock::time_point& tp) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count();
    return std::to_string(ms);
}

} // anonymous namespace

// =============================================================================
// Counter Implementation
// =============================================================================

Counter::Counter(const std::string& name, const std::string& help)
    : name_(name), help_(help) {}

void Counter::increment() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ += 1.0;
}

void Counter::increment(double value) {
    if (value < 0) {
        throw std::invalid_argument("Counter can only increase");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    value_ += value;
}

void Counter::increment(const Labels& labels, double value) {
    if (value < 0) {
        throw std::invalid_argument("Counter can only increase");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    labeled_values_[labels] += value;
}

double Counter::value() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

double Counter::value(const Labels& labels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = labeled_values_.find(labels);
    return it != labeled_values_.end() ? it->second : 0.0;
}

std::string Counter::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Help text
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " counter\n";
    
    // Default value
    if (labeled_values_.empty()) {
        oss << name_ << " " << std::fixed << std::setprecision(2) << value_ << "\n";
    }
    
    // Labeled values
    for (const auto& [labels, value] : labeled_values_) {
        oss << name_ << format_labels(labels) << " "
            << std::fixed << std::setprecision(2) << value << "\n";
    }
    
    return oss.str();
}

// =============================================================================
// Gauge Implementation
// =============================================================================

Gauge::Gauge(const std::string& name, const std::string& help)
    : name_(name), help_(help) {}

void Gauge::set(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = value;
}

void Gauge::set(const Labels& labels, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    labeled_values_[labels] = value;
}

void Gauge::increment(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ += value;
}

void Gauge::decrement(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ -= value;
}

double Gauge::value() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

double Gauge::value(const Labels& labels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = labeled_values_.find(labels);
    return it != labeled_values_.end() ? it->second : 0.0;
}

std::string Gauge::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Help text
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " gauge\n";
    
    // Default value
    if (labeled_values_.empty()) {
        oss << name_ << " " << std::fixed << std::setprecision(2) << value_ << "\n";
    }
    
    // Labeled values
    for (const auto& [labels, value] : labeled_values_) {
        oss << name_ << format_labels(labels) << " "
            << std::fixed << std::setprecision(2) << value << "\n";
    }
    
    return oss.str();
}

// =============================================================================
// Histogram Implementation
// =============================================================================

Histogram::Histogram(const std::string& name, const std::string& help)
    : Histogram(name, help, {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0}) {}

Histogram::Histogram(const std::string& name, const std::string& help,
                     const std::vector<double>& buckets)
    : name_(name), help_(help), buckets_(buckets) {
    // Ensure buckets are sorted and include +Inf
    std::sort(buckets_.begin(), buckets_.end());
    if (buckets_.empty() || buckets_.back() != INFINITY) {
        buckets_.push_back(INFINITY);
    }
    
    // Initialize bucket counts
    for (double bucket : buckets_) {
        data_.bucket_counts[bucket] = 0;
    }
}

void Histogram::observe(double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update buckets
    for (double bucket : buckets_) {
        if (value <= bucket) {
            data_.bucket_counts[bucket]++;
        }
    }
    
    // Update count and sum
    data_.count++;
    data_.sum += value;
}

void Histogram::observe(const Labels& labels, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& data = labeled_data_[labels];
    
    // Initialize buckets if needed
    if (data.bucket_counts.empty()) {
        for (double bucket : buckets_) {
            data.bucket_counts[bucket] = 0;
        }
    }
    
    // Update buckets
    for (double bucket : buckets_) {
        if (value <= bucket) {
            data.bucket_counts[bucket]++;
        }
    }
    
    // Update count and sum
    data.count++;
    data.sum += value;
}

std::map<double, uint64_t> Histogram::buckets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.bucket_counts;
}

uint64_t Histogram::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.count;
}

double Histogram::sum() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.sum;
}

std::string Histogram::serialize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    // Help text
    oss << "# HELP " << name_ << " " << help_ << "\n";
    oss << "# TYPE " << name_ << " histogram\n";
    
    // Default histogram
    if (labeled_data_.empty()) {
        for (const auto& [bucket, count] : data_.bucket_counts) {
            oss << name_ << "_bucket{le=\"";
            if (std::isinf(bucket)) {
                oss << "+Inf";
            } else {
                oss << std::fixed << std::setprecision(3) << bucket;
            }
            oss << "\"} " << count << "\n";
        }
        oss << name_ << "_sum " << std::fixed << std::setprecision(6) << data_.sum << "\n";
        oss << name_ << "_count " << data_.count << "\n";
    }
    
    // Labeled histograms
    for (const auto& [labels, data] : labeled_data_) {
        std::string label_str = format_labels(labels);
        // Insert le label into existing labels
        std::string base_labels = label_str.empty() ? "{" : label_str.substr(0, label_str.size() - 1) + ",";
        
        for (const auto& [bucket, count] : data.bucket_counts) {
            oss << name_ << "_bucket" << base_labels << "le=\"";
            if (std::isinf(bucket)) {
                oss << "+Inf";
            } else {
                oss << std::fixed << std::setprecision(3) << bucket;
            }
            oss << "\"} " << count << "\n";
        }
        oss << name_ << "_sum" << label_str << " "
            << std::fixed << std::setprecision(6) << data.sum << "\n";
        oss << name_ << "_count" << label_str << " " << data.count << "\n";
    }
    
    return oss.str();
}

// =============================================================================
// HealthMonitor Implementation
// =============================================================================

void HealthMonitor::register_check(const std::string& name, HealthCheckFunction check) {
    std::lock_guard<std::mutex> lock(mutex_);
    checks_[name] = check;
}

void HealthMonitor::unregister_check(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    checks_.erase(name);
    last_results_.erase(name);
}

std::vector<HealthCheck> HealthMonitor::check_all() {
    std::vector<HealthCheck> results;
    
    std::map<std::string, HealthCheckFunction> checks_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        checks_copy = checks_;
    }
    
    for (const auto& [name, check] : checks_copy) {
        try {
            auto result = check();
            results.push_back(result);
            
            std::lock_guard<std::mutex> lock(mutex_);
            last_results_[name] = result;
        } catch (const std::exception& e) {
            HealthCheck error_result;
            error_result.name = name;
            error_result.status = HealthStatus::UNHEALTHY;
            error_result.message = std::string("Health check failed: ") + e.what();
            error_result.timestamp = std::chrono::system_clock::now();
            results.push_back(error_result);
            
            std::lock_guard<std::mutex> lock(mutex_);
            last_results_[name] = error_result;
        }
    }
    
    return results;
}

HealthCheck HealthMonitor::check(const std::string& name) {
    HealthCheckFunction check_func;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = checks_.find(name);
        if (it == checks_.end()) {
            HealthCheck result;
            result.name = name;
            result.status = HealthStatus::UNHEALTHY;
            result.message = "Health check not found";
            result.timestamp = std::chrono::system_clock::now();
            return result;
        }
        check_func = it->second;
    }
    
    try {
        auto result = check_func();
        std::lock_guard<std::mutex> lock(mutex_);
        last_results_[name] = result;
        return result;
    } catch (const std::exception& e) {
        HealthCheck error_result;
        error_result.name = name;
        error_result.status = HealthStatus::UNHEALTHY;
        error_result.message = std::string("Health check failed: ") + e.what();
        error_result.timestamp = std::chrono::system_clock::now();
        
        std::lock_guard<std::mutex> lock(mutex_);
        last_results_[name] = error_result;
        return error_result;
    }
}

HealthStatus HealthMonitor::overall_status() {
    auto results = check_all();
    
    if (results.empty()) {
        return HealthStatus::HEALTHY;
    }
    
    bool has_unhealthy = false;
    bool has_degraded = false;
    
    for (const auto& result : results) {
        if (result.status == HealthStatus::UNHEALTHY) {
            has_unhealthy = true;
        } else if (result.status == HealthStatus::DEGRADED) {
            has_degraded = true;
        }
    }
    
    if (has_unhealthy) {
        return HealthStatus::UNHEALTHY;
    } else if (has_degraded) {
        return HealthStatus::DEGRADED;
    }
    return HealthStatus::HEALTHY;
}

std::string HealthMonitor::to_json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"checks\": [\n";
    
    bool first = true;
    for (const auto& [name, result] : last_results_) {
        if (!first) oss << ",\n";
        
        oss << "    {\n";
        oss << "      \"name\": \"" << escape_prometheus(result.name) << "\",\n";
        oss << "      \"status\": \"";
        switch (result.status) {
            case HealthStatus::HEALTHY: oss << "healthy"; break;
            case HealthStatus::DEGRADED: oss << "degraded"; break;
            case HealthStatus::UNHEALTHY: oss << "unhealthy"; break;
        }
        oss << "\",\n";
        oss << "      \"message\": \"" << escape_prometheus(result.message) << "\",\n";
        oss << "      \"timestamp\": \"" << format_timestamp(result.timestamp) << "\"";
        
        if (!result.details.empty()) {
            oss << ",\n      \"details\": {\n";
            bool first_detail = true;
            for (const auto& [key, value] : result.details) {
                if (!first_detail) oss << ",\n";
                oss << "        \"" << escape_prometheus(key) << "\": \""
                    << escape_prometheus(value) << "\"";
                first_detail = false;
            }
            oss << "\n      }";
        }
        
        oss << "\n    }";
        first = false;
    }
    
    oss << "\n  ]\n";
    oss << "}\n";
    
    return oss.str();
}

// =============================================================================
// ResourceMonitor Implementation
// =============================================================================

ResourceInfo ResourceMonitor::get_usage() {
    ResourceInfo info;
    info.timestamp = std::chrono::system_clock::now();
    
    info.cpu_percent = get_cpu_usage();
    info.memory_used_bytes = get_memory_usage();
    info.memory_total_bytes = get_total_memory();
    info.memory_percent = info.memory_total_bytes > 0 
        ? (static_cast<double>(info.memory_used_bytes) / info.memory_total_bytes) * 100.0 
        : 0.0;
    
    info.disk_used_bytes = get_disk_usage();
    info.disk_total_bytes = get_total_disk();
    info.disk_percent = info.disk_total_bytes > 0
        ? (static_cast<double>(info.disk_used_bytes) / info.disk_total_bytes) * 100.0
        : 0.0;
    
    info.open_files = get_open_files();
    info.thread_count = get_thread_count();
    
    std::lock_guard<std::mutex> lock(mutex_);
    last_snapshot_ = info;
    
    return info;
}

void ResourceMonitor::start_monitoring(int interval_ms) {
    if (monitoring_.load()) {
        return;
    }
    
    monitoring_.store(true);
    
    std::thread([this, interval_ms]() {
        while (monitoring_.load()) {
            get_usage();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    }).detach();
}

void ResourceMonitor::stop_monitoring() {
    monitoring_.store(false);
}

ResourceInfo ResourceMonitor::last_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_snapshot_;
}

// Platform-specific implementations

double ResourceMonitor::get_cpu_usage() {
#ifdef __APPLE__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        double sys_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
        return (user_time + sys_time) * 100.0; // Simplified, should track over time
    }
#elif __linux__
    std::ifstream stat_file("/proc/self/stat");
    if (stat_file.is_open()) {
        std::string line;
        std::getline(stat_file, line);
        // Parse CPU time from /proc/self/stat
        // This is simplified - proper implementation would track delta
        return 0.0;
    }
#endif
    return 0.0;
}

uint64_t ResourceMonitor::get_memory_usage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
#elif __linux__
    std::ifstream status_file("/proc/self/status");
    if (status_file.is_open()) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                uint64_t rss_kb;
                std::istringstream iss(line.substr(6));
                iss >> rss_kb;
                return rss_kb * 1024; // Convert KB to bytes
            }
        }
    }
#endif
    return 0;
}

uint64_t ResourceMonitor::get_total_memory() {
#ifdef __APPLE__
    int64_t mem_size;
    size_t len = sizeof(mem_size);
    if (sysctlbyname("hw.memsize", &mem_size, &len, nullptr, 0) == 0) {
        return static_cast<uint64_t>(mem_size);
    }
#elif __linux__
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.substr(0, 9) == "MemTotal:") {
                uint64_t total_kb;
                std::istringstream iss(line.substr(9));
                iss >> total_kb;
                return total_kb * 1024;
            }
        }
    }
#endif
    return 0;
}

uint64_t ResourceMonitor::get_disk_usage() {
    // Placeholder - would use statfs/statvfs
    return 0;
}

uint64_t ResourceMonitor::get_total_disk() {
    // Placeholder - would use statfs/statvfs
    return 0;
}

uint32_t ResourceMonitor::get_open_files() {
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        return static_cast<uint32_t>(limit.rlim_cur);
    }
    return 0;
}

uint32_t ResourceMonitor::get_thread_count() {
#ifdef __APPLE__
    thread_array_t thread_list;
    mach_msg_type_number_t thread_count;
    if (task_threads(mach_task_self(), &thread_list, &thread_count) == KERN_SUCCESS) {
        vm_deallocate(mach_task_self(), (vm_address_t)thread_list,
                     thread_count * sizeof(thread_act_t));
        return thread_count;
    }
#elif __linux__
    std::ifstream status_file("/proc/self/status");
    if (status_file.is_open()) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 8) == "Threads:") {
                uint32_t threads;
                std::istringstream iss(line.substr(8));
                iss >> threads;
                return threads;
            }
        }
    }
#endif
    return 0;
}

// =============================================================================
// TelemetryManager Implementation
// =============================================================================

TelemetryManager& TelemetryManager::instance() {
    static TelemetryManager instance;
    return instance;
}

std::shared_ptr<Counter> TelemetryManager::register_counter(
    const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto counter = std::dynamic_pointer_cast<Counter>(it->second);
        if (counter) {
            return counter;
        }
        throw std::runtime_error("Metric already registered with different type");
    }
    
    auto counter = std::make_shared<Counter>(name, help);
    metrics_[name] = counter;
    return counter;
}

std::shared_ptr<Gauge> TelemetryManager::register_gauge(
    const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto gauge = std::dynamic_pointer_cast<Gauge>(it->second);
        if (gauge) {
            return gauge;
        }
        throw std::runtime_error("Metric already registered with different type");
    }
    
    auto gauge = std::make_shared<Gauge>(name, help);
    metrics_[name] = gauge;
    return gauge;
}

std::shared_ptr<Histogram> TelemetryManager::register_histogram(
    const std::string& name, const std::string& help,
    const std::vector<double>& buckets) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_.find(name);
    if (it != metrics_.end()) {
        auto histogram = std::dynamic_pointer_cast<Histogram>(it->second);
        if (histogram) {
            return histogram;
        }
        throw std::runtime_error("Metric already registered with different type");
    }
    
    auto histogram = buckets.empty()
        ? std::make_shared<Histogram>(name, help)
        : std::make_shared<Histogram>(name, help, buckets);
    metrics_[name] = histogram;
    return histogram;
}

std::shared_ptr<Metric> TelemetryManager::get_metric(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    return it != metrics_.end() ? it->second : nullptr;
}

void TelemetryManager::unregister_metric(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.erase(name);
}

std::string TelemetryManager::export_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    for (const auto& [name, metric] : metrics_) {
        oss << metric->serialize();
    }
    
    return oss.str();
}

void TelemetryManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
}

// =============================================================================
// Timer Implementation
// =============================================================================

Timer::Timer(std::shared_ptr<Histogram> histogram)
    : histogram_(histogram), start_(std::chrono::steady_clock::now()) {}

Timer::Timer(std::shared_ptr<Histogram> histogram, const Labels& labels)
    : histogram_(histogram), labels_(labels),
      start_(std::chrono::steady_clock::now()) {}

Timer::~Timer() {
    double duration = elapsed();
    if (labels_.empty()) {
        histogram_->observe(duration);
    } else {
        histogram_->observe(labels_, duration);
    }
}

double Timer::elapsed() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_);
    return duration.count() / 1000000.0; // Convert to seconds
}

} // namespace fdc_scheduler
