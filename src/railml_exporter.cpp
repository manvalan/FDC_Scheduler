#include "fdc_scheduler/railml_exporter.hpp"
#include <pugixml.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>

namespace fdc_scheduler {

RailMLExporter::RailMLExporter() = default;
RailMLExporter::~RailMLExporter() = default;

void RailMLExporter::clear_statistics() {
    stations_exported_ = 0;
    tracks_exported_ = 0;
    trains_exported_ = 0;
}

void RailMLExporter::set_error(const std::string& error) {
    last_error_ = error;
    std::cerr << "RailML Exporter Error: " << error << std::endl;
}

std::map<std::string, int> RailMLExporter::get_statistics() const {
    return {
        {"stations", stations_exported_},
        {"tracks", tracks_exported_},
        {"trains", trains_exported_}
    };
}

bool RailMLExporter::export_to_file(
    const std::string& filename,
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    std::string xml = export_to_string(network, schedules, version, options);
    if (xml.empty()) {
        return false;
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot create file: " + filename);
        return false;
    }
    
    file << xml;
    return true;
}

std::string RailMLExporter::export_to_string(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    RailMLExportVersion version,
    const RailMLExportOptions& options) {
    
    clear_statistics();
    last_error_.clear();
    
    if (version == RailMLExportVersion::VERSION_2) {
        return export_railml2(network, schedules, options);
    } else {
        return export_railml3(network, schedules, options);
    }
}

std::string RailMLExporter::format_xml(const std::string& xml, bool pretty_print) {
    if (!pretty_print) {
        return xml;
    }
    
    pugi::xml_document doc;
    if (!doc.load_string(xml.c_str())) {
        return xml;
    }
    
    std::ostringstream oss;
    doc.save(oss, "  ");
    return oss.str();
}

//==============================================================================
// RailML 2.x Export
//==============================================================================

std::string RailMLExporter::export_railml2(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    
    // XML declaration
    auto declaration = doc.prepend_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";
    
    // Root element
    auto railml = doc.append_child("railml");
    railml.append_attribute("version") = "2.4";
    railml.append_attribute("xmlns") = "http://www.railml.org/schemas/2013";
    railml.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    
    // Metadata
    if (options.include_metadata) {
        auto metadata = railml.append_child("metadata");
        metadata.append_child("creator").text().set("FDC_Scheduler v2.0");
        
        auto creation_time = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(creation_time);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        metadata.append_child("created").text().set(ss.str().c_str());
    }
    
    // Infrastructure
    if (options.export_infrastructure) {
        std::string infra_xml = export_railml2_infrastructure(network, options);
        if (!infra_xml.empty()) {
            pugi::xml_document infra_doc;
            if (infra_doc.load_string(infra_xml.c_str())) {
                railml.append_copy(infra_doc.first_child());
            }
        }
    }
    
    // Timetable
    if (options.export_timetable && !schedules.empty()) {
        std::string tt_xml = export_railml2_timetable(schedules, network, options);
        if (!tt_xml.empty()) {
            pugi::xml_document tt_doc;
            if (tt_doc.load_string(tt_xml.c_str())) {
                railml.append_copy(tt_doc.first_child());
            }
        }
    }
    
    // Convert to string
    std::ostringstream oss;
    doc.save(oss, options.pretty_print ? "  " : "");
    return oss.str();
}

std::string RailMLExporter::export_railml2_infrastructure(
    const RailwayNetwork& network,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    auto infrastructure = doc.append_child("infrastructure");
    
    if (!options.infrastructure_id.empty()) {
        infrastructure.append_attribute("id") = options.infrastructure_id.c_str();
    } else {
        infrastructure.append_attribute("id") = "fdc_network";
    }
    
    // Export operational points (stations)
    auto ocp_container = infrastructure.append_child("operationalPoints");
    
    auto all_nodes = network.get_all_nodes();
    for (const auto& node_ptr : all_nodes) {
        auto ocp = ocp_container.append_child("ocp");
        ocp.append_attribute("id") = node_ptr->get_id().c_str();
        ocp.append_attribute("name") = node_ptr->get_name().c_str();
        
        // Properties
        auto prop = ocp.append_child("propOperational");
        std::string op_type = "station"; // Could be derived from node type
        prop.append_attribute("operationalType") = op_type.c_str();
        
        stations_exported_++;
    }
    
    // Export tracks (edges)
    auto tracks = infrastructure.append_child("tracks");
    
    auto all_edges = network.get_all_edges();
    int track_counter = 1;
    for (const auto& edge_ptr : all_edges) {
        auto track = tracks.append_child("track");
        
        std::string track_id = "track_" + std::to_string(track_counter++);
        track.append_attribute("id") = track_id.c_str();
        
        // Track begin/end
        auto track_begin = track.append_child("trackBegin");
        track_begin.append_attribute("ref") = edge_ptr->get_from_node().c_str();
        
        auto track_end = track.append_child("trackEnd");
        track_end.append_attribute("ref") = edge_ptr->get_to_node().c_str();
        
        // Track topology
        auto topology = track.append_child("trackTopology");
        auto elements = topology.append_child("trackElements");
        auto line = elements.append_child("line");
        line.append_attribute("length") = std::to_string(static_cast<int>(edge_ptr->get_distance() * 1000)).c_str(); // Convert km to m
        
        tracks_exported_++;
    }
    
    std::ostringstream oss;
    doc.save(oss, "");
    return oss.str();
}

std::string RailMLExporter::export_railml2_timetable(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailwayNetwork& network,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    auto timetable = doc.append_child("timetable");
    
    if (!options.timetable_id.empty()) {
        timetable.append_attribute("id") = options.timetable_id.c_str();
    } else {
        timetable.append_attribute("id") = "fdc_timetable";
    }
    
    // Export train parts
    auto train_parts = timetable.append_child("trainParts");
    
    for (const auto& schedule : schedules) {
        auto train_part = train_parts.append_child("trainPart");
        train_part.append_attribute("id") = schedule->get_train_id().c_str();
        train_part.append_attribute("code") = schedule->get_train_id().c_str();
        
        // Export stops
        auto ocps_tt = train_part.append_child("ocpsTT");
        
        const auto& stops = schedule->get_stops();
        for (const auto& stop : stops) {
            auto ocp_tt = ocps_tt.append_child("ocpTT");
            ocp_tt.append_attribute("ocpRef") = stop.node_id.c_str();
            
            auto times = ocp_tt.append_child("times");
            times.append_attribute("scope") = "scheduled";
            
            // Format times as HH:MM:SS
            auto format_time = [](const std::chrono::system_clock::time_point& tp) -> std::string {
                auto time_t = std::chrono::system_clock::to_time_t(tp);
                std::tm tm = *std::gmtime(&time_t);
                std::ostringstream oss;
                oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
                    << std::setfill('0') << std::setw(2) << tm.tm_min << ":"
                    << std::setfill('0') << std::setw(2) << tm.tm_sec;
                return oss.str();
            };
            
            times.append_attribute("arrival") = format_time(stop.arrival).c_str();
            times.append_attribute("departure") = format_time(stop.departure).c_str();
        }
        
        trains_exported_++;
    }
    
    std::ostringstream oss;
    doc.save(oss, "");
    return oss.str();
}

//==============================================================================
// RailML 3.x Export
//==============================================================================

std::string RailMLExporter::export_railml3(
    const RailwayNetwork& network,
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    
    // XML declaration
    auto declaration = doc.prepend_child(pugi::node_declaration);
    declaration.append_attribute("version") = "1.0";
    declaration.append_attribute("encoding") = "UTF-8";
    
    // Root element with RailML 3.x namespaces
    auto railml = doc.append_child("railml");
    railml.append_attribute("version") = "3.1";
    railml.append_attribute("xmlns") = "https://www.railml.org/schemas/3.1";
    railml.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    
    // Metadata
    if (options.include_metadata) {
        auto metadata = railml.append_child("metadata");
        metadata.append_child("creator").text().set("FDC_Scheduler v2.0");
        
        auto creation_time = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(creation_time);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        metadata.append_child("created").text().set(ss.str().c_str());
    }
    
    // Infrastructure
    if (options.export_infrastructure) {
        std::string infra_xml = export_railml3_infrastructure(network, options);
        if (!infra_xml.empty()) {
            pugi::xml_document infra_doc;
            if (infra_doc.load_string(infra_xml.c_str())) {
                railml.append_copy(infra_doc.first_child());
            }
        }
    }
    
    // Timetable
    if (options.export_timetable && !schedules.empty()) {
        std::string tt_xml = export_railml3_timetable(schedules, network, options);
        if (!tt_xml.empty()) {
            pugi::xml_document tt_doc;
            if (tt_doc.load_string(tt_xml.c_str())) {
                railml.append_copy(tt_doc.first_child());
            }
        }
    }
    
    // Convert to string
    std::ostringstream oss;
    doc.save(oss, options.pretty_print ? "  " : "");
    return oss.str();
}

std::string RailMLExporter::export_railml3_infrastructure(
    const RailwayNetwork& network,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    auto infrastructure = doc.append_child("infrastructure");
    
    if (!options.infrastructure_id.empty()) {
        infrastructure.append_attribute("id") = options.infrastructure_id.c_str();
    } else {
        infrastructure.append_attribute("id") = "fdc_network_3x";
    }
    
    // Functional infrastructure
    auto functional = infrastructure.append_child("functionalInfrastructure");
    auto op_points = functional.append_child("operationalPoints");
    
    auto all_nodes = network.get_all_nodes();
    for (const auto& node_ptr : all_nodes) {
        auto op = op_points.append_child("operationalPoint");
        op.append_attribute("id") = node_ptr->get_id().c_str();
        op.append_attribute("name") = node_ptr->get_name().c_str();
        
        stations_exported_++;
    }
    
    // Topology
    auto topology = infrastructure.append_child("topology");
    auto net_elements = topology.append_child("netElements");
    
    auto all_edges = network.get_all_edges();
    int ne_counter = 1;
    for (const auto& edge_ptr : all_edges) {
        auto net_element = net_elements.append_child("netElement");
        
        std::string ne_id = "ne_" + std::to_string(ne_counter++);
        net_element.append_attribute("id") = ne_id.c_str();
        
        // Relations
        auto relation = net_element.append_child("relation");
        relation.append_attribute("ref") = edge_ptr->get_to_node().c_str();
        
        tracks_exported_++;
    }
    
    std::ostringstream oss;
    doc.save(oss, "");
    return oss.str();
}

std::string RailMLExporter::export_railml3_timetable(
    const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
    const RailwayNetwork& network,
    const RailMLExportOptions& options) {
    
    pugi::xml_document doc;
    auto timetable = doc.append_child("timetable");
    
    if (!options.timetable_id.empty()) {
        timetable.append_attribute("id") = options.timetable_id.c_str();
    } else {
        timetable.append_attribute("id") = "fdc_timetable_3x";
    }
    
    // Export train parts (RailML 3.x structure)
    auto train_parts = timetable.append_child("trainParts");
    
    for (const auto& schedule : schedules) {
        auto train_part = train_parts.append_child("trainPart");
        train_part.append_attribute("id") = ("tp_" + schedule->get_train_id()).c_str();
        
        // Operational train number
        auto op_train_number = train_part.append_child("operationalTrainNumber");
        op_train_number.text().set(schedule->get_train_id().c_str());
        
        // Train part sequence with course
        auto sequence = train_part.append_child("trainPartSequence");
        sequence.append_attribute("sequence") = "1";
        
        auto course = sequence.append_child("course");
        
        // Export stops
        const auto& stops = schedule->get_stops();
        for (const auto& stop : stops) {
            auto ocp_ref = course.append_child("ocpRef");
            ocp_ref.append_attribute("ref") = stop.node_id.c_str();
            
            auto times = ocp_ref.append_child("times");
            times.append_attribute("scope") = "scheduled";
            
            // Format times
            auto format_time = [](const std::chrono::system_clock::time_point& tp) -> std::string {
                auto time_t = std::chrono::system_clock::to_time_t(tp);
                std::tm tm = *std::gmtime(&time_t);
                std::ostringstream oss;
                oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
                    << std::setfill('0') << std::setw(2) << tm.tm_min << ":"
                    << std::setfill('0') << std::setw(2) << tm.tm_sec;
                return oss.str();
            };
            
            times.append_attribute("arrival") = format_time(stop.arrival).c_str();
            times.append_attribute("departure") = format_time(stop.departure).c_str();
        }
        
        trains_exported_++;
    }
    
    std::ostringstream oss;
    doc.save(oss, "");
    return oss.str();
}

//==============================================================================
// Convenience Functions
//==============================================================================

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
