#include "fdc_scheduler/railway_network.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/connected_components.hpp>

namespace fdc_scheduler {

// ============================================================================
// Node Management
// ============================================================================

bool RailwayNetwork::add_node(const Node& node) {
    // Check if node already exists
    if (has_node(node.get_id())) {
        return false;
    }
    
    // Create vertex with node properties
    Vertex v = boost::add_vertex(VertexProperties(std::make_shared<Node>(node)), graph_);
    node_id_to_vertex_[node.get_id()] = v;
    
    return true;
}

bool RailwayNetwork::remove_node(const std::string& node_id) {
    auto vertex_opt = get_vertex(node_id);
    if (!vertex_opt) {
        return false;
    }
    
    Vertex v = *vertex_opt;
    
    // Remove vertex (automatically removes all connected edges)
    boost::clear_vertex(v, graph_);
    boost::remove_vertex(v, graph_);
    
    // Rebuild the node_id_to_vertex map (vertices are renumbered after removal)
    node_id_to_vertex_.clear();
    auto vertices = boost::vertices(graph_);
    for (auto it = vertices.first; it != vertices.second; ++it) {
        const auto& node = graph_[*it].node;
        if (node) {
            node_id_to_vertex_[node->get_id()] = *it;
        }
    }
    
    return true;
}

std::shared_ptr<Node> RailwayNetwork::get_node(const std::string& node_id) const {
    auto vertex_opt = get_vertex(node_id);
    if (!vertex_opt) {
        return nullptr;
    }
    return graph_[*vertex_opt].node;
}

std::vector<std::shared_ptr<Node>> RailwayNetwork::get_all_nodes() const {
    std::vector<std::shared_ptr<Node>> nodes;
    auto vertices = boost::vertices(graph_);
    for (auto it = vertices.first; it != vertices.second; ++it) {
        if (graph_[*it].node) {
            nodes.push_back(graph_[*it].node);
        }
    }
    return nodes;
}

bool RailwayNetwork::has_node(const std::string& node_id) const {
    return node_id_to_vertex_.find(node_id) != node_id_to_vertex_.end();
}

size_t RailwayNetwork::num_nodes() const {
    return boost::num_vertices(graph_);
}

// ============================================================================
// Edge Management
// ============================================================================

bool RailwayNetwork::add_edge(const Edge& edge) {
    // Check if both nodes exist
    auto from_vertex_opt = get_vertex(edge.get_from_node());
    auto to_vertex_opt = get_vertex(edge.get_to_node());
    
    if (!from_vertex_opt || !to_vertex_opt) {
        return false;
    }
    
    Vertex from_v = *from_vertex_opt;
    Vertex to_v = *to_vertex_opt;
    
    // Check if edge already exists
    auto existing = boost::edge(from_v, to_v, graph_);
    if (existing.second) {
        return false;  // Edge already exists
    }
    
    // Add forward edge
    double weight = edge.get_distance();  // Default weight is distance
    auto edge_ptr = std::make_shared<Edge>(edge);
    auto result = boost::add_edge(from_v, to_v, EdgeProperties(edge_ptr, weight), graph_);
    
    // If bidirectional, add reverse edge
    if (edge.is_bidirectional()) {
        // Create reverse edge (correct parameter order: from, to, distance, track_type, max_speed, capacity)
        Edge reverse_edge(edge.get_to_node(), edge.get_from_node(),
                         edge.get_distance(), edge.get_track_type(),
                         edge.get_max_speed(), edge.get_capacity());
        reverse_edge.set_bidirectional(true);
        
        auto reverse_edge_ptr = std::make_shared<Edge>(reverse_edge);
        boost::add_edge(to_v, from_v, EdgeProperties(reverse_edge_ptr, weight), graph_);
    }
    
    return result.second;
}

bool RailwayNetwork::remove_edge(const std::string& from_node, const std::string& to_node) {
    auto from_vertex_opt = get_vertex(from_node);
    auto to_vertex_opt = get_vertex(to_node);
    
    if (!from_vertex_opt || !to_vertex_opt) {
        return false;
    }
    
    Vertex from_v = *from_vertex_opt;
    Vertex to_v = *to_vertex_opt;
    
    // Check if edge exists
    auto edge_pair = boost::edge(from_v, to_v, graph_);
    if (!edge_pair.second) {
        return false;
    }
    
    // Check if it was bidirectional
    bool was_bidirectional = graph_[edge_pair.first].edge->is_bidirectional();
    
    // Remove forward edge
    boost::remove_edge(from_v, to_v, graph_);
    
    // Remove reverse edge if it was bidirectional
    if (was_bidirectional) {
        boost::remove_edge(to_v, from_v, graph_);
    }
    
    return true;
}

std::shared_ptr<Edge> RailwayNetwork::get_edge(const std::string& from_node,
                                                 const std::string& to_node) const {
    auto from_vertex_opt = get_vertex(from_node);
    auto to_vertex_opt = get_vertex(to_node);
    
    if (!from_vertex_opt || !to_vertex_opt) {
        return nullptr;
    }
    
    auto edge_pair = boost::edge(*from_vertex_opt, *to_vertex_opt, graph_);
    if (!edge_pair.second) {
        return nullptr;
    }
    
    return graph_[edge_pair.first].edge;
}

std::vector<std::shared_ptr<Edge>> RailwayNetwork::get_all_edges() const {
    std::vector<std::shared_ptr<Edge>> edges;
    auto edge_range = boost::edges(graph_);
    for (auto it = edge_range.first; it != edge_range.second; ++it) {
        if (graph_[*it].edge) {
            edges.push_back(graph_[*it].edge);
        }
    }
    return edges;
}

std::vector<std::shared_ptr<Edge>> RailwayNetwork::get_edges_from_node(
    const std::string& node_id) const {
    
    std::vector<std::shared_ptr<Edge>> edges;
    auto vertex_opt = get_vertex(node_id);
    if (!vertex_opt) {
        return edges;
    }
    
    auto out_edges = boost::out_edges(*vertex_opt, graph_);
    for (auto it = out_edges.first; it != out_edges.second; ++it) {
        if (graph_[*it].edge) {
            edges.push_back(graph_[*it].edge);
        }
    }
    
    return edges;
}

bool RailwayNetwork::has_edge(const std::string& from_node, 
                               const std::string& to_node) const {
    auto from_vertex_opt = get_vertex(from_node);
    auto to_vertex_opt = get_vertex(to_node);
    
    if (!from_vertex_opt || !to_vertex_opt) {
        return false;
    }
    
    return boost::edge(*from_vertex_opt, *to_vertex_opt, graph_).second;
}

size_t RailwayNetwork::num_edges() const {
    return boost::num_edges(graph_);
}

// ============================================================================
// Pathfinding
// ============================================================================

Path RailwayNetwork::find_shortest_path(const std::string& start_node,
                                       const std::string& end_node,
                                       bool use_distance) const {
    Path result;
    
    auto start_vertex_opt = get_vertex(start_node);
    auto end_vertex_opt = get_vertex(end_node);
    
    if (!start_vertex_opt || !end_vertex_opt) {
        return result;  // Invalid path
    }
    
    Vertex start_v = *start_vertex_opt;
    Vertex end_v = *end_vertex_opt;
    
    // Prepare for Dijkstra
    size_t n = boost::num_vertices(graph_);
    std::vector<Vertex> predecessors(n);
    std::vector<double> distances(n);
    
    // Get edge weight property map
    auto weight_map = boost::get(&EdgeProperties::weight, graph_);
    
    // Run Dijkstra's algorithm
    try {
        boost::dijkstra_shortest_paths(
            graph_, start_v,
            boost::predecessor_map(&predecessors[0])
            .distance_map(&distances[0])
            .weight_map(weight_map)
        );
    } catch (const std::exception& e) {
        return result;  // Algorithm failed
    }
    
    // Check if end is reachable
    if (distances[end_v] == std::numeric_limits<double>::max()) {
        return result;  // No path found
    }
    
    // Reconstruct path
    result = reconstruct_path(predecessors, start_v, end_v);
    
    return result;
}

std::vector<Path> RailwayNetwork::find_k_shortest_paths(
    const std::string& start_node,
    const std::string& end_node,
    size_t k,
    bool use_distance) const {
    
    std::vector<Path> paths;
    
    // For now, just return the single shortest path
    // TODO: Implement Yen's algorithm for k-shortest paths
    auto shortest = find_shortest_path(start_node, end_node, use_distance);
    if (shortest.is_valid()) {
        paths.push_back(shortest);
    }
    
    return paths;
}

// ============================================================================
// Network Analysis
// ============================================================================

NetworkStats RailwayNetwork::get_network_stats() const {
    NetworkStats stats;
    
    stats.num_nodes = num_nodes();
    stats.num_edges = num_edges();
    
    if (stats.num_edges == 0) {
        return stats;
    }
    
    double min_len = std::numeric_limits<double>::max();
    double max_len = 0.0;
    double total_len = 0.0;
    
    auto edges = get_all_edges();
    for (const auto& edge : edges) {
        double len = edge->get_distance();
        total_len += len;
        min_len = std::min(min_len, len);
        max_len = std::max(max_len, len);
        
        // Count track types
        switch (edge->get_track_type()) {
            case TrackType::SINGLE:
                stats.num_single_track++;
                break;
            case TrackType::DOUBLE:
                stats.num_double_track++;
                break;
            case TrackType::HIGH_SPEED:
                stats.num_high_speed++;
                break;
            default:
                break;
        }
    }
    
    stats.total_track_length = total_len;
    stats.average_edge_length = total_len / stats.num_edges;
    stats.min_edge_length = min_len;
    stats.max_edge_length = max_len;
    
    return stats;
}

bool RailwayNetwork::is_connected() const {
    if (num_nodes() <= 1) {
        return true;
    }
    
    std::vector<int> components(num_nodes());
    int num_components = boost::connected_components(graph_, &components[0]);
    
    return num_components == 1;
}

std::vector<std::string> RailwayNetwork::get_neighbors(const std::string& node_id) const {
    std::vector<std::string> neighbors;
    
    auto vertex_opt = get_vertex(node_id);
    if (!vertex_opt) {
        return neighbors;
    }
    
    auto out_edges = boost::out_edges(*vertex_opt, graph_);
    for (auto it = out_edges.first; it != out_edges.second; ++it) {
        Vertex target = boost::target(*it, graph_);
        if (graph_[target].node) {
            neighbors.push_back(graph_[target].node->get_id());
        }
    }
    
    return neighbors;
}

double RailwayNetwork::calculate_distance(const std::string& from_node,
                                          const std::string& to_node) const {
    auto path = find_shortest_path(from_node, to_node, true);
    return path.total_distance;
}

// ============================================================================
// Utility
// ============================================================================

void RailwayNetwork::clear() {
    graph_.clear();
    node_id_to_vertex_.clear();
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::optional<Vertex> RailwayNetwork::get_vertex(const std::string& node_id) const {
    auto it = node_id_to_vertex_.find(node_id);
    if (it == node_id_to_vertex_.end()) {
        return std::nullopt;
    }
    return it->second;
}

Path RailwayNetwork::reconstruct_path(const std::vector<Vertex>& predecessors,
                                     Vertex start, Vertex end) const {
    Path path;
    
    // Reconstruct path from end to start
    std::vector<Vertex> vertex_path;
    Vertex current = end;
    
    while (current != start) {
        vertex_path.push_back(current);
        current = predecessors[current];
        
        // Safety check for infinite loops
        if (vertex_path.size() > num_nodes()) {
            return path;  // Invalid path
        }
    }
    vertex_path.push_back(start);
    
    // Reverse to get path from start to end
    std::reverse(vertex_path.begin(), vertex_path.end());
    
    // Build path information
    for (size_t i = 0; i < vertex_path.size(); ++i) {
        auto node = graph_[vertex_path[i]].node;
        if (node) {
            path.nodes.push_back(node->get_id());
        }
        
        // Add edge information
        if (i < vertex_path.size() - 1) {
            auto edge_pair = boost::edge(vertex_path[i], vertex_path[i + 1], graph_);
            if (edge_pair.second) {
                auto edge = graph_[edge_pair.first].edge;
                if (edge) {
                    path.edges.push_back(edge->get_from_node() + "-" + edge->get_to_node());
                    path.total_distance += edge->get_distance();
                    
                    // Calculate minimum travel time for this segment
                    // (assuming max speed for the entire segment)
                    double time_hours = edge->get_distance() / edge->get_max_speed();
                    path.min_travel_time += time_hours;
                }
            }
        }
    }
    
    return path;
}

} // namespace fdc_scheduler


// Wrapper methods for easier API access
namespace fdc_scheduler {

std::vector<std::shared_ptr<Node>> RailwayNetwork::get_nodes() const {
    std::vector<std::shared_ptr<Node>> nodes;
    auto vertex_range = boost::vertices(graph_);
    for (auto it = vertex_range.first; it != vertex_range.second; ++it) {
        nodes.push_back(graph_[*it].node);
    }
    return nodes;
}

std::vector<std::shared_ptr<Edge>> RailwayNetwork::get_edges() const {
    std::vector<std::shared_ptr<Edge>> edges;
    auto edge_range = boost::edges(graph_);
    for (auto it = edge_range.first; it != edge_range.second; ++it) {
        edges.push_back(graph_[*it].edge);
    }
    return edges;
}

double RailwayNetwork::get_total_length() const {
    double total = 0.0;
    auto edge_range = boost::edges(graph_);
    for (auto it = edge_range.first; it != edge_range.second; ++it) {
        if (graph_[*it].edge) {
            total += graph_[*it].edge->get_distance();
        }
    }
    return total;
}

} // namespace fdc_scheduler
