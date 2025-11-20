#ifndef FDC_ROUTE_OPTIMIZER_HPP
#define FDC_ROUTE_OPTIMIZER_HPP

#include "railway_network.hpp"
#include "schedule.hpp"
#include "conflict_detector.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace fdc_scheduler {

/**
 * @brief Quality metrics for evaluating alternative routes
 */
struct RouteQuality {
    double distance_score = 0.0;       ///< Normalized distance (0-1, higher is better)
    double time_score = 0.0;           ///< Normalized travel time (0-1, higher is better)
    double conflict_score = 0.0;       ///< Conflict avoidance (0-1, higher is better)
    double track_quality_score = 0.0;  ///< Track type preference (0-1, higher is better)
    double overall_score = 0.0;        ///< Weighted combination
    
    // Additional metrics
    size_t num_stops = 0;              ///< Number of intermediate stops
    double total_distance_km = 0.0;    ///< Total route distance
    double estimated_time_hours = 0.0; ///< Estimated travel time
    bool has_conflicts = false;        ///< Whether route has conflicts
};

/**
 * @brief Configuration for route optimization
 */
struct RouteOptimizerConfig {
    // Scoring weights (should sum to 1.0)
    double distance_weight = 0.3;
    double time_weight = 0.3;
    double conflict_weight = 0.3;
    double track_quality_weight = 0.1;
    
    // Constraints
    double max_distance_multiplier = 1.5;  ///< Max distance vs direct path
    double max_time_multiplier = 1.5;      ///< Max time vs direct path
    size_t max_alternatives = 5;           ///< Maximum alternative routes to consider
    
    // Preferences
    bool prefer_high_speed = true;
    bool avoid_single_track = true;
    bool minimize_stops = true;
    
    RouteOptimizerConfig() = default;
};

/**
 * @brief Alternative route with quality assessment
 */
struct AlternativeRoute {
    Path path;                    ///< The route path
    RouteQuality quality;         ///< Quality metrics
    std::string description;      ///< Human-readable description
    
    bool operator<(const AlternativeRoute& other) const {
        return quality.overall_score > other.quality.overall_score;
    }
};

/**
 * @brief Route optimizer for dynamic rerouting
 * 
 * Finds and evaluates alternative routes when conflicts cannot be
 * resolved through delays or platform changes alone.
 */
class RouteOptimizer {
public:
    /**
     * @brief Construct optimizer with default configuration
     */
    explicit RouteOptimizer(const RailwayNetwork& network)
        : network_(network), config_() {}
    
    /**
     * @brief Construct optimizer with custom configuration
     */
    RouteOptimizer(const RailwayNetwork& network, const RouteOptimizerConfig& config)
        : network_(network), config_(config) {}
    
    /**
     * @brief Find alternative routes between two nodes
     * 
     * @param start_node Starting station
     * @param end_node Destination station
     * @param exclude_edges Edges to avoid (e.g., due to conflicts)
     * @return Vector of alternative routes sorted by quality
     */
    std::vector<AlternativeRoute> find_alternatives(
        const std::string& start_node,
        const std::string& end_node,
        const std::vector<std::string>& exclude_edges = {});
    
    /**
     * @brief Find best alternative route for a train schedule
     * 
     * Considers the entire schedule and finds optimal rerouting
     * that minimizes disruption.
     * 
     * @param schedule Train schedule to reroute
     * @param conflicts Known conflicts to avoid
     * @return Best alternative route, or nullopt if none found
     */
    std::optional<AlternativeRoute> find_best_reroute(
        const TrainSchedule& schedule,
        const std::vector<Conflict>& conflicts);
    
    /**
     * @brief Apply rerouting to a schedule
     * 
     * Modifies the schedule to use the alternative route,
     * updating all stops and timings.
     * 
     * @param schedule Schedule to modify
     * @param route Alternative route to apply
     * @return True if successful
     */
    bool apply_reroute(
        TrainSchedule& schedule,
        const AlternativeRoute& route);
    
    /**
     * @brief Evaluate quality of a specific route
     * 
     * @param path Path to evaluate
     * @param base_path Optional baseline path for comparison
     * @return Quality metrics
     */
    RouteQuality evaluate_route(
        const Path& path,
        const std::optional<Path>& base_path = std::nullopt);
    
    /**
     * @brief Get/set configuration
     */
    const RouteOptimizerConfig& get_config() const { return config_; }
    void set_config(const RouteOptimizerConfig& config) { config_ = config; }
    
    /**
     * @brief Get statistics about last optimization
     */
    struct OptimizationStats {
        size_t alternatives_considered = 0;
        size_t valid_alternatives = 0;
        double best_score = 0.0;
        double computation_time_ms = 0.0;
    };
    
    OptimizationStats get_last_stats() const { return last_stats_; }

private:
    const RailwayNetwork& network_;
    RouteOptimizerConfig config_;
    OptimizationStats last_stats_;
    
    /**
     * @brief Calculate distance score (normalized)
     */
    double calculate_distance_score(double distance_km, double base_distance_km) const;
    
    /**
     * @brief Calculate time score based on route
     */
    double calculate_time_score(const Path& path, double base_time_hours) const;
    
    /**
     * @brief Calculate conflict avoidance score
     */
    double calculate_conflict_score(
        const Path& path,
        const std::vector<Conflict>& conflicts) const;
    
    /**
     * @brief Calculate track quality score
     */
    double calculate_track_quality_score(const Path& path) const;
    
    /**
     * @brief Check if route meets constraints
     */
    bool meets_constraints(
        const Path& path,
        const std::optional<Path>& base_path) const;
    
    /**
     * @brief Generate description for alternative route
     */
    std::string generate_description(const AlternativeRoute& route) const;
};

/**
 * @brief Helper class for batch route optimization
 */
class BatchRouteOptimizer {
public:
    explicit BatchRouteOptimizer(const RailwayNetwork& network)
        : optimizer_(network) {}
    
    /**
     * @brief Optimize routes for multiple trains simultaneously
     * 
     * Considers global optimization to minimize total disruption.
     * 
     * @param schedules All train schedules
     * @param conflicts Detected conflicts
     * @return Map of train_id to best alternative route
     */
    std::map<std::string, AlternativeRoute> optimize_batch(
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const std::vector<Conflict>& conflicts);
    
    /**
     * @brief Get global optimization statistics
     */
    struct BatchStats {
        size_t trains_rerouted = 0;
        size_t total_alternatives = 0;
        double average_quality = 0.0;
        double total_extra_distance = 0.0;
        double computation_time_ms = 0.0;
    };
    
    BatchStats get_stats() const { return stats_; }

private:
    RouteOptimizer optimizer_;
    BatchStats stats_;
    
    /**
     * @brief Calculate global impact of rerouting decisions
     */
    double calculate_global_impact(
        const std::map<std::string, AlternativeRoute>& routes,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules) const;
};

} // namespace fdc_scheduler

#endif // FDC_ROUTE_OPTIMIZER_HPP
