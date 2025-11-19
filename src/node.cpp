#include "node.hpp"
#include <algorithm>

namespace fdc {

Node::Node(std::string id, 
           std::string name,
           NodeType type,
           double latitude,
           double longitude,
           int capacity,
           int platforms)
    : id_(std::move(id))
    , name_(std::move(name))
    , type_(type)
    , latitude_(latitude)
    , longitude_(longitude)
    , capacity_(capacity)
    , platforms_(platforms) {
}

bool Node::is_platform_available(
    int platform,
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    
    if (platform < 1 || platform > platforms_) {
        return false;
    }
    
    auto it = platform_schedule_.find(platform);
    if (it == platform_schedule_.end()) {
        return true; // No reservations on this platform
    }
    
    // Check for overlaps with existing reservations
    for (const auto& slot : it->second) {
        // Check if [start, end) overlaps with [slot.start, slot.end)
        if (start < slot.end && end > slot.start) {
            return false; // Overlap detected
        }
    }
    
    return true;
}

std::optional<int> Node::get_available_platform(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    
    for (int platform = 1; platform <= platforms_; ++platform) {
        if (is_platform_available(platform, start, end)) {
            return platform;
        }
    }
    
    return std::nullopt;
}

bool Node::reserve_platform(
    int platform,
    const std::string& train_id,
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
    
    if (!is_platform_available(platform, start, end)) {
        return false;
    }
    
    platform_schedule_[platform].emplace_back(start, end, train_id);
    
    // Keep schedule sorted by start time
    auto& schedule = platform_schedule_[platform];
    std::sort(schedule.begin(), schedule.end(),
              [](const TimeSlot& a, const TimeSlot& b) {
                  return a.start < b.start;
              });
    
    return true;
}

void Node::release_platform(int platform, const std::string& train_id) {
    auto it = platform_schedule_.find(platform);
    if (it == platform_schedule_.end()) {
        return;
    }
    
    auto& schedule = it->second;
    schedule.erase(
        std::remove_if(schedule.begin(), schedule.end(),
                      [&train_id](const TimeSlot& slot) {
                          return slot.train_id == train_id;
                      }),
        schedule.end());
}

void Node::clear_platform_schedule() {
    platform_schedule_.clear();
}

const std::vector<TimeSlot>& Node::get_platform_schedule(int platform) const {
    static const std::vector<TimeSlot> empty_schedule;
    
    auto it = platform_schedule_.find(platform);
    if (it == platform_schedule_.end()) {
        return empty_schedule;
    }
    
    return it->second;
}

// JSON serialization
void to_json(nlohmann::json& j, const Node& node) {
    j = nlohmann::json{
        {"id", node.get_id()},
        {"name", node.get_name()},
        {"type", node_type_to_string(node.get_type())},
        {"latitude", node.get_latitude()},
        {"longitude", node.get_longitude()},
        {"capacity", node.get_capacity()},
        {"platform_count", node.get_platforms()}
    };
}

void from_json(const nlohmann::json& j, Node& node) {
    std::string id = j.at("id").get<std::string>();
    std::string name = j.at("name").get<std::string>();
    std::string type_str = j.at("type").get<std::string>();
    double latitude = j.at("latitude").get<double>();
    double longitude = j.at("longitude").get<double>();
    int capacity = j.at("capacity").get<int>();
    
    // Handle both "platform_count" and "platforms" for backwards compatibility
    int platforms = 0;
    if (j.contains("platform_count")) {
        platforms = j.at("platform_count").get<int>();
    } else if (j.contains("platforms")) {
        platforms = j.at("platforms").get<int>();
    } else {
        // Default value based on node type
        NodeType type = string_to_node_type(type_str);
        if (type == NodeType::STATION || type == NodeType::INTERCHANGE) {
            platforms = 2;  // Default 2 platforms for stations
        }
    }
    
    NodeType type = string_to_node_type(type_str);
    
    node = Node(id, name, type, latitude, longitude, capacity, platforms);
}

} // namespace fdc
