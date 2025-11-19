#pragma once

#include "railway_network.hpp"
#include "schedule.hpp"
#include "conflict_detector.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>

namespace fdc_scheduler {

/**
 * @brief JSON API for FDC_Scheduler library
 * 
 * Provides a complete JSON-based interface for:
 * - Network management (load, export, query)
 * - Schedule management (add, update, delete trains)
 * - Conflict detection and resolution
 * - Integration with external optimization engines
 */
class JsonApi {
public:
    JsonApi();
    ~JsonApi();
    
    // ========================================================================
    // NETWORK OPERATIONS
    // ========================================================================
    
    /**
     * @brief Load railway network from file
     * @param path Path to network file (JSON, RailML 2.x, RailML 3.x)
     * @return JSON response with status and network info
     * 
     * Response format:
     * {
     *   "success": true,
     *   "message": "Network loaded successfully",
     *   "network": {
     *     "nodes": 15,
     *     "edges": 23,
     *     "total_length_km": 450.5
     *   }
     * }
     */
    std::string load_network(const std::string& path);
    
    /**
     * @brief Export network to file
     * @param path Output file path
     * @param format Output format: "json", "railml2", "railml3"
     * @return JSON response with status
     */
    std::string export_network(const std::string& path, const std::string& format = "json");
    
    /**
     * @brief Get network information as JSON
     * @return JSON representation of current network
     */
    std::string get_network_info();
    
    /**
     * @brief Add a station to the network
     * @param node_json JSON representation of station
     * @return JSON response with status
     * 
     * Input format:
     * {
     *   "id": "MILANO",
     *   "name": "Milano Centrale",
     *   "type": "station",
     *   "platforms": 12,
     *   "latitude": 45.4865,
     *   "longitude": 9.2041
     * }
     */
    std::string add_station(const std::string& node_json);
    
    /**
     * @brief Add a track section to the network
     * @param edge_json JSON representation of track section
     * @return JSON response with status
     */
    std::string add_track_section(const std::string& edge_json);
    
    // ========================================================================
    // SCHEDULE OPERATIONS
    // ========================================================================
    
    /**
     * @brief Add a new train schedule
     * @param train_json JSON representation of train
     * @return JSON response with status and train ID
     * 
     * Input format:
     * {
     *   "train_id": "IC101",
     *   "train_type": "InterCity",
     *   "stops": [
     *     {
     *       "node_id": "MILANO",
     *       "arrival": "08:00",
     *       "departure": "08:05",
     *       "platform": 3
     *     }
     *   ]
     * }
     */
    std::string add_train(const std::string& train_json);
    
    /**
     * @brief Get all train schedules
     * @return JSON array of all trains
     */
    std::string get_all_trains();
    
    /**
     * @brief Get specific train schedule
     * @param train_id Train identifier
     * @return JSON representation of train
     */
    std::string get_train(const std::string& train_id);
    
    /**
     * @brief Update train schedule
     * @param train_id Train identifier
     * @param updates_json JSON with fields to update
     * @return JSON response with status
     */
    std::string update_train(const std::string& train_id, const std::string& updates_json);
    
    /**
     * @brief Delete train schedule
     * @param train_id Train identifier
     * @return JSON response with status
     */
    std::string delete_train(const std::string& train_id);
    
    // ========================================================================
    // CONFLICT DETECTION
    // ========================================================================
    
    /**
     * @brief Detect all conflicts in current schedules
     * @return JSON array of conflicts
     * 
     * Response format:
     * {
     *   "conflicts": [
     *     {
     *       "type": "platform_conflict",
     *       "train1": "IC101",
     *       "train2": "R205",
     *       "location": "Como San Giovanni",
     *       "platform": 1,
     *       "time": "08:25",
     *       "severity": 8,
     *       "description": "Platform conflict at COMO"
     *     }
     *   ],
     *   "total": 1
     * }
     */
    std::string detect_conflicts();
    
    /**
     * @brief Detect conflicts for specific train
     * @param train_id Train identifier
     * @return JSON array of conflicts involving this train
     */
    std::string detect_conflicts_for_train(const std::string& train_id);
    
    /**
     * @brief Validate schedule (check for conflicts and violations)
     * @param train_id Optional: specific train to validate
     * @return JSON validation report
     */
    std::string validate_schedule(const std::string& train_id = "");
    
    // ========================================================================
    // AI INTEGRATION
    // ========================================================================
    
    /**
     * @brief Resolve conflicts using external AI optimizer
     * @param conflicts_json JSON array of conflicts
     * @param constraints_json Optional: JSON with constraints
     * @return JSON with suggested modifications
     * 
     * This method formats data for external AI engines like RailwayAI
     * and returns the optimization suggestions.
     */
    std::string resolve_conflicts_with_ai(
        const std::string& conflicts_json,
        const std::string& constraints_json = ""
    );
    
    /**
     * @brief Apply modifications from AI optimizer
     * @param modifications_json JSON array of modifications
     * @return JSON response with application results
     * 
     * Input format:
     * {
     *   "modifications": [
     *     {
     *       "train_id": "IC101",
     *       "type": "platform_change",
     *       "details": {"old_platform": 1, "new_platform": 2}
     *     }
     *   ]
     * }
     */
    std::string apply_modifications(const std::string& modifications_json);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    /**
     * @brief Get conflict detector configuration
     * @return JSON representation of config
     */
    std::string get_config();
    
    /**
     * @brief Update conflict detector configuration
     * @param config_json JSON with configuration updates
     * @return JSON response with status
     */
    std::string set_config(const std::string& config_json);
    
    /**
     * @brief Get statistics about library usage
     * @return JSON with statistics
     */
    std::string get_statistics();
    
    /**
     * @brief Reset library state (clear network and schedules)
     * @return JSON response with status
     */
    std::string reset();

private:
    std::unique_ptr<RailwayNetwork> network_;
    std::vector<std::shared_ptr<TrainSchedule>> schedules_;
    std::unique_ptr<ConflictDetector> detector_;
    
    // Helper methods
    nlohmann::json create_success_response(const std::string& message = "Success");
    nlohmann::json create_error_response(const std::string& error);
    nlohmann::json schedule_to_json(const TrainSchedule& schedule) const;
    nlohmann::json conflict_to_json(const Conflict& conflict) const;
    std::shared_ptr<TrainSchedule> json_to_schedule(const nlohmann::json& j) const;
    std::shared_ptr<TrainSchedule> find_schedule(const std::string& train_id);
};

} // namespace fdc_scheduler
