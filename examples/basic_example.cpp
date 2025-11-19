#include <fdc_scheduler.hpp>
#include <iostream>
#include <iomanip>

using namespace fdc_scheduler;

int main() {
    std::cout << "FDC_Scheduler Basic Example\n";
    std::cout << "Version: " << Version::to_string() << "\n\n";
    
    // Initialize library
    initialize();
    
    // ========================================================================
    // 1. CREATE RAILWAY NETWORK
    // ========================================================================
    std::cout << "=== Creating Railway Network ===\n";
    
    RailwayNetwork network;
    
    // Add stations
    network.add_station("MILANO", "Milano Centrale", 12);
    network.add_station("MONZA", "Monza", 4);
    network.add_station("COMO", "Como San Giovanni", 3);
    network.add_station("LECCO", "Lecco", 2);
    
    std::cout << "Added 4 stations\n";
    
    // Add track sections
    network.add_track_section("MILANO", "MONZA", 15.0, 140.0, TrackType::DOUBLE);
    network.add_track_section("MONZA", "COMO", 30.0, 120.0, TrackType::SINGLE);
    network.add_track_section("MONZA", "LECCO", 25.0, 100.0, TrackType::SINGLE);
    
    std::cout << "Added 3 track sections\n";
    std::cout << "Total network length: " << std::fixed << std::setprecision(1) 
              << network.get_total_length() << " km\n\n";
    
    // ========================================================================
    // 2. CREATE TRAIN SCHEDULES
    // ========================================================================
    std::cout << "=== Creating Train Schedules ===\n";
    
    std::vector<std::shared_ptr<TrainSchedule>> schedules;
    
    // Train IC101: Milano -> Como
    auto ic101 = std::make_shared<TrainSchedule>("IC101", "IC101", network);
    ic101->add_stop("MILANO", "08:00", "08:00", 1);
    ic101->add_stop("MONZA", "08:08", "08:10", 1);
    ic101->add_stop("COMO", "08:25", "08:25", 1);
    schedules.push_back(ic101);
    std::cout << "Created IC101: Milano -> Como\n";
    
    // Train R203: Milano -> Como (same route, later)
    auto r203 = std::make_shared<TrainSchedule>("R203", "R203", network);
    r203->add_stop("MILANO", "08:04", "08:04", 2);
    r203->add_stop("MONZA", "08:15", "08:17", 1);
    r203->add_stop("COMO", "08:33", "08:33", 2);
    schedules.push_back(r203);
    std::cout << "Created R203: Milano -> Como\n";
    
    // Train R205: Como -> Milano (opposite direction)
    auto r205 = std::make_shared<TrainSchedule>("R205", "R205", network);
    r205->add_stop("COMO", "08:20", "08:20", 1);  // CONFLICT: same platform as IC101!
    r205->add_stop("MONZA", "08:35", "08:37", 2);
    r205->add_stop("MILANO", "08:47", "08:47", 3);
    schedules.push_back(r205);
    std::cout << "Created R205: Como -> Milano\n";
    
    // Train R301: Milano -> Lecco
    auto r301 = std::make_shared<TrainSchedule>("R301", "R301", network);
    r301->add_stop("MILANO", "08:10", "08:10", 5);
    r301->add_stop("MONZA", "08:18", "08:20", 2);
    r301->add_stop("LECCO", "08:35", "08:35", 1);
    schedules.push_back(r301);
    std::cout << "Created R301: Milano -> Lecco\n\n";
    
    // ========================================================================
    // 3. DETECT CONFLICTS
    // ========================================================================
    std::cout << "=== Detecting Conflicts ===\n";
    
    ConflictDetector detector(network);
    auto conflicts = detector.detect_all(schedules);
    
    std::cout << "Found " << conflicts.size() << " conflict(s)\n\n";
    
    if (!conflicts.empty()) {
        std::cout << "Conflict Details:\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (size_t i = 0; i < conflicts.size(); ++i) {
            const auto& conflict = conflicts[i];
            
            std::cout << (i + 1) << ". ";
            
            // Print conflict type
            switch (conflict.type) {
                case ConflictType::SECTION_OVERLAP:
                    std::cout << "[SECTION] ";
                    break;
                case ConflictType::PLATFORM_CONFLICT:
                    std::cout << "[PLATFORM] ";
                    break;
                case ConflictType::HEAD_ON_COLLISION:
                    std::cout << "[HEAD-ON] ";
                    break;
                case ConflictType::TIMING_VIOLATION:
                    std::cout << "[TIMING] ";
                    break;
            }
            
            std::cout << conflict.train1_id << " vs " << conflict.train2_id << "\n";
            std::cout << "   Location: " << conflict.location;
            
            if (conflict.type == ConflictType::PLATFORM_CONFLICT) {
                std::cout << " (Platform " << conflict.platform << ")";
            }
            
            std::cout << "\n";
            std::cout << "   Description: " << conflict.description << "\n";
            std::cout << "   Severity: " << conflict.severity << "/10\n\n";
        }
    }
    
    // ========================================================================
    // 4. STATISTICS
    // ========================================================================
    std::cout << "=== Statistics ===\n";
    auto stats = detector.get_statistics();
    for (const auto& [key, value] : stats) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << "\n";
    
    // ========================================================================
    // 5. VALIDATE SPECIFIC TRAIN
    // ========================================================================
    std::cout << "=== Validating IC101 ===\n";
    auto ic101_conflicts = detector.detect_for_train(ic101, schedules);
    std::cout << "IC101 has " << ic101_conflicts.size() << " conflict(s)\n\n";
    
    // Cleanup library
    cleanup();
    
    std::cout << "Example completed successfully!\n";
    return 0;
}
