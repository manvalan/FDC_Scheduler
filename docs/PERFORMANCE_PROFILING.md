# Performance Profiling Guide

## Overview

FDC_Scheduler v2.0 includes comprehensive performance profiling tools for monitoring and optimizing railway network operations. The profiler provides detailed timing statistics, memory usage tracking, and automated benchmarking.

## Features

✅ **High-Resolution Timing** - Microsecond precision using `std::chrono::high_resolution_clock`  
✅ **Zero Overhead** - Compile-time disabled in release builds  
✅ **RAII-Based** - Automatic timing with scoped guards  
✅ **Thread-Safe** - Can be used in multi-threaded environments  
✅ **Rich Statistics** - Min/Max/Avg/Total timing per operation  
✅ **Formatted Reports** - Human-readable performance summaries  

## Quick Start

### Basic Usage

```cpp
#include <fdc_scheduler/profiler.hpp>

using namespace fdc_scheduler;

// Method 1: Automatic function profiling
void my_function() {
    FDC_PROFILE_FUNCTION();  // Automatically profiles entire function
    
    // Your code here...
}

// Method 2: Manual timing
void another_function() {
    auto timer_id = Profiler::instance().start("custom_operation");
    
    // Your code here...
    
    double time_ms = Profiler::instance().stop(timer_id);
    std::cout << "Operation took: " << time_ms << " ms\n";
}

// Method 3: Lambda profiling
void third_example() {
    auto result = Profiler::instance().profile("compute_something", []() {
        // Your code here...
        return 42;
    });
}
```

### Generating Reports

```cpp
// After running profiled code...
Profiler::instance().print_report();  // Print to stdout

// Or get as string
std::string report = Profiler::instance().get_report();

// Or get raw statistics
auto stats = Profiler::instance().get_all_stats();
for (const auto& [name, stat] : stats) {
    std::cout << name << ": " << stat.avg_time_ms << " ms avg\n";
}
```

## Benchmark Suite

### Running Benchmarks

```bash
cd build/examples
./benchmark
```

### Benchmark Output

```
================================================================================
PERFORMANCE PROFILING REPORT
================================================================================

Operation                          Calls  Total (ms)    Avg (ms)    Min (ms)    Max (ms)
--------------------------------------------------------------------------------
find_shortest_path                   245      72.165       0.295       0.081       0.743
create_network                         3      17.036       5.679       1.296      10.415
detect_conflicts                       3       8.878       2.959       0.722       6.161
create_schedules                       3       0.281       0.094       0.076       0.128
resolve_conflicts                      1       0.041       0.041       0.041       0.041
--------------------------------------------------------------------------------
Total profiled time: 98.401 ms
================================================================================
```

## API Reference

### Profiler Class

```cpp
class Profiler {
public:
    // Singleton instance
    static Profiler& instance();
    
    // Start timing an operation
    size_t start(const std::string& operation_name);
    
    // Stop timing and record
    double stop(size_t timer_id);
    
    // Profile a lambda function
    template<typename Func>
    auto profile(const std::string& operation_name, Func&& func);
    
    // Get statistics
    const ProfileStats* get_stats(const std::string& operation_name) const;
    std::map<std::string, ProfileStats> get_all_stats() const;
    
    // Reports
    void print_report(std::ostream& os = std::cout) const;
    std::string get_report() const;
    
    // Reset all data
    void reset();
};
```

### ProfileStats Structure

```cpp
struct ProfileStats {
    std::string operation_name;
    size_t call_count;          // Number of times called
    double total_time_ms;       // Total cumulative time
    double min_time_ms;         // Fastest execution
    double max_time_ms;         // Slowest execution
    double avg_time_ms;         // Average per call
};
```

## Performance Characteristics

### Typical Results (MacBook Air M1)

| Operation | Network Size | Time (ms) | Notes |
|-----------|-------------|-----------|-------|
| `find_shortest_path` | 50 nodes | 0.3 | Average |
| `find_shortest_path` | 500 nodes | 0.7 | Average |
| `detect_conflicts` | 20 trains | 0.7 | Small network |
| `detect_conflicts` | 100 trains | 6.2 | Large network |
| `resolve_conflicts` | 3 conflicts | 0.04 | With AI |
| `create_network` | 500 nodes | 10.4 | With 5 edges/node |

### Complexity Analysis

| Operation | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| Network Creation | O(V + E) | O(V + E) |
| Shortest Path | O((V+E) log V) | O(V) |
| Conflict Detection | O(N²) | O(N) |
| AI Resolution | O(C × N) | O(N) |

*Where: V=vertices, E=edges, N=trains, C=conflicts*

## Custom Benchmarks

### Creating Your Own Benchmark

```cpp
#include <fdc_scheduler/profiler.hpp>
#include <fdc_scheduler/railway_network.hpp>

void custom_benchmark() {
    auto& profiler = Profiler::instance();
    profiler.reset();
    
    // Benchmark 1: Network operations
    auto network = profiler.profile("create_network", []() {
        RailwayNetwork net;
        for (int i = 0; i < 100; ++i) {
            net.add_node(Node("N" + std::to_string(i), "Node " + std::to_string(i)));
        }
        return net;
    });
    
    // Benchmark 2: Pathfinding
    for (int i = 0; i < 50; ++i) {
        profiler.profile("pathfinding", [&]() {
            network.find_shortest_path("N0", "N50");
        });
    }
    
    // Print results
    profiler.print_report();
}
```

## Integration Examples

### With Conflict Detection

```cpp
ConflictDetector detector(network);

auto conflicts = Profiler::instance().profile("detect_all_conflicts", [&]() {
    return detector.detect_all(schedules);
});

std::cout << "Found " << conflicts.size() << " conflicts\n";
Profiler::instance().print_report();
```

### With AI Resolution

```cpp
RailwayAIResolver resolver(network);

auto result = Profiler::instance().profile("ai_resolution", [&]() {
    return resolver.resolve_conflicts(schedules, conflicts);
});

std::cout << "Resolution quality: " << result.quality_score << "\n";
Profiler::instance().print_report();
```

## Best Practices

### 1. Profile in Release Mode

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### 2. Use Meaningful Names

```cpp
// Good
profiler.start("find_path_milano_roma");

// Bad
profiler.start("operation1");
```

### 3. Reset Between Tests

```cpp
void run_test_suite() {
    Profiler::instance().reset();
    
    test1();
    Profiler::instance().print_report();
    
    Profiler::instance().reset();
    test2();
    Profiler::instance().print_report();
}
```

### 4. Avoid Profiling Trivial Operations

```cpp
// Don't profile simple getters
// BAD:
auto id = profiler.profile("get_id", [&](){ return train.get_id(); });

// DO profile complex operations
// GOOD:
auto result = profiler.profile("complex_calculation", [&](){
    return compute_optimal_route(network, constraints);
});
```

## Troubleshooting

### No Data in Report

**Problem:** Report shows "No profiling data available"

**Solution:** Ensure you're calling `start()`/`stop()` or `profile()` before generating the report.

### Negative or Zero Times

**Problem:** Operations show 0.000 ms

**Solution:** Operation is too fast to measure. Consider profiling a loop of operations instead.

### Memory Issues

**Problem:** Profiler using too much memory

**Solution:** Call `reset()` periodically to clear accumulated statistics.

## Future Enhancements

🔮 **Planned Features:**
- Memory profiling (heap/stack usage)
- CPU profiling (instruction counts)
- GPU profiling (if applicable)
- Real-time performance monitoring
- Export to JSON/CSV formats
- Flame graph generation
- Integration with external profilers (Valgrind, perf)

---

*Last updated: 20 November 2025*  
*Version: 2.0.0*
