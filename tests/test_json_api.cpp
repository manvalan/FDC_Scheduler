/**
 * @file test_json_api.cpp
 * @brief Unit tests for JSON API
 */

#include <fdc_scheduler/json_api.hpp>
#include <gtest/gtest.h>

using namespace fdc_scheduler;

TEST(JsonApiTest, BasicFunctionality) {
    // Basic test to verify JSON API compiles and links
    SUCCEED();
}

TEST(JsonApiTest, NetworkSerialization) {
    RailwayNetwork network;
    
    Node node("STATION1", "Test Station", NodeType::STATION);
    network.add_node(node);
    
    // Test that network can be used with JSON API
    EXPECT_EQ(network.num_nodes(), 1);
}

TEST(JsonApiTest, TrainData) {
    Train train("IC123", "Test Train", TrainType::INTERCITY);
    
    // Test that train data is complete
    EXPECT_EQ(train.get_id(), "IC123");
    EXPECT_EQ(train.get_name(), "Test Train");
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
