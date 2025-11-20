/**
 * @file telemetry_demo.cpp
 * @brief Demonstration of telemetry and monitoring system
 * 
 * Shows:
 * - Counter, Gauge, and Histogram metrics
 * - Prometheus format export
 * - Health check system
 * - Resource monitoring
 * - RAII performance timers
 * - Labeled metrics
 */

#include <fdc_scheduler/telemetry.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <random>

using namespace fdc_scheduler;

// =============================================================================
// Demo 1: Counter Metrics
// =============================================================================

void demo_counter_metrics() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 1: Counter Metrics\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Counters track cumulative values (monotonically increasing).\n";
    std::cout << "Examples: Total requests, errors, bytes transferred\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    
    // Register counters
    auto requests = telemetry.register_counter(
        "http_requests_total",
        "Total number of HTTP requests");
    
    auto errors = telemetry.register_counter(
        "http_errors_total",
        "Total number of HTTP errors");
    
    std::cout << "Simulating HTTP requests...\n";
    
    // Simulate requests
    for (int i = 0; i < 10; i++) {
        requests->increment();
        
        // Labels for HTTP method and status
        if (i % 3 == 0) {
            requests->increment({{"method", "GET"}, {"status", "200"}});
        } else if (i % 3 == 1) {
            requests->increment({{"method", "POST"}, {"status", "201"}});
        } else {
            requests->increment({{"method", "GET"}, {"status", "404"}});
            errors->increment({{"method", "GET"}, {"type", "not_found"}});
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "✓ Recorded " << static_cast<int>(requests->value()) << " total requests\n";
    std::cout << "✓ Recorded " << static_cast<int>(errors->value()) << " errors\n\n";
    
    std::cout << "Prometheus format:\n";
    std::cout << requests->serialize() << "\n";
}

// =============================================================================
// Demo 2: Gauge Metrics
// =============================================================================

void demo_gauge_metrics() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 2: Gauge Metrics\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Gauges track values that can go up and down.\n";
    std::cout << "Examples: Temperature, memory usage, active connections\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    
    // Register gauges
    auto memory = telemetry.register_gauge(
        "process_memory_bytes",
        "Process memory usage in bytes");
    
    auto connections = telemetry.register_gauge(
        "active_connections",
        "Number of active connections");
    
    std::cout << "Simulating connection lifecycle...\n";
    
    // Simulate connections
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> mem_dist(1000000, 5000000);
    
    for (int i = 0; i < 5; i++) {
        // Connections fluctuate
        connections->set(i * 2);
        
        // Memory grows
        memory->set(mem_dist(gen));
        
        std::cout << "  Active connections: " << static_cast<int>(connections->value())
                  << ", Memory: " << static_cast<int>(memory->value() / 1024 / 1024) << " MB\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n✓ Current memory: " << static_cast<int>(memory->value() / 1024 / 1024) << " MB\n";
    std::cout << "✓ Active connections: " << static_cast<int>(connections->value()) << "\n\n";
    
    std::cout << "Prometheus format:\n";
    std::cout << memory->serialize() << "\n";
}

// =============================================================================
// Demo 3: Histogram Metrics with Timer
// =============================================================================

void demo_histogram_metrics() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 3: Histogram Metrics & Performance Timer\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Histograms track distribution of values in buckets.\n";
    std::cout << "Examples: Request duration, response size, latency\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    
    // Register histogram with custom buckets (milliseconds)
    auto latency = telemetry.register_histogram(
        "api_latency_seconds",
        "API request latency in seconds",
        {0.001, 0.005, 0.010, 0.050, 0.100, 0.500, 1.0});
    
    std::cout << "Measuring API request latencies...\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> latency_dist(1, 100);
    
    // Simulate API requests with timer
    for (int i = 0; i < 10; i++) {
        Labels labels = {{"endpoint", i % 2 == 0 ? "/api/schedule" : "/api/conflicts"}};
        
        {
            Timer timer(latency, labels);
            
            // Simulate work
            int ms = latency_dist(gen);
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            
            std::cout << "  Request " << (i + 1) << ": " << timer.elapsed() * 1000 << " ms\n";
        } // Timer automatically records on destruction
    }
    
    std::cout << "\n✓ Recorded " << latency->count() << " observations\n";
    std::cout << "✓ Sum: " << latency->sum() << " seconds\n";
    std::cout << "✓ Average: " << (latency->sum() / latency->count()) * 1000 << " ms\n\n";
    
    std::cout << "Prometheus format (buckets):\n";
    std::cout << latency->serialize() << "\n";
}

// =============================================================================
// Demo 4: Health Checks
// =============================================================================

void demo_health_checks() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 4: Health Check System\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Health checks monitor system components.\n";
    std::cout << "Status: HEALTHY, DEGRADED, or UNHEALTHY\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    auto& health = telemetry.health();
    
    // Register health checks
    health.register_check("database", []() {
        HealthCheck check;
        check.name = "database";
        check.status = HealthStatus::HEALTHY;
        check.message = "Database connection is active";
        check.details["connections"] = "5/100";
        check.details["response_time_ms"] = "2.5";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    health.register_check("api_server", []() {
        HealthCheck check;
        check.name = "api_server";
        check.status = HealthStatus::DEGRADED;
        check.message = "API server running but slow";
        check.details["latency_ms"] = "850";
        check.details["threshold_ms"] = "500";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    health.register_check("scheduler", []() {
        HealthCheck check;
        check.name = "scheduler";
        check.status = HealthStatus::HEALTHY;
        check.message = "Scheduler processing normally";
        check.details["pending_jobs"] = "3";
        check.details["active_threads"] = "4";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    std::cout << "Running health checks...\n\n";
    
    auto results = health.check_all();
    
    for (const auto& result : results) {
        std::cout << "  ┌─ " << result.name << "\n";
        std::cout << "  │  Status: ";
        switch (result.status) {
            case HealthStatus::HEALTHY:
                std::cout << "✓ HEALTHY";
                break;
            case HealthStatus::DEGRADED:
                std::cout << "⚠ DEGRADED";
                break;
            case HealthStatus::UNHEALTHY:
                std::cout << "✗ UNHEALTHY";
                break;
        }
        std::cout << "\n";
        std::cout << "  │  Message: " << result.message << "\n";
        
        if (!result.details.empty()) {
            std::cout << "  │  Details:\n";
            for (const auto& [key, value] : result.details) {
                std::cout << "  │    - " << key << ": " << value << "\n";
            }
        }
        std::cout << "  └─\n\n";
    }
    
    std::cout << "Overall status: ";
    switch (health.overall_status()) {
        case HealthStatus::HEALTHY:
            std::cout << "✓ HEALTHY\n";
            break;
        case HealthStatus::DEGRADED:
            std::cout << "⚠ DEGRADED\n";
            break;
        case HealthStatus::UNHEALTHY:
            std::cout << "✗ UNHEALTHY\n";
            break;
    }
    
    std::cout << "\nJSON health report:\n";
    std::cout << health.to_json() << "\n";
}

// =============================================================================
// Demo 5: Resource Monitoring
// =============================================================================

void demo_resource_monitoring() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 5: Resource Monitoring\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Resource monitor tracks system metrics.\n";
    std::cout << "Metrics: CPU, memory, disk, open files, threads\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    auto& resources = telemetry.resources();
    
    std::cout << "Collecting resource usage...\n\n";
    
    auto info = resources.get_usage();
    
    std::cout << "  Resource Usage:\n";
    std::cout << "  ┌─────────────────────────────────\n";
    std::cout << "  │ CPU:     " << std::fixed << std::setprecision(1)
              << info.cpu_percent << "%\n";
    std::cout << "  │ Memory:  " << (info.memory_used_bytes / 1024 / 1024) << " MB / "
              << (info.memory_total_bytes / 1024 / 1024) << " MB ("
              << std::fixed << std::setprecision(1) << info.memory_percent << "%)\n";
    
    if (info.disk_total_bytes > 0) {
        std::cout << "  │ Disk:    " << (info.disk_used_bytes / 1024 / 1024 / 1024) << " GB / "
                  << (info.disk_total_bytes / 1024 / 1024 / 1024) << " GB ("
                  << std::fixed << std::setprecision(1) << info.disk_percent << "%)\n";
    }
    
    std::cout << "  │ Files:   " << info.open_files << " open file descriptors\n";
    std::cout << "  │ Threads: " << info.thread_count << " threads\n";
    std::cout << "  └─────────────────────────────────\n\n";
    
    std::cout << "Starting automatic monitoring (5 second interval)...\n";
    resources.start_monitoring(5000);
    
    std::cout << "✓ Resource monitoring active\n";
    std::cout << "  (would continue in background)\n\n";
    
    resources.stop_monitoring();
    std::cout << "✓ Resource monitoring stopped\n";
}

// =============================================================================
// Demo 6: Prometheus Metrics Export
// =============================================================================

void demo_prometheus_export() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 6: Prometheus Metrics Export\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Export all metrics in Prometheus exposition format.\n";
    std::cout << "This can be scraped by Prometheus server.\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    
    std::cout << "Prometheus metrics endpoint output:\n";
    std::cout << "────────────────────────────────────────────────────────────\n";
    std::cout << telemetry.export_metrics();
    std::cout << "────────────────────────────────────────────────────────────\n\n";
    
    std::cout << "✓ Ready for Prometheus scraping at /metrics endpoint\n";
}

// =============================================================================
// Demo 7: Integration Example
// =============================================================================

void demo_integration() {
    std::cout << "\n============================================================\n";
    std::cout << "  Demo 7: Complete Integration Example\n";
    std::cout << "============================================================\n\n";
    
    std::cout << "Real-world scenario: Monitoring scheduler operations\n\n";
    
    auto& telemetry = TelemetryManager::instance();
    telemetry.clear(); // Start fresh
    
    // Setup metrics
    auto schedules_processed = telemetry.register_counter(
        "fdc_schedules_processed_total",
        "Total number of schedules processed");
    
    auto conflicts_detected = telemetry.register_counter(
        "fdc_conflicts_detected_total",
        "Total number of conflicts detected");
    
    auto active_trains = telemetry.register_gauge(
        "fdc_active_trains",
        "Number of trains currently in the schedule");
    
    auto processing_time = telemetry.register_histogram(
        "fdc_schedule_processing_seconds",
        "Time to process a schedule",
        {0.1, 0.5, 1.0, 2.0, 5.0, 10.0});
    
    // Setup health checks
    auto& health = telemetry.health();
    health.register_check("pathfinding", []() {
        HealthCheck check;
        check.name = "pathfinding";
        check.status = HealthStatus::HEALTHY;
        check.message = "Pathfinding algorithm operational";
        check.details["graph_nodes"] = "150";
        check.details["graph_edges"] = "320";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    std::cout << "Processing schedules with telemetry...\n\n";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> conflict_dist(0, 5);
    std::uniform_int_distribution<> train_dist(5, 20);
    std::uniform_int_distribution<> time_dist(100, 2000);
    
    // Simulate schedule processing
    for (int i = 0; i < 5; i++) {
        std::cout << "  Schedule " << (i + 1) << ":\n";
        
        int trains = train_dist(gen);
        active_trains->set(trains);
        std::cout << "    - Trains: " << trains << "\n";
        
        {
            Timer timer(processing_time);
            std::this_thread::sleep_for(std::chrono::milliseconds(time_dist(gen)));
            
            int conflicts = conflict_dist(gen);
            if (conflicts > 0) {
                conflicts_detected->increment(conflicts);
                std::cout << "    - Conflicts: " << conflicts << " detected\n";
            }
            
            schedules_processed->increment();
            std::cout << "    - Processing: " << timer.elapsed() * 1000 << " ms\n";
        }
        
        std::cout << "\n";
    }
    
    std::cout << "Summary:\n";
    std::cout << "  ✓ Processed: " << static_cast<int>(schedules_processed->value()) << " schedules\n";
    std::cout << "  ✓ Conflicts: " << static_cast<int>(conflicts_detected->value()) << " total\n";
    std::cout << "  ✓ Active trains: " << static_cast<int>(active_trains->value()) << "\n";
    std::cout << "  ✓ Avg processing: " 
              << (processing_time->sum() / processing_time->count()) * 1000 << " ms\n\n";
    
    std::cout << "Health status: ";
    switch (health.overall_status()) {
        case HealthStatus::HEALTHY:
            std::cout << "✓ All systems operational\n";
            break;
        case HealthStatus::DEGRADED:
            std::cout << "⚠ Some issues detected\n";
            break;
        case HealthStatus::UNHEALTHY:
            std::cout << "✗ Critical issues\n";
            break;
    }
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "FDC_Scheduler Telemetry & Monitoring Demo\n";
    std::cout << "==========================================\n\n";
    
    std::cout << "This demo showcases the telemetry system:\n";
    std::cout << "- Prometheus metrics (Counter, Gauge, Histogram)\n";
    std::cout << "- Health check system with detailed status\n";
    std::cout << "- Resource monitoring (CPU, memory, etc.)\n";
    std::cout << "- RAII performance timers\n";
    std::cout << "- Labeled metrics for dimensions\n";
    std::cout << "- Prometheus exposition format\n";
    
    try {
        demo_counter_metrics();
        demo_gauge_metrics();
        demo_histogram_metrics();
        demo_health_checks();
        demo_resource_monitoring();
        demo_prometheus_export();
        demo_integration();
        
        std::cout << "\n============================================================\n";
        std::cout << "  All Demos Completed Successfully!\n";
        std::cout << "============================================================\n\n";
        
        std::cout << "Telemetry Features:\n";
        std::cout << "✓ Prometheus-compatible metrics\n";
        std::cout << "✓ Counter (monotonic increase)\n";
        std::cout << "✓ Gauge (up/down values)\n";
        std::cout << "✓ Histogram (distribution buckets)\n";
        std::cout << "✓ Health check system\n";
        std::cout << "✓ Resource monitoring\n";
        std::cout << "✓ RAII performance timers\n";
        std::cout << "✓ Multi-dimensional labels\n";
        std::cout << "✓ Thread-safe operations\n";
        std::cout << "✓ JSON health reports\n";
        
        std::cout << "\nIntegration:\n";
        std::cout << "1. Expose /metrics endpoint for Prometheus\n";
        std::cout << "2. Expose /health endpoint for health checks\n";
        std::cout << "3. Configure Prometheus to scrape metrics\n";
        std::cout << "4. Set up Grafana dashboards\n";
        std::cout << "5. Configure alerts in Prometheus\n";
        
        std::cout << "\nNext steps:\n";
        std::cout << "1. Add metrics to scheduler operations\n";
        std::cout << "2. Create health checks for all components\n";
        std::cout << "3. Set up Prometheus server\n";
        std::cout << "4. Build Grafana dashboards\n";
        std::cout << "5. Configure alerting rules\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
