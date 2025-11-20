#include <fdc_scheduler/railway_network.hpp>
#include <fdc_scheduler/schedule.hpp>
#include <fdc_scheduler/conflict_detector.hpp>
#include <fdc_scheduler/route_optimizer.hpp>
#include <fdc_scheduler/profiler.hpp>
#include <iostream>
#include <iomanip>

using namespace fdc_scheduler;

void print_separator() {
    std::cout << std::string(80, '=') << "\n";
}

void print_route(const Path& path, const std::string& label) {
    std::cout << "\n" << label << ":\n";
    std::cout << "  Nodes: ";
    for (size_t i = 0; i < path.nodes.size(); ++i) {
        std::cout << path.nodes[i];
        if (i < path.nodes.size() - 1) std::cout << " → ";
    }
    std::cout << "\n";
    std::cout << "  Distance: " << std::fixed << std::setprecision(1) 
              << path.total_distance << " km\n";
    std::cout << "  Est. Time: " << std::fixed << std::setprecision(1) 
              << path.min_travel_time * 60.0 << " min\n";
}

void print_quality(const RouteQuality& quality) {
    std::cout << "\n  Quality Metrics:\n";
    std::cout << "    Distance score:     " << std::fixed << std::setprecision(3) 
              << quality.distance_score << "\n";
    std::cout << "    Time score:         " << quality.time_score << "\n";
    std::cout << "    Conflict score:     " << quality.conflict_score << "\n";
    std::cout << "    Track quality:      " << quality.track_quality_score << "\n";
    std::cout << "    Overall score:      " << quality.overall_score << "\n";
}

int main() {
    print_separator();
    std::cout << "ROUTE REROUTING DEMONSTRATION\n";
    print_separator();
    
    // Create test network (Milan - Rome corridor with alternatives)
    std::cout << "\n1. Creating railway network...\n";
    RailwayNetwork network;
    
    // Main stations
    network.add_node(Node("MILANO", "Milano Centrale", NodeType::STATION));
    network.add_node(Node("BOLOGNA", "Bologna Centrale", NodeType::STATION));
    network.add_node(Node("FIRENZE", "Firenze SMN", NodeType::STATION));
    network.add_node(Node("ROMA", "Roma Termini", NodeType::STATION));
    
    // Alternative route stations
    network.add_node(Node("PARMA", "Parma", NodeType::STATION));
    network.add_node(Node("MODENA", "Modena", NodeType::STATION));
    network.add_node(Node("PRATO", "Prato", NodeType::STATION));
    network.add_node(Node("AREZZO", "Arezzo", NodeType::STATION));
    
    // Main route (high-speed)
    network.add_edge(Edge("MILANO", "BOLOGNA", 220.0, TrackType::HIGH_SPEED));
    network.add_edge(Edge("BOLOGNA", "FIRENZE", 95.0, TrackType::HIGH_SPEED));
    network.add_edge(Edge("FIRENZE", "ROMA", 275.0, TrackType::HIGH_SPEED));
    
    // Alternative route 1 (via Parma-Modena)
    network.add_edge(Edge("MILANO", "PARMA", 100.0, TrackType::DOUBLE));
    network.add_edge(Edge("PARMA", "MODENA", 60.0, TrackType::DOUBLE));
    network.add_edge(Edge("MODENA", "BOLOGNA", 50.0, TrackType::DOUBLE));
    
    // Alternative route 2 (via Prato-Arezzo)
    network.add_edge(Edge("FIRENZE", "PRATO", 18.0, TrackType::DOUBLE));
    network.add_edge(Edge("PRATO", "AREZZO", 70.0, TrackType::SINGLE));
    network.add_edge(Edge("AREZZO", "ROMA", 220.0, TrackType::DOUBLE));
    
    std::cout << "  Network created: " << network.num_nodes() << " stations, " 
              << network.num_edges() << " connections\n";
    
    // Find direct route
    std::cout << "\n2. Finding direct route...\n";
    auto direct_route = network.find_shortest_path("MILANO", "ROMA");
    print_route(direct_route, "Direct Route");
    
    // Initialize route optimizer
    std::cout << "\n3. Finding alternative routes...\n";
    
    RouteOptimizerConfig config;
    config.distance_weight = 0.4;
    config.time_weight = 0.3;
    config.track_quality_weight = 0.2;
    config.conflict_weight = 0.1;
    config.prefer_high_speed = true;
    config.max_alternatives = 5;
    
    RouteOptimizer optimizer(network, config);
    
    // Find alternatives
    auto alternatives = optimizer.find_alternatives("MILANO", "ROMA");
    
    std::cout << "\nFound " << alternatives.size() << " alternative routes:\n";
    
    for (size_t i = 0; i < alternatives.size(); ++i) {
        std::cout << "\n--- Alternative " << (i + 1) << " ---\n";
        std::cout << alternatives[i].description << "\n";
        print_route(alternatives[i].path, "Route");
        print_quality(alternatives[i].quality);
    }
    
    // Optimization statistics
    auto stats = optimizer.get_last_stats();
    std::cout << "\nOptimization Statistics:\n";
    std::cout << "  Alternatives considered: " << stats.alternatives_considered << "\n";
    std::cout << "  Valid alternatives: " << stats.valid_alternatives << "\n";
    std::cout << "  Best score: " << std::fixed << std::setprecision(3) 
              << stats.best_score << "\n";
    std::cout << "  Computation time: " << stats.computation_time_ms << " ms\n";
    
    // Demonstrate rerouting with conflicts
    std::cout << "\n4. Simulating conflict-based rerouting...\n";
    
    // Create a schedule
    auto now = std::chrono::system_clock::now();
    TrainSchedule schedule("FR1000");
    
    ScheduleStop stop1("MILANO", now, now + std::chrono::minutes(5), true);
    ScheduleStop stop2("FIRENZE", now + std::chrono::hours(2), 
                       now + std::chrono::hours(2) + std::chrono::minutes(5), true);
    ScheduleStop stop3("ROMA", now + std::chrono::hours(4), 
                       now + std::chrono::hours(4), true);
    
    schedule.add_stop(stop1);
    schedule.add_stop(stop2);
    schedule.add_stop(stop3);
    
    // Simulate conflict on main route
    std::vector<Conflict> conflicts;
    Conflict conflict;
    conflict.type = ConflictType::SECTION_OVERLAP;
    conflict.train1_id = "FR1000";
    conflict.train2_id = "FR2000";
    conflict.location = "BOLOGNA";
    conflict.description = "Track maintenance on high-speed line";
    conflicts.push_back(conflict);
    
    std::cout << "  Simulated conflict: " << conflict.description << "\n";
    std::cout << "  Location: " << conflict.location << "\n";
    
    // Find best reroute
    auto best_reroute = optimizer.find_best_reroute(schedule, conflicts);
    
    if (best_reroute.has_value()) {
        std::cout << "\n  ✓ Found optimal reroute:\n";
        std::cout << "    " << best_reroute->description << "\n";
        print_quality(best_reroute->quality);
        
        // Apply reroute
        std::cout << "\n5. Applying reroute to schedule...\n";
        TrainSchedule modified_schedule = schedule;
        if (optimizer.apply_reroute(modified_schedule, *best_reroute)) {
            std::cout << "  ✓ Reroute applied successfully\n";
            std::cout << "  New schedule has " << modified_schedule.get_stops().size() 
                      << " stops\n";
        }
    } else {
        std::cout << "  ✗ No viable alternative route found\n";
    }
    
    // Demonstrate batch optimization
    std::cout << "\n6. Batch optimization demonstration...\n";
    
    std::vector<std::shared_ptr<TrainSchedule>> schedules;
    schedules.push_back(std::make_shared<TrainSchedule>(schedule));
    
    BatchRouteOptimizer batch_optimizer(network);
    auto batch_results = batch_optimizer.optimize_batch(schedules, conflicts);
    
    std::cout << "  Optimized " << batch_results.size() << " train routes\n";
    
    auto batch_stats = batch_optimizer.get_stats();
    std::cout << "\n  Batch Statistics:\n";
    std::cout << "    Trains rerouted: " << batch_stats.trains_rerouted << "\n";
    std::cout << "    Total alternatives: " << batch_stats.total_alternatives << "\n";
    std::cout << "    Average quality: " << std::fixed << std::setprecision(3) 
              << batch_stats.average_quality << "\n";
    std::cout << "    Computation time: " << batch_stats.computation_time_ms << " ms\n";
    
    // Performance profiling
    std::cout << "\n7. Performance Profiling:\n";
    Profiler::instance().print_report();
    
    print_separator();
    std::cout << "✓ Route rerouting demonstration completed successfully!\n";
    print_separator();
    
    return 0;
}
