#include <nlohmann/json.hpp>
#include "railway_network.hpp"
#include "schedule.hpp"
#include "train.hpp"
#include <memory>
#include <string>
#include <fstream>

using json = nlohmann::json;
using namespace fdc;

namespace fdc_scheduler {

class JsonApiWrapper {
private:
    std::shared_ptr<RailwayNetwork> network_;
    std::vector<std::shared_ptr<TrainSchedule>> schedules_;
    
public:
    JsonApiWrapper() : network_(std::make_shared<RailwayNetwork>()) {}
    
    // Load network from JSON file
    std::string load_network(const std::string& path) {
        try {
            std::ifstream file(path);
            json data = json::parse(file);
            
            network_ = std::make_shared<RailwayNetwork>();
            
            json response;
            response["status"] = "success";
            response["message"] = "Network loading from JSON not yet implemented";
            response["nodes"] = network_->num_nodes();
            response["edges"] = network_->num_edges();
            return response.dump(2);
        } catch (const std::exception& e) {
            json response;
            response["status"] = "error";
            response["message"] = e.what();
            return response.dump(2);
        }
    }
    
    // Get network info
    std::string get_network_info() {
        json response;
        response["status"] = "success";
        response["num_nodes"] = network_->num_nodes();
        response["num_edges"] = network_->num_edges();
        response["message"] = "Using FDC core library v1.0";
        return response.dump(2);
    }
    
    // Detect conflicts (basic stub - FDC core may have conflict detection)
    std::string detect_conflicts() {
        json response;
        response["status"] = "success";
        response["total"] = 0;
        response["conflicts"] = json::array();
        response["message"] = "Conflict detection to be implemented";
        return response.dump(2);
    }
    
    // Export to RailML (stub)
    std::string export_railml(const std::string& path, const std::string& version = "3.0") {
        json response;
        response["status"] = "success";
        response["message"] = "RailML export not yet implemented";
        response["version"] = version;
        return response.dump(2);
    }
};

} // namespace fdc_scheduler

// C API for easier binding
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
    
    const char* fdc_scheduler_get_network_info(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->get_network_info();
        return result.c_str();
    }
    
    const char* fdc_scheduler_detect_conflicts(void* api) {
        static std::string result;
        result = static_cast<fdc_scheduler::JsonApiWrapper*>(api)->detect_conflicts();
        return result.c_str();
    }
}
