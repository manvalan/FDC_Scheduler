/**
 * @file test_node.cpp
 * @brief Unit tests for Node class
 */

#include <fdc_scheduler/node.hpp>
#include <gtest/gtest.h>

using namespace fdc_scheduler;

TEST(NodeTest, Construction) {
    Node node("STATION1", "Station 1", NodeType::STATION);
    
    EXPECT_EQ(node.get_id(), "STATION1");
    EXPECT_EQ(node.get_name(), "Station 1");
    EXPECT_EQ(node.get_type(), NodeType::STATION);
}

TEST(NodeTest, SetCoordinates) {
    Node node("STATION1", "Station 1", NodeType::STATION);
    
    node.set_coordinates(45.0, 9.0);
    
    EXPECT_DOUBLE_EQ(node.get_latitude(), 45.0);
    EXPECT_DOUBLE_EQ(node.get_longitude(), 9.0);
}

TEST(NodeTest, SetName) {
    Node node("ID1", "Original", NodeType::STATION);
    
    node.set_name("Milano Centrale");
    
    EXPECT_EQ(node.get_name(), "Milano Centrale");
}

TEST(NodeTest, Platforms) {
    Node node("STATION1", "Station 1", NodeType::STATION);
    
    node.set_platforms(12);
    
    EXPECT_EQ(node.get_platforms(), 12);
}

TEST(NodeTest, Equality) {
    Node node1("STATION1", "Station 1", NodeType::STATION);
    Node node2("STATION1", "Station 1", NodeType::STATION);
    Node node3("STATION2", "Station 2", NodeType::STATION);
    
    EXPECT_EQ(node1, node2);
    EXPECT_NE(node1, node3);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
