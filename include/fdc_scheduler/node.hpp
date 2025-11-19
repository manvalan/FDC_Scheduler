#ifndef FDC_NODE_HPP
#define FDC_NODE_HPP

#include "node_type.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace fdc_scheduler {

/**
 * @brief Represents a time slot for platform reservation
 */
struct TimeSlot {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    std::string train_id;
    
    TimeSlot(std::chrono::system_clock::time_point s, 
             std::chrono::system_clock::time_point e,
             std::string tid)
        : start(s), end(e), train_id(std::move(tid)) {}
};

/**
 * @brief Represents a railway node (station, junction, etc.)
 * 
 * A node in the railway network graph. Can represent different types
 * of facilities (stations, junctions, depots, etc.) and manages
 * platform availability and capacity.
 */
class Node {
public:
    /**
     * @brief Construct a new Node
     * 
     * @param id Unique identifier for the node
     * @param name Human-readable name
     * @param type Type of node (station, junction, etc.)
     * @param latitude Geographic latitude (degrees)
     * @param longitude Geographic longitude (degrees)
     * @param capacity Maximum number of trains that can be at the node
     * @param platforms Number of platforms/tracks available
     */
    Node(std::string id, 
         std::string name,
         NodeType type = NodeType::STATION,
         double latitude = 0.0,
         double longitude = 0.0,
         int capacity = 1,
         int platforms = 1);

    // Getters
    const std::string& get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    NodeType get_type() const { return type_; }
    double get_latitude() const { return latitude_; }
    double get_longitude() const { return longitude_; }
    int get_capacity() const { return capacity_; }
    int get_platforms() const { return platforms_; }

    // Setters
    void set_name(const std::string& name) { name_ = name; }
    void set_type(NodeType type) { type_ = type; }
    void set_coordinates(double lat, double lon) { 
        latitude_ = lat; 
        longitude_ = lon; 
    }
    void set_capacity(int capacity) { capacity_ = capacity; }
    void set_platforms(int platforms) { platforms_ = platforms; }

    /**
     * @brief Check if a platform is available during a time window
     * 
     * @param platform Platform number (1-indexed)
     * @param start Start time
     * @param end End time
     * @return true if platform is available, false otherwise
     */
    bool is_platform_available(int platform,
                               std::chrono::system_clock::time_point start,
                               std::chrono::system_clock::time_point end) const;

    /**
     * @brief Get first available platform in time window
     * 
     * @param start Start time
     * @param end End time
     * @return Platform number if available, std::nullopt otherwise
     */
    std::optional<int> get_available_platform(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const;

    /**
     * @brief Reserve a platform for a train
     * 
     * @param platform Platform number
     * @param train_id Train identifier
     * @param start Start time
     * @param end End time
     * @return true if reservation successful, false if already occupied
     */
    bool reserve_platform(int platform,
                         const std::string& train_id,
                         std::chrono::system_clock::time_point start,
                         std::chrono::system_clock::time_point end);

    /**
     * @brief Release platform reservation for a train
     * 
     * @param platform Platform number
     * @param train_id Train identifier
     */
    void release_platform(int platform, const std::string& train_id);

    /**
     * @brief Clear all platform reservations
     */
    void clear_platform_schedule();

    /**
     * @brief Get all reservations for a platform
     */
    const std::vector<TimeSlot>& get_platform_schedule(int platform) const;

    /**
     * @brief Equality comparison
     */
    bool operator==(const Node& other) const { return id_ == other.id_; }
    bool operator!=(const Node& other) const { return !(*this == other); }

private:
    std::string id_;
    std::string name_;
    NodeType type_;
    double latitude_;
    double longitude_;
    int capacity_;
    int platforms_;
    
    // Platform schedule: platform_number -> list of time slots
    mutable std::map<int, std::vector<TimeSlot>> platform_schedule_;
};

/**
 * @brief JSON serialization for Node
 */
void to_json(nlohmann::json& j, const Node& node);

/**
 * @brief JSON deserialization for Node
 */
void from_json(const nlohmann::json& j, Node& node);

} // namespace fdc_scheduler

#endif // FDC_NODE_HPP
