#ifndef FDC_TRAIN_HPP
#define FDC_TRAIN_HPP

#include "train_type.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace fdc_scheduler {

/**
 * @brief Represents a train with physical characteristics
 * 
 * Contains information about train type, speed, acceleration,
 * and braking capabilities. Used for realistic travel time calculations.
 */
class Train {
public:
    /**
     * @brief Construct a new Train
     * 
     * @param id Unique identifier
     * @param name Human-readable name
     * @param type Type of train
     * @param max_speed Maximum speed in km/h
     * @param acceleration Acceleration in m/s²
     * @param deceleration Braking deceleration in m/s²
     */
    Train(std::string id,
          std::string name,
          TrainType type = TrainType::REGIONAL,
          double max_speed = 160.0,
          double acceleration = 0.6,
          double deceleration = 0.8);

    // Getters
    const std::string& get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    TrainType get_type() const { return type_; }
    double get_max_speed() const { return max_speed_; }
    double get_acceleration() const { return acceleration_; }
    double get_deceleration() const { return deceleration_; }

    // Setters
    void set_name(const std::string& name) { name_ = name; }
    void set_type(TrainType type);
    void set_max_speed(double speed) { max_speed_ = speed; }
    void set_acceleration(double accel) { acceleration_ = accel; }
    void set_deceleration(double decel) { deceleration_ = decel; }

    /**
     * @brief Calculate realistic travel time for a distance
     * 
     * Takes into account acceleration, cruising at max speed, and braking.
     * 
     * @param distance_km Distance in kilometers
     * @param track_max_speed Maximum speed allowed on track (km/h)
     * @param initial_speed Initial speed (km/h), default 0
     * @param final_speed Final speed (km/h), default 0
     * @return Travel time in hours
     */
    double calculate_travel_time(double distance_km,
                                 double track_max_speed,
                                 double initial_speed = 0.0,
                                 double final_speed = 0.0) const;

    /**
     * @brief Factory method to create train by type with default parameters
     */
    static Train create_by_type(const std::string& id,
                               const std::string& name,
                               TrainType type);

private:
    std::string id_;
    std::string name_;
    TrainType type_;
    double max_speed_;      // km/h
    double acceleration_;   // m/s²
    double deceleration_;   // m/s² (positive value)
};

/**
 * @brief JSON serialization for Train
 */
void to_json(nlohmann::json& j, const Train& train);

/**
 * @brief JSON deserialization for Train
 */
void from_json(const nlohmann::json& j, Train& train);

} // namespace fdc_scheduler

#endif // FDC_TRAIN_HPP
