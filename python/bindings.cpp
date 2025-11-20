/**
 * @file bindings.cpp
 * @brief Python bindings for FDC_Scheduler using pybind11
 * 
 * Provides Pythonic interface to:
 * - RailwayNetwork (graph operations)
 * - TrainSchedule (timetable management)
 * - ConflictDetector (conflict detection)
 * - RailwayAIResolver (AI-powered resolution)
 * - RailML import/export
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/node.hpp"
#include "fdc_scheduler/edge.hpp"
#include "fdc_scheduler/train.hpp"
#include "fdc_scheduler/schedule.hpp"
#include "fdc_scheduler/conflict_detector.hpp"
#include "fdc_scheduler/railway_ai_resolver.hpp"
#include "fdc_scheduler/railml_parser.hpp"
#include "fdc_scheduler/railml_exporter.hpp"
#include "fdc_scheduler/json_api.hpp"

namespace py = pybind11;
using namespace fdc_scheduler;

PYBIND11_MODULE(pyfdc_scheduler, m) {
    m.doc() = "FDC_Scheduler - Complete Railway Network Scheduling Library";
    
    // Version info
    m.attr("__version__") = "2.0.0";
    
    //==========================================================================
    // Enumerations
    //==========================================================================
    
    py::enum_<NodeType>(m, "NodeType")
        .value("STATION", NodeType::STATION, "Major station with multiple platforms")
        .value("INTERCHANGE", NodeType::INTERCHANGE, "Major interchange/hub station")
        .value("JUNCTION", NodeType::JUNCTION, "Track junction point")
        .value("DEPOT", NodeType::DEPOT, "Train depot or maintenance facility")
        .value("YARD", NodeType::YARD, "Railway yard/storage")
        .export_values();
    
    py::enum_<TrackType>(m, "TrackType")
        .value("SINGLE", TrackType::SINGLE, "Single track (bidirectional)")
        .value("DOUBLE", TrackType::DOUBLE, "Double track (unidirectional)")
        .value("HIGH_SPEED", TrackType::HIGH_SPEED, "High-speed line")
        .value("FREIGHT", TrackType::FREIGHT, "Freight-only track")
        .export_values();
    
    py::enum_<TrainType>(m, "TrainType")
        .value("REGIONAL", TrainType::REGIONAL)
        .value("INTERCITY", TrainType::INTERCITY)
        .value("HIGH_SPEED", TrainType::HIGH_SPEED)
        .value("FREIGHT", TrainType::FREIGHT)
        .export_values();
    
    py::enum_<ConflictType>(m, "ConflictType")
        .value("SECTION_OVERLAP", ConflictType::SECTION_OVERLAP)
        .value("PLATFORM_CONFLICT", ConflictType::PLATFORM_CONFLICT)
        .value("HEAD_ON_COLLISION", ConflictType::HEAD_ON_COLLISION)
        .value("TIMING_VIOLATION", ConflictType::TIMING_VIOLATION)
        .export_values();
    
    py::enum_<ResolutionStrategy>(m, "ResolutionStrategy")
        .value("DELAY_TRAIN", ResolutionStrategy::DELAY_TRAIN)
        .value("CHANGE_PLATFORM", ResolutionStrategy::CHANGE_PLATFORM)
        .value("ADD_OVERTAKING_POINT", ResolutionStrategy::ADD_OVERTAKING_POINT)
        .value("ADJUST_SPEED", ResolutionStrategy::ADJUST_SPEED)
        .value("PRIORITY_BASED", ResolutionStrategy::PRIORITY_BASED)
        .export_values();
    
    py::enum_<RailMLVersion>(m, "RailMLVersion")
        .value("VERSION_2", RailMLVersion::VERSION_2)
        .value("VERSION_3", RailMLVersion::VERSION_3)
        .value("AUTO_DETECT", RailMLVersion::AUTO_DETECT)
        .export_values();
    
    py::enum_<RailMLExportVersion>(m, "RailMLExportVersion")
        .value("VERSION_2", RailMLExportVersion::VERSION_2)
        .value("VERSION_3", RailMLExportVersion::VERSION_3)
        .export_values();
    
    //==========================================================================
    // Node Class
    //==========================================================================
    
    py::class_<Node>(m, "Node")
        .def(py::init<const std::string&, const std::string&, NodeType, int>(),
             py::arg("id"),
             py::arg("name"),
             py::arg("type") = NodeType::STATION,
             py::arg("platforms") = 2,
             "Create a railway node (station, junction, halt)")
        .def("get_id", &Node::get_id, "Get node ID")
        .def("get_name", &Node::get_name, "Get node name")
        .def("get_type", &Node::get_type, "Get node type")
        .def("get_platforms", &Node::get_platforms, "Get number of platforms")
        .def("set_name", &Node::set_name, "Set node name")
        .def("set_platforms", &Node::set_platforms, "Set number of platforms")
        .def("__repr__", [](const Node& n) {
            return "<Node id='" + n.get_id() + "' name='" + n.get_name() + "'>";
        });
    
    //==========================================================================
    // Edge Class
    //==========================================================================
    
    py::class_<Edge>(m, "Edge")
        .def(py::init<const std::string&, const std::string&, double, TrackType, double>(),
             py::arg("from_node"),
             py::arg("to_node"),
             py::arg("distance"),
             py::arg("track_type") = TrackType::DOUBLE,
             py::arg("max_speed") = 120.0,
             "Create a track edge between two nodes")
        .def("get_from_node", &Edge::get_from_node, "Get source node ID")
        .def("get_to_node", &Edge::get_to_node, "Get destination node ID")
        .def("get_distance", &Edge::get_distance, "Get distance in kilometers")
        .def("get_track_type", &Edge::get_track_type, "Get track type")
        .def("get_max_speed", &Edge::get_max_speed, "Get maximum speed in km/h")
        .def("set_distance", &Edge::set_distance, "Set distance")
        .def("set_max_speed", &Edge::set_max_speed, "Set maximum speed")
        .def("__repr__", [](const Edge& e) {
            return "<Edge " + e.get_from_node() + " -> " + e.get_to_node() + 
                   " (" + std::to_string(e.get_distance()) + " km)>";
        });
    
    //==========================================================================
    // Train Class
    //==========================================================================
    
    py::class_<Train>(m, "Train")
        .def(py::init<const std::string&, const std::string&, TrainType, double, double, double>(),
             py::arg("id"),
             py::arg("name"),
             py::arg("type") = TrainType::REGIONAL,
             py::arg("max_speed") = 160.0,
             py::arg("acceleration") = 0.6,
             py::arg("deceleration") = 0.8,
             "Create a train with physical properties")
        .def("get_id", &Train::get_id, "Get train ID")
        .def("get_name", &Train::get_name, "Get train name")
        .def("get_type", &Train::get_type, "Get train type")
        .def("get_max_speed", &Train::get_max_speed, "Get maximum speed (km/h)")
        .def("get_acceleration", &Train::get_acceleration, "Get acceleration (m/s²)")
        .def("get_deceleration", &Train::get_deceleration, "Get deceleration (m/s²)")
        .def("set_max_speed", &Train::set_max_speed, py::arg("speed"), "Set maximum speed")
        .def("calculate_travel_time", &Train::calculate_travel_time,
             py::arg("distance_km"),
             py::arg("track_max_speed"),
             py::arg("initial_speed") = 0.0,
             py::arg("final_speed") = 0.0,
             "Calculate realistic travel time")
        .def("__repr__", [](const Train& t) {
            return "<Train id='" + t.get_id() + "' name='" + t.get_name() + "'>";
        });
    
    //==========================================================================
    // ScheduleStop Class
    //==========================================================================
    
    py::class_<ScheduleStop>(m, "ScheduleStop")
        .def(py::init<const std::string&, 
                      const std::chrono::system_clock::time_point&,
                      const std::chrono::system_clock::time_point&,
                      bool>(),
             py::arg("node_id"),
             py::arg("arrival"),
             py::arg("departure"),
             py::arg("is_stop") = true,
             "Create a schedule stop at a station")
        .def_readwrite("node_id", &ScheduleStop::node_id, "Station ID")
        .def_readwrite("arrival", &ScheduleStop::arrival, "Arrival time")
        .def_readwrite("departure", &ScheduleStop::departure, "Departure time")
        .def_readwrite("platform", &ScheduleStop::platform, "Platform number")
        .def_readwrite("is_stop", &ScheduleStop::is_stop, "True if stop, false if pass-through")
        .def("get_node_id", &ScheduleStop::get_node_id)
        .def("get_arrival", &ScheduleStop::get_arrival)
        .def("get_departure", &ScheduleStop::get_departure)
        .def("get_platform", &ScheduleStop::get_platform)
        .def("get_dwell_time", &ScheduleStop::get_dwell_time, "Get dwell time in seconds")
        .def("__repr__", [](const ScheduleStop& s) {
            return "<ScheduleStop at '" + s.node_id + "'>";
        });
    
    //==========================================================================
    // TrainSchedule Class
    //==========================================================================
    
    py::class_<TrainSchedule, std::shared_ptr<TrainSchedule>>(m, "TrainSchedule")
        .def(py::init<const std::string&>(),
             py::arg("train_id"),
             "Create a train schedule")
        .def("get_train_id", &TrainSchedule::get_train_id, "Get train ID")
        .def("add_stop", &TrainSchedule::add_stop, 
             py::arg("stop"),
             "Add a stop to the schedule")
        .def("get_stops", 
             py::overload_cast<>(&TrainSchedule::get_stops, py::const_),
             py::return_value_policy::reference_internal,
             "Get list of all stops")
        .def("get_stop_count", &TrainSchedule::get_stop_count, "Get number of stops")
        .def("__repr__", [](const TrainSchedule& s) {
            return "<TrainSchedule train='" + s.get_train_id() + "' stops=" + 
                   std::to_string(s.get_stops().size()) + ">";
        });
    
    //==========================================================================
    // RailwayNetwork Class
    //==========================================================================
    
    py::class_<RailwayNetwork, std::shared_ptr<RailwayNetwork>>(m, "RailwayNetwork")
        .def(py::init<>(), "Create an empty railway network")
        .def("add_node", &RailwayNetwork::add_node,
             py::arg("node"),
             "Add a node (station/junction) to the network")
        .def("add_edge", &RailwayNetwork::add_edge,
             py::arg("edge"),
             "Add an edge (track section) to the network")
        .def("has_node", &RailwayNetwork::has_node,
             py::arg("node_id"),
             "Check if node exists")
        .def("has_edge", &RailwayNetwork::has_edge,
             py::arg("from_node"),
             py::arg("to_node"),
             "Check if edge exists")
        .def("get_all_nodes", &RailwayNetwork::get_all_nodes,
             "Get all nodes in the network")
        .def("get_all_edges", &RailwayNetwork::get_all_edges,
             "Get all edges in the network")
        .def("num_nodes", &RailwayNetwork::num_nodes,
             "Get number of nodes")
        .def("num_edges", &RailwayNetwork::num_edges,
             "Get number of edges")
        .def("find_shortest_path", &RailwayNetwork::find_shortest_path,
             py::arg("start_node"),
             py::arg("end_node"),
             py::arg("use_distance") = true,
             "Find shortest path between two nodes")
        .def("__repr__", [](const RailwayNetwork& n) {
            return "<RailwayNetwork nodes=" + std::to_string(n.num_nodes()) + 
                   " edges=" + std::to_string(n.num_edges()) + ">";
        });
    
    //==========================================================================
    // Conflict Class
    //==========================================================================
    
    py::class_<Conflict>(m, "Conflict")
        .def_readonly("type", &Conflict::type, "Conflict type")
        .def_readonly("train1_id", &Conflict::train1_id, "First train ID")
        .def_readonly("train2_id", &Conflict::train2_id, "Second train ID")
        .def_readonly("location", &Conflict::location, "Conflict location")
        .def_readonly("conflict_time", &Conflict::conflict_time, "Conflict time")
        .def_readonly("severity", &Conflict::severity, "Conflict severity (0-10)")
        .def_readonly("description", &Conflict::description, "Human-readable description")
        .def("__repr__", [](const Conflict& c) {
            return "<Conflict type=" + std::to_string(static_cast<int>(c.type)) + 
                   " trains='" + c.train1_id + "','" + c.train2_id + "'>";
        });
    
    //==========================================================================
    // ConflictDetector Class
    //==========================================================================
    
    py::class_<ConflictDetector>(m, "ConflictDetector")
        .def(py::init<const RailwayNetwork&>(),
             py::arg("network"),
             "Create conflict detector for a network")
        .def("detect_all", &ConflictDetector::detect_all,
             py::arg("schedules"),
             "Detect all conflicts in the given schedules")
        .def("__repr__", [](const ConflictDetector&) {
            return "<ConflictDetector>";
        });
    
    //==========================================================================
    // RailwayAIConfig Class
    //==========================================================================
    
    py::class_<RailwayAIConfig>(m, "RailwayAIConfig")
        .def(py::init<>(), "Create default AI configuration")
        .def_readwrite("delay_weight", &RailwayAIConfig::delay_weight)
        .def_readwrite("platform_change_weight", &RailwayAIConfig::platform_change_weight)
        .def_readwrite("reroute_weight", &RailwayAIConfig::reroute_weight)
        .def_readwrite("passenger_impact_weight", &RailwayAIConfig::passenger_impact_weight)
        .def_readwrite("max_delay_minutes", &RailwayAIConfig::max_delay_minutes)
        .def_readwrite("min_headway_seconds", &RailwayAIConfig::min_headway_seconds)
        .def_readwrite("station_dwell_buffer_seconds", &RailwayAIConfig::station_dwell_buffer_seconds)
        .def_readwrite("allow_single_track_meets", &RailwayAIConfig::allow_single_track_meets)
        .def_readwrite("prefer_double_track_routing", &RailwayAIConfig::prefer_double_track_routing)
        .def_readwrite("allow_platform_reassignment", &RailwayAIConfig::allow_platform_reassignment)
        .def_readwrite("optimize_platform_usage", &RailwayAIConfig::optimize_platform_usage);
    
    //==========================================================================
    // ResolutionResult Class
    //==========================================================================
    
    py::class_<ResolutionResult>(m, "ResolutionResult")
        .def_readonly("success", &ResolutionResult::success)
        .def_readonly("strategy_used", &ResolutionResult::strategy_used)
        .def_readonly("quality_score", &ResolutionResult::quality_score)
        .def_readonly("modified_trains", &ResolutionResult::modified_trains)
        .def_readonly("description", &ResolutionResult::description)
        .def_readonly("total_delay", &ResolutionResult::total_delay)
        .def("__repr__", [](const ResolutionResult& r) {
            return "<ResolutionResult success=" + std::string(r.success ? "True" : "False") + 
                   " quality=" + std::to_string(r.quality_score) + ">";
        });
    
    //==========================================================================
    // RailwayAIResolver Class
    //==========================================================================
    
    py::class_<RailwayAIResolver>(m, "RailwayAIResolver")
        .def(py::init<RailwayNetwork&>(),
             py::arg("network"),
             "Create AI resolver for a network")
        .def(py::init<RailwayNetwork&, const RailwayAIConfig&>(),
             py::arg("network"),
             py::arg("config"),
             "Create AI resolver with custom configuration")
        .def("resolve_conflicts",
             [](RailwayAIResolver& self,
                std::vector<std::shared_ptr<TrainSchedule>>& schedules,
                const std::vector<Conflict>& conflicts) {
                 return self.resolve_conflicts(schedules, conflicts);
             },
             py::arg("schedules"),
             py::arg("conflicts"),
             "Resolve conflicts using AI strategies")
        .def("get_config", &RailwayAIResolver::get_config,
             py::return_value_policy::reference_internal,
             "Get current configuration")
        .def("__repr__", [](const RailwayAIResolver&) {
            return "<RailwayAIResolver>";
        });
    
    //==========================================================================
    // RailML Export Options
    //==========================================================================
    
    py::class_<RailMLExportOptions>(m, "RailMLExportOptions")
        .def(py::init<>())
        .def_readwrite("pretty_print", &RailMLExportOptions::pretty_print)
        .def_readwrite("include_metadata", &RailMLExportOptions::include_metadata)
        .def_readwrite("export_infrastructure", &RailMLExportOptions::export_infrastructure)
        .def_readwrite("export_timetable", &RailMLExportOptions::export_timetable)
        .def_readwrite("export_rolling_stock", &RailMLExportOptions::export_rolling_stock)
        .def_readwrite("infrastructure_id", &RailMLExportOptions::infrastructure_id)
        .def_readwrite("timetable_id", &RailMLExportOptions::timetable_id);
    
    //==========================================================================
    // RailML Parser
    //==========================================================================
    
    py::class_<RailMLParser>(m, "RailMLParser")
        .def(py::init<>(), "Create RailML parser")
        .def("parse_file", &RailMLParser::parse_file,
             py::arg("filename"),
             py::arg("version") = RailMLVersion::AUTO_DETECT,
             "Parse RailML file")
        .def("parse_string", &RailMLParser::parse_string,
             py::arg("xml_content"),
             py::arg("version") = RailMLVersion::AUTO_DETECT,
             "Parse RailML from string")
        .def("get_network", &RailMLParser::get_network,
             "Get parsed network")
        .def("get_schedules", &RailMLParser::get_schedules,
             "Get parsed schedules")
        .def("get_last_error", &RailMLParser::get_last_error,
             "Get last error message")
        .def("get_statistics", &RailMLParser::get_statistics,
             "Get parsing statistics");
    
    //==========================================================================
    // RailML Exporter
    //==========================================================================
    
    py::class_<RailMLExporter>(m, "RailMLExporter")
        .def(py::init<>(), "Create RailML exporter")
        .def("export_to_file",
             [](RailMLExporter& self,
                const std::string& filename,
                const RailwayNetwork& network,
                const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
                RailMLExportVersion version,
                const RailMLExportOptions& options) {
                 return self.export_to_file(filename, network, schedules, version, options);
             },
             py::arg("filename"),
             py::arg("network"),
             py::arg("schedules"),
             py::arg("version"),
             py::arg("options") = RailMLExportOptions(),
             "Export to RailML file")
        .def("export_to_string",
             [](RailMLExporter& self,
                const RailwayNetwork& network,
                const std::vector<std::shared_ptr<TrainSchedule>>& schedules,
                RailMLExportVersion version,
                const RailMLExportOptions& options) {
                 return self.export_to_string(network, schedules, version, options);
             },
             py::arg("network"),
             py::arg("schedules"),
             py::arg("version"),
             py::arg("options") = RailMLExportOptions(),
             "Export to RailML string")
        .def("get_last_error", &RailMLExporter::get_last_error,
             "Get last error message")
        .def("get_statistics", &RailMLExporter::get_statistics,
             "Get export statistics");
    
    //==========================================================================
    // Convenience Functions
    //==========================================================================
    
    m.def("load_railml_network", &load_railml_network,
          py::arg("filename"),
          py::arg("version") = RailMLVersion::AUTO_DETECT,
          "Load network from RailML file");
    
    m.def("export_railml_network", &export_railml_network,
          py::arg("filename"),
          py::arg("network"),
          py::arg("version") = RailMLExportVersion::VERSION_3,
          py::arg("options") = RailMLExportOptions(),
          "Export network to RailML file");
}
