#include "fdc_scheduler/railml_exporter.hpp"
#include <fstream>
#include <sstream>

namespace fdc_scheduler {

RailMLExporter::RailMLExporter() {}

bool RailMLExporter::export_to_file(
    const std::string& filename,
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    try {
        std::string xml_content = export_to_string(network, schedules, version, options);
        
        if (xml_content.empty()) {
            return false;
        }
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Failed to open file for writing: " + filename;
            return false;
        }
        
        file << xml_content;
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Error writing file: ") + e.what();
        return false;
    }
}

std::string RailMLExporter::export_to_string(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    try {
        switch (version) {
            case RailMLExportVersion::VERSION_2:
                return export_railml2(network, schedules, options);
            case RailMLExportVersion::VERSION_3:
                return export_railml3(network, schedules, options);
            default:
                last_error_ = "Unknown RailML version";
                return "";
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Error generating XML: ") + e.what();
        return "";
    }
}

std::string RailMLExporter::get_last_error() const {
    return last_error_;
}

std::map<std::string, int> RailMLExporter::get_statistics() const {
    return statistics_;
}

std::string RailMLExporter::export_railml2(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailMLExportOptions& options) {
    
    // TODO: Implement RailML 2.x exporter
    // This requires XML generation capabilities
    // For now, return stub implementation
    
    last_error_ = "RailML 2.x export not yet implemented. Please use JSON format or contribute XML generation support.";
    return "";
}

std::string RailMLExporter::export_railml3(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailMLExportOptions& options) {
    
    // TODO: Implement RailML 3.x exporter
    // This requires XML generation capabilities
    // For now, return stub implementation
    
    last_error_ = "RailML 3.x export not yet implemented. Please use JSON format or contribute XML generation support.";
    return "";
}

// Convenience functions

bool export_railml_network(
    const std::string& filename,
    const RailwayNetwork& network,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    RailMLExporter exporter;
    return exporter.export_to_file(filename, network, {}, version, options);
}

bool export_railml_schedules(
    const std::string& filename,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailwayNetwork& network,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    RailMLExporter exporter;
    return exporter.export_to_file(filename, network, schedules, version, options);
}

} // namespace fdc_scheduler
