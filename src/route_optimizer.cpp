#include "fdc_scheduler/route_optimizer.hpp"
#include "fdc_scheduler/profiler.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace fdc_scheduler {

std::vector<AlternativeRoute> RouteOptimizer::find_alternatives(
    const std::string& start_node,
    const std::string& end_node,
    const std::vector<std::string>& exclude_edges) {
    
    auto& profiler = Profiler::instance();
    auto timer = profiler.start("find_alternative_routes");
    
    last_stats_ = OptimizationStats();
    std::vector<AlternativeRoute> alternatives;
    
    // Get base path for comparison
    auto base_path = network_.find_shortest_path(start_node, end_node);
    if (!base_path.is_valid()) {
        profiler.stop(timer);
        return alternatives;
    }
    
    // Find k-shortest paths
    auto k_paths = network_.find_k_shortest_paths(
        start_node, end_node, config_.max_alternatives + 1);
    
    last_stats_.alternatives_considered = k_paths.size();
    
    // Evaluate each alternative
    for (const auto& path : k_paths) {
        // Skip if it's the base path
        if (path.nodes == base_path.nodes) {
            continue;
        }
        
        // Skip if uses excluded edges
        bool uses_excluded = false;
        for (const auto& edge : path.edges) {
            if (std::find(exclude_edges.begin(), exclude_edges.end(), edge) 
                != exclude_edges.end()) {
                uses_excluded = true;
                break;
            }
        }
        if (uses_excluded) continue;
        
        // Check constraints
        if (!meets_constraints(path, base_path)) {
            continue;
        }
        
        // Create alternative
        AlternativeRoute alt;
        alt.path = path;
        alt.quality = evaluate_route(path, base_path);
        alt.description = generate_description(alt);
        
        if (alt.quality.overall_score > 0.0) {
            alternatives.push_back(alt);
            last_stats_.valid_alternatives++;
        }
    }
    
    // Sort by quality
    std::sort(alternatives.begin(), alternatives.end());
    
    // Keep only top N
    if (alternatives.size() > config_.max_alternatives) {
        alternatives.resize(config_.max_alternatives);
    }
    
    if (!alternatives.empty()) {
        last_stats_.best_score = alternatives[0].quality.overall_score;
    }
    
    last_stats_.computation_time_ms = profiler.stop(timer);
    
    return alternatives;
}

std::optional<AlternativeRoute> RouteOptimizer::find_best_reroute(
    const TrainSchedule& schedule,
    const std::vector<Conflict>& conflicts) {
    
    const auto& stops = schedule.get_stops();
    if (stops.size() < 2) {
        return std::nullopt;
    }
    
    // Find alternatives for the entire route
    std::string start = stops.front().node_id;
    std::string end = stops.back().node_id;
    
    auto alternatives = find_alternatives(start, end);
    
    if (alternatives.empty()) {
        return std::nullopt;
    }
    
    // Return best alternative
    return alternatives[0];
}

bool RouteOptimizer::apply_reroute(
    TrainSchedule& schedule,
    const AlternativeRoute& route) {
    
    if (!route.path.is_valid()) {
        return false;
    }
    
    // Clear existing stops except first and last
    auto& stops = schedule.get_stops();
    if (stops.size() < 2) {
        return false;
    }
    
    auto first_stop = stops.front();
    auto last_stop = stops.back();
    
    // Clear intermediate stops
    stops.clear();
    stops.push_back(first_stop);
    
    // Add new intermediate stops based on route
    auto departure_time = first_stop.departure;
    
    for (size_t i = 1; i < route.path.nodes.size() - 1; ++i) {
        const auto& node_id = route.path.nodes[i];
        
        // Estimate arrival time (simplified)
        auto travel_minutes = static_cast<int>(route.path.total_distance * 60.0 / 150.0);
        auto arrival = departure_time + std::chrono::minutes(travel_minutes / route.path.nodes.size());
        auto dept = arrival + std::chrono::minutes(2);  // 2 min dwell time
        
        ScheduleStop stop(node_id, arrival, dept, true);
        stops.push_back(stop);
        
        departure_time = dept;
    }
    
    // Update last stop
    last_stop.arrival = departure_time + std::chrono::minutes(30);
    last_stop.departure = last_stop.arrival;
    stops.push_back(last_stop);
    
    return true;
}

RouteQuality RouteOptimizer::evaluate_route(
    const Path& path,
    const std::optional<Path>& base_path) {
    
    RouteQuality quality;
    
    quality.total_distance_km = path.total_distance;
    quality.estimated_time_hours = path.min_travel_time;
    quality.num_stops = path.nodes.size();
    
    // Calculate individual scores
    if (base_path.has_value()) {
        quality.distance_score = calculate_distance_score(
            path.total_distance, base_path->total_distance);
        quality.time_score = calculate_time_score(path, base_path->min_travel_time);
    } else {
        quality.distance_score = 0.8;  // Default decent score
        quality.time_score = 0.8;
    }
    
    quality.track_quality_score = calculate_track_quality_score(path);
    quality.conflict_score = 0.9;  // High score if no conflicts considered
    
    // Calculate overall score
    quality.overall_score = 
        config_.distance_weight * quality.distance_score +
        config_.time_weight * quality.time_score +
        config_.conflict_weight * quality.conflict_score +
        config_.track_quality_weight * quality.track_quality_score;
    
    return quality;
}

double RouteOptimizer::calculate_distance_score(
    double distance_km, double base_distance_km) const {
    
    if (base_distance_km <= 0.0) return 0.5;
    
    double ratio = distance_km / base_distance_km;
    
    // Score decreases as distance increases
    // 1.0 = same distance
    // 0.5 = 1.5x distance (max allowed)
    // 0.0 = 2x+ distance
    
    if (ratio <= 1.0) {
        return 1.0;  // Shorter is better
    } else if (ratio <= config_.max_distance_multiplier) {
        return 1.0 - (ratio - 1.0) / (config_.max_distance_multiplier - 1.0);
    } else {
        return 0.0;
    }
}

double RouteOptimizer::calculate_time_score(
    const Path& path, double base_time_hours) const {
    
    if (base_time_hours <= 0.0) return 0.5;
    
    double ratio = path.min_travel_time / base_time_hours;
    
    // Similar to distance score
    if (ratio <= 1.0) {
        return 1.0;
    } else if (ratio <= config_.max_time_multiplier) {
        return 1.0 - (ratio - 1.0) / (config_.max_time_multiplier - 1.0);
    } else {
        return 0.0;
    }
}

double RouteOptimizer::calculate_conflict_score(
    const Path& path,
    const std::vector<Conflict>& conflicts) const {
    
    // Check if any edges in path are involved in conflicts
    size_t conflict_count = 0;
    
    for (const auto& conflict : conflicts) {
        for (const auto& edge : path.edges) {
            if (edge.find(conflict.location) != std::string::npos) {
                conflict_count++;
                break;
            }
        }
    }
    
    // Score: 1.0 if no conflicts, decreases with more conflicts
    if (conflict_count == 0) return 1.0;
    
    double penalty = conflict_count * 0.2;
    return std::max(0.0, 1.0 - penalty);
}

double RouteOptimizer::calculate_track_quality_score(const Path& path) const {
    // Score based on track types used
    // High-speed tracks get higher score
    
    size_t high_speed_count = 0;
    size_t single_track_count = 0;
    size_t total_edges = path.edges.size();
    
    if (total_edges == 0) return 0.5;
    
    for (const auto& edge_id : path.edges) {
        // Parse edge ID to get nodes
        size_t dash_pos = edge_id.find('-');
        if (dash_pos == std::string::npos) continue;
        
        std::string from = edge_id.substr(0, dash_pos);
        std::string to = edge_id.substr(dash_pos + 1);
        
        auto edge = network_.get_edge(from, to);
        if (!edge) continue;
        
        if (edge->get_track_type() == TrackType::HIGH_SPEED) {
            high_speed_count++;
        } else if (edge->get_track_type() == TrackType::SINGLE) {
            single_track_count++;
        }
    }
    
    double score = 0.5;  // Base score
    
    // Bonus for high-speed tracks
    if (config_.prefer_high_speed) {
        score += 0.3 * (static_cast<double>(high_speed_count) / total_edges);
    }
    
    // Penalty for single tracks
    if (config_.avoid_single_track) {
        score -= 0.2 * (static_cast<double>(single_track_count) / total_edges);
    }
    
    return std::max(0.0, std::min(1.0, score));
}

bool RouteOptimizer::meets_constraints(
    const Path& path,
    const std::optional<Path>& base_path) const {
    
    if (!path.is_valid()) return false;
    
    if (base_path.has_value()) {
        // Check distance constraint
        double distance_ratio = path.total_distance / base_path->total_distance;
        if (distance_ratio > config_.max_distance_multiplier) {
            return false;
        }
        
        // Check time constraint
        double time_ratio = path.min_travel_time / base_path->min_travel_time;
        if (time_ratio > config_.max_time_multiplier) {
            return false;
        }
    }
    
    return true;
}

std::string RouteOptimizer::generate_description(const AlternativeRoute& route) const {
    std::ostringstream oss;
    
    oss << "Alternative route via ";
    const auto& nodes = route.path.nodes;
    
    if (nodes.size() > 2) {
        for (size_t i = 1; i < nodes.size() - 1; ++i) {
            oss << nodes[i];
            if (i < nodes.size() - 2) oss << ", ";
        }
    } else {
        oss << "direct";
    }
    
    oss << " (" << std::fixed << std::setprecision(1) 
        << route.quality.total_distance_km << " km, "
        << route.quality.estimated_time_hours * 60.0 << " min, "
        << "quality: " << std::setprecision(2) << route.quality.overall_score << ")";
    
    return oss.str();
}

// BatchRouteOptimizer implementation

std::map<std::string, AlternativeRoute> BatchRouteOptimizer::optimize_batch(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const std::vector<Conflict>& conflicts) {
    
    auto& profiler = Profiler::instance();
    auto timer = profiler.start("batch_route_optimization");
    
    stats_ = BatchStats();
    std::map<std::string, AlternativeRoute> results;
    
    // Group conflicts by train
    std::map<std::string, std::vector<Conflict>> conflicts_by_train;
    for (const auto& conflict : conflicts) {
        conflicts_by_train[conflict.train1_id].push_back(conflict);
        conflicts_by_train[conflict.train2_id].push_back(conflict);
    }
    
    // Find reroutes for affected trains
    for (const auto& schedule : schedules) {
        const auto& train_id = schedule->get_train_id();
        
        auto it = conflicts_by_train.find(train_id);
        if (it != conflicts_by_train.end()) {
            auto best_route = optimizer_.find_best_reroute(*schedule, it->second);
            
            if (best_route.has_value()) {
                results[train_id] = *best_route;
                stats_.trains_rerouted++;
                stats_.total_alternatives += optimizer_.get_last_stats().alternatives_considered;
                stats_.total_extra_distance += 
                    best_route->quality.total_distance_km;
            }
        }
    }
    
    // Calculate average quality
    if (!results.empty()) {
        double total_quality = 0.0;
        for (const auto& [_, route] : results) {
            total_quality += route.quality.overall_score;
        }
        stats_.average_quality = total_quality / results.size();
    }
    
    stats_.computation_time_ms = profiler.stop(timer);
    
    return results;
}

double BatchRouteOptimizer::calculate_global_impact(
    const std::map<std::string, AlternativeRoute>& routes,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules) const {
    
    // Calculate global metrics
    double total_extra_distance = 0.0;
    double total_extra_time = 0.0;
    
    for (const auto& [train_id, route] : routes) {
        // Find corresponding schedule
        auto it = std::find_if(schedules.begin(), schedules.end(),
            [&train_id](const auto& s) { return s->get_train_id() == train_id; });
        
        if (it != schedules.end()) {
            // Calculate impact (simplified)
            total_extra_distance += route.quality.total_distance_km;
            total_extra_time += route.quality.estimated_time_hours;
        }
    }
    
    // Lower is better (less disruption)
    return total_extra_distance + total_extra_time * 100.0;  // Time weighted more
}

} // namespace fdc_scheduler
