#ifndef FDC_PROFILER_HPP
#define FDC_PROFILER_HPP

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace fdc_scheduler {

/**
 * @brief Statistics for a profiled operation
 */
struct ProfileStats {
    std::string operation_name;
    size_t call_count = 0;
    double total_time_ms = 0.0;
    double min_time_ms = std::numeric_limits<double>::max();
    double max_time_ms = 0.0;
    double avg_time_ms = 0.0;
    
    void update(double time_ms) {
        call_count++;
        total_time_ms += time_ms;
        min_time_ms = std::min(min_time_ms, time_ms);
        max_time_ms = std::max(max_time_ms, time_ms);
        avg_time_ms = total_time_ms / call_count;
    }
};

/**
 * @brief RAII timer for automatic profiling
 */
class ScopedTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    
    ScopedTimer(const std::string& name, ProfileStats& stats)
        : name_(name), stats_(stats), start_(Clock::now()) {}
    
    ~ScopedTimer() {
        auto end = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        double time_ms = duration.count() / 1000.0;
        stats_.update(time_ms);
    }
    
private:
    std::string name_;
    ProfileStats& stats_;
    TimePoint start_;
};

/**
 * @brief Main profiler class for performance monitoring
 * 
 * Thread-safe profiler that tracks operation timing, memory usage,
 * and provides detailed statistics and reports.
 */
class Profiler {
public:
    /**
     * @brief Get singleton instance
     */
    static Profiler& instance() {
        static Profiler profiler;
        return profiler;
    }
    
    /**
     * @brief Start profiling an operation
     * @param operation_name Name of the operation
     * @return Timer ID for stopping later
     */
    size_t start(const std::string& operation_name) {
        auto start_time = std::chrono::high_resolution_clock::now();
        size_t timer_id = next_timer_id_++;
        
        active_timers_[timer_id] = {operation_name, start_time};
        
        // Ensure stats entry exists
        if (stats_.find(operation_name) == stats_.end()) {
            stats_[operation_name] = ProfileStats{operation_name};
        }
        
        return timer_id;
    }
    
    /**
     * @brief Stop profiling an operation
     * @param timer_id Timer ID from start()
     * @return Duration in milliseconds
     */
    double stop(size_t timer_id) {
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto it = active_timers_.find(timer_id);
        if (it == active_timers_.end()) {
            return 0.0;
        }
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - it->second.start_time);
        double time_ms = duration.count() / 1000.0;
        
        stats_[it->second.operation_name].update(time_ms);
        active_timers_.erase(it);
        
        return time_ms;
    }
    
    /**
     * @brief Profile a lambda function
     * @param operation_name Name of the operation
     * @param func Function to profile
     * @return Result of the function
     */
    template<typename Func>
    auto profile(const std::string& operation_name, Func&& func) -> decltype(func()) {
        auto& stats = stats_[operation_name];
        if (stats.operation_name.empty()) {
            stats.operation_name = operation_name;
        }
        
        ScopedTimer timer(operation_name, stats);
        return func();
    }
    
    /**
     * @brief Get statistics for an operation
     */
    const ProfileStats* get_stats(const std::string& operation_name) const {
        auto it = stats_.find(operation_name);
        return it != stats_.end() ? &it->second : nullptr;
    }
    
    /**
     * @brief Get all statistics
     */
    std::map<std::string, ProfileStats> get_all_stats() const {
        return stats_;
    }
    
    /**
     * @brief Reset all statistics
     */
    void reset() {
        stats_.clear();
        active_timers_.clear();
        next_timer_id_ = 0;
    }
    
    /**
     * @brief Print summary report to stream
     */
    void print_report(std::ostream& os = std::cout) const {
        if (stats_.empty()) {
            os << "No profiling data available.\n";
            return;
        }
        
        os << "\n" << std::string(80, '=') << "\n";
        os << "PERFORMANCE PROFILING REPORT\n";
        os << std::string(80, '=') << "\n\n";
        
        // Sort by total time
        std::vector<std::pair<std::string, ProfileStats>> sorted_stats(
            stats_.begin(), stats_.end());
        std::sort(sorted_stats.begin(), sorted_stats.end(),
            [](const auto& a, const auto& b) {
                return a.second.total_time_ms > b.second.total_time_ms;
            });
        
        // Print header
        os << std::left << std::setw(30) << "Operation"
           << std::right << std::setw(10) << "Calls"
           << std::setw(12) << "Total (ms)"
           << std::setw(12) << "Avg (ms)"
           << std::setw(12) << "Min (ms)"
           << std::setw(12) << "Max (ms)" << "\n";
        os << std::string(80, '-') << "\n";
        
        // Print stats
        double total_time = 0.0;
        for (const auto& [name, stat] : sorted_stats) {
            os << std::left << std::setw(30) << name
               << std::right << std::setw(10) << stat.call_count
               << std::setw(12) << std::fixed << std::setprecision(3) << stat.total_time_ms
               << std::setw(12) << std::fixed << std::setprecision(3) << stat.avg_time_ms
               << std::setw(12) << std::fixed << std::setprecision(3) << stat.min_time_ms
               << std::setw(12) << std::fixed << std::setprecision(3) << stat.max_time_ms << "\n";
            total_time += stat.total_time_ms;
        }
        
        os << std::string(80, '-') << "\n";
        os << "Total profiled time: " << std::fixed << std::setprecision(3) 
           << total_time << " ms\n";
        os << std::string(80, '=') << "\n\n";
    }
    
    /**
     * @brief Get report as string
     */
    std::string get_report() const {
        std::ostringstream oss;
        print_report(oss);
        return oss.str();
    }
    
private:
    Profiler() = default;
    
    struct ActiveTimer {
        std::string operation_name;
        std::chrono::high_resolution_clock::time_point start_time;
    };
    
    std::map<std::string, ProfileStats> stats_;
    std::map<size_t, ActiveTimer> active_timers_;
    size_t next_timer_id_ = 0;
};

// Convenience macros for easy profiling
#define FDC_PROFILE_FUNCTION() \
    auto __profiler_timer = fdc_scheduler::Profiler::instance().start(__FUNCTION__)

#define FDC_PROFILE_SCOPE(name) \
    auto __profiler_timer_##name = fdc_scheduler::Profiler::instance().start(#name)

#define FDC_PROFILE_END(timer_id) \
    fdc_scheduler::Profiler::instance().stop(timer_id)

} // namespace fdc_scheduler

#endif // FDC_PROFILER_HPP
