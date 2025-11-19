#pragma once

#include "railway_network.hpp"
#include "schedule.hpp"
#include <vector>
#include <string>
#include <memory>
#include <chrono>

namespace fdc_scheduler {

/**
 * @brief Types of conflicts that can occur in railway scheduling
 */
enum class ConflictType {
    SECTION_OVERLAP,    ///< Two trains on same track section
    PLATFORM_CONFLICT,  ///< Two trains at same platform
    HEAD_ON_COLLISION,  ///< Trains approaching each other on single track
    TIMING_VIOLATION    ///< Insufficient time buffer
};

/**
 * @brief Detected conflict information
 */
struct Conflict {
    ConflictType type;
    std::string train1_id;
    std::string train2_id;
    std::string location;           ///< Station name or section identifier
    std::chrono::system_clock::time_point conflict_time;
    std::string description;
    
    // Additional details for specific conflict types
    int platform = -1;              ///< Platform number (for PLATFORM_CONFLICT)
    std::string section_from;       ///< Section start (for SECTION_OVERLAP)
    std::string section_to;         ///< Section end (for SECTION_OVERLAP)
    int severity = 0;               ///< Conflict severity (0-10)
};

/**
 * @brief Configuration for conflict detection
 */
struct ConflictDetectorConfig {
    int section_buffer_seconds = 119;    ///< Buffer between trains on same section
    int platform_buffer_seconds = 300;   ///< Buffer between trains at same platform (5 min)
    int head_on_buffer_seconds = 600;    ///< Buffer for head-on collision (10 min)
    bool detect_section_conflicts = true;
    bool detect_platform_conflicts = true;
    bool detect_head_on_collisions = true;
    bool detect_timing_violations = true;
};

/**
 * @brief Main class for detecting conflicts in railway schedules
 */
class ConflictDetector {
public:
    /**
     * @brief Construct conflict detector with default configuration
     * @param network Railway network to analyze
     */
    explicit ConflictDetector(const RailwayNetwork& network);
    
    /**
     * @brief Construct conflict detector with custom configuration
     * @param network Railway network to analyze
     * @param config Custom detection configuration
     */
    ConflictDetector(const RailwayNetwork& network, const ConflictDetectorConfig& config);
    
    /**
     * @brief Detect all conflicts between given schedules
     * @param schedules Vector of train schedules to check
     * @return Vector of detected conflicts
     */
    std::vector<Conflict> detect_all(const std::vector<std::shared_ptr<TrainSchedule>>& schedules);
    
    /**
     * @brief Detect conflicts for a specific train
     * @param schedule Train schedule to check
     * @param other_schedules Other trains to check against
     * @return Vector of conflicts involving this train
     */
    std::vector<Conflict> detect_for_train(
        const std::shared_ptr<TrainSchedule>& schedule,
        const std::vector<std::shared_ptr<TrainSchedule>>& other_schedules
    );
    
    /**
     * @brief Detect section conflicts (track occupation)
     * @param schedule1 First train schedule
     * @param schedule2 Second train schedule
     * @return Vector of section conflicts
     */
    std::vector<Conflict> detect_section_conflicts(
        const std::shared_ptr<TrainSchedule>& schedule1,
        const std::shared_ptr<TrainSchedule>& schedule2
    );
    
    /**
     * @brief Detect platform conflicts at stations
     * @param schedule1 First train schedule
     * @param schedule2 Second train schedule
     * @return Vector of platform conflicts
     */
    std::vector<Conflict> detect_platform_conflicts(
        const std::shared_ptr<TrainSchedule>& schedule1,
        const std::shared_ptr<TrainSchedule>& schedule2
    );
    
    /**
     * @brief Detect head-on collision risks on single-track sections
     * @param schedule1 First train schedule
     * @param schedule2 Second train schedule
     * @return Vector of potential head-on collisions
     */
    std::vector<Conflict> detect_head_on_collisions(
        const std::shared_ptr<TrainSchedule>& schedule1,
        const std::shared_ptr<TrainSchedule>& schedule2
    );
    
    /**
     * @brief Validate timing constraints for a schedule
     * @param schedule Train schedule to validate
     * @return Vector of timing violations
     */
    std::vector<Conflict> validate_timing(const std::shared_ptr<TrainSchedule>& schedule);
    
    /**
     * @brief Get current configuration
     * @return Current detector configuration
     */
    const ConflictDetectorConfig& get_config() const { return config_; }
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void set_config(const ConflictDetectorConfig& config) { config_ = config; }
    
    /**
     * @brief Get statistics about last detection run
     * @return Map of statistic name to value
     */
    std::map<std::string, int> get_statistics() const;

private:
    const RailwayNetwork& network_;
    ConflictDetectorConfig config_;
    
    // Statistics
    mutable int total_checks_ = 0;
    mutable int section_conflicts_found_ = 0;
    mutable int platform_conflicts_found_ = 0;
    mutable int head_on_collisions_found_ = 0;
    mutable int timing_violations_found_ = 0;
    
    /**
     * @brief Check if two time windows overlap with buffer
     */
    bool time_windows_overlap(
        std::chrono::system_clock::time_point start1,
        std::chrono::system_clock::time_point end1,
        std::chrono::system_clock::time_point start2,
        std::chrono::system_clock::time_point end2,
        int buffer_seconds
    ) const;
    
    /**
     * @brief Calculate conflict severity
     */
    int calculate_severity(const Conflict& conflict) const;
};

/**
 * @brief Convert conflict type to string
 */
std::string conflict_type_to_string(ConflictType type);

/**
 * @brief Convert conflict to human-readable string
 */
std::string conflict_to_string(const Conflict& conflict);

} // namespace fdc_scheduler
