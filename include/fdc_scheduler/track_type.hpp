#ifndef FDC_TRACK_TYPE_HPP
#define FDC_TRACK_TYPE_HPP

#include <string>
#include <stdexcept>

namespace fdc_scheduler {

/**
 * @brief Enumeration representing different types of railway tracks
 */
enum class TrackType {
    SINGLE,      ///< Single track (one direction at a time)
    DOUBLE,      ///< Double track (bidirectional, separate tracks)
    HIGH_SPEED,  ///< High-speed rail track
    FREIGHT      ///< Freight-only track
};

/**
 * @brief Convert TrackType to string representation
 */
inline std::string track_type_to_string(TrackType type) {
    switch (type) {
        case TrackType::SINGLE:     return "single";
        case TrackType::DOUBLE:     return "double";
        case TrackType::HIGH_SPEED: return "high_speed";
        case TrackType::FREIGHT:    return "freight";
        default: throw std::invalid_argument("Unknown TrackType");
    }
}

/**
 * @brief Convert string to TrackType
 */
inline TrackType string_to_track_type(const std::string& str) {
    if (str == "single")     return TrackType::SINGLE;
    if (str == "double")     return TrackType::DOUBLE;
    if (str == "high_speed") return TrackType::HIGH_SPEED;
    if (str == "freight")    return TrackType::FREIGHT;
    throw std::invalid_argument("Unknown TrackType string: " + str);
}

} // namespace fdc_scheduler

#endif // FDC_TRACK_TYPE_HPP
