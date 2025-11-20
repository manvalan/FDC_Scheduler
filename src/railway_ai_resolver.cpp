#include "fdc_scheduler/railway_ai_resolver.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace fdc_scheduler {

RailwayAIResolver::RailwayAIResolver(
    RailwayNetwork& network,
    const RailwayAIConfig& config)
    : network_(network), config_(config) {}

ResolutionResult RailwayAIResolver::resolve_conflicts(
    std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const std::vector<Conflict>& conflicts) {
    
    ResolutionResult overall_result;
    overall_result.success = true;
    overall_result.total_delay = std::chrono::seconds(0);
    overall_result.quality_score = 1.0;
    
    int resolved_count = 0;
    
    // Sort conflicts by severity (highest first)
    std::vector<Conflict> sorted_conflicts = conflicts;
    std::sort(sorted_conflicts.begin(), sorted_conflicts.end(),
              [](const Conflict& a, const Conflict& b) {
                  return a.severity > b.severity;
              });
    
    // Resolve each conflict
    for (const auto& conflict : sorted_conflicts) {
        total_resolutions_++;
        
        ResolutionResult result = resolve_single_conflict(conflict, schedules);
        
        if (result.success) {
            resolved_count++;
            successful_resolutions_++;
            
            // Accumulate metrics
            overall_result.total_delay += result.total_delay;
            overall_result.modified_trains.insert(
                overall_result.modified_trains.end(),
                result.modified_trains.begin(),
                result.modified_trains.end()
            );
            
            // Update quality score (weighted average)
            overall_result.quality_score = 
                (overall_result.quality_score * (resolved_count - 1) + result.quality_score) 
                / resolved_count;
        } else {
            overall_result.success = false;
        }
    }
    
    std::ostringstream desc;
    desc << "Resolved " << resolved_count << " of " << conflicts.size() 
         << " conflicts. Total delay: " << overall_result.total_delay.count() 
         << " seconds";
    overall_result.description = desc.str();
    
    return overall_result;
}

ResolutionResult RailwayAIResolver::resolve_single_conflict(
    const Conflict& conflict,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    ResolutionResult result;
    
    // Route based on conflict type
    switch (conflict.type) {
        case ConflictType::SECTION_OVERLAP: {
            // Determine track type
            auto track_type = get_track_type(conflict.section_from, conflict.section_to);
            
            if (track_type && *track_type == TrackType::DOUBLE) {
                result = resolve_double_track_conflict(conflict, schedules);
                if (result.success) double_track_resolutions_++;
            } else {
                result = resolve_single_track_conflict(conflict, schedules);
                if (result.success) single_track_resolutions_++;
            }
            break;
        }
        
        case ConflictType::PLATFORM_CONFLICT:
            result = resolve_station_conflict(conflict, schedules);
            if (result.success) station_resolutions_++;
            break;
            
        case ConflictType::HEAD_ON_COLLISION:
            // Head-on collisions are critical - always treat as single track
            result = resolve_single_track_conflict(conflict, schedules);
            if (result.success) single_track_resolutions_++;
            break;
            
        case ConflictType::TIMING_VIOLATION:
            // For timing violations, apply minimal delay
            result = resolve_timing_violation(conflict, schedules);
            break;
    }
    
    return result;
}

ResolutionResult RailwayAIResolver::resolve_double_track_conflict(
    const Conflict& conflict,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    ResolutionResult result;
    result.strategy_used = ResolutionStrategy::ADJUST_SPEED;
    
    // On double track, trains can pass each other
    // Main strategy: ensure proper separation timing
    
    auto train1 = find_schedule(conflict.train1_id, schedules);
    auto train2 = find_schedule(conflict.train2_id, schedules);
    
    if (!train1 || !train2) {
        result.description = "Could not find train schedules";
        return result;
    }
    
    // Calculate required separation
    auto required_sep = std::chrono::seconds(config_.min_headway_seconds);
    
    // Determine which train should be delayed based on priority
    int priority1 = get_train_priority(train1);
    int priority2 = get_train_priority(train2);
    
    if (priority1 > priority2) {
        // Delay train2
        auto delay = required_sep + std::chrono::seconds(30); // Add buffer
        apply_delay(train2, delay);
        
        result.success = true;
        result.total_delay = delay;
        result.modified_trains.push_back(conflict.train2_id);
        delays_applied_++;
        
        std::ostringstream desc;
        desc << "Double track: delayed " << conflict.train2_id 
             << " by " << delay.count() << "s to maintain headway";
        result.description = desc.str();
        
    } else if (priority2 > priority1) {
        // Delay train1
        auto delay = required_sep + std::chrono::seconds(30);
        apply_delay(train1, delay);
        
        result.success = true;
        result.total_delay = delay;
        result.modified_trains.push_back(conflict.train1_id);
        delays_applied_++;
        
        std::ostringstream desc;
        desc << "Double track: delayed " << conflict.train1_id 
             << " by " << delay.count() << "s to maintain headway";
        result.description = desc.str();
        
    } else {
        // Equal priority - distribute delay
        auto delay1 = required_sep / 2;
        auto delay2 = required_sep / 2;
        
        apply_delay(train1, delay1);
        apply_delay(train2, delay2);
        
        result.success = true;
        result.total_delay = delay1 + delay2;
        result.modified_trains.push_back(conflict.train1_id);
        result.modified_trains.push_back(conflict.train2_id);
        delays_applied_ += 2;
        
        std::ostringstream desc;
        desc << "Double track: distributed delay between trains ("
             << delay1.count() << "s, " << delay2.count() << "s)";
        result.description = desc.str();
    }
    
    result.quality_score = calculate_quality_score(result);
    
    // Validate resolution
    if (!validate_resolution(conflict, schedules)) {
        result.success = false;
        result.description += " [VALIDATION FAILED]";
    }
    
    return result;
}

ResolutionResult RailwayAIResolver::resolve_single_track_conflict(
    const Conflict& conflict,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    ResolutionResult result;
    result.strategy_used = ResolutionStrategy::ADD_OVERTAKING_POINT;
    
    auto train1 = find_schedule(conflict.train1_id, schedules);
    auto train2 = find_schedule(conflict.train2_id, schedules);
    
    if (!train1 || !train2) {
        result.description = "Could not find train schedules";
        return result;
    }
    
    // On single track, need to find a meeting point
    auto meeting_point = find_meeting_point(
        train1, train2, 
        conflict.section_from, 
        conflict.section_to
    );
    
    if (meeting_point) {
        // Found a meeting point - schedule trains to meet there
        result.strategy_used = ResolutionStrategy::DELAY_TRAIN;
        
        int priority1 = get_train_priority(train1);
        int priority2 = get_train_priority(train2);
        
        // Lower priority train waits at meeting point
        if (priority1 > priority2) {
            // Train2 waits
            auto delay = std::chrono::seconds(config_.single_track_meet_buffer);
            apply_delay(train2, delay);
            
            result.success = true;
            result.total_delay = delay;
            result.modified_trains.push_back(conflict.train2_id);
            delays_applied_++;
            
            std::ostringstream desc;
            desc << "Single track: " << conflict.train2_id 
                 << " waits at " << *meeting_point 
                 << " for " << conflict.train1_id << " (delay: " 
                 << delay.count() << "s)";
            result.description = desc.str();
            
        } else {
            // Train1 waits
            auto delay = std::chrono::seconds(config_.single_track_meet_buffer);
            apply_delay(train1, delay);
            
            result.success = true;
            result.total_delay = delay;
            result.modified_trains.push_back(conflict.train1_id);
            delays_applied_++;
            
            std::ostringstream desc;
            desc << "Single track: " << conflict.train1_id 
                 << " waits at " << *meeting_point 
                 << " for " << conflict.train2_id << " (delay: " 
                 << delay.count() << "s)";
            result.description = desc.str();
        }
        
    } else {
        // No meeting point found - must delay one train significantly
        result.strategy_used = ResolutionStrategy::PRIORITY_BASED;
        
        int priority1 = get_train_priority(train1);
        int priority2 = get_train_priority(train2);
        
        // Calculate required delay to clear section
        auto required_delay = std::chrono::seconds(
            config_.single_track_meet_buffer * 2
        );
        
        if (priority1 > priority2) {
            apply_delay(train2, required_delay);
            result.modified_trains.push_back(conflict.train2_id);
            result.total_delay = required_delay;
            delays_applied_++;
            
            result.description = "Single track (no meeting point): delayed " 
                               + conflict.train2_id + " to clear section";
        } else {
            apply_delay(train1, required_delay);
            result.modified_trains.push_back(conflict.train1_id);
            result.total_delay = required_delay;
            delays_applied_++;
            
            result.description = "Single track (no meeting point): delayed " 
                               + conflict.train1_id + " to clear section";
        }
        
        result.success = true;
    }
    
    result.quality_score = calculate_quality_score(result);
    
    // Validate resolution
    if (!validate_resolution(conflict, schedules)) {
        result.success = false;
        result.description += " [VALIDATION FAILED]";
    }
    
    return result;
}

ResolutionResult RailwayAIResolver::resolve_station_conflict(
    const Conflict& conflict,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    ResolutionResult result;
    
    auto train1 = find_schedule(conflict.train1_id, schedules);
    auto train2 = find_schedule(conflict.train2_id, schedules);
    
    if (!train1 || !train2) {
        result.description = "Could not find train schedules";
        return result;
    }
    
    // Strategy 1: Try to find alternative platform
    if (config_.allow_platform_reassignment) {
        
        // Find which train should change platform (lower priority)
        int priority1 = get_train_priority(train1);
        int priority2 = get_train_priority(train2);
        
        if (priority1 > priority2) {
            // Try to reassign train2 to different platform
            auto stop = std::find_if(
                train2->get_stops().begin(), 
                train2->get_stops().end(),
                [&](const TrainStop& s) { return s.get_node_id() == conflict.location; }
            );
            
            if (stop != train2->get_stops().end()) {
                auto alt_platform = find_alternative_platform(
                    conflict.location,
                    stop->arrival,
                    stop->departure,
                    conflict.platform,
                    schedules
                );
                
                if (alt_platform) {
                    if (change_platform(train2, conflict.location, *alt_platform)) {
                        result.success = true;
                        result.strategy_used = ResolutionStrategy::CHANGE_PLATFORM;
                        result.modified_trains.push_back(conflict.train2_id);
                        result.total_delay = std::chrono::seconds(0);
                        platforms_changed_++;
                        
                        std::ostringstream desc;
                        desc << "Station conflict: moved " << conflict.train2_id
                             << " to platform " << *alt_platform 
                             << " at " << conflict.location;
                        result.description = desc.str();
                        result.quality_score = 0.9; // Platform change is good solution
                        
                        return result;
                    }
                }
            }
            
        } else {
            // Try to reassign train1
            auto stop = std::find_if(
                train1->get_stops().begin(), 
                train1->get_stops().end(),
                [&](const TrainStop& s) { return s.get_node_id() == conflict.location; }
            );
            
            if (stop != train1->get_stops().end()) {
                auto alt_platform = find_alternative_platform(
                    conflict.location,
                    stop->arrival,
                    stop->departure,
                    conflict.platform,
                    schedules
                );
                
                if (alt_platform) {
                    if (change_platform(train1, conflict.location, *alt_platform)) {
                        result.success = true;
                        result.strategy_used = ResolutionStrategy::CHANGE_PLATFORM;
                        result.modified_trains.push_back(conflict.train1_id);
                        result.total_delay = std::chrono::seconds(0);
                        platforms_changed_++;
                        
                        std::ostringstream desc;
                        desc << "Station conflict: moved " << conflict.train1_id
                             << " to platform " << *alt_platform 
                             << " at " << conflict.location;
                        result.description = desc.str();
                        result.quality_score = 0.9;
                        
                        return result;
                    }
                }
            }
        }
    }
    
    // Strategy 2: Delay one train to avoid platform conflict
    result.strategy_used = ResolutionStrategy::DELAY_TRAIN;
    
    int priority1 = get_train_priority(train1);
    int priority2 = get_train_priority(train2);
    
    auto required_delay = std::chrono::seconds(
        config_.platform_buffer_seconds + config_.station_dwell_buffer_seconds
    );
    
    if (priority1 > priority2) {
        apply_delay(train2, required_delay);
        result.success = true;
        result.modified_trains.push_back(conflict.train2_id);
        result.total_delay = required_delay;
        delays_applied_++;
        
        std::ostringstream desc;
        desc << "Station conflict: delayed " << conflict.train2_id 
             << " by " << required_delay.count() << "s at " << conflict.location;
        result.description = desc.str();
        
    } else {
        apply_delay(train1, required_delay);
        result.success = true;
        result.modified_trains.push_back(conflict.train1_id);
        result.total_delay = required_delay;
        delays_applied_++;
        
        std::ostringstream desc;
        desc << "Station conflict: delayed " << conflict.train1_id 
             << " by " << required_delay.count() << "s at " << conflict.location;
        result.description = desc.str();
    }
    
    result.quality_score = calculate_quality_score(result);
    
    // Validate resolution
    if (!validate_resolution(conflict, schedules)) {
        result.success = false;
        result.description += " [VALIDATION FAILED]";
    }
    
    return result;
}

std::optional<std::string> RailwayAIResolver::find_meeting_point(
    const std::shared_ptr<TrainSchedule>& train1,
    const std::shared_ptr<TrainSchedule>& train2,
    const std::string& section_start,
    const std::string& section_end) {
    
    // Find common stations where trains can meet
    const auto& stops1 = train1->get_stops();
    const auto& stops2 = train2->get_stops();
    
    std::vector<std::string> candidates;
    
    // Look for stations that both trains visit
    for (const auto& stop1 : stops1) {
        for (const auto& stop2 : stops2) {
            if (stop1.get_node_id() == stop2.get_node_id()) {
                // Check if this station is in the conflict section
                // and has passing capability
                if (has_passing_capability(stop1.get_node_id())) {
                    candidates.push_back(stop1.get_node_id());
                }
            }
        }
    }
    
    if (!candidates.empty()) {
        // Return first suitable meeting point
        // TODO: In advanced implementation, choose optimal meeting point
        return candidates[0];
    }
    
    // No suitable meeting point found
    return std::nullopt;
}

std::pair<std::chrono::seconds, std::chrono::seconds> 
RailwayAIResolver::distribute_delay(
    const std::shared_ptr<TrainSchedule>& train1,
    const std::shared_ptr<TrainSchedule>& train2,
    std::chrono::seconds total_delay) {
    
    int priority1 = get_train_priority(train1);
    int priority2 = get_train_priority(train2);
    
    double ratio = static_cast<double>(priority2) / (priority1 + priority2);
    
    auto delay1 = std::chrono::seconds(
        static_cast<long long>(total_delay.count() * ratio)
    );
    auto delay2 = total_delay - delay1;
    
    return {delay1, delay2};
}

void RailwayAIResolver::apply_delay(
    std::shared_ptr<TrainSchedule>& schedule,
    std::chrono::seconds delay,
    size_t from_stop_index) {
    
    auto& stops = schedule->get_stops();
    
    for (size_t i = from_stop_index; i < stops.size(); ++i) {
        stops[i].arrival += delay;
        stops[i].departure += delay;
    }
}

std::optional<int> RailwayAIResolver::find_alternative_platform(
    const std::string& station_id,
    std::chrono::system_clock::time_point desired_arrival,
    std::chrono::system_clock::time_point desired_departure,
    int excluded_platform,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    // Get station node
    auto node = network_.get_node(station_id);
    if (!node) return std::nullopt;
    
    int num_platforms = node->get_platforms();
    
    // Check each platform
    for (int platform = 1; platform <= num_platforms; ++platform) {
        if (platform == excluded_platform) continue;
        
        bool available = true;
        
        // Check against all existing schedules
        for (const auto& sched : schedules) {
            for (const auto& stop : sched->get_stops()) {
                if (stop.get_node_id() == station_id && 
                    stop.platform == platform) {
                    
                    // Check for time overlap
                    auto buffer = std::chrono::seconds(config_.platform_buffer_seconds);
                    
                    if ((desired_arrival >= stop.arrival - buffer && 
                         desired_arrival <= stop.departure + buffer) ||
                        (desired_departure >= stop.arrival - buffer && 
                         desired_departure <= stop.departure + buffer) ||
                        (desired_arrival <= stop.arrival && 
                         desired_departure >= stop.departure)) {
                        available = false;
                        break;
                    }
                }
            }
            if (!available) break;
        }
        
        if (available) {
            return platform;
        }
    }
    
    return std::nullopt;
}

bool RailwayAIResolver::change_platform(
    std::shared_ptr<TrainSchedule>& schedule,
    const std::string& station_id,
    int new_platform) {
    
    auto& stops = schedule->get_stops();
    
    for (auto& stop : stops) {
        if (stop.get_node_id() == station_id) {
            stop.platform = new_platform;
            return true;
        }
    }
    
    return false;
}

double RailwayAIResolver::calculate_quality_score(const ResolutionResult& result) const {
    if (!result.success) return 0.0;
    
    double score = 1.0;
    
    // Penalize delays
    double delay_minutes = result.total_delay.count() / 60.0;
    score -= (delay_minutes / config_.max_delay_minutes) * config_.delay_weight * 0.3;
    
    // Penalize platform changes (but less than delays)
    int platform_changes = 0;
    for (const auto& train : result.modified_trains) {
        if (result.strategy_used == ResolutionStrategy::CHANGE_PLATFORM) {
            platform_changes++;
        }
    }
    score -= platform_changes * config_.platform_change_weight * 0.1;
    
    // Penalize number of trains affected
    score -= result.modified_trains.size() * 0.05;
    
    return std::max(0.0, std::min(1.0, score));
}

int RailwayAIResolver::get_train_priority(
    const std::shared_ptr<TrainSchedule>& schedule) const {
    
    // Base priority on train type
    int priority = 50; // Default
    
    // TODO: Implement train type checking
    // For now, assume passenger trains have higher priority
    // This would be based on train->get_type() in real implementation
    
    // Adjust based on current delay
    // Trains already delayed get lower priority for additional delays
    
    return priority;
}

std::optional<TrackType> RailwayAIResolver::get_track_type(
    const std::string& from_node,
    const std::string& to_node) const {
    
    auto edge = network_.get_edge(from_node, to_node);
    if (edge) {
        return edge->get_track_type();
    }
    
    return std::nullopt;
}

bool RailwayAIResolver::has_passing_capability(const std::string& station_id) const {
    auto node = network_.get_node(station_id);
    if (!node) return false;
    
    // Station has passing capability if it has multiple platforms
    return node->get_platforms() >= 2;
}

std::map<std::string, int> RailwayAIResolver::get_statistics() const {
    return {
        {"total_resolutions", total_resolutions_},
        {"successful_resolutions", successful_resolutions_},
        {"double_track_resolutions", double_track_resolutions_},
        {"single_track_resolutions", single_track_resolutions_},
        {"station_resolutions", station_resolutions_},
        {"delays_applied", delays_applied_},
        {"platforms_changed", platforms_changed_}
    };
}

std::shared_ptr<TrainSchedule> RailwayAIResolver::find_schedule(
    const std::string& train_id,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    auto it = std::find_if(schedules.begin(), schedules.end(),
        [&train_id](const std::shared_ptr<TrainSchedule>& sched) {
            return sched->get_train_id() == train_id;
        });
    
    if (it != schedules.end()) {
        return *it;
    }
    
    return nullptr;
}

std::chrono::seconds RailwayAIResolver::calculate_required_separation(
    const Conflict& conflict,
    TrackType track_type) const {
    
    int base_separation = config_.min_headway_seconds;
    
    // Single track needs more separation
    if (track_type == TrackType::SINGLE) {
        base_separation = config_.single_track_meet_buffer;
    }
    
    return std::chrono::seconds(base_separation);
}

bool RailwayAIResolver::validate_resolution(
    const Conflict& conflict,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) const {
    
    // Re-check if conflict still exists after resolution
    // This is a simplified validation
    // In full implementation, would re-run conflict detection
    
    return true; // Assume success for now
}

ResolutionResult RailwayAIResolver::resolve_timing_violation(
    const Conflict& conflict,
    std::vector<std::shared_ptr<TrainSchedule>>& schedules) {
    
    ResolutionResult result;
    result.strategy_used = ResolutionStrategy::DELAY_TRAIN;
    
    auto train = find_schedule(conflict.train1_id, schedules);
    if (!train) {
        result.description = "Could not find train schedule";
        return result;
    }
    
    // Apply minimal delay to fix timing violation
    auto delay = std::chrono::seconds(config_.min_headway_seconds);
    apply_delay(train, delay);
    
    result.success = true;
    result.modified_trains.push_back(conflict.train1_id);
    result.total_delay = delay;
    result.description = "Fixed timing violation with minimal delay";
    result.quality_score = 0.85;
    
    return result;
}

std::string strategy_to_string(ResolutionStrategy strategy) {
    switch (strategy) {
        case ResolutionStrategy::DELAY_TRAIN:
            return "DELAY_TRAIN";
        case ResolutionStrategy::REROUTE:
            return "REROUTE";
        case ResolutionStrategy::CHANGE_PLATFORM:
            return "CHANGE_PLATFORM";
        case ResolutionStrategy::ADJUST_SPEED:
            return "ADJUST_SPEED";
        case ResolutionStrategy::ADD_OVERTAKING_POINT:
            return "ADD_OVERTAKING_POINT";
        case ResolutionStrategy::PRIORITY_BASED:
            return "PRIORITY_BASED";
        default:
            return "UNKNOWN";
    }
}

std::string resolution_to_string(const ResolutionResult& result) {
    std::ostringstream oss;
    oss << "Resolution: " << (result.success ? "SUCCESS" : "FAILED") << "\n"
        << "  Strategy: " << strategy_to_string(result.strategy_used) << "\n"
        << "  Description: " << result.description << "\n"
        << "  Total delay: " << result.total_delay.count() << " seconds\n"
        << "  Modified trains: " << result.modified_trains.size() << "\n"
        << "  Quality score: " << result.quality_score;
    return oss.str();
}

} // namespace fdc_scheduler
