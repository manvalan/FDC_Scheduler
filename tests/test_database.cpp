#include <gtest/gtest.h>
#include "fdc_scheduler/database.hpp"

using namespace fdc_scheduler;

// Stub test - Basic database operations
TEST(DatabaseTest, BasicOperations) {
    DatabaseConfig config(":memory:");
    Database db(config);
    
    EXPECT_TRUE(db.is_open());
    db.close();
    EXPECT_FALSE(db.is_open());
}
