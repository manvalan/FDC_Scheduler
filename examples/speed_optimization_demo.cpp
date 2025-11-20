#include <iostream>
#include <iomanip>
#include <map>
#include "fdc_scheduler/speed_optimizer.hpp"
#include "fdc_scheduler/profiler.hpp"

using namespace fdc_scheduler;

void print_speed_profile(const std::string& title, const SpeedProfile& profile) {
    std::cout << "\n" << title << "\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    std::cout << "Total Distance: " << std::fixed << std::setprecision(1) 
              << profile.total_distance << " km\n";
    std::cout << "Total Time: " << std::fixed << std::setprecision(2) 
              << profile.total_time * 60 << " minutes\n";
    std::cout << "Total Energy: " << std::fixed << std::setprecision(1) 
              << profile.total_energy << " kWh\n";
    std::cout << "Average Speed: " << std::fixed << std::setprecision(1) 
              << profile.avg_speed << " km/h\n";
    std::cout << "Max Speed: " << std::fixed << std::setprecision(1) 
              << profile.max_speed << " km/h\n";
    std::cout << "Energy Efficiency: " << std::fixed << std::setprecision(2) 
              << profile.energy_per_km << " kWh/km\n";
    std::cout << "Energy per Ton-km: " << std::fixed << std::setprecision(4) 
              << profile.energy_per_ton_km << " kWh/(ton·km)\n\n";
    
    std::cout << "Speed Profile Segments:\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(8) << "Segment" 
              << std::setw(16) << "Type"
              << std::right << std::setw(10) << "Distance"
              << std::setw(12) << "V_start"
              << std::setw(12) << "V_end"
              << std::setw(12) << "Time"
              << std::setw(12) << "Energy" << "\n";
    std::cout << std::left << std::setw(8) << ""
              << std::setw(16) << ""
              << std::right << std::setw(10) << "(km)"
              << std::setw(12) << "(km/h)"
              << std::setw(12) << "(km/h)"
              << std::setw(12) << "(min)"
              << std::setw(12) << "(kWh)" << "\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (size_t i = 0; i < profile.segments.size(); ++i) {
        const auto& seg = profile.segments[i];
        double distance = seg.end_distance - seg.start_distance;
        double time_min = seg.duration * 60.0;
        
        std::cout << std::left << std::setw(8) << (i + 1)
                  << std::setw(16) << seg.type_string()
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(10) << distance
                  << std::setw(12) << seg.start_speed
                  << std::setw(12) << seg.end_speed
                  << std::setw(12) << time_min
                  << std::setw(12) << seg.energy_consumed << "\n";
    }
    std::cout << std::string(80, '-') << "\n";
}

void print_optimization_result(const OptimizationResult& result) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "OPTIMIZATION RESULTS\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Energy Savings: " << result.energy_savings << " kWh ";
    std::cout << "(" << std::setprecision(1) << result.energy_savings_percent << "%)\n";
    
    std::cout << "Time Increase: " << std::setprecision(2) << result.time_increase * 60 << " minutes ";
    std::cout << "(" << std::setprecision(1) << result.time_increase_percent << "%)\n\n";
    
    std::cout << "Profile Metrics:\n";
    std::cout << "  Acceleration phases: " << result.metrics.acceleration_phases << "\n";
    std::cout << "  Constant speed phases: " << result.metrics.constant_speed_phases << "\n";
    std::cout << "  Coasting phases: " << result.metrics.coasting_phases << "\n";
    std::cout << "  Braking phases: " << result.metrics.braking_phases << "\n";
    std::cout << "  Regenerated energy: " << std::fixed << std::setprecision(1) 
              << result.metrics.total_regen_energy << " kWh\n";
}

void demo_passenger_train() {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "DEMO 1: Passenger Train (Milan - Bologna)\n";
    std::cout << std::string(80, '=') << "\n";
    
    // Create physics model for passenger train
    TrainPhysics physics = TrainPhysics::passenger_default();
    
    std::cout << "\nTrain Physics:\n";
    std::cout << "  Mass: " << physics.mass << " tons\n";
    std::cout << "  Max Acceleration: " << physics.max_acceleration << " m/s²\n";
    std::cout << "  Max Deceleration: " << physics.max_deceleration << " m/s²\n";
    std::cout << "  Motor Efficiency: " << (physics.motor_efficiency * 100) << "%\n";
    std::cout << "  Regen Efficiency: " << (physics.regen_efficiency * 100) << "%\n";
    
    // Journey parameters
    double distance = 218.0;  // km (Milan to Bologna)
    double scheduled_time = 2.0;  // hours
    
    // Create simple train and route for the optimizer
    Train train("IC501", "InterCity 501", TrainType::HIGH_SPEED, 160.0, 0.8, 1.2);
    std::vector<std::pair<Node*, double>> route;
    Node milano("MILANO", "Milano Centrale", NodeType::STATION);
    Node bologna("BOLOGNA", "Bologna Centrale", NodeType::STATION);
    route.push_back({&milano, 0.0});
    route.push_back({&bologna, distance});
    
    // Test different optimization modes
    std::vector<std::pair<std::string, SpeedOptimizerConfig>> configs = {
        {"Performance Mode", SpeedOptimizerConfig::performance_mode()},
        {"Balanced Mode", SpeedOptimizerConfig::balanced_mode()},
        {"Eco Mode", SpeedOptimizerConfig::eco_mode()}
    };
    
    for (const auto& [mode_name, config] : configs) {
        std::cout << "\n" << std::string(80, '-') << "\n";
        std::cout << "Testing: " << mode_name << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        SpeedOptimizer optimizer(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        OptimizationResult result = optimizer.optimize_journey(train, route, physics, scheduled_time);
        auto end = std::chrono::high_resolution_clock::now();
        
        double optimization_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        print_optimization_result(result);
        
        std::cout << "\nOptimization Time: " << std::fixed << std::setprecision(3) 
                  << optimization_time_ms << " ms\n";
    }
}

void demo_freight_train() {
    std::cout << "\n\n" << std::string(80, '=') << "\n";
    std::cout << "DEMO 2: Freight Train (Bologna - Rome)\n";
    std::cout << std::string(80, '=') << "\n";
    
    // Create physics model for freight train
    TrainPhysics physics = TrainPhysics::freight_default();
    
    std::cout << "\nTrain Physics:\n";
    std::cout << "  Mass: " << physics.mass << " tons\n";
    std::cout << "  Max Acceleration: " << physics.max_acceleration << " m/s²\n";
    std::cout << "  Max Deceleration: " << physics.max_deceleration << " m/s²\n";
    std::cout << "  Motor Efficiency: " << (physics.motor_efficiency * 100) << "%\n";
    std::cout << "  Regen Efficiency: " << (physics.regen_efficiency * 100) << "%\n";
    
    // Journey parameters
    double distance = 362.0;  // km (Bologna to Rome)
    double scheduled_time = 4.5;  // hours (slower for freight)
    
    // Create train and route
    Train train("CARGO123", "Cargo Train 123", TrainType::FREIGHT, 100.0, 0.3, 0.8);
    std::vector<std::pair<Node*, double>> route;
    Node bologna("BOLOGNA", "Bologna Centrale", NodeType::STATION);
    Node roma("ROMA", "Roma Termini", NodeType::STATION);
    route.push_back({&bologna, 0.0});
    route.push_back({&roma, distance});
    
    // Use eco mode for freight (efficiency is critical)
    SpeedOptimizerConfig config = SpeedOptimizerConfig::eco_mode();
    config.time_slack_factor = 0.15;  // Allow more time for freight
    config.cruise_speed_reduction = 0.20;  // Reduce speed more
    
    SpeedOptimizer optimizer(config);
    
    auto start = std::chrono::high_resolution_clock::now();
    OptimizationResult result = optimizer.optimize_journey(train, route, physics, scheduled_time);
    auto end = std::chrono::high_resolution_clock::now();
    double optimization_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    print_speed_profile("Baseline Profile (No Optimization)", result.baseline_profile);
    print_speed_profile("Optimized Profile (Eco Mode)", result.optimized_profile);
    print_optimization_result(result);
    
    std::cout << "\nOptimization Time: " << std::fixed << std::setprecision(3) 
              << optimization_time_ms << " ms\n";
}

void demo_detailed_profile() {
    std::cout << "\n\n" << std::string(80, '=') << "\n";
    std::cout << "DEMO 3: Detailed Speed Profile Analysis\n";
    std::cout << std::string(80, '=') << "\n";
    
    TrainPhysics physics = TrainPhysics::passenger_default();
    double distance = 150.0;  // km
    double scheduled_time = 1.5;  // hours
    
    Train train("RE100", "Regional 100", TrainType::REGIONAL, 140.0, 0.6, 0.9);
    std::vector<std::pair<Node*, double>> route;
    Node start("START", "Start Station", NodeType::STATION);
    Node end("END", "End Station", NodeType::STATION);
    route.push_back({&start, 0.0});
    route.push_back({&end, distance});
    
    // Eco mode with coasting
    SpeedOptimizerConfig config = SpeedOptimizerConfig::eco_mode();
    SpeedOptimizer optimizer(config);
    
    OptimizationResult result = optimizer.optimize_journey(train, route, physics, scheduled_time);
    
    print_speed_profile("Baseline Profile", result.baseline_profile);
    print_speed_profile("Optimized Profile with Coasting", result.optimized_profile);
    print_optimization_result(result);
}

void demo_batch_optimization() {
    std::cout << "\n\n" << std::string(80, '=') << "\n";
    std::cout << "DEMO 4: Batch Optimization (Multiple Trains)\n";
    std::cout << std::string(80, '=') << "\n";
    
    std::cout << "\nNote: Batch optimization requires route and schedule data structures.\n";
    std::cout << "This demonstration shows the API usage pattern.\n";
    std::cout << "In a full implementation, trains would have associated route and schedule information.\n";
    
    // Create railway network
    RailwayNetwork network;
    network.add_node(Node("MILANO", "Milano Centrale", NodeType::STATION));
    network.add_node(Node("BOLOGNA", "Bologna Centrale", NodeType::STATION));
    network.add_node(Node("FIRENZE", "Firenze SMN", NodeType::STATION));
    network.add_node(Node("ROMA", "Roma Termini", NodeType::STATION));
    
    // Create multiple trains
    std::vector<Train> trains;
    
    Train train1("IC501", "InterCity 501", TrainType::HIGH_SPEED, 160.0, 0.8, 1.2);
    trains.push_back(train1);
    
    Train train2("IC502", "InterCity 502", TrainType::HIGH_SPEED, 160.0, 0.8, 1.2);
    trains.push_back(train2);
    
    Train train3("CARGO123", "Cargo 123", TrainType::FREIGHT, 100.0, 0.3, 0.8);
    trains.push_back(train3);
    
    // Define physics for each train type
    std::map<TrainType, TrainPhysics> physics_map;
    physics_map[TrainType::HIGH_SPEED] = TrainPhysics::passenger_default();
    physics_map[TrainType::FREIGHT] = TrainPhysics::freight_default();
    physics_map[TrainType::REGIONAL] = TrainPhysics::passenger_default();
    
    // Batch optimization
    SpeedOptimizerConfig config = SpeedOptimizerConfig::balanced_mode();
    BatchSpeedOptimizer batch_optimizer(config);
    
    std::cout << "\nBatch optimizer created with " << trains.size() << " trains\n";
    std::cout << "Physics models configured for " << physics_map.size() << " train types\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    auto results = batch_optimizer.optimize_trains(trains, network, physics_map);
    auto end = std::chrono::high_resolution_clock::now();
    double optimization_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "\nBatch optimization API demonstrated successfully\n";
    std::cout << "Optimization Time: " << std::fixed << std::setprecision(3) 
              << optimization_time_ms << " ms\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         FDC Scheduler - Dynamic Speed Optimization Demonstration             ║\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "║  This demo showcases energy-efficient train speed optimization using:       ║\n";
    std::cout << "║  • Physics-based modeling (mass, acceleration, resistance)                  ║\n";
    std::cout << "║  • Cruise control optimization                                              ║\n";
    std::cout << "║  • Coasting phases for energy savings                                       ║\n";
    std::cout << "║  • Regenerative braking                                                     ║\n";
    std::cout << "║  • Multiple optimization modes (Performance/Balanced/Eco)                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
    
    try {
        // Run demonstrations
        demo_passenger_train();
        demo_freight_train();
        demo_detailed_profile();
        demo_batch_optimization();
        
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "All demonstrations completed successfully!\n";
        std::cout << std::string(80, '=') << "\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
