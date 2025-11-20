#pragma once

#include "conflict_detector.hpp"
#include "railway_network.hpp"
#include "schedule.hpp"
#include "track_type.hpp"
#include <vector>
#include <memory>
#include <map>
#include <optional>
#include <chrono>

namespace fdc_scheduler {

/**
 * @brief Strategy for conflict resolution
 */
enum class ResolutionStrategy {
    DELAY_TRAIN,           ///< Delay one or both trains
    REROUTE,               ///< Change route through alternative path
    CHANGE_PLATFORM,       ///< Assign different platform at station
    ADJUST_SPEED,          ///< Modify train speed to avoid conflict
    ADD_OVERTAKING_POINT,  ///< Use passing loop on single track
    PRIORITY_BASED         ///< Use train priority to determine resolution
};

/**
 * @brief Result of a conflict resolution attempt
 */
struct ResolutionResult {
    bool success;
    ResolutionStrategy strategy_used;
    std::string description;
    std::vector<std::string> modified_trains;
    std::chrono::seconds total_delay;
    double quality_score;  ///< 0.0-1.0, where 1.0 is optimal
    
    ResolutionResult() 
        : success(false), strategy_used(ResolutionStrategy::DELAY_TRAIN), 
          total_delay(0), quality_score(0.0) {}
};

/**
 * @brief Configuration for RailwayAI-based conflict resolution
 */
struct RailwayAIConfig {
    // Priority weights for different objectives
    double delay_weight = 1.0;              ///< Weight for minimizing delays
    double platform_change_weight = 0.5;    ///< Weight for platform changes
    double reroute_weight = 0.8;            ///< Weight for route changes
    double passenger_impact_weight = 1.2;   ///< Weight for passenger trains
    
    // Constraints
    int max_delay_minutes = 30;             ///< Maximum acceptable delay per train
    int min_headway_seconds = 120;          ///< Minimum time between trains (2 min)
    int station_dwell_buffer_seconds = 60;  ///< Extra buffer at stations
    
    // Track-specific settings
    bool allow_single_track_meets = true;   ///< Allow meets on single track
    bool prefer_double_track_routing = true;///< Prefer double track when available
    int single_track_meet_buffer = 300;     ///< 5 min buffer for single track meets
    
    // Station-specific settings
    bool allow_platform_reassignment = true;
    bool optimize_platform_usage = true;
    int platform_buffer_seconds = 180;      ///< Buffer between trains at platform (3 min)
    int platform_change_cost_seconds = 180; ///< Cost of changing platform (3 min)
};

/**
 * @brief Main class for AI-powered conflict resolution
 * 
 * Integrates with RailwayAI to resolve conflicts detected in railway schedules.
 * Handles different track types (single, double) and station configurations.
 */
class RailwayAIResolver {
public:
    /**
     * @brief Construct resolver with network and configuration
     * @param network Railway network
     * @param config AI resolution configuration
     */
    explicit RailwayAIResolver(
        RailwayNetwork& network,
        const RailwayAIConfig& config = RailwayAIConfig()
    );
    
    /**
     * @brief Resolve all conflicts in a set of schedules
     * @param schedules Train schedules to optimize
     * @param conflicts Detected conflicts to resolve
     * @return Overall resolution result
     */
    ResolutionResult resolve_conflicts(
        std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const std::vector<Conflict>& conflicts
    );
    
    /**
     * @brief Resolve a single conflict between two trains
     * @param conflict The conflict to resolve
     * @param schedules All train schedules
     * @return Resolution result
     */
    ResolutionResult resolve_single_conflict(
        const Conflict& conflict,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Resolve double-track section conflicts
     * 
     * On double tracks, trains can pass each other. Resolve by:
     * - Ensuring trains use opposite tracks
     * - Minimal speed adjustments
     * - Priority-based ordering
     * 
     * @param conflict Section conflict on double track
     * @param schedules All train schedules
     * @return Resolution result
     */
    ResolutionResult resolve_double_track_conflict(
        const Conflict& conflict,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Resolve single-track section conflicts
     * 
     * On single tracks, trains cannot pass. Resolve by:
     * - Finding passing loops/sidings
     * - Scheduling meets at appropriate stations
     * - Delaying lower-priority train
     * - Ensuring sufficient headway
     * 
     * @param conflict Section conflict on single track
     * @param schedules All train schedules
     * @return Resolution result
     */
    ResolutionResult resolve_single_track_conflict(
        const Conflict& conflict,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Resolve station/platform conflicts
     * 
     * At stations, resolve by:
     * - Reassigning platforms if available
     * - Adjusting arrival/departure times
     * - Optimizing platform utilization
     * - Considering connection times
     * 
     * @param conflict Platform conflict at station
     * @param schedules All train schedules
     * @return Resolution result
     */
    ResolutionResult resolve_station_conflict(
        const Conflict& conflict,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Find optimal meeting point for two trains on single track
     * 
     * Searches for stations or passing loops where trains can meet safely.
     * Considers travel times, priorities, and passenger impact.
     * 
     * @param train1 First train schedule
     * @param train2 Second train schedule
     * @param section_start Start of conflict section
     * @param section_end End of conflict section
     * @return Station ID for meeting, or nullopt if none found
     */
    std::optional<std::string> find_meeting_point(
        const std::shared_ptr<TrainSchedule>& train1,
        const std::shared_ptr<TrainSchedule>& train2,
        const std::string& section_start,
        const std::string& section_end
    );
    
    /**
     * @brief Calculate optimal delay distribution between two trains
     * 
     * Distributes delay based on:
     * - Train priorities (passenger vs freight)
     * - Current delay status
     * - Connection impacts
     * 
     * @param train1 First train schedule
     * @param train2 Second train schedule
     * @param total_delay Total delay to distribute
     * @return Pair of delays (train1_delay, train2_delay)
     */
    std::pair<std::chrono::seconds, std::chrono::seconds> 
    distribute_delay(
        const std::shared_ptr<TrainSchedule>& train1,
        const std::shared_ptr<TrainSchedule>& train2,
        std::chrono::seconds total_delay
    );
    
    /**
     * @brief Apply delay to a train schedule
     * 
     * Propagates delay through all subsequent stops.
     * 
     * @param schedule Train schedule to modify
     * @param delay Amount of delay to add
     * @param from_stop_index Index of stop where delay starts (default: 0)
     */
    void apply_delay(
        std::shared_ptr<TrainSchedule>& schedule,
        std::chrono::seconds delay,
        size_t from_stop_index = 0
    );
    
    /**
     * @brief Find alternative platform at a station
     * 
     * @param station_id Station identifier
     * @param desired_arrival Desired arrival time
     * @param desired_departure Desired departure time
     * @param excluded_platform Platform to exclude
     * @param schedules All current schedules
     * @return Available platform number, or nullopt if none found
     */
    std::optional<int> find_alternative_platform(
        const std::string& station_id,
        std::chrono::system_clock::time_point desired_arrival,
        std::chrono::system_clock::time_point desired_departure,
        int excluded_platform,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Change platform assignment for a train at a station
     * 
     * @param schedule Train schedule to modify
     * @param station_id Station where platform changes
     * @param new_platform New platform number
     * @return true if successful, false otherwise
     */
    bool change_platform(
        std::shared_ptr<TrainSchedule>& schedule,
        const std::string& station_id,
        int new_platform
    );
    
    /**
     * @brief Calculate quality score for a resolution
     * 
     * Scores based on:
     * - Total delay introduced
     * - Number of trains affected
     * - Platform changes required
     * - Passenger impact
     * 
     * @param result Resolution result to score
     * @return Quality score (0.0-1.0)
     */
    double calculate_quality_score(const ResolutionResult& result) const;
    
    /**
     * @brief Get train priority for conflict resolution
     * 
     * Higher priority trains get preference in conflicts.
     * Based on train type (passenger > freight) and delays.
     * 
     * @param schedule Train schedule
     * @return Priority value (higher = more important)
     */
    int get_train_priority(const std::shared_ptr<TrainSchedule>& schedule) const;
    
    /**
     * @brief Get track type between two nodes
     * 
     * @param from_node Start node
     * @param to_node End node
     * @return Track type, or nullopt if edge not found
     */
    std::optional<TrackType> get_track_type(
        const std::string& from_node,
        const std::string& to_node
    ) const;
    
    /**
     * @brief Check if a station has passing/siding capability
     * 
     * @param station_id Station identifier
     * @return true if station can handle train meets
     */
    bool has_passing_capability(const std::string& station_id) const;
    
    /**
     * @brief Get configuration
     */
    const RailwayAIConfig& get_config() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void set_config(const RailwayAIConfig& config) { config_ = config; }
    
    /**
     * @brief Get resolution statistics
     */
    std::map<std::string, int> get_statistics() const;
    
private:
    RailwayNetwork& network_;
    RailwayAIConfig config_;
    
    // Statistics
    mutable int total_resolutions_ = 0;
    mutable int successful_resolutions_ = 0;
    mutable int double_track_resolutions_ = 0;
    mutable int single_track_resolutions_ = 0;
    mutable int station_resolutions_ = 0;
    mutable int delays_applied_ = 0;
    mutable int platforms_changed_ = 0;
    
    /**
     * @brief Find train schedule by ID
     */
    std::shared_ptr<TrainSchedule> find_schedule(
        const std::string& train_id,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
    
    /**
     * @brief Calculate minimum separation time needed
     */
    std::chrono::seconds calculate_required_separation(
        const Conflict& conflict,
        TrackType track_type
    ) const;
    
    /**
     * @brief Validate that resolution actually fixes the conflict
     */
    bool validate_resolution(
        const Conflict& conflict,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules
    ) const;
    
    /**
     * @brief Resolve timing violations
     */
    ResolutionResult resolve_timing_violation(
        const Conflict& conflict,
        std::vector<std::shared_ptr<TrainSchedule>>& schedules
    );
};

/**
 * @brief Convert resolution strategy to string
 */
std::string strategy_to_string(ResolutionStrategy strategy);

/**
 * @brief Format resolution result as human-readable string
 */
std::string resolution_to_string(const ResolutionResult& result);

} // namespace fdc_scheduler
