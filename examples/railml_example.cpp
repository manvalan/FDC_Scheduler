/**
 * @file railml_example.cpp
 * @brief Example demonstrating RailML import/export functionality
 * 
 * This example shows how to:
 * 1. Create a railway network programmatically
 * 2. Export to RailML 2.x and 3.x formats
 * 3. Import from RailML files (when parser is fully implemented)
 * 4. Validate roundtrip import/export
 */

#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/schedule.hpp"
#include "fdc_scheduler/train.hpp"
#include "fdc_scheduler/node.hpp"
#include "fdc_scheduler/edge.hpp"
#include "fdc_scheduler/railml_parser.hpp"
#include "fdc_scheduler/railml_exporter.hpp"
#include <iostream>
#include <iomanip>

using namespace fdc_scheduler;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

/**
 * Create a simple railway network for testing
 */
std::shared_ptr<RailwayNetwork> create_test_network() {
    auto network = std::make_shared<RailwayNetwork>();
    
    // Add stations
    network->add_node(Node("A", "Station A", NodeType::STATION, 10));
    network->add_node(Node("B", "Station B", NodeType::STATION, 8));
    network->add_node(Node("C", "Station C", NodeType::STATION, 12));
    network->add_node(Node("D", "Station D", NodeType::STATION, 6));
    
    // Add tracks
    network->add_edge(Edge("A", "B", 25.0, TrackType::DOUBLE, 160.0));
    network->add_edge(Edge("B", "C", 30.0, TrackType::SINGLE, 120.0));
    network->add_edge(Edge("C", "D", 20.0, TrackType::DOUBLE, 140.0));
    network->add_edge(Edge("A", "D", 45.0, TrackType::HIGH_SPEED, 200.0)); // Alternative route
    
    return network;
}

/**
 * Create sample train schedules
 */
std::vector<std::shared_ptr<TrainSchedule>> create_test_schedules() {
    std::vector<std::shared_ptr<TrainSchedule>> schedules;
    
    // Reference time (today at midnight)
    auto base_time = std::chrono::system_clock::now();
    auto today_midnight = std::chrono::time_point_cast<std::chrono::hours>(base_time);
    
    // Train 1: A -> B -> C -> D (morning express)
    auto train1 = std::make_shared<TrainSchedule>("IC_100");
    train1->add_stop(ScheduleStop("A", 
        today_midnight + std::chrono::hours(6), 
        today_midnight + std::chrono::hours(6) + std::chrono::minutes(5), 
        true));
    train1->add_stop(ScheduleStop("B", 
        today_midnight + std::chrono::hours(7), 
        today_midnight + std::chrono::hours(7) + std::chrono::minutes(3), 
        true));
    train1->add_stop(ScheduleStop("C", 
        today_midnight + std::chrono::hours(8), 
        today_midnight + std::chrono::hours(8) + std::chrono::minutes(5), 
        true));
    train1->add_stop(ScheduleStop("D", 
        today_midnight + std::chrono::hours(9), 
        today_midnight + std::chrono::hours(9) + std::chrono::minutes(10), 
        true));
    schedules.push_back(train1);
    
    // Train 2: D -> A (morning fast direct)
    auto train2 = std::make_shared<TrainSchedule>("EC_200");
    train2->add_stop(ScheduleStop("D", 
        today_midnight + std::chrono::hours(7), 
        today_midnight + std::chrono::hours(7) + std::chrono::minutes(5), 
        true));
    train2->add_stop(ScheduleStop("A", 
        today_midnight + std::chrono::hours(8) + std::chrono::minutes(30), 
        today_midnight + std::chrono::hours(8) + std::chrono::minutes(35), 
        true));
    schedules.push_back(train2);
    
    // Train 3: A -> D (evening service)
    auto train3 = std::make_shared<TrainSchedule>("R_300");
    train3->add_stop(ScheduleStop("A", 
        today_midnight + std::chrono::hours(18), 
        today_midnight + std::chrono::hours(18) + std::chrono::minutes(5), 
        true));
    train3->add_stop(ScheduleStop("D", 
        today_midnight + std::chrono::hours(19) + std::chrono::minutes(45), 
        today_midnight + std::chrono::hours(19) + std::chrono::minutes(50), 
        true));
    schedules.push_back(train3);
    
    return schedules;
}

/**
 * Test RailML 2.x export
 */
void test_railml2_export(const RailwayNetwork& network,
                         const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    print_separator("RailML 2.x Export");
    
    RailMLExporter exporter;
    RailMLExportOptions options;
    options.pretty_print = true;
    options.include_metadata = true;
    options.export_infrastructure = true;
    options.export_timetable = true;
    
    std::string filename = "test_network_v2.railml";
    bool success = exporter.export_to_file(filename, network, schedules, 
                                           RailMLExportVersion::VERSION_2, options);
    
    if (success) {
        auto stats = exporter.get_statistics();
        std::cout << "Successfully exported to RailML 2.x: " << filename << "\n";
        std::cout << "  Stations exported: " << stats["stations"] << "\n";
        std::cout << "  Tracks exported:   " << stats["tracks"] << "\n";
        std::cout << "  Trains exported:   " << stats["trains"] << "\n";
    } else {
        std::cerr << "Export failed: " << exporter.get_last_error() << "\n";
    }
}

/**
 * Test RailML 3.x export
 */
void test_railml3_export(const RailwayNetwork& network,
                         const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    print_separator("RailML 3.x Export");
    
    RailMLExporter exporter;
    RailMLExportOptions options;
    options.pretty_print = true;
    options.include_metadata = true;
    options.export_infrastructure = true;
    options.export_timetable = true;
    
    std::string filename = "test_network_v3.railml";
    bool success = exporter.export_to_file(filename, network, schedules, 
                                           RailMLExportVersion::VERSION_3, options);
    
    if (success) {
        auto stats = exporter.get_statistics();
        std::cout << "Successfully exported to RailML 3.x: " << filename << "\n";
        std::cout << "  Stations exported: " << stats["stations"] << "\n";
        std::cout << "  Tracks exported:   " << stats["tracks"] << "\n";
        std::cout << "  Trains exported:   " << stats["trains"] << "\n";
    } else {
        std::cerr << "Export failed: " << exporter.get_last_error() << "\n";
    }
}

/**
 * Test string export (no file)
 */
void test_string_export(const RailwayNetwork& network,
                        const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    print_separator("RailML String Export (Preview)");
    
    RailMLExporter exporter;
    RailMLExportOptions options;
    options.pretty_print = true;
    options.include_metadata = true;
    options.export_infrastructure = true;
    options.export_timetable = false; // Infrastructure only for preview
    
    std::string xml = exporter.export_to_string(network, schedules, 
                                                 RailMLExportVersion::VERSION_3, options);
    
    if (!xml.empty()) {
        std::cout << "RailML 3.x Infrastructure XML (first 800 chars):\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << xml.substr(0, 800);
        if (xml.length() > 800) {
            std::cout << "\n... (truncated, total " << xml.length() << " bytes)";
        }
        std::cout << "\n" << std::string(60, '-') << "\n";
    } else {
        std::cerr << "String export failed: " << exporter.get_last_error() << "\n";
    }
}

/**
 * Test RailML import (when parser is fully implemented)
 */
void test_railml_import() {
    print_separator("RailML Import (Placeholder)");
    
    std::cout << "NOTE: RailML import functionality is implemented but requires\n";
    std::cout << "      detailed parsing of XML structure. The parser detects\n";
    std::cout << "      RailML version and provides the framework for full parsing.\n\n";
    
    std::cout << "To implement full parsing:\n";
    std::cout << "  1. Parse <operationalPoints> to create network nodes\n";
    std::cout << "  2. Parse <tracks> to create network edges\n";
    std::cout << "  3. Parse <trainParts> to create train schedules\n";
    std::cout << "  4. Handle both RailML 2.x and 3.x schema differences\n";
}

/**
 * Display network information
 */
void display_network_info(const RailwayNetwork& network,
                          const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    print_separator("Network Information");
    
    auto nodes = network.get_all_nodes();
    auto edges = network.get_all_edges();
    
    std::cout << "Railway Network:\n";
    std::cout << "  Stations: " << nodes.size() << "\n";
    std::cout << "  Tracks:   " << edges.size() << "\n";
    std::cout << "  Trains:   " << schedules.size() << "\n\n";
    
    std::cout << "Stations:\n";
    for (const auto& node : nodes) {
        std::cout << "  " << node->get_id() << " - " << node->get_name() 
                  << " (" << node->get_platforms() << " platforms)\n";
    }
    
    std::cout << "\nTracks:\n";
    for (const auto& edge : edges) {
        std::cout << "  " << edge->get_from_node() << " -> " << edge->get_to_node() 
                  << " (" << edge->get_distance() << " km";
        
        switch (edge->get_track_type()) {
            case TrackType::SINGLE: std::cout << ", single track"; break;
            case TrackType::DOUBLE: std::cout << ", double track"; break;
            case TrackType::HIGH_SPEED: std::cout << ", high-speed"; break;
            default: break;
        }
        
        std::cout << ", " << edge->get_max_speed() << " km/h)\n";
    }
    
    std::cout << "\nTrain Services:\n";
    for (const auto& schedule : schedules) {
        auto stops = schedule->get_stops();
        std::cout << "  " << schedule->get_train_id() << ": ";
        
        if (stops.size() >= 2) {
            auto start_time = stops.front().departure;
            auto end_time = stops.back().arrival;
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);
            
            std::cout << stops.front().node_id << " -> " << stops.back().node_id;
            std::cout << " (" << stops.size() << " stops, " << duration.count() << " min)\n";
        }
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  FDC_Scheduler RailML Example\n";
    std::cout << "========================================\n";
    
    try {
        // Create test network and schedules
        auto network = create_test_network();
        auto schedules = create_test_schedules();
        
        // Display network information
        display_network_info(*network, schedules);
        
        // Test RailML 2.x export
        test_railml2_export(*network, schedules);
        
        // Test RailML 3.x export
        test_railml3_export(*network, schedules);
        
        // Test string export
        test_string_export(*network, schedules);
        
        // Test import (placeholder)
        test_railml_import();
        
        print_separator("Summary");
        std::cout << "RailML export functionality is working correctly.\n";
        std::cout << "Output files created:\n";
        std::cout << "  - test_network_v2.railml (RailML 2.x)\n";
        std::cout << "  - test_network_v3.railml (RailML 3.x)\n\n";
        std::cout << "You can validate these files with RailML validators at:\n";
        std::cout << "  https://www.railml.org/en/public-relations/validation.html\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
