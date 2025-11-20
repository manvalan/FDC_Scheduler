#ifndef FDC_SCHEDULER_SPEED_OPTIMIZER_HPP
#define FDC_SCHEDULER_SPEED_OPTIMIZER_HPP

#include "train.hpp"
#include "railway_network.hpp"
#include <vector>
#include <optional>
#include <cmath>
#include <map>

namespace fdc_scheduler {

/**
 * @brief Speed profile segment representing a portion of the journey
 */
struct SpeedSegment {
    double start_distance;      // Starting position (km)
    double end_distance;        // Ending position (km)
    double start_speed;         // Speed at start (km/h)
    double end_speed;           // Speed at end (km/h)
    double duration;            // Time for this segment (hours)
    double energy_consumed;     // Energy consumed (kWh)
    
    enum class Type {
        ACCELERATION,
        CONSTANT_SPEED,
        COASTING,
        BRAKING
    } type;
    
    std::string type_string() const {
        switch (type) {
            case Type::ACCELERATION: return "ACCELERATION";
            case Type::CONSTANT_SPEED: return "CONSTANT_SPEED";
            case Type::COASTING: return "COASTING";
            case Type::BRAKING: return "BRAKING";
        }
        return "UNKNOWN";
    }
};

/**
 * @brief Complete speed profile for a train journey
 */
struct SpeedProfile {
    std::vector<SpeedSegment> segments;
    double total_distance;      // Total distance (km)
    double total_time;          // Total time (hours)
    double total_energy;        // Total energy (kWh)
    double avg_speed;           // Average speed (km/h)
    double max_speed;           // Maximum speed reached (km/h)
    
    // Energy efficiency metrics
    double energy_per_km;       // kWh/km
    double energy_per_ton_km;   // kWh/(ton·km)
};

/**
 * @brief Train physics model parameters
 */
struct TrainPhysics {
    double mass;                // Train mass (tons)
    double max_acceleration;    // Maximum acceleration (m/s²)
    double max_deceleration;    // Maximum braking deceleration (m/s²)
    double rolling_resistance;  // Rolling resistance coefficient
    double air_resistance;      // Air drag coefficient (kg/m)
    double motor_efficiency;    // Motor efficiency (0-1)
    double regen_efficiency;    // Regenerative braking efficiency (0-1)
    
    // Default values for typical passenger train
    static TrainPhysics passenger_default() {
        return {
            .mass = 400.0,                    // 400 tons
            .max_acceleration = 0.8,          // 0.8 m/s²
            .max_deceleration = 1.2,          // 1.2 m/s²
            .rolling_resistance = 0.002,      // Typical for steel wheels
            .air_resistance = 5.0,            // kg/m at reference speed
            .motor_efficiency = 0.85,         // 85% efficient
            .regen_efficiency = 0.65          // 65% regeneration
        };
    }
    
    // Default values for freight train
    static TrainPhysics freight_default() {
        return {
            .mass = 2000.0,                   // 2000 tons
            .max_acceleration = 0.3,          // 0.3 m/s²
            .max_deceleration = 0.8,          // 0.8 m/s²
            .rolling_resistance = 0.003,      // Higher for heavier load
            .air_resistance = 8.0,            // Higher drag
            .motor_efficiency = 0.82,         // 82% efficient
            .regen_efficiency = 0.50          // 50% regeneration
        };
    }
};

/**
 * @brief Speed optimization configuration
 */
struct SpeedOptimizerConfig {
    bool enable_coasting;           // Allow coasting phases
    bool enable_regeneration;       // Use regenerative braking
    double time_slack_factor;       // Allow % extra time (0.05 = 5%)
    double cruise_speed_reduction;  // Reduce cruise speed by % (0.1 = 10%)
    int optimization_iterations;    // Number of optimization passes
    
    static SpeedOptimizerConfig eco_mode() {
        return {
            .enable_coasting = true,
            .enable_regeneration = true,
            .time_slack_factor = 0.10,
            .cruise_speed_reduction = 0.15,
            .optimization_iterations = 5
        };
    }
    
    static SpeedOptimizerConfig balanced_mode() {
        return {
            .enable_coasting = true,
            .enable_regeneration = true,
            .time_slack_factor = 0.05,
            .cruise_speed_reduction = 0.08,
            .optimization_iterations = 3
        };
    }
    
    static SpeedOptimizerConfig performance_mode() {
        return {
            .enable_coasting = false,
            .enable_regeneration = true,
            .time_slack_factor = 0.0,
            .cruise_speed_reduction = 0.0,
            .optimization_iterations = 1
        };
    }
};

/**
 * @brief Optimization results and statistics
 */
struct OptimizationResult {
    SpeedProfile optimized_profile;
    SpeedProfile baseline_profile;
    
    double energy_savings;          // kWh saved
    double energy_savings_percent;  // % reduction
    double time_increase;           // Hours added
    double time_increase_percent;   // % increase
    
    struct Metrics {
        int acceleration_phases;
        int constant_speed_phases;
        int coasting_phases;
        int braking_phases;
        double total_regen_energy;  // kWh recovered
    } metrics;
};

/**
 * @brief Speed optimizer for energy-efficient train operation
 * 
 * Generates optimal speed profiles that minimize energy consumption
 * while maintaining schedule adherence. Uses physics-based modeling
 * of train dynamics, rolling resistance, air drag, and motor efficiency.
 */
class SpeedOptimizer {
public:
    SpeedOptimizer(const SpeedOptimizerConfig& config = SpeedOptimizerConfig::balanced_mode());
    
    /**
     * @brief Generate optimized speed profile for a train journey
     * @param train Train to optimize
     * @param route Route nodes with distances
     * @param physics Train physics parameters
     * @param scheduled_time Target travel time (hours)
     * @return Optimized speed profile
     */
    OptimizationResult optimize_journey(
        const Train& train,
        const std::vector<std::pair<Node*, double>>& route,
        const TrainPhysics& physics,
        double scheduled_time
    );
    
    /**
     * @brief Generate baseline (non-optimized) speed profile
     * @param distance Total distance (km)
     * @param scheduled_time Target time (hours)
     * @param physics Train physics
     * @return Baseline speed profile
     */
    SpeedProfile generate_baseline_profile(
        double distance,
        double scheduled_time,
        const TrainPhysics& physics
    );
    
    /**
     * @brief Calculate energy consumption for a speed profile
     * @param profile Speed profile
     * @param physics Train physics
     * @return Total energy in kWh
     */
    double calculate_energy(const SpeedProfile& profile, const TrainPhysics& physics) const;
    
    /**
     * @brief Optimize speed profile using cruise control strategies
     * @param baseline Initial profile
     * @param physics Train physics
     * @param max_time Maximum allowed time
     * @return Optimized profile
     */
    SpeedProfile apply_cruise_control(
        const SpeedProfile& baseline,
        const TrainPhysics& physics,
        double max_time
    );
    
    /**
     * @brief Insert coasting phases to save energy
     * @param profile Current profile
     * @param physics Train physics
     * @param max_time Maximum allowed time
     * @return Profile with coasting phases
     */
    SpeedProfile insert_coasting_phases(
        const SpeedProfile& profile,
        const TrainPhysics& physics,
        double max_time
    );
    
    /**
     * @brief Get optimizer configuration
     */
    const SpeedOptimizerConfig& get_config() const { return config_; }
    
    /**
     * @brief Set optimizer configuration
     */
    void set_config(const SpeedOptimizerConfig& config) { config_ = config; }
    
private:
    SpeedOptimizerConfig config_;
    
    // Physics calculations
    double calculate_acceleration_energy(double mass, double v_start, double v_end, double distance, double efficiency) const;
    double calculate_constant_speed_energy(double mass, double speed, double distance, const TrainPhysics& physics) const;
    double calculate_resistance_force(double speed, const TrainPhysics& physics) const;
    double calculate_braking_energy(double mass, double v_start, double v_end, double regen_efficiency) const;
    
    // Profile generation helpers
    SpeedSegment create_acceleration_segment(double start_dist, double v_start, double v_target, const TrainPhysics& physics);
    SpeedSegment create_constant_segment(double start_dist, double distance, double speed, const TrainPhysics& physics);
    SpeedSegment create_coasting_segment(double start_dist, double v_start, double distance, const TrainPhysics& physics);
    SpeedSegment create_braking_segment(double start_dist, double v_start, double v_target, const TrainPhysics& physics);
    
    // Optimization helpers
    double estimate_total_time(const SpeedProfile& profile) const;
    void adjust_cruise_speed(SpeedProfile& profile, double target_time, const TrainPhysics& physics);
};

/**
 * @brief Batch optimizer for multiple trains
 */
class BatchSpeedOptimizer {
public:
    BatchSpeedOptimizer(const SpeedOptimizerConfig& config = SpeedOptimizerConfig::balanced_mode());
    
    /**
     * @brief Optimize speed profiles for multiple trains
     * @param trains Vector of trains to optimize
     * @param network Railway network
     * @param physics_map Map from train type to physics parameters
     * @return Vector of optimization results
     */
    std::vector<OptimizationResult> optimize_trains(
        const std::vector<Train>& trains,
        const RailwayNetwork& network,
        const std::map<TrainType, TrainPhysics>& physics_map
    );
    
    /**
     * @brief Get total energy savings from batch optimization
     */
    double get_total_energy_savings() const { return total_energy_savings_; }
    
    /**
     * @brief Get average energy savings percentage
     */
    double get_avg_energy_savings_percent() const { return avg_energy_savings_percent_; }
    
private:
    SpeedOptimizer optimizer_;
    double total_energy_savings_;
    double avg_energy_savings_percent_;
};

} // namespace fdc_scheduler

#endif // FDC_SCHEDULER_SPEED_OPTIMIZER_HPP
