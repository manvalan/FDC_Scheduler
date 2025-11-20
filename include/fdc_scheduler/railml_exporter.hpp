#pragma once

#include "railway_network.hpp"
#include "schedule.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace fdc_scheduler {

/**
 * @brief RailML version to export
 */
enum class RailMLExportVersion {
    VERSION_2,  ///< RailML 2.x (classic XML format)
    VERSION_3   ///< RailML 3.x (new standard)
};

/**
 * @brief Export options for RailML files
 */
struct RailMLExportOptions {
    bool pretty_print = true;           ///< Format XML with indentation
    bool include_metadata = true;       ///< Include FDC_Scheduler metadata
    bool export_infrastructure = true;  ///< Export network infrastructure
    bool export_timetable = true;       ///< Export train schedules
    bool export_rolling_stock = false;  ///< Export rolling stock definitions
    std::string infrastructure_id = ""; ///< Custom infrastructure ID
    std::string timetable_id = "";      ///< Custom timetable ID
};

/**
 * @brief Exporter for RailML format files
 * 
 * Supports both RailML 2.x and 3.x formats.
 * Can export infrastructure, timetables, and rolling stock.
 */
class RailMLExporter {
public:
    RailMLExporter();
    ~RailMLExporter();
    
    /**
     * @brief Export network and schedules to RailML file
     * @param filename Output file path
     * @param network Railway network to export
     * @param schedules Train schedules to export
     * @param version RailML version to use
     * @param options Export options
     * @return True if export succeeded
     */
    bool export_to_file(
        const std::string& filename,
        const RailwayNetwork& network,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        RailMLExportVersion version,
        const RailMLExportOptions& options = RailMLExportOptions()
    );
    
    /**
     * @brief Export to RailML string
     * @param network Railway network to export
     * @param schedules Train schedules to export
     * @param version RailML version to use
     * @param options Export options
     * @return RailML XML string (empty if failed)
     */
    std::string export_to_string(
        const RailwayNetwork& network,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        RailMLExportVersion version,
        const RailMLExportOptions& options = RailMLExportOptions()
    );
    
    /**
     * @brief Get last error message
     * @return Error description if export failed
     */
    std::string get_last_error() const { return last_error_; }
    
    /**
     * @brief Get export statistics
     * @return Map of statistic name to value
     */
    std::map<std::string, int> get_statistics() const;

private:
    std::string last_error_;
    
    // Statistics
    int stations_exported_ = 0;
    int tracks_exported_ = 0;
    int trains_exported_ = 0;
    
    // RailML 2.x export
    std::string export_railml2(
        const RailwayNetwork& network,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const RailMLExportOptions& options
    );
    
    std::string export_railml2_infrastructure(
        const RailwayNetwork& network,
        const RailMLExportOptions& options
    );
    
    std::string export_railml2_timetable(
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const RailwayNetwork& network,
        const RailMLExportOptions& options
    );
    
    // RailML 3.x export
    std::string export_railml3(
        const RailwayNetwork& network,
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const RailMLExportOptions& options
    );
    
    std::string export_railml3_infrastructure(
        const RailwayNetwork& network,
        const RailMLExportOptions& options
    );
    
    std::string export_railml3_timetable(
        const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
        const RailwayNetwork& network,
        const RailMLExportOptions& options
    );
    
    // Helper methods
    void clear_statistics();
    void set_error(const std::string& error);
    std::string format_xml(const std::string& xml, bool pretty_print);
};

/**
 * @brief Convenience function to export network to RailML file
 * @param filename Output file path
 * @param network Railway network to export
 * @param version RailML version to use
 * @param options Export options
 * @return True if export succeeded
 */
bool export_railml_network(
    const std::string& filename,
    const RailwayNetwork& network,
    RailMLExportVersion version = RailMLExportVersion::VERSION_3,
    const RailMLExportOptions& options = RailMLExportOptions()
);

/**
 * @brief Convenience function to export schedules to RailML file
 * @param filename Output file path
 * @param schedules Train schedules to export
 * @param network Network associated with schedules
 * @param version RailML version to use
 * @param options Export options
 * @return True if export succeeded
 */
bool export_railml_schedules(
    const std::string& filename,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailwayNetwork& network,
    RailMLExportVersion version = RailMLExportVersion::VERSION_3,
    const RailMLExportOptions& options = RailMLExportOptions()
);

} // namespace fdc_scheduler
