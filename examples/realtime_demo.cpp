#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "fdc_scheduler/realtime_optimizer.hpp"

using namespace fdc_scheduler;
using namespace std::chrono_literals;

void print_separator(char c = '=') {
    std::cout << std::string(80, c) << "\n";
}

void print_position(const TrainPosition& pos) {
    std::cout << "  Train: " << pos.train_id << "\n";
    std::cout << "    Current Node: " << (pos.current_node ? pos.current_node->get_id() : "N/A") << "\n";
    std::cout << "    Next Node: " << (pos.next_node ? pos.next_node->get_id() : "N/A") << "\n";
    std::cout << "    Progress: " << std::fixed << std::setprecision(1) 
              << (pos.progress * 100) << "%\n";
    std::cout << "    Speed: " << pos.current_speed << " km/h\n";
}

void print_conflict(const PredictedConflict& conflict) {
    std::cout << "\n⚠️  PREDICTED CONFLICT\n";
    std::cout << "  Trains: " << conflict.train1_id << " vs " << conflict.train2_id << "\n";
    std::cout << "  Location: " << conflict.conflict_node->get_id() << "\n";
    std::cout << "  Confidence: " << std::fixed << std::setprecision(1) 
              << (conflict.confidence * 100) << "%\n";
    std::cout << "  Type: ";
    switch (conflict.type) {
        case ConflictType::HEAD_ON_COLLISION:
            std::cout << "Head-On Collision\n";
            break;
        case ConflictType::SECTION_OVERLAP:
            std::cout << "Section Overlap\n";
            break;
        case ConflictType::PLATFORM_CONFLICT:
            std::cout << "Platform Conflict\n";
            break;
        case ConflictType::TIMING_VIOLATION:
            std::cout << "Timing Violation\n";
            break;
    }
}

void print_adjustment(const ScheduleAdjustment& adj) {
    std::cout << "\n✓ RECOMMENDED ADJUSTMENT\n";
    std::cout << "  Train: " << adj.train_id << "\n";
    std::cout << "  Type: " << adj.type_string() << "\n";
    
    switch (adj.type) {
        case ScheduleAdjustment::Type::SPEED_CHANGE:
            if (adj.new_speed) {
                std::cout << "  New Speed: " << std::fixed << std::setprecision(1) 
                         << *adj.new_speed << " km/h\n";
            }
            break;
        case ScheduleAdjustment::Type::HOLD_AT_STATION:
            if (adj.hold_duration_minutes) {
                std::cout << "  Hold Duration: " << *adj.hold_duration_minutes << " minutes\n";
            }
            break;
        case ScheduleAdjustment::Type::ROUTE_CHANGE:
            std::cout << "  Route Change Required\n";
            break;
        default:
            break;
    }
    
    std::cout << "  Expected Delay Reduction: " << std::fixed << std::setprecision(1)
              << adj.estimated_delay_reduction << " minutes\n";
    std::cout << "  Confidence: " << (adj.confidence * 100) << "%\n";
    std::cout << "  Justification: " << adj.justification << "\n";
}

void demo_basic_tracking() {
    std::cout << "\n";
    print_separator();
    std::cout << "DEMO 1: Basic Train Position Tracking\n";
    print_separator();
    
    // Create network
    RailwayNetwork network;
    network.add_node(Node("MILANO", "Milano Centrale", NodeType::STATION));
    network.add_node(Node("ROGOREDO", "Milano Rogoredo", NodeType::STATION));
    network.add_node(Node("PIACENZA", "Piacenza", NodeType::STATION));
    network.add_node(Node("PARMA", "Parma", NodeType::STATION));
    
    // Create standalone nodes for positions
    Node milano("MILANO", "Milano Centrale", NodeType::STATION);
    Node rogoredo("ROGOREDO", "Milano Rogoredo", NodeType::STATION);
    Node piacenza("PIACENZA", "Piacenza", NodeType::STATION);
    Node parma("PARMA", "Parma", NodeType::STATION);
    
    // Create real-time optimizer
    RealTimeConfig config = RealTimeConfig::balanced();
    RealTimeOptimizer optimizer(network, config);
    
    std::cout << "\nConfiguration:\n";
    std::cout << "  Prediction Horizon: " << config.prediction_horizon_minutes << " minutes\n";
    std::cout << "  Update Frequency: " << config.update_frequency_seconds << " seconds\n";
    std::cout << "  Speed Adjustments: " << (config.enable_speed_adjustments ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Route Changes: " << (config.enable_route_changes ? "Enabled" : "Disabled") << "\n";
    
    // Simulate train positions
    std::cout << "\n--- Initial Positions ---\n";
    
    TrainPosition pos1;
    pos1.train_id = "IC501";
    pos1.current_node = &milano;
    pos1.next_node = &rogoredo;
    pos1.progress = 0.3; // 30% to next node
    pos1.current_speed = 120.0;
    pos1.timestamp = std::chrono::system_clock::now();
    
    optimizer.update_train_position(pos1);
    print_position(pos1);
    
    TrainPosition pos2;
    pos2.train_id = "IC502";
    pos2.current_node = &piacenza;
    pos2.next_node = &parma;
    pos2.progress = 0.6;
    pos2.current_speed = 140.0;
    pos2.timestamp = std::chrono::system_clock::now();
    
    optimizer.update_train_position(pos2);
    print_position(pos2);
    
    std::cout << "\n✓ Tracking " << optimizer.get_all_positions().size() << " trains\n";
}

void demo_delay_tracking() {
    std::cout << "\n\n";
    print_separator();
    std::cout << "DEMO 2: Delay Tracking and Reporting\n";
    print_separator();
    
    RailwayNetwork network;
    network.add_node(Node("MILANO", "Milano Centrale", NodeType::STATION));
    network.add_node(Node("BOLOGNA", "Bologna Centrale", NodeType::STATION));
    
    Node milano("MILANO", "Milano Centrale", NodeType::STATION);
    Node bologna("BOLOGNA", "Bologna Centrale", NodeType::STATION);
    
    RealTimeOptimizer optimizer(network, RealTimeConfig::balanced());
    
    // Train with delay
    TrainPosition pos;
    pos.train_id = "IC503";
    pos.current_node = &milano;
    pos.next_node = &bologna;
    pos.progress = 0.5;
    pos.current_speed = 80.0; // Slower than normal
    pos.timestamp = std::chrono::system_clock::now();
    
    optimizer.update_train_position(pos);
    
    // Report delay
    TrainDelay delay;
    delay.train_id = "IC503";
    delay.delay_minutes = 12.5;
    delay.reason = "Signal failure at previous station";
    delay.detected_at = std::chrono::system_clock::now();
    delay.is_recovering = true;
    
    optimizer.report_delay(delay);
    
    std::cout << "\n📊 Delay Report:\n";
    std::cout << "  Train: " << delay.train_id << "\n";
    std::cout << "  Delay: " << std::fixed << std::setprecision(1) 
              << delay.delay_minutes << " minutes\n";
    std::cout << "  Reason: " << delay.reason << "\n";
    std::cout << "  Status: " << (delay.is_recovering ? "Recovering" : "Accumulating") << "\n";
    
    auto current_delay = optimizer.calculate_current_delay("IC503");
    if (current_delay) {
        std::cout << "\n✓ Current delay confirmed: " << *current_delay << " minutes\n";
    }
}

void demo_conflict_prediction() {
    std::cout << "\n\n";
    print_separator();
    std::cout << "DEMO 3: Conflict Prediction\n";
    print_separator();
    
    // Create network
    RailwayNetwork network;
    network.add_node(Node("STATION_A", "Station A", NodeType::STATION));
    network.add_node(Node("JUNCTION", "Junction", NodeType::JUNCTION));
    network.add_node(Node("STATION_B", "Station B", NodeType::STATION));
    network.add_node(Node("STATION_C", "Station C", NodeType::STATION));
    
    Node station_a("STATION_A", "Station A", NodeType::STATION);
    Node junction("JUNCTION", "Junction", NodeType::JUNCTION);
    Node station_b("STATION_B", "Station B", NodeType::STATION);
    Node station_c("STATION_C", "Station C", NodeType::STATION);
    
    RealTimeOptimizer optimizer(network, RealTimeConfig::balanced());
    
    // Two trains heading to same junction
    std::cout << "\nSimulation: Two trains approaching same junction\n\n";
    
    TrainPosition train1;
    train1.train_id = "FREIGHT_001";
    train1.current_node = &station_a;
    train1.next_node = &junction;
    train1.progress = 0.75; // Almost at junction
    train1.current_speed = 80.0;
    train1.timestamp = std::chrono::system_clock::now();
    
    TrainPosition train2;
    train2.train_id = "EXPRESS_201";
    train2.current_node = &station_c;
    train2.next_node = &junction;
    train2.progress = 0.70; // Also close to junction
    train2.current_speed = 120.0;
    train2.timestamp = std::chrono::system_clock::now();
    
    std::cout << "Initial Positions:\n";
    print_position(train1);
    std::cout << "\n";
    print_position(train2);
    
    optimizer.update_train_position(train1);
    optimizer.update_train_position(train2);
    
    // Predict conflicts
    std::cout << "\n--- Analyzing Conflict Risk ---\n";
    auto conflicts = optimizer.predict_conflicts();
    
    if (conflicts.empty()) {
        std::cout << "\n✓ No conflicts predicted\n";
    } else {
        std::cout << "\n⚠️  Predicted " << conflicts.size() << " conflict(s):\n";
        for (const auto& conflict : conflicts) {
            print_conflict(conflict);
        }
    }
}

void demo_schedule_adjustments() {
    std::cout << "\n\n";
    print_separator();
    std::cout << "DEMO 4: Dynamic Schedule Adjustments\n";
    print_separator();
    
    // Create network
    RailwayNetwork network;
    network.add_node(Node("START", "Start Station", NodeType::STATION));
    network.add_node(Node("MID", "Middle Junction", NodeType::JUNCTION));
    network.add_node(Node("END", "End Station", NodeType::STATION));
    
    Node start("START", "Start Station", NodeType::STATION);
    Node mid("MID", "Middle Junction", NodeType::JUNCTION);
    Node end("END", "End Station", NodeType::STATION);
    
    // Use aggressive config for more adjustments
    RealTimeConfig config = RealTimeConfig::aggressive();
    RealTimeOptimizer optimizer(network, config);
    
    std::cout << "\nUsing AGGRESSIVE optimization mode\n";
    std::cout << "  Delay Tolerance: " << config.delay_tolerance_minutes << " minutes\n";
    std::cout << "  Max Adjustments: " << config.max_adjustments_per_cycle << "\n";
    
    // Create conflict situation
    TrainPosition pos1;
    pos1.train_id = "LOCAL_101";
    pos1.current_node = &start;
    pos1.next_node = &mid;
    pos1.progress = 0.8;
    pos1.current_speed = 100.0;
    pos1.timestamp = std::chrono::system_clock::now();
    
    TrainPosition pos2;
    pos2.train_id = "EXPRESS_202";
    pos2.current_node = &mid;
    pos2.next_node = &end;
    pos2.progress = 0.1;
    pos2.current_speed = 140.0;
    pos2.timestamp = std::chrono::system_clock::now();
    
    optimizer.update_train_position(pos1);
    optimizer.update_train_position(pos2);
    
    // Run full optimization cycle
    std::cout << "\n--- Running Optimization Cycle ---\n";
    
    auto adjustments = optimizer.optimize();
    
    if (adjustments.empty()) {
        std::cout << "\n✓ No adjustments needed\n";
    } else {
        std::cout << "\n📋 Generated " << adjustments.size() << " adjustment(s):\n";
        for (const auto& adj : adjustments) {
            print_adjustment(adj);
        }
        
        // Apply first adjustment
        if (!adjustments.empty()) {
            std::cout << "\n--- Applying First Adjustment ---\n";
            bool success = optimizer.apply_adjustment(adjustments[0]);
            if (success) {
                std::cout << "✓ Adjustment applied successfully\n";
            } else {
                std::cout << "✗ Failed to apply adjustment\n";
            }
        }
    }
}

void demo_realtime_simulation() {
    std::cout << "\n\n";
    print_separator();
    std::cout << "DEMO 5: Real-Time Simulation with Updates\n";
    print_separator();
    
    // Create network
    RailwayNetwork network;
    network.add_node(Node("A", "Station A", NodeType::STATION));
    network.add_node(Node("B", "Station B", NodeType::STATION));
    network.add_node(Node("C", "Station C", NodeType::STATION));
    
    Node a("A", "Station A", NodeType::STATION);
    Node b("B", "Station B", NodeType::STATION);
    Node c("C", "Station C", NodeType::STATION);
    
    RealTimeOptimizer optimizer(network, RealTimeConfig::balanced());
    
    // Register callbacks
    int position_updates = 0;
    int conflicts_detected = 0;
    int adjustments_generated = 0;
    
    optimizer.on_position_update([&](const TrainPosition& /* pos */) {
        position_updates++;
    });
    
    optimizer.on_conflict_predicted([&](const PredictedConflict& /* conflict */) {
        conflicts_detected++;
    });
    
    optimizer.on_adjustment_generated([&](const ScheduleAdjustment& /* adj */) {
        adjustments_generated++;
    });
    
    std::cout << "\nSimulating 5 update cycles...\n\n";
    
    TrainPosition train_pos;
    train_pos.train_id = "SIM_TRAIN";
    train_pos.current_node = &a;
    train_pos.next_node = &b;
    train_pos.current_speed = 100.0;
    train_pos.timestamp = std::chrono::system_clock::now();
    
    for (int i = 0; i < 5; ++i) {
        std::cout << "Update " << (i + 1) << ": ";
        
        // Update progress
        train_pos.progress = 0.2 * i;
        train_pos.timestamp = std::chrono::system_clock::now();
        
        if (train_pos.progress >= 1.0) {
            train_pos.current_node = &b;
            train_pos.next_node = &c;
            train_pos.progress = 0.0;
        }
        
        optimizer.update_train_position(train_pos);
        
        std::cout << "Train at " << std::fixed << std::setprecision(0)
                  << (train_pos.progress * 100) << "% progress, "
                  << train_pos.current_speed << " km/h\n";
        
        std::this_thread::sleep_for(200ms); // Simulate time passing
    }
    
    // Get statistics
    const auto& stats = optimizer.get_stats();
    
    std::cout << "\n--- Session Statistics ---\n";
    std::cout << "  Total Updates: " << stats.total_updates << "\n";
    std::cout << "  Conflicts Predicted: " << stats.conflicts_predicted << "\n";
    std::cout << "  Conflicts Avoided: " << stats.conflicts_avoided << "\n";
    std::cout << "  Adjustments Applied: " << stats.adjustments_applied << "\n";
    std::cout << "  Avg Delay Reduction: " << std::fixed << std::setprecision(2)
              << stats.avg_delay_reduction << " minutes\n";
    
    std::cout << "\n--- Callback Statistics ---\n";
    std::cout << "  Position Updates: " << position_updates << "\n";
    std::cout << "  Conflicts Detected: " << conflicts_detected << "\n";
    std::cout << "  Adjustments Generated: " << adjustments_generated << "\n";
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "      FDC Scheduler - Real-Time Optimization Demonstration\n";
    print_separator();
    
    std::cout << "\nThis demo showcases real-time train tracking and optimization:\n";
    std::cout << "  • Live position tracking\n";
    std::cout << "  • Delay monitoring and reporting\n";
    std::cout << "  • Predictive conflict detection\n";
    std::cout << "  • Dynamic schedule adjustments\n";
    std::cout << "  • Real-time simulation with callbacks\n";
    
    try {
        demo_basic_tracking();
        demo_delay_tracking();
        demo_conflict_prediction();
        demo_schedule_adjustments();
        demo_realtime_simulation();
        
        std::cout << "\n\n";
        print_separator();
        std::cout << "All demonstrations completed successfully!\n";
        print_separator();
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n ERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
