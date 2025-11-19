#include "fdc_scheduler/json_api.hpp"
#include "fdc_scheduler/track_type.hpp"
#include <sstream>
#include <iomanip>

namespace fdc_scheduler {
using json = nlohmann::json;


// Helper per convertire time_point in stringa ISO 8601
static std::string time_to_string(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// Helper per convertire stringa ISO 8601 in time_point
static std::chrono::system_clock::time_point parse_time(const std::string& time_str) {
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

JsonApi::JsonApi() 
    : network_(std::make_shared<RailwayNetwork>()) {
}

JsonApi::~JsonApi() = default;

// Network operations
std::string JsonApi::load_network(const std::string& path) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

std::string JsonApi::export_network(const std::string& path, const std::string& format) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

std::string JsonApi::get_network_info() {
    json response;
    response["status"] = "success";
    response["num_nodes"] = network_->num_nodes();
    response["num_edges"] = network_->num_edges();
    return response.dump(2);
}

std::string JsonApi::add_station(const std::string& node_json) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

std::string JsonApi::add_track_section(const std::string& edge_json) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

// Schedule operations
std::string JsonApi::add_train(const std::string& train_json) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

std::string JsonApi::get_all_trains() {
    json response;
    response["status"] = "success";
    response["trains"] = json::array();
    return response.dump(2);
}

std::string JsonApi::get_train(const std::string& train_id) {
    json response;
    response["status"] = "error";
    response["message"] = "Train not found";
    return response.dump(2);
}

std::string JsonApi::update_train(const std::string& train_id, const std::string& updates_json) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

std::string JsonApi::delete_train(const std::string& train_id) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

// Conflict detection
std::string JsonApi::detect_conflicts() {
    json response;
    response["status"] = "success";
    response["conflicts"] = json::array();
    response["total"] = 0;
    return response.dump(2);
}

std::string JsonApi::detect_conflicts_for_train(const std::string& train_id) {
    json response;
    response["status"] = "success";
    response["conflicts"] = json::array();
    response["total"] = 0;
    return response.dump(2);
}

std::string JsonApi::validate_schedule(const std::string& train_id) {
    json response;
    response["status"] = "success";
    response["valid"] = true;
    response["errors"] = json::array();
    return response.dump(2);
}

// Configuration
std::string JsonApi::get_config() {
    json response;
    response["status"] = "success";
    response["config"] = {
        {"section_buffer_seconds", 119},
        {"platform_buffer_seconds", 300},
        {"head_on_buffer_seconds", 600}
    };
    return response.dump(2);
}

std::string JsonApi::update_config(const std::string& config_json) {
    json response;
    response["status"] = "error";
    response["message"] = "Not implemented yet";
    return response.dump(2);
}

// Private helper methods
nlohmann::json JsonApi::node_to_json(const Node& node) const {
    json j;
    j["id"] = node.get_id();
    j["name"] = node.get_name();
    j["type"] = static_cast<int>(node.get_type());
    return j;
}

nlohmann::json JsonApi::edge_to_json(const Edge& edge) const {
    json j;
    j["from"] = edge.get_from_id();
    j["to"] = edge.get_to_id();
    j["distance"] = edge.get_distance();
    j["track_type"] = track_type_to_string(edge.get_track_type());
    return j;
}

nlohmann::json JsonApi::schedule_to_json(const TrainSchedule& schedule) const {
    json j;
    j["train_id"] = schedule.get_train_id();
    j["stops"] = json::array();
    return j;
}

nlohmann::json JsonApi::conflict_to_json(const Conflict& conflict) const {
    json j;
    j["type"] = static_cast<int>(conflict.type);
    j["train1_id"] = conflict.train1_id;
    j["train2_id"] = conflict.train2_id;
    j["severity"] = conflict.severity;
    j["description"] = conflict.description;
    return j;
}

} // namespace fdc_scheduler
