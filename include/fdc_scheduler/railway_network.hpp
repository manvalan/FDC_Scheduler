#ifndef FDC_RAILWAY_NETWORK_HPP
#define FDC_RAILWAY_NETWORK_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "node.hpp"
#include "edge.hpp"
#include "train.hpp"

namespace fdc_scheduler {

/**
 * @brief Properties stored on graph vertices (nodes)
 */
struct VertexProperties {
    std::shared_ptr<Node> node;
    
    VertexProperties() = default;
    explicit VertexProperties(std::shared_ptr<Node> n) : node(std::move(n)) {}
};

/**
 * @brief Properties stored on graph edges
 */
struct EdgeProperties {
    std::shared_ptr<Edge> edge;
    double weight;  // Weight for pathfinding (typically distance or travel time)
    
    EdgeProperties() : weight(0.0) {}
    EdgeProperties(std::shared_ptr<Edge> e, double w) 
        : edge(std::move(e)), weight(w) {}
};

/**
 * @brief Graph type definition using Boost.Graph
 * 
 * Uses adjacency_list with:
 * - vecS: vertices stored in std::vector (allows efficient iteration)
 * - vecS: edges stored in std::vector
 * - directedS: directed graph (but we handle bidirectional with edge properties)
 * - VertexProperties: custom properties for vertices
 * - EdgeProperties: custom properties for edges
 */
using Graph = boost::adjacency_list<
    boost::vecS,           // OutEdgeList
    boost::vecS,           // VertexList
    boost::directedS,      // Directed
    VertexProperties,      // VertexProperties
    EdgeProperties         // EdgeProperties
>;

using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
using EdgeDescriptor = boost::graph_traits<Graph>::edge_descriptor;

/**
 * @brief Represents a path in the railway network
 */
struct Path {
    std::vector<std::string> nodes;     // Node IDs in order
    std::vector<std::string> edges;     // Edge identifiers (from_node-to_node)
    double total_distance;              // Total distance in km
    double min_travel_time;             // Minimum travel time at max speeds (hours)
    
    Path() : total_distance(0.0), min_travel_time(0.0) {}
    
    bool is_valid() const { return !nodes.empty(); }
};

/**
 * @brief Statistics about the railway network
 */
struct NetworkStats {
    size_t num_nodes;
    size_t num_edges;
    double total_track_length;          // km
    double average_edge_length;         // km
    double max_edge_length;             // km
    double min_edge_length;             // km
    size_t num_single_track;
    size_t num_double_track;
    size_t num_high_speed;
    
    NetworkStats() 
        : num_nodes(0), num_edges(0), total_track_length(0.0),
          average_edge_length(0.0), max_edge_length(0.0), min_edge_length(0.0),
          num_single_track(0), num_double_track(0), num_high_speed(0) {}
};

/**
 * @brief Main railway network class
 * 
 * Manages a graph of railway stations (nodes) connected by tracks (edges).
 * Provides pathfinding, network analysis, and management operations.
 */
class RailwayNetwork {
public:
    RailwayNetwork() = default;
    ~RailwayNetwork() = default;
    
    // Node management
    bool add_node(const Node& node);
    bool remove_node(const std::string& node_id);
    std::shared_ptr<Node> get_node(const std::string& node_id) const;
    std::vector<std::shared_ptr<Node>> get_all_nodes() const;
    bool has_node(const std::string& node_id) const;
    size_t num_nodes() const;
    
    // Edge management
    bool add_edge(const Edge& edge);
    bool remove_edge(const std::string& from_node, const std::string& to_node);
    std::shared_ptr<Edge> get_edge(const std::string& from_node, 
                                     const std::string& to_node) const;
    std::vector<std::shared_ptr<Edge>> get_all_edges() const;
    std::vector<std::shared_ptr<Edge>> get_edges_from_node(const std::string& node_id) const;
    bool has_edge(const std::string& from_node, const std::string& to_node) const;
    size_t num_edges() const;
    
    // Pathfinding
    Path find_shortest_path(const std::string& start_node, 
                           const std::string& end_node,
                           bool use_distance = true) const;
    
    std::vector<Path> find_k_shortest_paths(const std::string& start_node,
                                           const std::string& end_node,
                                           size_t k,
                                           bool use_distance = true) const;
    
    // Network analysis
    NetworkStats get_network_stats() const;
    bool is_connected() const;
    std::vector<std::string> get_neighbors(const std::string& node_id) const;
    double calculate_distance(const std::string& from_node,
                             const std::string& to_node) const;
    
    // Utility
    void clear();
    bool is_empty() const { return num_nodes() == 0; }
    
private:
    Graph graph_;
    std::map<std::string, Vertex> node_id_to_vertex_;  // Quick lookup
    
    // Helper methods
    std::optional<Vertex> get_vertex(const std::string& node_id) const;
    void update_edge_weight(EdgeDescriptor ed, bool use_distance);
    Path reconstruct_path(const std::vector<Vertex>& predecessors,
                         Vertex start, Vertex end) const;
};

} // namespace fdc_scheduler

#endif // FDC_RAILWAY_NETWORK_HPP
