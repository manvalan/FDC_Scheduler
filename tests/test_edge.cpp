/**
 * @file test_edge.cpp
 * @brief Unit tests for Edge class
 */

#include <fdc_scheduler/edge.hpp>
#include <gtest/gtest.h>

using namespace fdc_scheduler;

TEST(EdgeTest, Construction) {
    Edge edge("NODE1", "NODE2", 100.0);
    
    EXPECT_EQ(edge.get_from_node(), "NODE1");
    EXPECT_EQ(edge.get_to_node(), "NODE2");
    EXPECT_DOUBLE_EQ(edge.get_distance(), 100.0);
}

TEST(EdgeTest, SetTrackType) {
    Edge edge("A", "B", 50.0);
    
    edge.set_track_type(TrackType::HIGH_SPEED);
    
    EXPECT_EQ(edge.get_track_type(), TrackType::HIGH_SPEED);
}

TEST(EdgeTest, SetMaxSpeed) {
    Edge edge("A", "B", 50.0);
    
    edge.set_max_speed(250.0);
    
    EXPECT_DOUBLE_EQ(edge.get_max_speed(), 250.0);
}

TEST(EdgeTest, SetCapacity) {
    Edge edge("A", "B", 50.0);
    
    edge.set_capacity(10);
    
    EXPECT_EQ(edge.get_capacity(), 10);
}

TEST(EdgeTest, TravelTime) {
    Edge edge("A", "B", 100.0);  // 100 km
    edge.set_max_speed(200.0);   // 200 km/h
    
    // Expected: 0.5 hours
    // Basic calculation: distance / speed = 100 / 200 = 0.5 hours
    double distance = edge.get_distance();
    double speed = edge.get_max_speed();
    double expected_time = distance / speed;
    EXPECT_NEAR(expected_time, 0.5, 0.1);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
