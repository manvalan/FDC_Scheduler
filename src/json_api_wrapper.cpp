#include <nlohmann/json.hpp>
#include "railway_network.hpp"
#include "schedule.hpp"
#include "train.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "node_type.hpp"
#include "track_type.hpp"
#include "train_type.hpp"
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

using json = nlohmann::json;
using namespace fdc;

namespace fdc_scheduler {

// Helper: time_point to ISO 8601 string
static std::string time_to_string(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// Helper: ISO 8601 string to time_point
static std::chrono::system_clock::time_point parse_time(const std::string& time_str) {
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

class JsonApiWrapper {
private:
    std::shared_ptr<RailwayNetwork> network_;
    std::map<std::string, std::shared_ptr<TrainSchedule>> schedules_;
    
    // Convert Node to JSON
    json node_to_json(const Node& node) const {
        json j;
        j["id"] = node.get_id();
        j["name"] = node.get_name();
        j["type"] = node_type_to_string(node.get_type());
        j["platforms"] = node.get_platforms();
        j["latitude"] = node.get_latitude();
        j["longitude"] = node.get_longitude();
        j["capacity"] = node.get_capacity();
        
        return j;
    }
    
    // Convert Edge to JSON
    json edge_to_json(const Edge& edge) const {
        json j;
        j["from"] = edge.get_from_node();
        j["to"] = edge.get_to_node();
        j["distance"] = edge.get_distance();
        j["max_speed"] = edge.get_max_speed();
        j["track_type"] = track_type_to_string(edge.get_track_type());
        j["bidirectional"] = edge.is_bidirectional();
        return j;
    }
    
    // Convert Schedule to JSON
    json schedule_to_json(const TrainSchedule& schedule) const {
        json j;
        j["train_id"] = schedule.get_train_id();
        j["schedule_id"] = schedule.get_schedule_id();
        
        json stops = json::array();
        for (const auto& stop : schedule.get_stops()) {
            json stop_json;
            stop_json["node_id"] = stop.get_node_id();
            stop_json["arrival"] = time_to_string(stop.get_arrival());
            stop_json["departure"] = time_to_string(stop.get_departure());
            
            auto platform = stop.get_platform();
            if (platform.has_value()) {
                stop_json["platform"] = platform.value();
            }
            
            stops.push_back(stop_json);
        }
        j["stops"] = stops;
        
        return j;
    }
    
    // Parse Node from JSON
    std::shared_ptr<Node> parse_node(const json& node_data) {
        auto node = std::make_shared<Node>(
            node_data["id"].get<std::string>(),
            node_data["name"].get<std::string>()
        );
        
        if (node_data.contains("type")) {
            node->set_type(string_to_node_type(node_data["type"].get<std::string>()));
        }
        
        if (node_data.contains("platforms")) {
            node->set_platforms(node_data["platforms"].get<int>());
        }
        
        if (node_data.contains("capacity")) {
            node->set_capacity(node_data["capacity"].get<int>());
        }
        
        if (node_data.contains("latitude") && node_data.contains("longitude")) {
            node->set_coordinates(
                node_data["latitude"].get<double>(),
                node_data["longitude"].get<double>()
            );
        }
        
        return node;
    }
    
    // Parse Edge from JSON
    std::shared_ptr<Edge> parse_edge(const json& edge_data) {
        auto edge = std::make_shared<Edge>(
            edge_data["from"].get<std::string>(),
            edge_data["to"].get<std::string>(),
            edge_data["distance"].get<double>()
        );
        
        if (edge_data.contains("max_speed")) {
            edge->set_max_speed(edge_data["max_speed"].get<double>());
        }
        
        if (edge_data.contains("track_type")) {
            edge->set_track_type(string_to_track_type(edge_data["track_type"].get<std::string>()));
        }
        
        if (edge_data.contains("bidirectional")) {
            edge->set_bidirectional(edge_data["bidirectional"].get<bool>());
        }
        
        return edge;
    }
    
    // Parse Schedule from JSON
    std::shared_ptr<TrainSchedule> parse_schedule(const json& schedule_data) {
        std::string train_id = schedule_data["train_id"].get<std::string>();
        std::string schedule_id = schedule_data.contains("schedule_id") ? 
            schedule_data["schedule_id"].get<std::string>() : train_id;
        
        auto schedule = std::make_shared<TrainSchedule>(
            train_id, schedule_id, network_
        );
        
        if (schedule_data.contains("stops")) {
            for (const auto& stop_data : schedule_data["stops"]) {
                auto arrival = parse_time(stop_data["arrival"].get<std::string>());
                auto departure = parse_time(stop_data["departure"].get<std::string>());
                
                int platform = stop_data.contains("platform") ? 
                              stop_data["platform"].get<int>() : 1;
                
                ScheduleStop stop(
                    stop_data["node_id"].get<std::string>(),
                    arrival,
                    departure,
                    platform
                );
                schedule->add_stop(stop);
            }
        }
        
        return schedule;
    }
    
public:
    JsonApiWrapper() : network_(std::make_shared<RailwayNetwork>()) {}
    
    // ========== NETWORK OPERATIONS ==========
    
    // Load network from JSON file
    std::string load_network(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                json response;
                response["status"] = "error";
                response["message"] = "Cannot open file: " + path;
                return response.dump(2);
            }
            
            json data = json::parse(file);
            network_ = std::make_shared<RailwayNetwork>();
            
            // Load nodes
            if (data.contains("network") && data["network"].contains("nodes")) {
                for (const auto& node_data : data["network"]["nodes"]) {
                    auto node = parse_node(node_data);
                    network_->add_node(*node);
                }
            }
            
            // Load edges
            if (data.contains("network") && data["network"].contains("edges")) {
                for (const auto& edge_data : data["network"]["edges"]) {
                    auto edge = parse_edge(edge_data);
                    network_->add_edge(*edge);
                }
            }
            
            // Load schedules
            schedules_.clear();
            if (data.contains("schedules")) {
                for (const auto& schedule_data : data["schedules"]) {
                    auto schedule = parse_schedule(schedule_data);
                    schedules_[schedule->get_train_id()] = schedule;
                }
            }
            
            json response;
            response["status"] = "success";
            response["message"] = "Network loaded successfully";
            response["nodes"] = network_->num_nodes();
            response["edges"] = network_->num_edges();
            response["trains"] = schedules_.size();
            return response.dump(2);
            
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = std::string("Load failed: ") + e.what();
            return response.dump(2);
        }
    }
    
    // Save network to JSON file
    std::string save_network(const std::string& path) {
        try {
            json data;
            
            // Save network
            json network;
            json nodes = json::array();
            for (const auto& node : network_->get_all_nodes()) {
                nodes.push_back(node_to_json(*node));
            }
            network["nodes"] = nodes;
            
            json edges = json::array();
            for (const auto& edge : network_->get_all_edges()) {
                edges.push_back(edge_to_json(*edge));
            }
            network["edges"] = edges;
            
            data["network"] = network;
            
            // Save schedules
            json schedules = json::array();
            for (const auto& [id, schedule] : schedules_) {
                schedules.push_back(schedule_to_json(*schedule));
            }
            data["schedules"] = schedules;
            
            // Write to file
            std::ofstream file(path);
            file << data.dump(2);
            file.close();
            
            json response;
            response["status"] = "success";
            response["message"] = "Network saved successfully";
            response["path"] = path;
            return response.dump(2);
            
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = std::string("Save failed: ") + e.what();
            return response.dump(2);
        }
    }
    
    // Get network info
    std::string get_network_info() {
        try {
            auto stats = network_->get_network_stats();
            
            json response;
            response["status"] = "success";
            response["version"] = "FDC_Scheduler 1.0 (using libfdc_core.a)";
            response["num_nodes"] = stats.num_nodes;
            response["num_edges"] = stats.num_edges;
            response["total_track_length_km"] = stats.total_track_length;
            response["average_edge_length_km"] = stats.average_edge_length;
            response["num_trains"] = schedules_.size();
            
            json track_types;
            track_types["single"] = stats.num_single_track;
            track_types["double"] = stats.num_double_track;
            track_types["high_speed"] = stats.num_high_speed;
            response["track_types"] = track_types;
            
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Add station
    std::string add_station(const std::string& node_json) {
        try {
            json node_data = json::parse(node_json);
            auto node = parse_node(node_data);
            
            if (network_->add_node(*node)) {
                json response;
                response["status"] = "success";
                response["message"] = "Station added";
                response["node_id"] = node->get_id();
                return response.dump(2);
            } else {
                json response;
                response["status"] = "error";
                response["message"] = "Failed to add station (maybe already exists)";
                return response.dump(2);
            }
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Add track section
    std::string add_track_section(const std::string& edge_json) {
        try {
            json edge_data = json::parse(edge_json);
            auto edge = parse_edge(edge_data);
            
            if (network_->add_edge(*edge)) {
                json response;
                response["status"] = "success";
                response["message"] = "Track section added";
                response["from"] = edge->get_from_node();
                response["to"] = edge->get_to_node();
                return response.dump(2);
            } else {
                json response;
                response["status"] = "error";
                response["message"] = "Failed to add track (maybe nodes don't exist)";
                return response.dump(2);
            }
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Get all nodes
    std::string get_all_nodes() {
        try {
            json nodes = json::array();
            for (const auto& node : network_->get_all_nodes()) {
                nodes.push_back(node_to_json(*node));
            }
            
            json response;
            response["status"] = "success";
            response["nodes"] = nodes;
            response["total"] = nodes.size();
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Get all edges
    std::string get_all_edges() {
        try {
            json edges = json::array();
            for (const auto& edge : network_->get_all_edges()) {
                edges.push_back(edge_to_json(*edge));
            }
            
            json response;
            response["status"] = "success";
            response["edges"] = edges;
            response["total"] = edges.size();
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // ========== SCHEDULE OPERATIONS ==========
    
    // Add train
    std::string add_train(const std::string& train_json) {
        try {
            json schedule_data = json::parse(train_json);
            auto schedule = parse_schedule(schedule_data);
            
            schedules_[schedule->get_train_id()] = schedule;
            
            json response;
            response["status"] = "success";
            response["message"] = "Train added";
            response["train_id"] = schedule->get_train_id();
            response["num_stops"] = schedule->get_stops().size();
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Get all trains
    std::string get_all_trains() {
        try {
            json trains = json::array();
            for (const auto& [id, schedule] : schedules_) {
                trains.push_back(schedule_to_json(*schedule));
            }
            
            json response;
            response["status"] = "success";
            response["trains"] = trains;
            response["total"] = trains.size();
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Get specific train
    std::string get_train(const std::string& train_id) {
        try {
            auto it = schedules_.find(train_id);
            if (it == schedules_.end()) {
                json response;
                response["status"] = "error";
                response["message"] = "Train not found: " + train_id;
                return response.dump(2);
            }
            
            json response;
            response["status"] = "success";
            response["train"] = schedule_to_json(*it->second);
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Delete train
    std::string delete_train(const std::string& train_id) {
        try {
            auto it = schedules_.find(train_id);
            if (it == schedules_.end()) {
                json response;
                response["status"] = "error";
                response["message"] = "Train not found: " + train_id;
                return response.dump(2);
            }
            
            schedules_.erase(it);
            
            json response;
            response["status"] = "success";
            response["message"] = "Train deleted";
            response["train_id"] = train_id;
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // ========== PATH FINDING ==========
    
    // Find shortest path
    std::string find_shortest_path(const std::string& from, const std::string& to) {
        try {
            auto path = network_->find_shortest_path(from, to);
            
            if (!path.is_valid()) {
                json response;
                response["status"] = "error";
                response["message"] = "No path found between " + from + " and " + to;
                return response.dump(2);
            }
            
            json response;
            response["status"] = "success";
            response["from"] = from;
            response["to"] = to;
            response["nodes"] = path.nodes;
            response["edges"] = path.edges;
            response["total_distance_km"] = path.total_distance;
            response["min_travel_time_hours"] = path.min_travel_time;
            
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Find k shortest paths (alternative routes)
    std::string find_k_shortest_paths(const std::string& from, const std::string& to, int k, bool use_distance = true) {
        try {
            if (k < 1 || k > 10) {
                json response;
                response["status"] = "error";
                response["message"] = "k must be between 1 and 10";
                return response.dump(2);
            }
            
            auto paths = network_->find_k_shortest_paths(from, to, k, use_distance);
            
            if (paths.empty()) {
                json response;
                response["status"] = "error";
                response["message"] = "No paths found between " + from + " and " + to;
                return response.dump(2);
            }
            
            json response;
            response["status"] = "success";
            response["from"] = from;
            response["to"] = to;
            response["requested"] = k;
            response["found"] = paths.size();
            response["metric"] = use_distance ? "distance" : "time";
            
            json paths_array = json::array();
            int rank = 1;
            for (const auto& path : paths) {
                json path_json;
                path_json["rank"] = rank++;
                path_json["nodes"] = path.nodes;
                path_json["edges"] = path.edges;
                path_json["total_distance_km"] = path.total_distance;
                path_json["min_travel_time_hours"] = path.min_travel_time;
                path_json["num_nodes"] = path.nodes.size();
                
                // Calculate difference from shortest
                if (rank == 2) {
                    path_json["delta_distance_km"] = 0.0;
                    path_json["delta_time_hours"] = 0.0;
                } else {
                    path_json["delta_distance_km"] = path.total_distance - paths[0].total_distance;
                    path_json["delta_time_hours"] = path.min_travel_time - paths[0].min_travel_time;
                }
                
                paths_array.push_back(path_json);
            }
            
            response["paths"] = paths_array;
            
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // ========== CONFLICT DETECTION ==========
    
    // Detect conflicts (stub - da implementare con conflict detector)
    std::string detect_conflicts() {
        json response;
        response["status"] = "success";
        response["total"] = 0;
        response["conflicts"] = json::array();
        response["message"] = "Conflict detection integration pending";
        return response.dump(2);
    }
    
    // Validate schedule
    std::string validate_schedule(const std::string& train_id) {
        try {
            if (train_id.empty()) {
                // Validate all
                json errors = json::array();
                int valid_count = 0;
                
                for (const auto& [id, schedule] : schedules_) {
                    if (schedule->is_valid()) {
                        valid_count++;
                    } else {
                        errors.push_back(id);
                    }
                }
                
                json response;
                response["status"] = "success";
                response["total_trains"] = schedules_.size();
                response["valid_trains"] = valid_count;
                response["invalid_trains"] = errors;
                return response.dump(2);
                
            } else {
                // Validate specific train
                auto it = schedules_.find(train_id);
                if (it == schedules_.end()) {
                    json response;
                    response["status"] = "error";
                    response["message"] = "Train not found: " + train_id;
                    return response.dump(2);
                }
                
                bool valid = it->second->is_valid();
                
                json response;
                response["status"] = "success";
                response["train_id"] = train_id;
                response["valid"] = valid;
                return response.dump(2);
            }
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // ========== RAILML EXPORT ==========
    
    std::string export_railml(const std::string& path, const std::string& version = "3.0") {
        json response;
        response["status"] = "success";
        response["message"] = "RailML export not yet implemented";
        response["version"] = version;
        response["path"] = path;
        return response.dump(2);
    }
};

} // namespace fdc_scheduler

// ========== C API ==========

extern "C" {
    void* fdc_scheduler_create() {
        return new fdc_scheduler::JsonApiWrapper();
    }
    
    void fdc_scheduler_destroy(void* api) {
        delete static_cast<fdc_scheduler::JsonApiWrapper*>(api);
    }
    
    const char* fdc_scheduler_load_network(void* api, const char* path) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->load_network(path);
        return result.c_str();
    }
    
    const char* fdc_scheduler_save_network(void* api, const char* path) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->save_network(path);
        return result.c_str();
    }
    
    const char* fdc_scheduler_get_network_info(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_network_info();
        return result.c_str();
    }
    
    const char* fdc_scheduler_add_station(void* api, const char* node_json) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->add_station(node_json);
        return result.c_str();
    }
    
    const char* fdc_scheduler_add_track(void* api, const char* edge_json) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->add_track_section(edge_json);
        return result.c_str();
    }
    
    const char* fdc_scheduler_get_all_nodes(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_all_nodes();
        return result.c_str();
    }
    
    const char* fdc_scheduler_get_all_edges(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_all_edges();
        return result.c_str();
    }
    
    const char* fdc_scheduler_add_train(void* api, const char* train_json) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->add_train(train_json);
        return result.c_str();
    }
    
    const char* fdc_scheduler_get_all_trains(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_all_trains();
        return result.c_str();
    }
    
    const char* fdc_scheduler_get_train(void* api, const char* train_id) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_train(train_id);
        return result.c_str();
    }
    
    const char* fdc_scheduler_delete_train(void* api, const char* train_id) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->delete_train(train_id);
        return result.c_str();
    }
    
    const char* fdc_scheduler_find_shortest_path(void* api, const char* from, const char* to) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->find_shortest_path(from, to);
        return result.c_str();
    }
    
    const char* fdc_scheduler_find_k_shortest_paths(void* api, const char* from, const char* to, int k, int use_distance) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->find_k_shortest_paths(from, to, k, use_distance != 0);
        return result.c_str();
    }
    
    const char* fdc_scheduler_detect_conflicts(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->detect_conflicts();
        return result.c_str();
    }
    
    const char* fdc_scheduler_validate_schedule(void* api, const char* train_id) {
        static std::string result;
        std::string id = train_id ? train_id : "";
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->validate_schedule(id);
        return result.c_str();
    }
    
    const char* fdc_scheduler_export_railml(void* api, const char* path, const char* version) {
        static std::string result;
        std::string ver = version ? version : "3.0";
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->export_railml(path, ver);
        return result.c_str();
    }
}
