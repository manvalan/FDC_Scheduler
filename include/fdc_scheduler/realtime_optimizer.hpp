#ifndef FDC_SCHEDULER_REALTIME_OPTIMIZER_HPP
#define FDC_SCHEDULER_REALTIME_OPTIMIZER_HPP

#include "train.hpp"
#include "railway_network.hpp"
#include "conflict_detector.hpp"
#include "route_optimizer.hpp"
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include <functional>

namespace fdc_scheduler {

/**
 * @brief Real-time position of a train
 */
struct TrainPosition {
    std::string train_id;
    Node* current_node;         // Current node (or last passed)
    Node* next_node;            // Next target node
    double progress;            // Progress to next node (0.0-1.0)
    double current_speed;       // Current speed (km/h)
    std::chrono::system_clock::time_point timestamp;
    
    // GPS coordinates (if available)
    std::optional<double> latitude;
    std::optional<double> longitude;
};

/**
 * @brief Delay information for a train
 */
struct TrainDelay {
    std::string train_id;
    double delay_minutes;       // Positive = late, negative = early
    std::string reason;         // Delay reason (optional)
    std::chrono::system_clock::time_point detected_at;
    bool is_recovering;         // Whether train is catching up
};

/**
 * @brief Predicted conflict in the near future
 */
struct PredictedConflict {
    std::string train1_id;
    std::string train2_id;
    Node* conflict_node;
    std::chrono::system_clock::time_point predicted_time;
    double confidence;          // 0.0-1.0
    ConflictType type;
    
    // Predicted positions at conflict time
    TrainPosition train1_position;
    TrainPosition train2_position;
};

/**
 * @brief Schedule adjustment recommendation
 */
struct ScheduleAdjustment {
    std::string train_id;
    
    enum class Type {
        SPEED_CHANGE,           // Adjust speed
        HOLD_AT_STATION,        // Hold at next station
        SKIP_STOP,              // Skip optional stop
        ROUTE_CHANGE,           // Change route
        PRIORITY_CHANGE         // Change priority
    } type;
    
    // Type-specific data
    std::optional<double> new_speed;
    std::optional<double> hold_duration_minutes;
    std::optional<std::string> station_to_skip;
    std::optional<std::vector<Node*>> new_route;
    std::optional<int> new_priority;
    
    double estimated_delay_reduction;   // Minutes saved
    double confidence;                  // 0.0-1.0
    std::string justification;
    
    std::string type_string() const {
        switch (type) {
            case Type::SPEED_CHANGE: return "SPEED_CHANGE";
            case Type::HOLD_AT_STATION: return "HOLD_AT_STATION";
            case Type::SKIP_STOP: return "SKIP_STOP";
            case Type::ROUTE_CHANGE: return "ROUTE_CHANGE";
            case Type::PRIORITY_CHANGE: return "PRIORITY_CHANGE";
        }
        return "UNKNOWN";
    }
};

/**
 * @brief Real-time optimization configuration
 */
struct RealTimeConfig {
    double prediction_horizon_minutes;      // How far ahead to predict (default: 30)
    double conflict_detection_threshold;    // Confidence threshold (default: 0.7)
    double delay_tolerance_minutes;         // Min delay to trigger action (default: 5)
    bool enable_speed_adjustments;          // Allow speed changes
    bool enable_route_changes;              // Allow rerouting
    bool enable_stop_skipping;              // Allow skipping stops
    int max_adjustments_per_cycle;          // Max changes per update (default: 5)
    double update_frequency_seconds;        // How often to recalculate (default: 10)
    
    static RealTimeConfig conservative() {
        return {
            .prediction_horizon_minutes = 15.0,
            .conflict_detection_threshold = 0.8,
            .delay_tolerance_minutes = 10.0,
            .enable_speed_adjustments = true,
            .enable_route_changes = false,
            .enable_stop_skipping = false,
            .max_adjustments_per_cycle = 3,
            .update_frequency_seconds = 30.0
        };
    }
    
    static RealTimeConfig balanced() {
        return {
            .prediction_horizon_minutes = 30.0,
            .conflict_detection_threshold = 0.7,
            .delay_tolerance_minutes = 5.0,
            .enable_speed_adjustments = true,
            .enable_route_changes = true,
            .enable_stop_skipping = false,
            .max_adjustments_per_cycle = 5,
            .update_frequency_seconds = 10.0
        };
    }
    
    static RealTimeConfig aggressive() {
        return {
            .prediction_horizon_minutes = 45.0,
            .conflict_detection_threshold = 0.6,
            .delay_tolerance_minutes = 2.0,
            .enable_speed_adjustments = true,
            .enable_route_changes = true,
            .enable_stop_skipping = true,
            .max_adjustments_per_cycle = 10,
            .update_frequency_seconds = 5.0
        };
    }
};

/**
 * @brief Statistics for real-time optimization
 */
struct RealTimeStats {
    int total_updates;
    int conflicts_predicted;
    int conflicts_avoided;
    int adjustments_applied;
    double avg_delay_reduction;
    double avg_prediction_accuracy;
    std::chrono::system_clock::time_point last_update;
};

/**
 * @brief Real-time optimization engine
 * 
 * Monitors train positions, predicts conflicts, and generates
 * dynamic schedule adjustments to minimize delays and conflicts.
 */
class RealTimeOptimizer {
public:
    RealTimeOptimizer(
        RailwayNetwork& network,
        const RealTimeConfig& config = RealTimeConfig::balanced()
    );
    
    /**
     * @brief Update position of a train
     * @param position Current position
     */
    void update_train_position(const TrainPosition& position);
    
    /**
     * @brief Update multiple train positions
     * @param positions Vector of positions
     */
    void update_train_positions(const std::vector<TrainPosition>& positions);
    
    /**
     * @brief Report a delay for a train
     * @param delay Delay information
     */
    void report_delay(const TrainDelay& delay);
    
    /**
     * @brief Get current position of a train
     * @param train_id Train identifier
     * @return Current position if available
     */
    std::optional<TrainPosition> get_train_position(const std::string& train_id) const;
    
    /**
     * @brief Get all tracked train positions
     * @return Map of train_id to position
     */
    const std::map<std::string, TrainPosition>& get_all_positions() const {
        return positions_;
    }
    
    /**
     * @brief Predict conflicts in the near future
     * @return Vector of predicted conflicts
     */
    std::vector<PredictedConflict> predict_conflicts();
    
    /**
     * @brief Generate schedule adjustments to avoid conflicts
     * @param conflicts Predicted conflicts
     * @return Vector of recommended adjustments
     */
    std::vector<ScheduleAdjustment> generate_adjustments(
        const std::vector<PredictedConflict>& conflicts
    );
    
    /**
     * @brief Perform full optimization cycle
     * 
     * Updates predictions, detects conflicts, and generates adjustments
     * 
     * @return Vector of recommended adjustments
     */
    std::vector<ScheduleAdjustment> optimize();
    
    /**
     * @brief Apply an adjustment to the schedule
     * @param adjustment Adjustment to apply
     * @return True if applied successfully
     */
    bool apply_adjustment(const ScheduleAdjustment& adjustment);
    
    /**
     * @brief Estimate train arrival time at a node
     * @param train_id Train identifier
     * @param target_node Target node
     * @return Estimated arrival time
     */
    std::optional<std::chrono::system_clock::time_point> estimate_arrival_time(
        const std::string& train_id,
        const Node* target_node
    ) const;
    
    /**
     * @brief Calculate delay for a train
     * @param train_id Train identifier
     * @return Delay in minutes (positive = late)
     */
    std::optional<double> calculate_current_delay(const std::string& train_id) const;
    
    /**
     * @brief Get real-time statistics
     */
    const RealTimeStats& get_stats() const { return stats_; }
    
    /**
     * @brief Get configuration
     */
    const RealTimeConfig& get_config() const { return config_; }
    
    /**
     * @brief Set configuration
     */
    void set_config(const RealTimeConfig& config) { config_ = config; }
    
    /**
     * @brief Register callback for position updates
     * @param callback Function called when position is updated
     */
    void on_position_update(std::function<void(const TrainPosition&)> callback) {
        position_callback_ = callback;
    }
    
    /**
     * @brief Register callback for conflict predictions
     * @param callback Function called when conflict is predicted
     */
    void on_conflict_predicted(std::function<void(const PredictedConflict&)> callback) {
        conflict_callback_ = callback;
    }
    
    /**
     * @brief Register callback for adjustment generation
     * @param callback Function called when adjustment is generated
     */
    void on_adjustment_generated(std::function<void(const ScheduleAdjustment&)> callback) {
        adjustment_callback_ = callback;
    }
    
private:
    RailwayNetwork& network_;
    RealTimeConfig config_;
    
    // Current state
    std::map<std::string, TrainPosition> positions_;
    std::map<std::string, TrainDelay> delays_;
    std::vector<PredictedConflict> active_predictions_;
    
    // Statistics
    RealTimeStats stats_;
    
    // Callbacks
    std::function<void(const TrainPosition&)> position_callback_;
    std::function<void(const PredictedConflict&)> conflict_callback_;
    std::function<void(const ScheduleAdjustment&)> adjustment_callback_;
    
    // Helper methods
    double predict_position_at_time(
        const TrainPosition& current,
        std::chrono::system_clock::time_point future_time
    ) const;
    
    bool will_conflict(
        const TrainPosition& pos1,
        const TrainPosition& pos2,
        std::chrono::system_clock::time_point check_time
    ) const;
    
    ScheduleAdjustment generate_speed_adjustment(
        const std::string& train_id,
        const PredictedConflict& conflict
    );
    
    ScheduleAdjustment generate_hold_adjustment(
        const std::string& train_id,
        const PredictedConflict& conflict
    );
    
    ScheduleAdjustment generate_route_adjustment(
        const std::string& train_id,
        const PredictedConflict& conflict
    );
    
    double calculate_conflict_confidence(
        const TrainPosition& pos1,
        const TrainPosition& pos2
    ) const;
};

} // namespace fdc_scheduler

#endif // FDC_SCHEDULER_REALTIME_OPTIMIZER_HPP
