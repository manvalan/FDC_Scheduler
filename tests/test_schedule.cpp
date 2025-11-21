#include <gtest/gtest.h>
#include "fdc_scheduler/schedule.hpp"
#include <chrono>

using namespace fdc_scheduler;

// Test basic TrainSchedule construction
TEST(TrainScheduleTest, Construction) {
    TrainSchedule schedule("T1");
    EXPECT_EQ(schedule.get_train_id(), "T1");
    EXPECT_EQ(schedule.get_stop_count(), 0);
}

// Test adding stops with chrono::time_point
TEST(TrainScheduleTest, AddStop) {
    TrainSchedule schedule("T1");
    
    ScheduleStop stop;
    stop.node_id = "A";
    stop.arrival = std::chrono::system_clock::now();
    stop.departure = stop.arrival + std::chrono::minutes(5);
    stop.is_stop = true;
    
    schedule.add_stop(stop);
    EXPECT_EQ(schedule.get_stop_count(), 1);
}
