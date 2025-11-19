#include "fdc_scheduler/edge.hpp"
#include <algorithm>
#include <cmath>

namespace fdc_scheduler {

Edge::Edge(std::string from_node_id,
           std::string to_node_id,
           double distance,
           TrackType track_type,
           double max_speed,
           int capacity,
           bool bidirectional)
    : from_node_id_(std::move(from_node_id))
    , to_node_id_(std::move(to_node_id))
    , distance_(distance)
    , track_type_(track_type)
    , max_speed_(max_speed)
    , capacity_(capacity)
    , bidirectional_(bidirectional) {
}

double Edge::calculate_travel_time(double train_speed) const {
    // Effective speed is the minimum of train speed and track max speed
    double effective_speed = std::min(train_speed, max_speed_);
    
    if (effective_speed <= 0) {
        return 0.0;
    }
    
    // Time = Distance / Speed (hours)
    return distance_ / effective_speed;
}

bool Edge::connects(const std::string& node1, const std::string& node2) const {
    return (from_node_id_ == node1 && to_node_id_ == node2) ||
           (bidirectional_ && from_node_id_ == node2 && to_node_id_ == node1);
}

std::string Edge::get_other_endpoint(const std::string& node_id) const {
    if (from_node_id_ == node_id) {
        return to_node_id_;
    } else if (bidirectional_ && to_node_id_ == node_id) {
        return from_node_id_;
    }
    return "";
}

// JSON serialization
void to_json(nlohmann::json& j, const Edge& edge) {
    j = nlohmann::json{
        {"from_node", edge.get_from_node()},
        {"to_node", edge.get_to_node()},
        {"distance", edge.get_distance()},
        {"track_type", track_type_to_string(edge.get_track_type())},
        {"max_speed", edge.get_max_speed()},
        {"capacity", edge.get_capacity()},
        {"bidirectional", edge.is_bidirectional()}
    };
}

void from_json(const nlohmann::json& j, Edge& edge) {
    std::string from_node = j.at("from_node").get<std::string>();
    std::string to_node = j.at("to_node").get<std::string>();
    double distance = j.at("distance").get<double>();
    std::string track_type_str = j.at("track_type").get<std::string>();
    double max_speed = j.at("max_speed").get<double>();
    int capacity = j.at("capacity").get<int>();
    bool bidirectional = j.at("bidirectional").get<bool>();
    
    TrackType track_type = string_to_track_type(track_type_str);
    
    edge = Edge(from_node, to_node, distance, track_type, max_speed, capacity, bidirectional);
}

} // namespace fdc_scheduler
