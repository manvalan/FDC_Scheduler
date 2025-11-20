#include "fdc_scheduler/realtime_optimizer.hpp"
#include <algorithm>
#include <cmath>

namespace fdc_scheduler {

RealTimeOptimizer::RealTimeOptimizer(
    RailwayNetwork& network,
    const RealTimeConfig& config
) : network_(network), config_(config) {
    stats_ = {
        .total_updates = 0,
        .conflicts_predicted = 0,
        .conflicts_avoided = 0,
        .adjustments_applied = 0,
        .avg_delay_reduction = 0.0,
        .avg_prediction_accuracy = 0.0,
        .last_update = std::chrono::system_clock::now()
    };
}

void RealTimeOptimizer::update_train_position(const TrainPosition& position) {
    positions_[position.train_id] = position;
    
    // Call callback if registered
    if (position_callback_) {
        position_callback_(position);
    }
    
    stats_.total_updates++;
    stats_.last_update = std::chrono::system_clock::now();
}

void RealTimeOptimizer::update_train_positions(const std::vector<TrainPosition>& positions) {
    for (const auto& pos : positions) {
        update_train_position(pos);
    }
}

void RealTimeOptimizer::report_delay(const TrainDelay& delay) {
    delays_[delay.train_id] = delay;
}

std::optional<TrainPosition> RealTimeOptimizer::get_train_position(const std::string& train_id) const {
    auto it = positions_.find(train_id);
    if (it != positions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PredictedConflict> RealTimeOptimizer::predict_conflicts() {
    std::vector<PredictedConflict> conflicts;
    auto now = std::chrono::system_clock::now();
    
    // Get all train pairs
    std::vector<std::string> train_ids;
    for (const auto& [id, pos] : positions_) {
        train_ids.push_back(id);
    }
    
    // Check each pair for potential conflicts
    for (size_t i = 0; i < train_ids.size(); ++i) {
        for (size_t j = i + 1; j < train_ids.size(); ++j) {
            const auto& pos1 = positions_.at(train_ids[i]);
            const auto& pos2 = positions_.at(train_ids[j]);
            
            // Predict positions over time horizon
            double horizon_seconds = config_.prediction_horizon_minutes * 60.0;
            int num_checks = static_cast<int>(horizon_seconds / 10.0); // Check every 10 seconds
            
            for (int step = 1; step <= num_checks; ++step) {
                auto check_time = now + std::chrono::seconds(step * 10);
                
                if (will_conflict(pos1, pos2, check_time)) {
                    double confidence = calculate_conflict_confidence(pos1, pos2);
                    
                    if (confidence >= config_.conflict_detection_threshold) {
                        PredictedConflict conflict;
                        conflict.train1_id = train_ids[i];
                        conflict.train2_id = train_ids[j];
                        conflict.conflict_node = pos1.next_node; // Simplified
                        conflict.predicted_time = check_time;
                        conflict.confidence = confidence;
                        conflict.type = ConflictType::HEAD_ON_COLLISION;
                        conflict.train1_position = pos1;
                        conflict.train2_position = pos2;
                        
                        conflicts.push_back(conflict);
                        
                        // Call callback if registered
                        if (conflict_callback_) {
                            conflict_callback_(conflict);
                        }
                        
                        break; // Only report first conflict for this pair
                    }
                }
            }
        }
    }
    
    active_predictions_ = conflicts;
    stats_.conflicts_predicted += conflicts.size();
    
    return conflicts;
}

std::vector<ScheduleAdjustment> RealTimeOptimizer::generate_adjustments(
    const std::vector<PredictedConflict>& conflicts
) {
    std::vector<ScheduleAdjustment> adjustments;
    
    for (const auto& conflict : conflicts) {
        // Try different adjustment strategies
        std::vector<ScheduleAdjustment> candidates;
        
        // Speed adjustment (always available if enabled)
        if (config_.enable_speed_adjustments) {
            candidates.push_back(generate_speed_adjustment(conflict.train1_id, conflict));
            candidates.push_back(generate_speed_adjustment(conflict.train2_id, conflict));
        }
        
        // Hold at station
        candidates.push_back(generate_hold_adjustment(conflict.train1_id, conflict));
        candidates.push_back(generate_hold_adjustment(conflict.train2_id, conflict));
        
        // Route change (if enabled)
        if (config_.enable_route_changes) {
            candidates.push_back(generate_route_adjustment(conflict.train1_id, conflict));
            candidates.push_back(generate_route_adjustment(conflict.train2_id, conflict));
        }
        
        // Select best adjustment (highest delay reduction with acceptable confidence)
        std::sort(candidates.begin(), candidates.end(),
            [](const ScheduleAdjustment& a, const ScheduleAdjustment& b) {
                return a.estimated_delay_reduction > b.estimated_delay_reduction;
            });
        
        if (!candidates.empty() && candidates[0].confidence > 0.5) {
            adjustments.push_back(candidates[0]);
            
            // Call callback if registered
            if (adjustment_callback_) {
                adjustment_callback_(candidates[0]);
            }
        }
        
        // Limit adjustments per cycle
        if (adjustments.size() >= static_cast<size_t>(config_.max_adjustments_per_cycle)) {
            break;
        }
    }
    
    return adjustments;
}

std::vector<ScheduleAdjustment> RealTimeOptimizer::optimize() {
    // Predict conflicts
    auto conflicts = predict_conflicts();
    
    // Generate adjustments
    auto adjustments = generate_adjustments(conflicts);
    
    return adjustments;
}

bool RealTimeOptimizer::apply_adjustment(const ScheduleAdjustment& adjustment) {
    // In a real implementation, this would modify the actual schedule
    // For now, we just track statistics
    
    stats_.adjustments_applied++;
    
    // Update average delay reduction
    double total_reduction = stats_.avg_delay_reduction * (stats_.adjustments_applied - 1);
    total_reduction += adjustment.estimated_delay_reduction;
    stats_.avg_delay_reduction = total_reduction / stats_.adjustments_applied;
    
    return true;
}

std::optional<std::chrono::system_clock::time_point> RealTimeOptimizer::estimate_arrival_time(
    const std::string& train_id,
    const Node* target_node
) const {
    auto pos_it = positions_.find(train_id);
    if (pos_it == positions_.end()) {
        return std::nullopt;
    }
    
    const auto& pos = pos_it->second;
    
    // Simplified: estimate based on current speed and distance
    // In a real implementation, would use network pathfinding
    double distance_km = 50.0; // Placeholder
    double speed_kmh = pos.current_speed > 0 ? pos.current_speed : 80.0;
    double time_hours = distance_km / speed_kmh;
    
    auto arrival = pos.timestamp + std::chrono::seconds(static_cast<int>(time_hours * 3600));
    return arrival;
}

std::optional<double> RealTimeOptimizer::calculate_current_delay(const std::string& train_id) const {
    auto delay_it = delays_.find(train_id);
    if (delay_it != delays_.end()) {
        return delay_it->second.delay_minutes;
    }
    return std::nullopt;
}

// Helper methods

double RealTimeOptimizer::predict_position_at_time(
    const TrainPosition& current,
    std::chrono::system_clock::time_point future_time
) const {
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(
        future_time - current.timestamp
    ).count();
    
    // Simple linear prediction based on current speed
    double hours = time_diff / 3600.0;
    double distance_traveled = current.current_speed * hours; // km
    
    return current.progress + (distance_traveled / 50.0); // Normalize to 0-1 range
}

bool RealTimeOptimizer::will_conflict(
    const TrainPosition& pos1,
    const TrainPosition& pos2,
    std::chrono::system_clock::time_point check_time
) const {
    // Check if trains will be at same node around the same time
    if (pos1.next_node != pos2.next_node) {
        return false;
    }
    
    // Predict positions
    double pred1 = predict_position_at_time(pos1, check_time);
    double pred2 = predict_position_at_time(pos2, check_time);
    
    // Check if positions overlap (within threshold)
    double distance = std::abs(pred1 - pred2);
    return distance < 0.1; // 10% threshold
}

ScheduleAdjustment RealTimeOptimizer::generate_speed_adjustment(
    const std::string& train_id,
    const PredictedConflict& conflict
) {
    ScheduleAdjustment adj;
    adj.train_id = train_id;
    adj.type = ScheduleAdjustment::Type::SPEED_CHANGE;
    
    auto pos_it = positions_.find(train_id);
    if (pos_it != positions_.end()) {
        // Reduce speed by 10-20% to avoid conflict
        double current_speed = pos_it->second.current_speed;
        adj.new_speed = current_speed * 0.85; // 15% reduction
        adj.estimated_delay_reduction = 3.0; // Estimated 3 minutes
        adj.confidence = 0.75;
        adj.justification = "Reduce speed to avoid predicted conflict at " + 
                           conflict.conflict_node->get_id();
    } else {
        adj.confidence = 0.0;
    }
    
    return adj;
}

ScheduleAdjustment RealTimeOptimizer::generate_hold_adjustment(
    const std::string& train_id,
    const PredictedConflict& conflict
) {
    ScheduleAdjustment adj;
    adj.train_id = train_id;
    adj.type = ScheduleAdjustment::Type::HOLD_AT_STATION;
    
    // Hold for 5 minutes to let other train pass
    adj.hold_duration_minutes = 5.0;
    adj.estimated_delay_reduction = 2.0;
    adj.confidence = 0.80;
    adj.justification = "Hold at station to avoid conflict with " + 
                       (train_id == conflict.train1_id ? conflict.train2_id : conflict.train1_id);
    
    return adj;
}

ScheduleAdjustment RealTimeOptimizer::generate_route_adjustment(
    const std::string& train_id,
    const PredictedConflict& conflict
) {
    ScheduleAdjustment adj;
    adj.train_id = train_id;
    adj.type = ScheduleAdjustment::Type::ROUTE_CHANGE;
    
    // In a real implementation, would use RouteOptimizer to find alternative
    // For now, just indicate route change is possible
    adj.estimated_delay_reduction = 5.0;
    adj.confidence = 0.65;
    adj.justification = "Alternative route available to avoid conflict";
    
    return adj;
}

double RealTimeOptimizer::calculate_conflict_confidence(
    const TrainPosition& pos1,
    const TrainPosition& pos2
) const {
    // Confidence based on:
    // - Position accuracy
    // - Speed consistency
    // - Time to predicted conflict
    
    double base_confidence = 0.7;
    
    // Reduce confidence if trains are far apart
    if (pos1.current_node != pos2.current_node && 
        pos1.next_node != pos2.next_node) {
        base_confidence *= 0.8;
    }
    
    // Increase confidence if speeds are known
    if (pos1.current_speed > 0 && pos2.current_speed > 0) {
        base_confidence *= 1.1;
    }
    
    return std::min(base_confidence, 1.0);
}

} // namespace fdc_scheduler
