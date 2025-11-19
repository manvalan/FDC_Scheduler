#ifndef FDC_TRAIN_TYPE_HPP
#define FDC_TRAIN_TYPE_HPP

#include <string>
#include <stdexcept>

namespace fdc_scheduler {

/**
 * @brief Enumeration representing different types of trains
 */
enum class TrainType {
    REGIONAL,      ///< Regional train (slow, stops at all stations)
    INTERCITY,     ///< InterCity train (medium speed, major stations)
    HIGH_SPEED,    ///< High-speed train (fast, limited stops)
    FREIGHT        ///< Freight train (slow, cargo)
};

/**
 * @brief Convert TrainType to string representation
 */
inline std::string train_type_to_string(TrainType type) {
    switch (type) {
        case TrainType::REGIONAL:   return "regional";
        case TrainType::INTERCITY:  return "intercity";
        case TrainType::HIGH_SPEED: return "high_speed";
        case TrainType::FREIGHT:    return "freight";
        default: throw std::invalid_argument("Unknown TrainType");
    }
}

/**
 * @brief Convert string to TrainType
 */
inline TrainType string_to_train_type(const std::string& str) {
    if (str == "regional")   return TrainType::REGIONAL;
    if (str == "intercity")  return TrainType::INTERCITY;
    if (str == "high_speed") return TrainType::HIGH_SPEED;
    if (str == "freight")    return TrainType::FREIGHT;
    throw std::invalid_argument("Unknown TrainType string: " + str);
}

} // namespace fdc_scheduler

#endif // FDC_TRAIN_TYPE_HPP
