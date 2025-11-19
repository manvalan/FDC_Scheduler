#include <fdc_scheduler.hpp>
#include <iostream>

using namespace fdc_scheduler;

int main() {
    std::cout << "FDC_Scheduler JSON API Example\n";
    std::cout << "Version: " << Version::to_string() << "\n\n";
    
    // Create JSON API instance
    JsonApi api;
    
    // ========================================================================
    // 1. ADD STATIONS
    // ========================================================================
    std::cout << "=== Adding Stations ===\n";
    
    std::string milano_json = R"({
        "id": "MILANO",
        "name": "Milano Centrale",
        "type": "station",
        "platforms": 12,
        "latitude": 45.4865,
        "longitude": 9.2041
    })";
    
    std::string result = api.add_station(milano_json);
    std::cout << "Add Milano: " << result << "\n";
    
    std::string como_json = R"({
        "id": "COMO",
        "name": "Como San Giovanni",
        "type": "station",
        "platforms": 3,
        "latitude": 45.8081,
        "longitude": 9.0852
    })";
    
    result = api.add_station(como_json);
    std::cout << "Add Como: " << result << "\n\n";
    
    // ========================================================================
    // 2. ADD TRACK SECTION
    // ========================================================================
    std::cout << "=== Adding Track Section ===\n";
    
    std::string track_json = R"({
        "from": "MILANO",
        "to": "COMO",
        "distance": 45.0,
        "max_speed": 140.0,
        "track_type": "double",
        "bidirectional": true
    })";
    
    result = api.add_track_section(track_json);
    std::cout << "Add track: " << result << "\n\n";
    
    // ========================================================================
    // 3. GET NETWORK INFO
    // ========================================================================
    std::cout << "=== Network Information ===\n";
    std::string network_info = api.get_network_info();
    std::cout << network_info << "\n\n";
    
    // ========================================================================
    // 4. ADD TRAINS
    // ========================================================================
    std::cout << "=== Adding Trains ===\n";
    
    std::string ic101_json = R"({
        "train_id": "IC101",
        "train_type": "InterCity",
        "stops": [
            {
                "node_id": "MILANO",
                "arrival": "08:00",
                "departure": "08:00",
                "platform": 1
            },
            {
                "node_id": "COMO",
                "arrival": "08:30",
                "departure": "08:30",
                "platform": 1
            }
        ]
    })";
    
    result = api.add_train(ic101_json);
    std::cout << "Add IC101: " << result << "\n";
    
    std::string r205_json = R"({
        "train_id": "R205",
        "train_type": "Regionale",
        "stops": [
            {
                "node_id": "COMO",
                "arrival": "08:25",
                "departure": "08:25",
                "platform": 1
            },
            {
                "node_id": "MILANO",
                "arrival": "09:10",
                "departure": "09:10",
                "platform": 3
            }
        ]
    })";
    
    result = api.add_train(r205_json);
    std::cout << "Add R205: " << result << "\n\n";
    
    // ========================================================================
    // 5. LIST ALL TRAINS
    // ========================================================================
    std::cout << "=== All Trains ===\n";
    std::string all_trains = api.get_all_trains();
    std::cout << all_trains << "\n\n";
    
    // ========================================================================
    // 6. DETECT CONFLICTS
    // ========================================================================
    std::cout << "=== Conflict Detection ===\n";
    std::string conflicts = api.detect_conflicts();
    std::cout << conflicts << "\n\n";
    
    // ========================================================================
    // 7. VALIDATE SCHEDULE
    // ========================================================================
    std::cout << "=== Schedule Validation ===\n";
    std::string validation = api.validate_schedule();
    std::cout << validation << "\n\n";
    
    // ========================================================================
    // 8. GET STATISTICS
    // ========================================================================
    std::cout << "=== Library Statistics ===\n";
    std::string stats = api.get_statistics();
    std::cout << stats << "\n\n";
    
    std::cout << "JSON API example completed successfully!\n";
    return 0;
}
