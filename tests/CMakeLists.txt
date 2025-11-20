# Test executables
set(TEST_SOURCES
    test_pathfinding.cpp
    test_conflicts.cpp
    test_analysis.cpp
    test_optimization.cpp
)

# Create test executables
foreach(TEST_SOURCE ${TEST_SOURCES})
    # Get test name from filename
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Create executable
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Link libraries
    target_link_libraries(${TEST_NAME}
        PRIVATE
            fdc_scheduler
            GTest::gtest_main
    )
    
    # Register with CTest
    gtest_discover_tests(${TEST_NAME})
endforeach()

# Optional: Add test data files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_network.json
    ${CMAKE_CURRENT_BINARY_DIR}/test_network.json
    COPYONLY
)
