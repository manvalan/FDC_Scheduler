# ============================================================================
# FDC_Scheduler Unit Tests
# Google Test based test suite
# ============================================================================

# Find or fetch Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Include GoogleTest module for test discovery
include(GoogleTest)

# Enable code coverage if requested
if(FDC_SCHEDULER_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Code coverage requested but not supported for this compiler")
    endif()
endif()

# Test sources
set(TEST_SOURCES
    test_node.cpp
    test_edge.cpp
    test_train.cpp
    test_railway_network.cpp
    test_schedule.cpp
    test_conflict_detector.cpp
    test_json_api.cpp
    test_database.cpp
    test_telemetry.cpp
    test_config_manager.cpp
    test_logger.cpp
    test_heavy_scenarios.cpp
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
            GTest::gmock
    )
    
    # Set test properties
    set_target_properties(${TEST_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
    
    # Register with CTest
    gtest_discover_tests(${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
            LABELS "unit"
    )
endforeach()

# Copy test data files to build directory
file(GLOB TEST_DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.json" "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")
foreach(TEST_DATA_FILE ${TEST_DATA_FILES})
    get_filename_component(FILE_NAME ${TEST_DATA_FILE} NAME)
    configure_file(
        ${TEST_DATA_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}
        COPYONLY
    )
endforeach()

# Summary
message(STATUS "Unit tests configured:")
message(STATUS "  Google Test: v1.14.0")
message(STATUS "  Test count: ${TEST_SOURCES}")
message(STATUS "  Coverage: ${FDC_SCHEDULER_ENABLE_COVERAGE}")
