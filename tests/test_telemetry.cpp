/**
 * @file test_telemetry.cpp
 * @brief Unit tests for telemetry and monitoring system
 */

#include <fdc_scheduler/telemetry.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>

using namespace fdc_scheduler;
using namespace testing;

// =============================================================================
// Counter Tests
// =============================================================================

TEST(CounterTest, Initialization) {
    Counter counter("test_counter", "Test counter metric");
    
    EXPECT_EQ(counter.name(), "test_counter");
    EXPECT_EQ(counter.help(), "Test counter metric");
    EXPECT_EQ(counter.type(), MetricType::COUNTER);
    EXPECT_DOUBLE_EQ(counter.value(), 0.0);
}

TEST(CounterTest, Increment) {
    Counter counter("test_counter", "Test counter");
    
    counter.increment();
    EXPECT_DOUBLE_EQ(counter.value(), 1.0);
    
    counter.increment(5.0);
    EXPECT_DOUBLE_EQ(counter.value(), 6.0);
}

TEST(CounterTest, NegativeIncrementThrows) {
    Counter counter("test_counter", "Test counter");
    
    EXPECT_THROW(counter.increment(-1.0), std::invalid_argument);
}

TEST(CounterTest, LabeledIncrement) {
    Counter counter("test_counter", "Test counter");
    
    Labels labels1 = {{"method", "GET"}, {"status", "200"}};
    Labels labels2 = {{"method", "POST"}, {"status", "201"}};
    
    counter.increment(labels1, 3.0);
    counter.increment(labels2, 5.0);
    
    EXPECT_DOUBLE_EQ(counter.value(labels1), 3.0);
    EXPECT_DOUBLE_EQ(counter.value(labels2), 5.0);
}

TEST(CounterTest, Serialization) {
    Counter counter("http_requests_total", "Total HTTP requests");
    counter.increment({{"method", "GET"}}, 10.0);
    
    std::string output = counter.serialize();
    
    EXPECT_THAT(output, HasSubstr("# HELP http_requests_total"));
    EXPECT_THAT(output, HasSubstr("# TYPE http_requests_total counter"));
    EXPECT_THAT(output, HasSubstr("http_requests_total{method=\"GET\"}"));
}

// =============================================================================
// Gauge Tests
// =============================================================================

TEST(GaugeTest, Initialization) {
    Gauge gauge("test_gauge", "Test gauge metric");
    
    EXPECT_EQ(gauge.name(), "test_gauge");
    EXPECT_EQ(gauge.help(), "Test gauge metric");
    EXPECT_EQ(gauge.type(), MetricType::GAUGE);
    EXPECT_DOUBLE_EQ(gauge.value(), 0.0);
}

TEST(GaugeTest, SetValue) {
    Gauge gauge("test_gauge", "Test gauge");
    
    gauge.set(42.0);
    EXPECT_DOUBLE_EQ(gauge.value(), 42.0);
    
    gauge.set(10.0);
    EXPECT_DOUBLE_EQ(gauge.value(), 10.0);
}

TEST(GaugeTest, IncrementDecrement) {
    Gauge gauge("test_gauge", "Test gauge");
    
    gauge.set(100.0);
    gauge.increment(25.0);
    EXPECT_DOUBLE_EQ(gauge.value(), 125.0);
    
    gauge.decrement(50.0);
    EXPECT_DOUBLE_EQ(gauge.value(), 75.0);
}

TEST(GaugeTest, LabeledValues) {
    Gauge gauge("temperature", "Temperature gauge");
    
    Labels room1 = {{"location", "room1"}};
    Labels room2 = {{"location", "room2"}};
    
    gauge.set(room1, 22.5);
    gauge.set(room2, 24.0);
    
    EXPECT_DOUBLE_EQ(gauge.value(room1), 22.5);
    EXPECT_DOUBLE_EQ(gauge.value(room2), 24.0);
}

TEST(GaugeTest, Serialization) {
    Gauge gauge("memory_bytes", "Memory usage in bytes");
    gauge.set({{"type", "heap"}}, 1024000.0);
    
    std::string output = gauge.serialize();
    
    EXPECT_THAT(output, HasSubstr("# HELP memory_bytes"));
    EXPECT_THAT(output, HasSubstr("# TYPE memory_bytes gauge"));
    EXPECT_THAT(output, HasSubstr("memory_bytes{type=\"heap\"}"));
}

// =============================================================================
// Histogram Tests
// =============================================================================

TEST(HistogramTest, Initialization) {
    Histogram hist("test_histogram", "Test histogram");
    
    EXPECT_EQ(hist.name(), "test_histogram");
    EXPECT_EQ(hist.help(), "Test histogram");
    EXPECT_EQ(hist.type(), MetricType::HISTOGRAM);
    EXPECT_EQ(hist.count(), 0);
    EXPECT_DOUBLE_EQ(hist.sum(), 0.0);
}

TEST(HistogramTest, Observe) {
    Histogram hist("latency", "Request latency", {0.1, 0.5, 1.0, 5.0});
    
    hist.observe(0.05);
    hist.observe(0.3);
    hist.observe(0.8);
    hist.observe(2.0);
    
    EXPECT_EQ(hist.count(), 4);
    EXPECT_DOUBLE_EQ(hist.sum(), 3.15);
    
    auto buckets = hist.buckets();
    EXPECT_EQ(buckets[0.1], 1);  // 0.05 <= 0.1
    EXPECT_EQ(buckets[0.5], 2);  // 0.05, 0.3 <= 0.5
    EXPECT_EQ(buckets[1.0], 3);  // 0.05, 0.3, 0.8 <= 1.0
    EXPECT_EQ(buckets[5.0], 4);  // All values <= 5.0
}

TEST(HistogramTest, LabeledObservations) {
    Histogram hist("response_time", "Response time", {0.1, 0.5, 1.0});
    
    Labels endpoint1 = {{"endpoint", "/api/v1"}};
    Labels endpoint2 = {{"endpoint", "/api/v2"}};
    
    hist.observe(endpoint1, 0.2);
    hist.observe(endpoint1, 0.4);
    hist.observe(endpoint2, 0.8);
    
    EXPECT_EQ(hist.count(), 3);
}

TEST(HistogramTest, Serialization) {
    Histogram hist("duration_seconds", "Duration", {0.1, 1.0, 10.0});
    hist.observe(0.5);
    hist.observe(5.0);
    
    std::string output = hist.serialize();
    
    EXPECT_THAT(output, HasSubstr("# HELP duration_seconds"));
    EXPECT_THAT(output, HasSubstr("# TYPE duration_seconds histogram"));
    EXPECT_THAT(output, HasSubstr("duration_seconds_bucket{le=\"0.100\"}"));
    EXPECT_THAT(output, HasSubstr("duration_seconds_bucket{le=\"+Inf\"}"));
    EXPECT_THAT(output, HasSubstr("duration_seconds_sum"));
    EXPECT_THAT(output, HasSubstr("duration_seconds_count"));
}

// =============================================================================
// Timer Tests
// =============================================================================

TEST(TimerTest, AutomaticObservation) {
    auto hist = std::make_shared<Histogram>("timer_test", "Timer test");
    
    {
        Timer timer(hist);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } // Timer destructor should record observation
    
    EXPECT_EQ(hist->count(), 1);
    EXPECT_GT(hist->sum(), 0.009);  // At least 9ms
    EXPECT_LT(hist->sum(), 0.020);  // Less than 20ms (with margin)
}

TEST(TimerTest, LabeledTimer) {
    auto hist = std::make_shared<Histogram>("timer_labeled", "Labeled timer test");
    Labels labels = {{"operation", "test"}};
    
    {
        Timer timer(hist, labels);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    EXPECT_EQ(hist->count(), 1);
    EXPECT_GT(hist->sum(), 0.004);
}

TEST(TimerTest, Elapsed) {
    auto hist = std::make_shared<Histogram>("timer_elapsed", "Elapsed test");
    Timer timer(hist);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    
    double elapsed = timer.elapsed();
    EXPECT_GT(elapsed, 0.014);
    EXPECT_LT(elapsed, 0.025);
}

// =============================================================================
// TelemetryManager Tests
// =============================================================================

TEST(TelemetryManagerTest, Singleton) {
    auto& tm1 = TelemetryManager::instance();
    auto& tm2 = TelemetryManager::instance();
    
    EXPECT_EQ(&tm1, &tm2);
}

TEST(TelemetryManagerTest, RegisterCounter) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto counter = tm.register_counter("test_counter", "Test counter");
    
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(counter->name(), "test_counter");
    
    // Registering again should return same instance
    auto counter2 = tm.register_counter("test_counter", "Different help");
    EXPECT_EQ(counter.get(), counter2.get());
}

TEST(TelemetryManagerTest, RegisterGauge) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto gauge = tm.register_gauge("test_gauge", "Test gauge");
    
    ASSERT_NE(gauge, nullptr);
    EXPECT_EQ(gauge->name(), "test_gauge");
}

TEST(TelemetryManagerTest, RegisterHistogram) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto hist = tm.register_histogram("test_histogram", "Test histogram");
    
    ASSERT_NE(hist, nullptr);
    EXPECT_EQ(hist->name(), "test_histogram");
}

TEST(TelemetryManagerTest, GetMetric) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto counter = tm.register_counter("my_counter", "My counter");
    counter->increment(42.0);
    
    auto retrieved = tm.get_metric("my_counter");
    ASSERT_NE(retrieved, nullptr);
    
    auto retrieved_counter = std::dynamic_pointer_cast<Counter>(retrieved);
    ASSERT_NE(retrieved_counter, nullptr);
    EXPECT_DOUBLE_EQ(retrieved_counter->value(), 42.0);
}

TEST(TelemetryManagerTest, UnregisterMetric) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    tm.register_counter("temp_counter", "Temp");
    ASSERT_NE(tm.get_metric("temp_counter"), nullptr);
    
    tm.unregister_metric("temp_counter");
    EXPECT_EQ(tm.get_metric("temp_counter"), nullptr);
}

TEST(TelemetryManagerTest, ExportMetrics) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto counter = tm.register_counter("requests_total", "Total requests");
    auto gauge = tm.register_gauge("memory_bytes", "Memory");
    
    counter->increment(100.0);
    gauge->set(1024000.0);
    
    std::string output = tm.export_metrics();
    
    EXPECT_THAT(output, HasSubstr("requests_total"));
    EXPECT_THAT(output, HasSubstr("memory_bytes"));
    EXPECT_THAT(output, HasSubstr("100.00"));
    EXPECT_THAT(output, HasSubstr("1024000.00"));
}

// =============================================================================
// HealthMonitor Tests
// =============================================================================

TEST(HealthMonitorTest, RegisterCheck) {
    HealthMonitor monitor;
    
    monitor.register_check("test_check", []() {
        HealthCheck check;
        check.name = "test_check";
        check.status = HealthStatus::HEALTHY;
        check.message = "All good";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    auto result = monitor.check("test_check");
    EXPECT_EQ(result.name, "test_check");
    EXPECT_EQ(result.status, HealthStatus::HEALTHY);
    EXPECT_EQ(result.message, "All good");
}

TEST(HealthMonitorTest, CheckAll) {
    HealthMonitor monitor;
    
    monitor.register_check("check1", []() {
        HealthCheck check;
        check.name = "check1";
        check.status = HealthStatus::HEALTHY;
        check.message = "OK";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    monitor.register_check("check2", []() {
        HealthCheck check;
        check.name = "check2";
        check.status = HealthStatus::DEGRADED;
        check.message = "Slow";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    auto results = monitor.check_all();
    EXPECT_EQ(results.size(), 2);
}

TEST(HealthMonitorTest, OverallStatus) {
    HealthMonitor monitor;
    
    monitor.register_check("healthy", []() {
        HealthCheck check;
        check.name = "healthy";
        check.status = HealthStatus::HEALTHY;
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    EXPECT_EQ(monitor.overall_status(), HealthStatus::HEALTHY);
    
    monitor.register_check("degraded", []() {
        HealthCheck check;
        check.name = "degraded";
        check.status = HealthStatus::DEGRADED;
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    EXPECT_EQ(monitor.overall_status(), HealthStatus::DEGRADED);
    
    monitor.register_check("unhealthy", []() {
        HealthCheck check;
        check.name = "unhealthy";
        check.status = HealthStatus::UNHEALTHY;
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    EXPECT_EQ(monitor.overall_status(), HealthStatus::UNHEALTHY);
}

TEST(HealthMonitorTest, JSONExport) {
    HealthMonitor monitor;
    
    monitor.register_check("test", []() {
        HealthCheck check;
        check.name = "test";
        check.status = HealthStatus::HEALTHY;
        check.message = "OK";
        check.details["version"] = "1.0";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    monitor.check_all();  // Run checks to populate results
    
    std::string json = monitor.to_json();
    
    EXPECT_THAT(json, HasSubstr("\"name\": \"test\""));
    EXPECT_THAT(json, HasSubstr("\"status\": \"healthy\""));
    EXPECT_THAT(json, HasSubstr("\"message\": \"OK\""));
    EXPECT_THAT(json, HasSubstr("\"version\": \"1.0\""));
}

// =============================================================================
// ResourceMonitor Tests
// =============================================================================

TEST(ResourceMonitorTest, GetUsage) {
    ResourceMonitor monitor;
    
    auto info = monitor.get_usage();
    
    // Basic sanity checks
    EXPECT_GE(info.cpu_percent, 0.0);
    EXPECT_GE(info.memory_used_bytes, 0);
    EXPECT_GT(info.memory_total_bytes, 0);
    EXPECT_GE(info.memory_percent, 0.0);
    EXPECT_LE(info.memory_percent, 100.0);
}

TEST(ResourceMonitorTest, LastSnapshot) {
    ResourceMonitor monitor;
    
    monitor.get_usage();
    auto snapshot = monitor.last_snapshot();
    
    EXPECT_GT(snapshot.memory_total_bytes, 0);
}

TEST(ResourceMonitorTest, StartStopMonitoring) {
    ResourceMonitor monitor;
    
    EXPECT_FALSE(monitor.is_monitoring());
    
    monitor.start_monitoring(1000);
    EXPECT_TRUE(monitor.is_monitoring());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    monitor.stop_monitoring();
    EXPECT_FALSE(monitor.is_monitoring());
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(TelemetryIntegrationTest, CompleteWorkflow) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    // Register metrics
    auto requests = tm.register_counter("http_requests", "HTTP requests");
    auto memory = tm.register_gauge("memory_usage", "Memory usage");
    auto latency = tm.register_histogram("request_latency", "Request latency",
                                         {0.01, 0.05, 0.1, 0.5, 1.0});
    
    // Simulate application behavior
    for (int i = 0; i < 10; i++) {
        requests->increment();
        memory->set(1024000 + i * 1000);
        
        {
            Timer timer(latency);
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 2));
        }
    }
    
    // Verify metrics
    EXPECT_DOUBLE_EQ(requests->value(), 10.0);
    EXPECT_DOUBLE_EQ(memory->value(), 1033000.0);
    EXPECT_EQ(latency->count(), 10);
    
    // Export
    std::string output = tm.export_metrics();
    EXPECT_THAT(output, HasSubstr("http_requests"));
    EXPECT_THAT(output, HasSubstr("memory_usage"));
    EXPECT_THAT(output, HasSubstr("request_latency"));
}

TEST(TelemetryIntegrationTest, HealthAndMetrics) {
    auto& tm = TelemetryManager::instance();
    tm.clear();
    
    auto& health = tm.health();
    
    // Register health check
    health.register_check("system", []() {
        HealthCheck check;
        check.name = "system";
        check.status = HealthStatus::HEALTHY;
        check.message = "System operational";
        check.timestamp = std::chrono::system_clock::now();
        return check;
    });
    
    // Register metric for health status
    auto health_status = tm.register_gauge("health_status", "Health status (0=unhealthy, 1=degraded, 2=healthy)");
    
    auto status = health.overall_status();
    health_status->set(static_cast<double>(status));
    
    EXPECT_EQ(status, HealthStatus::HEALTHY);
    EXPECT_DOUBLE_EQ(health_status->value(), 2.0);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
