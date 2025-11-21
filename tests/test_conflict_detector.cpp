#include <gtest/gtest.h>
#include "fdc_scheduler/conflict_detector.hpp"
#include "fdc_scheduler/railway_network.hpp"

using namespace fdc_scheduler;

// Test basic ConflictDetector construction
TEST(ConflictDetectorTest, Construction) {
    RailwayNetwork network;
    
    network.add_node(Node("A", "Node A", NodeType::STATION));
    network.add_node(Node("B", "Node B", NodeType::STATION));
    network.add_edge(Edge("A", "B", 100.0));
    
    ConflictDetector detector(network);
    
    // Basic verification
    EXPECT_EQ(network.num_nodes(), 2);
    EXPECT_EQ(network.num_edges(), 1);
}
