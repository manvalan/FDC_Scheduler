#include "fdc_scheduler/railml_parser.hpp"
#include <fstream>
#include <sstream>

namespace fdc_scheduler {

RailMLParser::RailMLParser() {}

bool RailMLParser::parse_file(const std::string& filename, RailMLVersion version) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            last_error_ = "Failed to open file: " + filename;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string xml_content = buffer.str();
        
        return parse_string(xml_content, version);
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Error reading file: ") + e.what();
        return false;
    }
}

bool RailMLParser::parse_string(const std::string& xml_content, RailMLVersion version) {
    try {
        // Detect version if auto-detect
        RailMLVersion actual_version = version;
        if (version == RailMLVersion::AUTO_DETECT) {
            actual_version = detect_version(xml_content);
        }
        
        // Parse based on version
        switch (actual_version) {
            case RailMLVersion::VERSION_2:
                return parse_railml2(xml_content);
            case RailMLVersion::VERSION_3:
                return parse_railml3(xml_content);
            default:
                last_error_ = "Unable to detect RailML version";
                return false;
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Error parsing XML: ") + e.what();
        return false;
    }
}





RailMLVersion RailMLParser::detect_version(const std::string& xml_content) {
    // Simple version detection based on XML namespace
    if (xml_content.find("railml.org/schemas/2") != std::string::npos ||
        xml_content.find("railML version=\"2") != std::string::npos) {
        return RailMLVersion::VERSION_2;
    } else if (xml_content.find("railml.org/schemas/3") != std::string::npos ||
               xml_content.find("railML version=\"3") != std::string::npos) {
        return RailMLVersion::VERSION_3;
    }
    
    return RailMLVersion::VERSION_2; // Default to version 2
}

bool RailMLParser::parse_railml2(const std::string& xml_content) {
    // TODO: Implement RailML 2.x parser
    // This requires an XML parsing library like libxml2 or pugixml
    // For now, return stub implementation
    
    last_error_ = "RailML 2.x parsing not yet implemented. Please use JSON format or contribute XML parsing support.";
    return false;
}

bool RailMLParser::parse_railml3(const std::string& xml_content) {
    // TODO: Implement RailML 3.x parser
    // This requires an XML parsing library like libxml2 or pugixml
    // For now, return stub implementation
    
    last_error_ = "RailML 3.x parsing not yet implemented. Please use JSON format or contribute XML parsing support.";
    return false;
}

// Convenience functions

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
