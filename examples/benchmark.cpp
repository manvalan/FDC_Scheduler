#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/schedule.hpp>
#include <fdc_scheduler/conflict_detector.hpp>
#include <fdc_scheduler/railway_ai_resolver.hpp>
#include <fdc_scheduler/profiler.hpp>
#include <iostream>
#include <random>

using namespace fdc_scheduler;

/**
 * @brief Benchmark suite for FDC_Scheduler performance testing
 */
class BenchmarkSuite {
public:
    BenchmarkSuite() = default;
    
    /**
     * @brief Create a test network with specified size
     */
    std::shared_ptr<RailwayNetwork> create_network(size_t num_stations, size_t connections_per_station) {
        auto& profiler = Profiler::instance();
        
        return profiler.profile("create_network", [&]() {
            auto network = std::make_shared<RailwayNetwork>();
            
            // Create stations
            for (size_t i = 0; i < num_stations; ++i) {
                std::string id = "STATION_" + std::to_string(i);
                std::string name = "Station " + std::to_string(i);
                Node node(id, name, NodeType::STATION);
                network->add_node(node);
            }
            
            // Create connections
            std::random_device rd;
            std::mt19937 gen(42); // Fixed seed for reproducibility
            std::uniform_int_distribution<> dis(0, num_stations - 1);
            std::uniform_real_distribution<> dist_gen(50.0, 500.0);
            
            for (size_t i = 0; i < num_stations; ++i) {
                for (size_t j = 0; j < connections_per_station; ++j) {
                    size_t target = dis(gen);
                    if (target != i && !network->has_edge("STATION_" + std::to_string(i), 
                                                           "STATION_" + std::to_string(target))) {
                        std::string from = "STATION_" + std::to_string(i);
                        std::string to = "STATION_" + std::to_string(target);
                        double distance = dist_gen(gen);
                        
                        Edge edge(from, to, distance, TrackType::DOUBLE);
                        network->add_edge(edge);
                    }
                }
            }
            
            return network;
        });
    }
    
    /**
     * @brief Benchmark pathfinding operations
     */
    void benchmark_pathfinding(std::shared_ptr<RailwayNetwork> network, size_t iterations) {
        auto& profiler = Profiler::instance();
        
        std::cout << "\nBenchmarking pathfinding (" << iterations << " iterations)...\n";
        
        std::random_device rd;
        std::mt19937 gen(42);
        std::uniform_int_distribution<> dis(0, network->num_nodes() - 1);
        
        for (size_t i = 0; i < iterations; ++i) {
            size_t start_idx = dis(gen);
            size_t end_idx = dis(gen);
            
            if (start_idx == end_idx) continue;
            
            std::string start = "STATION_" + std::to_string(start_idx);
            std::string end = "STATION_" + std::to_string(end_idx);
            
            profiler.profile("find_shortest_path", [&]() {
                return network->find_shortest_path(start, end);
            });
        }
    }
    
    /**
     * @brief Benchmark conflict detection
     */
    void benchmark_conflict_detection(std::shared_ptr<RailwayNetwork> network, size_t num_trains) {
        auto& profiler = Profiler::instance();
        
        std::cout << "Benchmarking conflict detection (" << num_trains << " trains)...\n";
        
        // Create schedules
        std::vector<std::shared_ptr<TrainSchedule>> schedules;
        auto now = std::chrono::system_clock::now();
        
        profiler.profile("create_schedules", [&]() {
            for (size_t i = 0; i < num_trains; ++i) {
                std::string train_id = "TRAIN_" + std::to_string(i);
                auto schedule = std::make_shared<TrainSchedule>(train_id);
                
                // Add 3-5 stops
                size_t num_stops = 3 + (i % 3);
                auto departure = now + std::chrono::minutes(i * 10);
                
                for (size_t j = 0; j < num_stops; ++j) {
                    std::string station = "STATION_" + std::to_string((i + j) % network->num_nodes());
                    auto arrival = departure + std::chrono::minutes(j * 30);
                    auto dept = arrival + std::chrono::minutes(2);
                    
                    ScheduleStop stop(station, arrival, dept, true);
                    stop.platform = (j % 10) + 1;
                    schedule->add_stop(stop);
                }
                
                schedules.push_back(schedule);
            }
        });
        
        // Detect conflicts
        profiler.profile("detect_conflicts", [&]() {
            ConflictDetector detector(*network);
            return detector.detect_all(schedules);
        });
    }
    
    /**
     * @brief Benchmark AI resolution
     */
    void benchmark_ai_resolution(std::shared_ptr<RailwayNetwork> network) {
        auto& profiler = Profiler::instance();
        
        std::cout << "Benchmarking AI conflict resolution...\n";
        
        // Create test schedules with conflicts
        std::vector<std::shared_ptr<TrainSchedule>> schedules;
        auto now = std::chrono::system_clock::now();
        
        // Two trains on same route at same time (guaranteed conflict)
        for (int i = 0; i < 2; ++i) {
            auto schedule = std::make_shared<TrainSchedule>("CONFLICT_TRAIN_" + std::to_string(i));
            
            auto arrival1 = now + std::chrono::minutes(i * 2);
            auto dept1 = arrival1 + std::chrono::minutes(2);
            ScheduleStop stop1("STATION_0", arrival1, dept1, true);
            stop1.platform = 1; // Same platform!
            
            auto arrival2 = arrival1 + std::chrono::minutes(30);
            auto dept2 = arrival2 + std::chrono::minutes(2);
            ScheduleStop stop2("STATION_1", arrival2, dept2, true);
            stop2.platform = 1;
            
            schedule->add_stop(stop1);
            schedule->add_stop(stop2);
            schedules.push_back(schedule);
        }
        
        // Detect conflicts
        ConflictDetector detector(*network);
        auto conflicts = detector.detect_all(schedules);
        
        if (!conflicts.empty()) {
            std::cout << "  Found " << conflicts.size() << " conflicts to resolve\n";
            
            // Resolve
            profiler.profile("resolve_conflicts", [&]() {
                RailwayAIResolver resolver(*network);
                return resolver.resolve_conflicts(schedules, conflicts);
            });
        }
    }
    
    /**
     * @brief Run complete benchmark suite
     */
    void run_all_benchmarks() {
        auto& profiler = Profiler::instance();
        profiler.reset();
        
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "FDC_SCHEDULER PERFORMANCE BENCHMARK SUITE\n";
        std::cout << std::string(80, '=') << "\n";
        
        // Small network benchmarks
        std::cout << "\n--- Small Network (50 stations) ---\n";
        auto small_network = create_network(50, 3);
        std::cout << "  Network: " << small_network->num_nodes() << " nodes, " 
                  << small_network->num_edges() << " edges\n";
        benchmark_pathfinding(small_network, 100);
        benchmark_conflict_detection(small_network, 20);
        benchmark_ai_resolution(small_network);
        
        // Medium network benchmarks
        std::cout << "\n--- Medium Network (200 stations) ---\n";
        auto medium_network = create_network(200, 4);
        std::cout << "  Network: " << medium_network->num_nodes() << " nodes, " 
                  << medium_network->num_edges() << " edges\n";
        benchmark_pathfinding(medium_network, 100);
        benchmark_conflict_detection(medium_network, 50);
        
        // Large network benchmarks
        std::cout << "\n--- Large Network (500 stations) ---\n";
        auto large_network = create_network(500, 5);
        std::cout << "  Network: " << large_network->num_nodes() << " nodes, " 
                  << large_network->num_edges() << " edges\n";
        benchmark_pathfinding(large_network, 50);
        benchmark_conflict_detection(large_network, 100);
        
        // Print final report
        profiler.print_report();
    }
};

int main() {
    try {
        BenchmarkSuite suite;
        suite.run_all_benchmarks();
        
        std::cout << "\n✓ Benchmarks completed successfully!\n\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
