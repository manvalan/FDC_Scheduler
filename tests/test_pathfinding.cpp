#include <gtest/gtest.h>
#include <fdc_scheduler/json_api_wrapper.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;
using namespace fdc_scheduler;

class PathfindingTest : public ::testing::Test {
protected:
    JsonApiWrapper* api;
    
    void SetUp() override {
        api = new JsonApiWrapper();
        
        // Create simple 5-node test network
        json network = {
            {"nodes", json::array({
                {{"id", "A"}, {"name", "Station A"}, {"type", "station"}, {"platforms", 2}},
                {{"id", "B"}, {"name", "Station B"}, {"type", "station"}, {"platforms", 2}},
                {{"id", "C"}, {"name", "Station C"}, {"type", "station"}, {"platforms", 2}},
                {{"id", "D"}, {"name", "Station D"}, {"type", "station"}, {"platforms", 2}},
                {{"id", "E"}, {"name", "Station E"}, {"type", "station"}, {"platforms", 2}}
            })},
            {"edges", json::array({
                {{"from", "A"}, {"to", "B"}, {"distance", 10.0}, {"max_speed", 120.0}, {"track_type", "double"}, {"bidirectional", true}},
                {{"from", "A"}, {"to", "D"}, {"distance", 15.0}, {"max_speed", 100.0}, {"track_type", "single"}, {"bidirectional", true}},
                {{"from", "B"}, {"to", "C"}, {"distance", 8.0}, {"max_speed", 140.0}, {"track_type", "double"}, {"bidirectional", true}},
                {{"from", "D"}, {"to", "C"}, {"distance", 12.0}, {"max_speed", 110.0}, {"track_type", "single"}, {"bidirectional", true}},
                {{"from", "D"}, {"to", "E"}, {"distance", 20.0}, {"max_speed", 90.0}, {"track_type", "single"}, {"bidirectional", true}}
            })}
        };
        
        api->load_network(network.dump());
    }
    
    void TearDown() override {
        delete api;
    }
};

// Test 1: Single shortest path (k=1)
TEST_F(PathfindingTest, SingleShortestPath) {
    std::string result = api->find_k_shortest_paths("A", "C", 1, true);
    json paths = json::parse(result);
    
    ASSERT_FALSE(paths.empty());
    EXPECT_EQ(paths[0]["rank"], 1);
    EXPECT_EQ(paths[0]["nodes"].size(), 3); // A->B->C
    EXPECT_DOUBLE_EQ(paths[0]["total_distance"], 18.0); // 10+8
}

// Test 2: Multiple alternative paths (k=3)
TEST_F(PathfindingTest, ThreeAlternativePaths) {
    std::string result = api->find_k_shortest_paths("A", "C", 3, true);
    json paths = json::parse(result);
    
    ASSERT_GE(paths.size(), 2); // At least 2 paths exist
    EXPECT_EQ(paths[0]["rank"], 1);
    EXPECT_EQ(paths[1]["rank"], 2);
    
    // First path should be shorter than second
    EXPECT_LT(paths[0]["total_distance"], paths[1]["total_distance"]);
}

// Test 3: Time-based metric instead of distance
TEST_F(PathfindingTest, TimeBasedMetric) {
    std::string result = api->find_k_shortest_paths("A", "C", 2, false);
    json paths = json::parse(result);
    
    ASSERT_FALSE(paths.empty());
    EXPECT_TRUE(paths[0].contains("total_time"));
    EXPECT_GT(paths[0]["total_time"], 0.0);
}

// Test 4: Path to isolated node (no connection)
TEST_F(PathfindingTest, NoPathAvailable) {
    // E is only connected to D, not to A-B-C cluster in one direction
    std::string result = api->find_k_shortest_paths("B", "E", 1, true);
    json paths = json::parse(result);
    
    // Should find path B->A->D->E or similar
    ASSERT_FALSE(paths.empty());
}

// Test 5: Invalid k parameter (k < 1)
TEST_F(PathfindingTest, InvalidKParameterTooSmall) {
    EXPECT_THROW({
        api->find_k_shortest_paths("A", "C", 0, true);
    }, std::invalid_argument);
}

// Test 6: Invalid k parameter (k > 10)
TEST_F(PathfindingTest, InvalidKParameterTooLarge) {
    EXPECT_THROW({
        api->find_k_shortest_paths("A", "C", 15, true);
    }, std::invalid_argument);
}

// Test 7: Non-existent source node
TEST_F(PathfindingTest, NonExistentSourceNode) {
    EXPECT_THROW({
        api->find_k_shortest_paths("X", "C", 1, true);
    }, std::runtime_error);
}

// Test 8: Non-existent destination node
TEST_F(PathfindingTest, NonExistentDestinationNode) {
    EXPECT_THROW({
        api->find_k_shortest_paths("A", "Z", 1, true);
    }, std::runtime_error);
}

// Test 9: Same source and destination
TEST_F(PathfindingTest, SameSourceAndDestination) {
    std::string result = api->find_k_shortest_paths("A", "A", 1, true);
    json paths = json::parse(result);
    
    // Should return empty path or single-node path
    ASSERT_FALSE(paths.empty());
    EXPECT_EQ(paths[0]["nodes"].size(), 1);
    EXPECT_DOUBLE_EQ(paths[0]["total_distance"], 0.0);
}

// Test 10: Verify path delta increases monotonically
TEST_F(PathfindingTest, PathDeltaMonotonic) {
    std::string result = api->find_k_shortest_paths("A", "C", 3, true);
    json paths = json::parse(result);
    
    if (paths.size() >= 2) {
        // Delta should increase with each alternative path
        for (size_t i = 1; i < paths.size(); ++i) {
            EXPECT_GE(paths[i]["delta_from_shortest"], 
                     paths[i-1]["delta_from_shortest"]);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
