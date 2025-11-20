#include "fdc_scheduler/conflict_detector.hpp"
#include <algorithm>
#include <cmath>

namespace fdc_scheduler {

ConflictDetector::ConflictDetector(const RailwayNetwork& network)
    : network_(network) {}

std::vector<Conflict> ConflictDetector::detect_all(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    std::vector<Conflict> all_conflicts;
    
    if (config_.detect_section_conflicts) {
        auto section_conflicts = detect_section_conflicts(schedules);
        all_conflicts.insert(all_conflicts.end(), 
                           section_conflicts.begin(), 
                           section_conflicts.end());
    }
    
    if (config_.detect_platform_conflicts) {
        auto platform_conflicts = detect_platform_conflicts(schedules);
        all_conflicts.insert(all_conflicts.end(),
                           platform_conflicts.begin(),
                           platform_conflicts.end());
    }
    
    if (config_.detect_head_on_collisions) {
        auto head_on_conflicts = detect_head_on_collisions(schedules);
        all_conflicts.insert(all_conflicts.end(),
                           head_on_conflicts.begin(),
                           head_on_conflicts.end());
    }
    
    if (config_.detect_timing_violations) {
        auto timing_conflicts = validate_timing(schedules);
        all_conflicts.insert(all_conflicts.end(),
                           timing_conflicts.begin(),
                           timing_conflicts.end());
    }
    
    // Update statistics
    for (const auto& conflict : all_conflicts) {
        switch (conflict.type) {
            case ConflictType::SECTION_OVERLAP:
                break;
            case ConflictType::PLATFORM_CONFLICT:
                break;
            case ConflictType::HEAD_ON_COLLISION:
                break;
            case ConflictType::TIMING_VIOLATION:
                break;
        }
    }
    
    return all_conflicts;
}

std::vector<Conflict> ConflictDetector::detect_for_train(
    const std::shared_ptr<TrainSchedule>& train,
    const std::vector<std::shared_ptr<TrainSchedule>>& all_schedules) {
    
    std::vector<Conflict> conflicts;
    
    // Check against all other trains
    for (const auto& other : all_schedules) {
        if (other->get_train_id() == train->get_train_id()) {
            continue;
        }
        
        // Section conflicts
        if (config_.detect_section_conflicts) {
            auto section_conflicts = check_section_conflicts(train, other);
            conflicts.insert(conflicts.end(),
                           section_conflicts.begin(),
                           section_conflicts.end());
        }
        
        // Platform conflicts
        if (config_.detect_platform_conflicts) {
            auto platform_conflicts = check_platform_conflicts(train, other);
            conflicts.insert(conflicts.end(),
                           platform_conflicts.begin(),
                           platform_conflicts.end());
        }
        
        // Head-on collisions
        if (config_.detect_head_on_collisions) {
            auto head_on = check_head_on_collision(train, other);
            conflicts.insert(conflicts.end(), head_on.begin(), head_on.end());
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::detect_section_conflicts(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    std::vector<Conflict> conflicts;
    
    // Check each pair of trains
    for (size_t i = 0; i < schedules.size(); ++i) {
        for (size_t j = i + 1; j < schedules.size(); ++j) {
            auto section_conflicts = check_section_conflicts(schedules[i], schedules[j]);
            conflicts.insert(conflicts.end(),
                           section_conflicts.begin(),
                           section_conflicts.end());
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::check_section_conflicts(
    const std::shared_ptr<TrainSchedule>& train1,
    const std::shared_ptr<TrainSchedule>& train2) {
    
    std::vector<Conflict> conflicts;
    
    const auto& stops1 = train1->get_stops();
    const auto& stops2 = train2->get_stops();
    
    // Check each segment of train1 against each segment of train2
    for (size_t i = 0; i + 1 < stops1.size(); ++i) {
        const auto& from1 = stops1[i];
        const auto& to1 = stops1[i + 1];
        
        for (size_t j = 0; j + 1 < stops2.size(); ++j) {
            const auto& from2 = stops2[j];
            const auto& to2 = stops2[j + 1];
            
            // Check if same section
            if ((from1.get_node_id() == from2.get_node_id() && to1.get_node_id() == to2.get_node_id()) ||
                (from1.get_node_id() == to2.get_node_id() && to1.get_node_id() == from2.get_node_id())) {
                
                // Calculate time windows with buffer
                auto start1 = from1.departure;
                auto end1 = to1.arrival;
                auto start2 = from2.departure;
                auto end2 = to2.arrival;
                
                if (time_windows_overlap(start1, end1, start2, end2, 
                                        config_.section_buffer_seconds)) {
                    Conflict conflict;
                    conflict.type = ConflictType::SECTION_OVERLAP;
                    conflict.train1_id = train1->get_train_id();
                    conflict.train2_id = train2->get_train_id();
                    conflict.location = from1.get_node_id() + " → " + to1.get_node_id();
                    conflict.section_from = from1.get_node_id();
                    conflict.section_to = to1.get_node_id();
                    conflict.conflict_time = std::max(start1, start2);
                    conflict.description = "Trains on same section with insufficient buffer time";
                    
                    // Calculate severity based on overlap
                    auto overlap_start = std::max(start1, start2);
                    auto overlap_end = std::min(end1, end2);
                    auto overlap_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                        overlap_end - overlap_start).count();
                    conflict.severity = std::min(10, static_cast<int>(overlap_seconds / 60) + 1);
                    
                    conflicts.push_back(conflict);
                }
            }
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::detect_platform_conflicts(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    std::vector<Conflict> conflicts;
    
    // Check each pair of trains
    for (size_t i = 0; i < schedules.size(); ++i) {
        for (size_t j = i + 1; j < schedules.size(); ++j) {
            auto platform_conflicts = check_platform_conflicts(schedules[i], schedules[j]);
            conflicts.insert(conflicts.end(),
                           platform_conflicts.begin(),
                           platform_conflicts.end());
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::check_platform_conflicts(
    const std::shared_ptr<TrainSchedule>& train1,
    const std::shared_ptr<TrainSchedule>& train2) {
    
    std::vector<Conflict> conflicts;
    
    const auto& stops1 = train1->get_stops();
    const auto& stops2 = train2->get_stops();
    
    // Check each stop of train1 against each stop of train2
    for (const auto& stop1 : stops1) {
        for (const auto& stop2 : stops2) {
            // Same station and same platform
            if (stop1.get_node_id() == stop2.get_node_id() && 
                stop1.platform == stop2.platform &&
                stop1.platform > 0) {
                
                // Calculate occupation windows (arrival to departure + buffer)
                auto start1 = stop1.arrival;
                auto end1 = stop1.departure;
                auto start2 = stop2.arrival;
                auto end2 = stop2.departure;
                
                if (time_windows_overlap(start1, end1, start2, end2,
                                        config_.platform_buffer_seconds)) {
                    Conflict conflict;
                    conflict.type = ConflictType::PLATFORM_CONFLICT;
                    conflict.train1_id = train1->get_train_id();
                    conflict.train2_id = train2->get_train_id();
                    conflict.location = stop1.get_node_id();
                    conflict.platform = stop1.platform;
                    conflict.conflict_time = std::max(start1, start2);
                    conflict.description = "Platform conflict at " + stop1.get_node_id() + 
                                         ", platform " + std::to_string(stop1.platform);
                    
                    // Calculate severity based on overlap
                    auto overlap_start = std::max(start1, start2);
                    auto overlap_end = std::min(end1, end2);
                    auto overlap_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                        overlap_end - overlap_start).count();
                    conflict.severity = std::min(10, static_cast<int>(overlap_seconds / 60) + 1);
                    
                    conflicts.push_back(conflict);
                }
            }
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::detect_head_on_collisions(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    std::vector<Conflict> conflicts;
    
    // Check each pair of trains
    for (size_t i = 0; i < schedules.size(); ++i) {
        for (size_t j = i + 1; j < schedules.size(); ++j) {
            auto head_on = check_head_on_collision(schedules[i], schedules[j]);
            conflicts.insert(conflicts.end(), head_on.begin(), head_on.end());
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::check_head_on_collision(
    const std::shared_ptr<TrainSchedule>& train1,
    const std::shared_ptr<TrainSchedule>& train2) {
    
    std::vector<Conflict> conflicts;
    
    const auto& stops1 = train1->get_stops();
    const auto& stops2 = train2->get_stops();
    
    // Check for opposite directions on same section
    for (size_t i = 0; i + 1 < stops1.size(); ++i) {
        const auto& from1 = stops1[i];
        const auto& to1 = stops1[i + 1];
        
        for (size_t j = 0; j + 1 < stops2.size(); ++j) {
            const auto& from2 = stops2[j];
            const auto& to2 = stops2[j + 1];
            
            // Check if opposite directions on same section
            if (from1.get_node_id() == to2.get_node_id() && to1.get_node_id() == from2.get_node_id()) {
                
                // Get edge to check if single track
                auto edge = network_.get_edge(from1.get_node_id(), to1.get_node_id());
                if (edge && edge->get_track_type() == TrackType::SINGLE) {
                    
                    auto start1 = from1.departure;
                    auto end1 = to1.arrival;
                    auto start2 = from2.departure;
                    auto end2 = to2.arrival;
                    
                    if (time_windows_overlap(start1, end1, start2, end2,
                                            config_.head_on_buffer_seconds)) {
                        Conflict conflict;
                        conflict.type = ConflictType::HEAD_ON_COLLISION;
                        conflict.train1_id = train1->get_train_id();
                        conflict.train2_id = train2->get_train_id();
                        conflict.location = from1.get_node_id() + " ↔ " + to1.get_node_id();
                        conflict.section_from = from1.get_node_id();
                        conflict.section_to = to1.get_node_id();
                        conflict.conflict_time = std::max(start1, start2);
                        conflict.description = "Head-on collision risk on single track section";
                        conflict.severity = 10; // Maximum severity
                        
                        conflicts.push_back(conflict);
                    }
                }
            }
        }
    }
    
    return conflicts;
}

std::vector<Conflict> ConflictDetector::validate_timing(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    std::vector<Conflict> violations;
    
    for (const auto& schedule : schedules) {
        const auto& stops = schedule->get_stops();
        
        for (size_t i = 0; i < stops.size(); ++i) {
            const auto& stop = stops[i];
            
            // Check minimum dwell time
            auto dwell_time = std::chrono::duration_cast<std::chrono::seconds>(
                stop.departure - stop.arrival).count();
            
            if (dwell_time < 60) { // Less than 1 minute
                Conflict violation;
                violation.type = ConflictType::TIMING_VIOLATION;
                violation.train1_id = schedule->get_train_id();
                violation.location = stop.get_node_id();
                violation.conflict_time = stop.arrival;
                violation.description = "Insufficient dwell time: " + 
                                      std::to_string(dwell_time) + " seconds";
                violation.severity = 5;
                
                violations.push_back(violation);
            }
            
            // Check travel time between stops
            if (i > 0) {
                const auto& prev_stop = stops[i - 1];
                auto travel_time = std::chrono::duration_cast<std::chrono::minutes>(
                    stop.arrival - prev_stop.departure).count();
                
                // Get edge distance
                auto edge = network_.get_edge(prev_stop.get_node_id(), stop.get_node_id());
                if (edge) {
                    double distance = edge->get_distance();
                    double max_speed = edge->get_max_speed();
                    
                    // Minimum travel time (assuming max speed)
                    double min_travel_minutes = (distance / max_speed) * 60.0;
                    
                    if (travel_time < min_travel_minutes * 0.8) { // Less than 80% of minimum
                        Conflict violation;
                        violation.type = ConflictType::TIMING_VIOLATION;
                        violation.train1_id = schedule->get_train_id();
                        violation.location = prev_stop.get_node_id() + " → " + stop.get_node_id();
                        violation.section_from = prev_stop.get_node_id();
                        violation.section_to = stop.get_node_id();
                        violation.conflict_time = prev_stop.departure;
                        violation.description = "Unrealistic travel time: " + 
                                              std::to_string(travel_time) + " minutes for " +
                                              std::to_string(distance) + " km";
                        violation.severity = 7;
                        
                        violations.push_back(violation);
                    }
                }
            }
        }
    }
    
    return violations;
}

bool ConflictDetector::time_windows_overlap(
    std::chrono::system_clock::time_point start1,
    std::chrono::system_clock::time_point end1,
    std::chrono::system_clock::time_point start2,
    std::chrono::system_clock::time_point end2,
    int buffer_seconds) const {
    
    // Add buffer to both windows
    start1 -= std::chrono::seconds(buffer_seconds);
    end1 += std::chrono::seconds(buffer_seconds);
    start2 -= std::chrono::seconds(buffer_seconds);
    end2 += std::chrono::seconds(buffer_seconds);
    
    // Check overlap: windows overlap if start of one is before end of other
    return !(end1 < start2 || end2 < start1);
}

int ConflictDetector::calculate_severity(const Conflict& conflict) const {
    // Calculate severity based on conflict type
    // HEAD_ON_COLLISION and TIMING_VIOLATION are most severe
    // PLATFORM_CONFLICT and SECTION_OVERLAP severity depends on buffer violation
    
    switch (conflict.type) {
        case ConflictType::HEAD_ON_COLLISION:
            return 10;  // Maximum severity
            
        case ConflictType::TIMING_VIOLATION:
            return 8;   // High severity
            
        case ConflictType::PLATFORM_CONFLICT:
            return 6;   // Medium severity
            
        case ConflictType::SECTION_OVERLAP:
            return 5;   // Medium-low severity
            
        default:
            return 3;   // Default severity
    }
}

std::map<std::string, int> ConflictDetector::get_statistics() const {
    return {
        {"total_checks", total_checks_},
        {"section_conflicts", section_conflicts_found_},
        {"platform_conflicts", platform_conflicts_found_},
        {"head_on_collisions", head_on_collisions_found_},
        {"timing_violations", timing_violations_found_}
    };
}

ConflictDetector::ConflictDetector(const RailwayNetwork& network, const ConflictDetectorConfig& config)
    : network_(network), config_(config) {}

std::vector<Conflict> ConflictDetector::detect_section_conflicts(
    const std::shared_ptr<TrainSchedule>& schedule1,
    const std::shared_ptr<TrainSchedule>& schedule2) {
    return check_section_conflicts(schedule1, schedule2);
}

std::vector<Conflict> ConflictDetector::detect_platform_conflicts(
    const std::shared_ptr<TrainSchedule>& schedule1,
    const std::shared_ptr<TrainSchedule>& schedule2) {
    return check_platform_conflicts(schedule1, schedule2);
}

std::vector<Conflict> ConflictDetector::detect_head_on_collisions(
    const std::shared_ptr<TrainSchedule>& schedule1,
    const std::shared_ptr<TrainSchedule>& schedule2) {
    return check_head_on_collision(schedule1, schedule2);
}

std::vector<Conflict> ConflictDetector::validate_timing(
    const std::shared_ptr<TrainSchedule>& schedule) {
    std::vector<std::shared_ptr<TrainSchedule>> single = {schedule};
    return validate_timing(single);
}

std::string conflict_type_to_string(ConflictType type) {
    switch (type) {
        case ConflictType::SECTION_OVERLAP:
            return "SECTION_OVERLAP";
        case ConflictType::PLATFORM_CONFLICT:
            return "PLATFORM_CONFLICT";
        case ConflictType::HEAD_ON_COLLISION:
            return "HEAD_ON_COLLISION";
        case ConflictType::TIMING_VIOLATION:
            return "TIMING_VIOLATION";
        default:
            return "UNKNOWN";
    }
}

std::string conflict_to_string(const Conflict& conflict) {
    std::ostringstream oss;
    oss << "Conflict [" << conflict_type_to_string(conflict.type) << "]\n"
        << "  Trains: " << conflict.train1_id;
    if (!conflict.train2_id.empty()) {
        oss << " <-> " << conflict.train2_id;
    }
    oss << "\n  Location: " << conflict.location
        << "\n  Severity: " << conflict.severity
        << "\n  Description: " << conflict.description;
    return oss.str();
}

} // namespace fdc_scheduler
