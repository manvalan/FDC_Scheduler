#include "fdc_scheduler/train.hpp"
#include <cmath>
#include <algorithm>

namespace fdc_scheduler {

Train::Train(std::string id,
             std::string name,
             TrainType type,
             double max_speed,
             double acceleration,
             double deceleration)
    : id_(std::move(id))
    , name_(std::move(name))
    , type_(type)
    , max_speed_(max_speed)
    , acceleration_(acceleration)
    , deceleration_(deceleration) {
}

void Train::set_type(TrainType type) {
    type_ = type;
    
    // Update default parameters based on type
    switch (type) {
        case TrainType::REGIONAL:
            max_speed_ = 160.0;
            acceleration_ = 0.6;
            deceleration_ = 0.8;
            break;
        case TrainType::INTERCITY:
            max_speed_ = 200.0;
            acceleration_ = 0.5;
            deceleration_ = 0.7;
            break;
        case TrainType::HIGH_SPEED:
            max_speed_ = 300.0;
            acceleration_ = 0.4;
            deceleration_ = 0.6;
            break;
        case TrainType::FREIGHT:
            max_speed_ = 100.0;
            acceleration_ = 0.3;
            deceleration_ = 0.5;
            break;
    }
}

double Train::calculate_travel_time(double distance_km,
                                    double track_max_speed,
                                    double initial_speed,
                                    double final_speed) const {
    // Convert to m/s
    const double distance = distance_km * 1000.0;  // meters
    const double v_max = std::min(max_speed_, track_max_speed) / 3.6;  // m/s
    const double v_start = initial_speed / 3.6;  // m/s
    const double v_end = final_speed / 3.6;  // m/s
    const double a = acceleration_;  // m/s²
    const double d = deceleration_;  // m/s²
    
    // Calculate distances for acceleration and braking
    const double accel_distance = (v_max * v_max - v_start * v_start) / (2.0 * a);
    const double brake_distance = (v_max * v_max - v_end * v_end) / (2.0 * d);
    
    // Time for acceleration and braking
    const double t_accel = (v_max - v_start) / a;
    const double t_brake = (v_max - v_end) / d;
    
    // Check if we reach max speed
    if (accel_distance + brake_distance <= distance) {
        // We reach max speed and cruise
        const double cruise_distance = distance - accel_distance - brake_distance;
        const double t_cruise = cruise_distance / v_max;
        return (t_accel + t_cruise + t_brake) / 3600.0;  // Convert to hours
    } else {
        // We don't reach max speed - calculate peak velocity
        // Using physics: d = (v_peak² - v_start²)/(2a) + (v_peak² - v_end²)/(2d)
        const double numerator = distance + 
                                (v_start * v_start) / (2.0 * a) + 
                                (v_end * v_end) / (2.0 * d);
        const double denominator = 1.0 / (2.0 * a) + 1.0 / (2.0 * d);
        const double v_peak_squared = numerator / denominator;
        
        if (v_peak_squared < 0) {
            // Fallback: constant speed
            const double avg_speed = (v_start + v_end) / 2.0;
            return distance / (avg_speed * 3600.0);
        }
        
        const double v_peak = std::sqrt(v_peak_squared);
        const double t_up = (v_peak - v_start) / a;
        const double t_down = (v_peak - v_end) / d;
        return (t_up + t_down) / 3600.0;  // Convert to hours
    }
}

Train Train::create_by_type(const std::string& id,
                            const std::string& name,
                            TrainType type) {
    Train train(id, name, type);
    train.set_type(type);  // This sets default parameters
    return train;
}

// JSON serialization
void to_json(nlohmann::json& j, const Train& train) {
    j = nlohmann::json{
        {"id", train.get_id()},
        {"name", train.get_name()},
        {"type", train_type_to_string(train.get_type())},
        {"max_speed", train.get_max_speed()},
        {"acceleration", train.get_acceleration()},
        {"deceleration", train.get_deceleration()}
    };
}

void from_json(const nlohmann::json& j, Train& train) {
    std::string id = j.at("id").get<std::string>();
    std::string name = j.at("name").get<std::string>();
    std::string type_str = j.at("type").get<std::string>();
    double max_speed = j.at("max_speed").get<double>();
    double acceleration = j.at("acceleration").get<double>();
    double deceleration = j.at("deceleration").get<double>();
    
    TrainType type = string_to_train_type(type_str);
    
    train = Train(id, name, type, max_speed, acceleration, deceleration);
}

} // namespace fdc_scheduler
