#ifndef FDC_NODE_TYPE_HPP
#define FDC_NODE_TYPE_HPP

#include <string>
#include <stdexcept>

namespace fdc_scheduler {

/**
 * @brief Enumeration representing different types of railway nodes
 */
enum class NodeType {
    STATION,      ///< Regular passenger station
    INTERCHANGE,  ///< Major interchange/hub station
    JUNCTION,     ///< Railway junction (no passenger service)
    DEPOT,        ///< Train depot/maintenance facility
    YARD          ///< Railway yard/storage
};

/**
 * @brief Convert NodeType to string representation
 */
inline std::string node_type_to_string(NodeType type) {
    switch (type) {
        case NodeType::STATION:     return "station";
        case NodeType::INTERCHANGE: return "interchange";
        case NodeType::JUNCTION:    return "junction";
        case NodeType::DEPOT:       return "depot";
        case NodeType::YARD:        return "yard";
        default: throw std::invalid_argument("Unknown NodeType");
    }
}

/**
 * @brief Convert string to NodeType
 */
inline NodeType string_to_node_type(const std::string& str) {
    if (str == "station")     return NodeType::STATION;
    if (str == "interchange") return NodeType::INTERCHANGE;
    if (str == "junction")    return NodeType::JUNCTION;
    if (str == "depot")       return NodeType::DEPOT;
    if (str == "yard")        return NodeType::YARD;
    throw std::invalid_argument("Unknown NodeType string: " + str);
}

} // namespace fdc_scheduler

#endif // FDC_NODE_TYPE_HPP
