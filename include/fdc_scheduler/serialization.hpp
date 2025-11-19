#ifndef FDC_SERIALIZATION_HPP
#define FDC_SERIALIZATION_HPP

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include "railway_network.hpp"
#include "schedule.hpp"

namespace fdc_scheduler {

/**
 * @brief Serialize RailwayNetwork to JSON
 * 
 * Serializes the complete network including:
 * - All nodes with their properties
 * - All edges with track information
 * 
 * @param network The network to serialize
 * @return JSON object representing the network
 */
nlohmann::json railway_network_to_json(const RailwayNetwork& network);

/**
 * @brief Deserialize RailwayNetwork from JSON
 * 
 * Reconstructs a complete railway network from JSON data.
 * 
 * @param j JSON object containing network data
 * @return Shared pointer to reconstructed network
 */
std::shared_ptr<RailwayNetwork> railway_network_from_json(const nlohmann::json& j);

/**
 * @brief Save RailwayNetwork to JSON file
 * 
 * @param network Network to save
 * @param filename Output file path
 * @throws std::runtime_error if file cannot be written
 */
void save_network_to_file(const RailwayNetwork& network, const std::string& filename);

/**
 * @brief Load RailwayNetwork from JSON file
 * 
 * @param filename Input file path
 * @return Shared pointer to loaded network
 * @throws std::runtime_error if file cannot be read or JSON is invalid
 */
std::shared_ptr<RailwayNetwork> load_network_from_file(const std::string& filename);

/**
 * @brief Save schedules to JSON file
 * 
 * @param schedules Vector of schedules to save
 * @param filename Output file path
 * @throws std::runtime_error if file cannot be written
 */
void save_schedules_to_file(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const std::string& filename);

/**
 * @brief Load schedules from JSON file
 * 
 * @param filename Input file path
 * @param network Network reference for schedule construction
 * @return Vector of loaded schedules
 * @throws std::runtime_error if file cannot be read or JSON is invalid
 */
std::vector<std::shared_ptr<TrainSchedule>> load_schedules_from_file(
    const std::string& filename,
    std::shared_ptr<RailwayNetwork> network);

} // namespace fdc_scheduler

#endif // FDC_SERIALIZATION_HPP
