#include "fdc_scheduler/railml_parser.hpp"
#include <pugixml.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

namespace fdc_scheduler {

RailMLParser::RailMLParser() 
    : network_(std::make_shared<RailwayNetwork>()), 
      detected_version_(RailMLVersion::AUTO_DETECT) {
}

RailMLParser::~RailMLParser() = default;

void RailMLParser::clear() {
    network_ = std::make_shared<RailwayNetwork>();
    schedules_.clear();
    last_error_.clear();
    stations_parsed_ = 0;
    tracks_parsed_ = 0;
    trains_parsed_ = 0;
}

void RailMLParser::set_error(const std::string& error) {
    last_error_ = error;
    std::cerr << "RailML Parser Error: " << error << std::endl;
}

bool RailMLParser::parse_file(const std::string& filename, RailMLVersion version) {
    clear();
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        set_error("Cannot open file: " + filename);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse_string(buffer.str(), version);
}

bool RailMLParser::parse_string(const std::string& xml_content, RailMLVersion version) {
    clear();
    
    // Auto-detect version if needed
    RailMLVersion actual_version = version;
    if (version == RailMLVersion::AUTO_DETECT) {
        actual_version = detect_version(xml_content);
        if (actual_version == RailMLVersion::AUTO_DETECT) {
            set_error("Cannot detect RailML version");
            return false;
        }
    }
    
    detected_version_ = actual_version;
    
    // Parse based on version
    if (actual_version == RailMLVersion::VERSION_2) {
        return parse_railml2(xml_content);
    } else {
        return parse_railml3(xml_content);
    }
}

RailMLVersion RailMLParser::detect_version(const std::string& xml_content) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_content.c_str());
    
    if (!result) {
        return RailMLVersion::AUTO_DETECT;
    }
    
    // Check root element and namespace
    pugi::xml_node root = doc.first_child();
    std::string root_name = root.name();
    
    if (root_name == "railml") {
        std::string version = root.attribute("version").value();
        if (version.find("2.") == 0) {
            return RailMLVersion::VERSION_2;
        } else if (version.find("3.") == 0) {
            return RailMLVersion::VERSION_3;
        }
    }
    
    return RailMLVersion::AUTO_DETECT;
}

std::map<std::string, int> RailMLParser::get_statistics() const {
    return {
        {"stations", stations_parsed_},
        {"tracks", tracks_parsed_},
        {"trains", trains_parsed_}
    };
}

//==============================================================================
// RailML 2.x Parsing
//==============================================================================

bool RailMLParser::parse_railml2(const std::string& xml_content) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_content.c_str());
    
    if (!result) {
        set_error("XML parsing failed: " + std::string(result.description()));
        return false;
    }
    
    pugi::xml_node railml = doc.child("railml");
    if (!railml) {
        set_error("Invalid RailML 2.x: missing <railml> root element");
        return false;
    }
    
    // Parse infrastructure
    pugi::xml_node infrastructure = railml.child("infrastructure");
    if (infrastructure) {
        if (!parse_railml2_infrastructure(infrastructure.path())) {
            return false;
        }
    }
    
    // Parse timetable
    pugi::xml_node timetable = railml.child("timetable");
    if (timetable) {
        if (!parse_railml2_timetable(timetable.path())) {
            return false;
        }
    }
    
    return true;
}

bool RailMLParser::parse_railml2_infrastructure(const std::string& xml_path) {
    pugi::xml_document full_doc;
    // In real usage, we'd pass the xml_node directly instead of path
    // For now, this shows the parsing structure
    
    stations_parsed_ = 0;
    tracks_parsed_ = 0;
    
    // Parse from the full document we already have
    pugi::xml_document doc;
    std::ifstream dummy; // This is a structural placeholder
    
    // RailML 2.x structure:
    // <railml>
    //   <infrastructure>
    //     <operationalPoints>
    //       <ocp id="ocp_1" name="Station A">
    //         <propOperational operationalType="station"/>
    //         <geoCoord coord="lat=45.0 lon=9.0"/>
    //       </ocp>
    //     </operationalPoints>
    //     <tracks>
    //       <track>
    //         <trackBegin ref="ocp_1"/>
    //         <trackEnd ref="ocp_2"/>
    //         <trackTopology>
    //           <trackElements>
    //             <line length="10000"/>
    //           </trackElements>
    //         </trackTopology>
    //       </track>
    //     </tracks>
    //   </infrastructure>
    // </railml>
    
    // This would parse operational points and create network nodes
    // Then parse tracks and create edges
    
    return true;
}

bool RailMLParser::parse_railml2_timetable(const std::string& xml_path) {
    trains_parsed_ = 0;
    
    // RailML 2.x timetable structure:
    // <railml>
    //   <timetable>
    //     <trainParts>
    //       <trainPart id="train_1" code="IC123">
    //         <ocpsTT>
    //           <ocpTT ocpRef="ocp_1">
    //             <times scope="scheduled" arrival="10:00:00" departure="10:05:00"/>
    //           </ocpTT>
    //           <ocpTT ocpRef="ocp_2">
    //             <times scope="scheduled" arrival="10:30:00" departure="10:35:00"/>
    //           </ocpTT>
    //         </ocpsTT>
    //       </trainPart>
    //     </trainParts>
    //   </timetable>
    // </railml>
    
    // This would parse each trainPart and create TrainSchedule objects
    // with stops at each operational point
    
    return true;
}

//==============================================================================
// RailML 3.x Parsing
//==============================================================================

bool RailMLParser::parse_railml3(const std::string& xml_content) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml_content.c_str());
    
    if (!result) {
        set_error("XML parsing failed: " + std::string(result.description()));
        return false;
    }
    
    pugi::xml_node railml = doc.child("railml");
    if (!railml) {
        set_error("Invalid RailML 3.x: missing <railml> root element");
        return false;
    }
    
    // RailML 3.x has different structure with namespaces
    // Parse infrastructure
    pugi::xml_node infrastructure = railml.child("infrastructure");
    if (infrastructure) {
        if (!parse_railml3_infrastructure(infrastructure.path())) {
            return false;
        }
    }
    
    // Parse timetable
    pugi::xml_node timetable = railml.child("timetable");
    if (timetable) {
        if (!parse_railml3_timetable(timetable.path())) {
            return false;
        }
    }
    
    return true;
}

bool RailMLParser::parse_railml3_infrastructure(const std::string& xml_path) {
    stations_parsed_ = 0;
    tracks_parsed_ = 0;
    
    // RailML 3.x infrastructure structure (more complex):
    // <railml>
    //   <infrastructure>
    //     <functionalInfrastructure>
    //       <operationalPoints>
    //         <operationalPoint id="op_1" name="Station A">
    //           <designator register="R" entry="StationA"/>
    //           <geoCoordinate>
    //             <coordinate x="45.0" y="9.0"/>
    //           </geoCoordinate>
    //         </operationalPoint>
    //       </operationalPoints>
    //     </functionalInfrastructure>
    //     <topology>
    //       <netElements>
    //         <netElement id="ne_1">
    //           <relation ref="ne_2"/>
    //           <associatedPositioningSystem>
    //             <intrinsicCoordinate id="ic_1" intrinsicCoord="0"/>
    //           </associatedPositioningSystem>
    //         </netElement>
    //       </netElements>
    //     </topology>
    //   </infrastructure>
    // </railml>
    
    // This would parse the more complex RailML 3.x structure
    // with separate functional and topological views
    
    return true;
}

bool RailMLParser::parse_railml3_timetable(const std::string& xml_path) {
    trains_parsed_ = 0;
    
    // RailML 3.x timetable structure:
    // <railml>
    //   <timetable>
    //     <trainParts>
    //       <trainPart id="tp_1">
    //         <operationalTrainNumber>IC123</operationalTrainNumber>
    //         <trainPartSequence sequence="1">
    //           <course>
    //             <ocpRef ref="op_1">
    //               <times scope="scheduled" arrival="10:00:00" departure="10:05:00"/>
    //             </ocpRef>
    //             <ocpRef ref="op_2">
    //               <times scope="scheduled" arrival="10:30:00" departure="10:35:00"/>
    //             </ocpRef>
    //           </course>
    //         </trainPartSequence>
    //       </trainPart>
    //     </trainParts>
    //   </timetable>
    // </railml>
    
    // This would parse RailML 3.x trains with their more structured format
    // including better support for train compositions and routes
    
    return true;
}

//==============================================================================
// Convenience Functions
//==============================================================================

std::shared_ptr<RailwayNetwork> load_railml_network(
    const std::string& filename,
    RailMLVersion version) {
    
    RailMLParser parser;
    if (parser.parse_file(filename, version)) {
        return parser.get_network();
    }
    return nullptr;
}

std::vector<std::shared_ptr<TrainSchedule>> load_railml_schedules(
    const std::string& filename,
    const RailwayNetwork& network,
    RailMLVersion version) {
    
    RailMLParser parser;
    if (parser.parse_file(filename, version)) {
        return parser.get_schedules();
    }
    return {};
}

} // namespace fdc_scheduler
