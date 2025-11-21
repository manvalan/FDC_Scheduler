/**
 * @file test_railway_network.cpp
 * @brief Unit tests for RailwayNetwork class
 */

#include <fdc_scheduler/railway_network.hpp>
#include <gtest/gtest.h>

using namespace fdc_scheduler;

TEST(RailwayNetworkTest, Construction) {
    RailwayNetwork network;
    
    EXPECT_EQ(network.num_nodes(), 0);
    EXPECT_EQ(network.num_edges(), 0);
}

TEST(RailwayNetworkTest, AddNode) {
    RailwayNetwork network;
    
    Node node("STATION1", "Station 1", NodeType::STATION);
    network.add_node(node);
    
    EXPECT_EQ(network.num_nodes(), 1);
    EXPECT_TRUE(network.has_node("STATION1"));
}

TEST(RailwayNetworkTest, AddEdge) {
    RailwayNetwork network;
    
    Node node1("A", "Node A", NodeType::STATION);
    Node node2("B", "Node B", NodeType::STATION);
    network.add_node(node1);
    network.add_node(node2);
    
    Edge edge("A", "B", 100.0);
    network.add_edge(edge);
    
    EXPECT_EQ(network.num_edges(), 1);
}

TEST(RailwayNetworkTest, GetNode) {
    RailwayNetwork network;
    
    Node node("STATION1", "Test Station", NodeType::STATION);
    network.add_node(node);
    
    auto retrieved = network.get_node("STATION1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->get_id(), "STATION1");
    EXPECT_EQ(retrieved->get_name(), "Test Station");
}

TEST(RailwayNetworkTest, Clear) {
    RailwayNetwork network;
    
    network.add_node(Node("A", "Node A", NodeType::STATION));
    network.add_node(Node("B", "Node B", NodeType::STATION));
    
    network.clear();
    
    EXPECT_EQ(network.num_nodes(), 0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
