#pragma once

#include "railway_network.hpp"
#include "schedule.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace fdc_scheduler {

/**
 * @brief RailML version to parse
 */
enum class RailMLVersion {
    VERSION_2,  ///< RailML 2.x (classic XML format)
    VERSION_3,  ///< RailML 3.x (new standard)
    AUTO_DETECT ///< Automatically detect version from file
};

/**
 * @brief Parser for RailML format files
 * 
 * Supports both RailML 2.x and 3.x formats.
 * Can import infrastructure, timetables, and rolling stock.
 */
class RailMLParser {
public:
    RailMLParser();
    ~RailMLParser();
    
    /**
     * @brief Parse RailML file into network and schedules
     * @param filename Path to RailML file
     * @param version RailML version (AUTO_DETECT by default)
     * @return True if parsing succeeded
     */
    bool parse_file(const std::string& filename, RailMLVersion version = RailMLVersion::AUTO_DETECT);
    
    /**
     * @brief Parse RailML from string
     * @param xml_content RailML XML content
     * @param version RailML version (AUTO_DETECT by default)
     * @return True if parsing succeeded
     */
    bool parse_string(const std::string& xml_content, RailMLVersion version = RailMLVersion::AUTO_DETECT);
    
    /**
     * @brief Get parsed network
     * @return Shared pointer to railway network
     */
    std::shared_ptr<RailwayNetwork> get_network() const { return network_; }
    
    /**
     * @brief Get parsed schedules
     * @return Vector of train schedules
     */
    const std::vector<std::shared_ptr<TrainSchedule>>& get_schedules() const { return schedules_; }
    
    /**
     * @brief Get last error message
     * @return Error description if parsing failed
     */
    std::string get_last_error() const { return last_error_; }
    
    /**
     * @brief Get parsing statistics
     * @return Map of statistic name to value
     */
    std::map<std::string, int> get_statistics() const;

private:
    std::shared_ptr<RailwayNetwork> network_;
    std::vector<std::shared_ptr<TrainSchedule>> schedules_;
    std::string last_error_;
    RailMLVersion detected_version_;
    
    // Statistics
    int stations_parsed_ = 0;
    int tracks_parsed_ = 0;
    int trains_parsed_ = 0;
    
    // Version detection
    RailMLVersion detect_version(const std::string& xml_content);
    
    // RailML 2.x parsing
    bool parse_railml2(const std::string& xml_content);
    bool parse_railml2_infrastructure(const std::string& xml_infrastructure);
    bool parse_railml2_timetable(const std::string& xml_timetable);
    
    // RailML 3.x parsing
    bool parse_railml3(const std::string& xml_content);
    bool parse_railml3_infrastructure(const std::string& xml_infrastructure);
    bool parse_railml3_timetable(const std::string& xml_timetable);
    
    // Helper methods
    void clear();
    void set_error(const std::string& error);
};

/**
 * @brief Convenience function to load network from RailML file
 * @param filename Path to RailML file
 * @param version RailML version (AUTO_DETECT by default)
 * @return Shared pointer to railway network (null if failed)
 */
std::shared_ptr<RailwayNetwork> load_railml_network(
    const std::string& filename,
    RailMLVersion version = RailMLVersion::AUTO_DETECT
);

/**
 * @brief Convenience function to load schedules from RailML file
 * @param filename Path to RailML file
 * @param network Network to associate schedules with
 * @param version RailML version (AUTO_DETECT by default)
 * @return Vector of train schedules (empty if failed)
 */
std::vector<std::shared_ptr<TrainSchedule>> load_railml_schedules(
    const std::string& filename,
    const RailwayNetwork& network,
    RailMLVersion version = RailMLVersion::AUTO_DETECT
);

} // namespace fdc_scheduler
