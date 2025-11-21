/**
 * @file test_train.cpp
 * @brief Unit tests for Train class
 */

#include <fdc_scheduler/train.hpp>
#include <gtest/gtest.h>

using namespace fdc_scheduler;

TEST(TrainTest, Construction) {
    Train train("IC123", "Intercity 123", TrainType::INTERCITY);
    
    EXPECT_EQ(train.get_id(), "IC123");
    EXPECT_EQ(train.get_name(), "Intercity 123");
    EXPECT_EQ(train.get_type(), TrainType::INTERCITY);
}

TEST(TrainTest, SetName) {
    Train train("IC123", "Original Name", TrainType::INTERCITY);
    
    train.set_name("Milano - Roma Express");
    
    EXPECT_EQ(train.get_name(), "Milano - Roma Express");
}

TEST(TrainTest, SetMaxSpeed) {
    Train train("FR001", "Frecciarossa", TrainType::HIGH_SPEED);
    
    train.set_max_speed(300.0);
    
    EXPECT_DOUBLE_EQ(train.get_max_speed(), 300.0);
}

TEST(TrainTest, BasicProperties) {
    Train train("IC123", "Intercity", TrainType::INTERCITY);
    
    // Just test basic properties exist
    EXPECT_GT(train.get_max_speed(), 0.0);
    EXPECT_GT(train.get_acceleration(), 0.0);
    EXPECT_GT(train.get_deceleration(), 0.0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
