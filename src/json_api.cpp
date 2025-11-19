#include "fdc_scheduler/json_api.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace fdc_scheduler {

JsonApi::JsonApi() 
    : network_(std::make_shared<RailwayNetwork>()),
      conflict_detector_(std::make_unique<ConflictDetector>(*network_)) {}

// Network operations

std::string JsonApi::load_network(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return create_error_response("Failed to open file: " + path);
        }
        
        json j;
        file >> j;
        
        // Load nodes
        if (j.contains("nodes")) {
            for (const auto& node_data : j["nodes"]) {
                auto node = std::make_shared<Node>(
                    node_data["id"].get<std::string>(),
                    node_data.value("name", ""),
                    node_data.value("type", "station")
                );
                
                if (node_data.contains("platforms")) {
                    node->set_platforms(node_data["platforms"].get<int>());
                }
                if (node_data.contains("latitude") && node_data.contains("longitude")) {
                    node->set_coordinates(
                        node_data["latitude"].get<double>(),
                        node_data["longitude"].get<double>()
                    );
                }
                
                network_->add_node(node);
            }
        }
        
        // Load edges
        if (j.contains("edges")) {
            for (const auto& edge_data : j["edges"]) {
                auto edge = std::make_shared<Edge>(
                    edge_data["from"].get<std::string>(),
                    edge_data["to"].get<std::string>(),
                    edge_data["distance"].get<double>()
                );
                
                if (edge_data.contains("max_speed")) {
                    edge->set_max_speed(edge_data["max_speed"].get<double>());
                }
                if (edge_data.contains("track_type")) {
                    edge->set_track_type(edge_data["track_type"].get<std::string>());
                }
                if (edge_data.contains("bidirectional")) {
                    edge->set_bidirectional(edge_data["bidirectional"].get<bool>());
                }
                
                network_->add_edge(edge);
            }
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Network loaded successfully";
        response["network"] = {
            {"nodes", network_->get_nodes().size()},
            {"edges", network_->get_edges().size()},
            {"total_length_km", network_->get_total_length()}
        };
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error loading network: ") + e.what());
    }
}

std::string JsonApi::export_network(const std::string& path, const std::string& format) {
    try {
        json j;
        
        // Export nodes
        json nodes_array = json::array();
        for (const auto& node : network_->get_nodes()) {
            json node_data;
            node_data["id"] = node->get_id();
            node_data["name"] = node->get_name();
            node_data["type"] = node->get_type();
            node_data["platforms"] = node->get_platforms();
            
            auto coords = node->get_coordinates();
            if (coords.first != 0.0 || coords.second != 0.0) {
                node_data["latitude"] = coords.first;
                node_data["longitude"] = coords.second;
            }
            
            nodes_array.push_back(node_data);
        }
        j["nodes"] = nodes_array;
        
        // Export edges
        json edges_array = json::array();
        for (const auto& edge : network_->get_edges()) {
            json edge_data;
            edge_data["from"] = edge->get_from();
            edge_data["to"] = edge->get_to();
            edge_data["distance"] = edge->get_distance();
            edge_data["max_speed"] = edge->get_max_speed();
            edge_data["track_type"] = edge->get_track_type();
            edge_data["bidirectional"] = edge->is_bidirectional();
            
            edges_array.push_back(edge_data);
        }
        j["edges"] = edges_array;
        
        // Write to file
        std::ofstream file(path);
        if (!file.is_open()) {
            return create_error_response("Failed to open file for writing: " + path);
        }
        
        file << j.dump(2);
        file.close();
        
        json response;
        response["success"] = true;
        response["message"] = "Network exported successfully";
        response["path"] = path;
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error exporting network: ") + e.what());
    }
}

std::string JsonApi::get_network_info() {
    try {
        json response;
        response["success"] = true;
        response["network"] = {
            {"nodes", network_->get_nodes().size()},
            {"edges", network_->get_edges().size()},
            {"total_length_km", network_->get_total_length()}
        };
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error getting network info: ") + e.what());
    }
}

std::string JsonApi::add_station(const std::string& node_json) {
    try {
        json j = json::parse(node_json);
        
        auto node = std::make_shared<Node>(
            j["id"].get<std::string>(),
            j.value("name", ""),
            j.value("type", "station")
        );
        
        if (j.contains("platforms")) {
            node->set_platforms(j["platforms"].get<int>());
        }
        if (j.contains("latitude") && j.contains("longitude")) {
            node->set_coordinates(
                j["latitude"].get<double>(),
                j["longitude"].get<double>()
            );
        }
        
        network_->add_node(node);
        
        json response;
        response["success"] = true;
        response["message"] = "Station added successfully";
        response["node_id"] = node->get_id();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error adding station: ") + e.what());
    }
}

std::string JsonApi::add_track_section(const std::string& edge_json) {
    try {
        json j = json::parse(edge_json);
        
        auto edge = std::make_shared<Edge>(
            j["from"].get<std::string>(),
            j["to"].get<std::string>(),
            j["distance"].get<double>()
        );
        
        if (j.contains("max_speed")) {
            edge->set_max_speed(j["max_speed"].get<double>());
        }
        if (j.contains("track_type")) {
            edge->set_track_type(j["track_type"].get<std::string>());
        }
        if (j.contains("bidirectional")) {
            edge->set_bidirectional(j["bidirectional"].get<bool>());
        }
        
        network_->add_edge(edge);
        
        json response;
        response["success"] = true;
        response["message"] = "Track section added successfully";
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error adding track section: ") + e.what());
    }
}

// Schedule operations

std::string JsonApi::add_train(const std::string& train_json) {
    try {
        json j = json::parse(train_json);
        
        auto schedule = std::make_shared<TrainSchedule>(
            j["train_id"].get<std::string>(),
            j.value("train_type", "Regional")
        );
        
        if (j.contains("stops")) {
            for (const auto& stop_data : j["stops"]) {
                TrainStop stop;
                stop.node_id = stop_data["node_id"].get<std::string>();
                
                // Parse times (format: "HH:MM")
                if (stop_data.contains("arrival")) {
                    stop.arrival = parse_time(stop_data["arrival"].get<std::string>());
                }
                if (stop_data.contains("departure")) {
                    stop.departure = parse_time(stop_data["departure"].get<std::string>());
                }
                
                if (stop_data.contains("platform")) {
                    stop.platform = stop_data["platform"].get<int>();
                }
                
                schedule->add_stop(stop);
            }
        }
        
        schedules_.push_back(schedule);
        
        json response;
        response["success"] = true;
        response["message"] = "Train added successfully";
        response["train_id"] = schedule->get_train_id();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error adding train: ") + e.what());
    }
}

std::string JsonApi::get_all_trains() {
    try {
        json trains_array = json::array();
        
        for (const auto& schedule : schedules_) {
            json train_data = schedule_to_json(schedule);
            trains_array.push_back(train_data);
        }
        
        json response;
        response["trains"] = trains_array;
        response["total"] = schedules_.size();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error getting trains: ") + e.what());
    }
}

std::string JsonApi::get_train(const std::string& train_id) {
    try {
        for (const auto& schedule : schedules_) {
            if (schedule->get_train_id() == train_id) {
                json response;
                response["success"] = true;
                response["train"] = schedule_to_json(schedule);
                
                return response.dump(2);
            }
        }
        
        return create_error_response("Train not found: " + train_id);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error getting train: ") + e.what());
    }
}

std::string JsonApi::update_train(const std::string& train_id, const std::string& updates_json) {
    try {
        // Find train
        std::shared_ptr<TrainSchedule> schedule = nullptr;
        for (auto& s : schedules_) {
            if (s->get_train_id() == train_id) {
                schedule = s;
                break;
            }
        }
        
        if (!schedule) {
            return create_error_response("Train not found: " + train_id);
        }
        
        json j = json::parse(updates_json);
        
        // Update stops if provided
        if (j.contains("stops")) {
            schedule->clear_stops();
            for (const auto& stop_data : j["stops"]) {
                TrainStop stop;
                stop.node_id = stop_data["node_id"].get<std::string>();
                
                if (stop_data.contains("arrival")) {
                    stop.arrival = parse_time(stop_data["arrival"].get<std::string>());
                }
                if (stop_data.contains("departure")) {
                    stop.departure = parse_time(stop_data["departure"].get<std::string>());
                }
                if (stop_data.contains("platform")) {
                    stop.platform = stop_data["platform"].get<int>();
                }
                
                schedule->add_stop(stop);
            }
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Train updated successfully";
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error updating train: ") + e.what());
    }
}

std::string JsonApi::delete_train(const std::string& train_id) {
    try {
        auto it = std::remove_if(schedules_.begin(), schedules_.end(),
            [&train_id](const std::shared_ptr<TrainSchedule>& s) {
                return s->get_train_id() == train_id;
            });
        
        if (it == schedules_.end()) {
            return create_error_response("Train not found: " + train_id);
        }
        
        schedules_.erase(it, schedules_.end());
        
        json response;
        response["success"] = true;
        response["message"] = "Train deleted successfully";
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error deleting train: ") + e.what());
    }
}

// Conflict detection

std::string JsonApi::detect_conflicts() {
    try {
        auto conflicts = conflict_detector_->detect_all(schedules_);
        
        json conflicts_array = json::array();
        std::map<std::string, int> by_type;
        
        for (const auto& conflict : conflicts) {
            conflicts_array.push_back(conflict_to_json(conflict));
            
            std::string type_str = conflict_type_to_string(conflict.type);
            by_type[type_str]++;
        }
        
        json response;
        response["conflicts"] = conflicts_array;
        response["total"] = conflicts.size();
        response["by_type"] = by_type;
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error detecting conflicts: ") + e.what());
    }
}

std::string JsonApi::detect_conflicts_for_train(const std::string& train_id) {
    try {
        // Find train
        std::shared_ptr<TrainSchedule> schedule = nullptr;
        for (const auto& s : schedules_) {
            if (s->get_train_id() == train_id) {
                schedule = s;
                break;
            }
        }
        
        if (!schedule) {
            return create_error_response("Train not found: " + train_id);
        }
        
        auto conflicts = conflict_detector_->detect_for_train(schedule, schedules_);
        
        json conflicts_array = json::array();
        for (const auto& conflict : conflicts) {
            conflicts_array.push_back(conflict_to_json(conflict));
        }
        
        json response;
        response["train_id"] = train_id;
        response["conflicts"] = conflicts_array;
        response["total"] = conflicts.size();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error detecting conflicts: ") + e.what());
    }
}

std::string JsonApi::validate_schedule(const std::string& train_id) {
    try {
        auto conflicts = conflict_detector_->detect_all(schedules_);
        auto violations = conflict_detector_->validate_timing(schedules_);
        
        json response;
        response["valid"] = (conflicts.empty() && violations.empty());
        
        json conflicts_array = json::array();
        for (const auto& conflict : conflicts) {
            conflicts_array.push_back(conflict_to_json(conflict));
        }
        response["conflicts"] = conflicts_array;
        
        json violations_array = json::array();
        for (const auto& violation : violations) {
            violations_array.push_back(conflict_to_json(violation));
        }
        response["violations"] = violations_array;
        
        response["total_issues"] = conflicts.size() + violations.size();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error validating schedule: ") + e.what());
    }
}

// Configuration

std::string JsonApi::get_config() {
    try {
        const auto& config = conflict_detector_->get_config();
        
        json response;
        response["section_buffer_seconds"] = config.section_buffer_seconds;
        response["platform_buffer_seconds"] = config.platform_buffer_seconds;
        response["head_on_buffer_seconds"] = config.head_on_buffer_seconds;
        response["detect_section_conflicts"] = config.detect_section_conflicts;
        response["detect_platform_conflicts"] = config.detect_platform_conflicts;
        response["detect_head_on_collisions"] = config.detect_head_on_collisions;
        response["detect_timing_violations"] = config.detect_timing_violations;
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error getting config: ") + e.what());
    }
}

std::string JsonApi::set_config(const std::string& config_json) {
    try {
        json j = json::parse(config_json);
        
        ConflictDetectorConfig config = conflict_detector_->get_config();
        
        if (j.contains("section_buffer_seconds")) {
            config.section_buffer_seconds = j["section_buffer_seconds"].get<int>();
        }
        if (j.contains("platform_buffer_seconds")) {
            config.platform_buffer_seconds = j["platform_buffer_seconds"].get<int>();
        }
        if (j.contains("head_on_buffer_seconds")) {
            config.head_on_buffer_seconds = j["head_on_buffer_seconds"].get<int>();
        }
        if (j.contains("detect_section_conflicts")) {
            config.detect_section_conflicts = j["detect_section_conflicts"].get<bool>();
        }
        if (j.contains("detect_platform_conflicts")) {
            config.detect_platform_conflicts = j["detect_platform_conflicts"].get<bool>();
        }
        if (j.contains("detect_head_on_collisions")) {
            config.detect_head_on_collisions = j["detect_head_on_collisions"].get<bool>();
        }
        if (j.contains("detect_timing_violations")) {
            config.detect_timing_violations = j["detect_timing_violations"].get<bool>();
        }
        
        conflict_detector_->set_config(config);
        
        json response;
        response["success"] = true;
        response["message"] = "Configuration updated successfully";
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error setting config: ") + e.what());
    }
}

std::string JsonApi::get_statistics() {
    try {
        auto stats = conflict_detector_->get_statistics();
        
        json response;
        for (const auto& [key, value] : stats) {
            response[key] = value;
        }
        response["trains_loaded"] = schedules_.size();
        response["nodes_loaded"] = network_->get_nodes().size();
        response["edges_loaded"] = network_->get_edges().size();
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error getting statistics: ") + e.what());
    }
}

std::string JsonApi::reset() {
    try {
        schedules_.clear();
        network_ = std::make_shared<RailwayNetwork>();
        conflict_detector_ = std::make_unique<ConflictDetector>(*network_);
        
        json response;
        response["success"] = true;
        response["message"] = "API reset successfully";
        
        return response.dump(2);
        
    } catch (const std::exception& e) {
        return create_error_response(std::string("Error resetting: ") + e.what());
    }
}

// Helper methods

std::string JsonApi::create_error_response(const std::string& error_message) {
    json response;
    response["success"] = false;
    response["error"] = error_message;
    return response.dump(2);
}

json JsonApi::schedule_to_json(const std::shared_ptr<TrainSchedule>& schedule) {
    json train_data;
    train_data["train_id"] = schedule->get_train_id();
    train_data["train_type"] = schedule->get_train_type();
    
    json stops_array = json::array();
    for (const auto& stop : schedule->get_stops()) {
        json stop_data;
        stop_data["node_id"] = stop.node_id;
        stop_data["arrival"] = format_time(stop.arrival);
        stop_data["departure"] = format_time(stop.departure);
        stop_data["platform"] = stop.platform;
        
        stops_array.push_back(stop_data);
    }
    train_data["stops"] = stops_array;
    
    return train_data;
}

json JsonApi::conflict_to_json(const Conflict& conflict) {
    json conflict_data;
    conflict_data["type"] = conflict_type_to_string(conflict.type);
    conflict_data["train1"] = conflict.train1_id;
    if (!conflict.train2_id.empty()) {
        conflict_data["train2"] = conflict.train2_id;
    }
    conflict_data["location"] = conflict.location;
    conflict_data["time"] = format_time(conflict.conflict_time);
    conflict_data["description"] = conflict.description;
    conflict_data["severity"] = conflict.severity;
    
    if (conflict.platform > 0) {
        conflict_data["platform"] = conflict.platform;
    }
    if (!conflict.section_from.empty()) {
        conflict_data["section_from"] = conflict.section_from;
        conflict_data["section_to"] = conflict.section_to;
    }
    
    return conflict_data;
}

std::string JsonApi::conflict_type_to_string(ConflictType type) {
    switch (type) {
        case ConflictType::SECTION_OVERLAP:
            return "section_overlap";
        case ConflictType::PLATFORM_CONFLICT:
            return "platform_conflict";
        case ConflictType::HEAD_ON_COLLISION:
            return "head_on_collision";
        case ConflictType::TIMING_VIOLATION:
            return "timing_violation";
        default:
            return "unknown";
    }
}

std::chrono::system_clock::time_point JsonApi::parse_time(const std::string& time_str) {
    // Parse "HH:MM" format
    int hour, minute;
    sscanf(time_str.c_str(), "%d:%d", &hour, &minute);
    
    // Create time point (using today's date as base)
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&tt);
    
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string JsonApi::format_time(std::chrono::system_clock::time_point time_point) {
    auto tt = std::chrono::system_clock::to_time_t(time_point);
    auto tm = *std::localtime(&tt);
    
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", tm.tm_hour, tm.tm_min);
    
    return std::string(buffer);
}

} // namespace fdc_scheduler
