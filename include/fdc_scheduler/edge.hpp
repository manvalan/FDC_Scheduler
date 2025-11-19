#ifndef FDC_EDGE_HPP
#define FDC_EDGE_HPP

#include "track_type.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace fdc_scheduler {

/**
 * @brief Represents a railway edge (track connection between nodes)
 * 
 * An edge in the railway network graph representing a physical
 * track connection between two nodes. Contains information about
 * track type, distance, speed limits, and capacity.
 */
class Edge {
public:
    /**
     * @brief Construct a new Edge
     * 
     * @param from_node_id ID of the source node
     * @param to_node_id ID of the destination node
     * @param distance Distance in kilometers
     * @param track_type Type of track (single, double, high-speed, freight)
     * @param max_speed Maximum speed in km/h
     * @param capacity Maximum number of trains on track simultaneously
     * @param bidirectional Whether track can be traversed in both directions
     */
    Edge(std::string from_node_id,
         std::string to_node_id,
         double distance,
         TrackType track_type = TrackType::SINGLE,
         double max_speed = 120.0,
         int capacity = 1,
         bool bidirectional = true);

    // Getters
    const std::string& get_from_node() const { return from_node_id_; }
    const std::string& get_to_node() const { return to_node_id_; }
    double get_distance() const { return distance_; }
    TrackType get_track_type() const { return track_type_; }
    double get_max_speed() const { return max_speed_; }
    int get_capacity() const { return capacity_; }
    bool is_bidirectional() const { return bidirectional_; }

    // Setters
    void set_distance(double distance) { distance_ = distance; }
    void set_track_type(TrackType type) { track_type_ = type; }
    void set_max_speed(double speed) { max_speed_ = speed; }
    void set_capacity(int capacity) { capacity_ = capacity; }
    void set_bidirectional(bool bidirectional) { bidirectional_ = bidirectional; }

    /**
     * @brief Calculate travel time for this edge
     * 
     * @param train_speed Maximum speed of the train (km/h)
     * @return Travel time in hours
     */
    double calculate_travel_time(double train_speed) const;

    /**
     * @brief Check if edge connects two specific nodes (in any direction)
     */
    bool connects(const std::string& node1, const std::string& node2) const;

    /**
     * @brief Get the other endpoint of the edge
     * 
     * @param node_id One endpoint
     * @return The other endpoint, or empty string if node_id not found
     */
    std::string get_other_endpoint(const std::string& node_id) const;

private:
    std::string from_node_id_;
    std::string to_node_id_;
    double distance_;        // km
    TrackType track_type_;
    double max_speed_;       // km/h
    int capacity_;
    bool bidirectional_;
};

/**
 * @brief JSON serialization for Edge
 */
void to_json(nlohmann::json& j, const Edge& edge);

/**
 * @brief JSON deserialization for Edge
 */
void from_json(const nlohmann::json& j, Edge& edge);

} // namespace fdc_scheduler

#endif // FDC_EDGE_HPP
