cmake_minimum_required(VERSION 3.15)
project(FDC_Scheduler VERSION 2.0.0 LANGUAGES CXX)

# ============================================================================
# FDC_Scheduler - Standalone Railway Network Scheduling Library
# Version 2.0: Complete and autonomous, no external dependencies
# ============================================================================

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Options
option(FDC_SCHEDULER_BUILD_EXAMPLES "Build examples" ON)
option(FDC_SCHEDULER_BUILD_TESTS "Build tests" OFF)
option(FDC_SCHEDULER_BUILD_SHARED "Build shared library" OFF)
option(FDC_SCHEDULER_BUILD_PYTHON "Build Python bindings" OFF)

# Include our own headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Fetch dependencies
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# pugixml for RailML XML parsing
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)
FetchContent_MakeAvailable(pugixml)

# pybind11 for Python bindings (only if Python bindings enabled)
if(FDC_SCHEDULER_BUILD_PYTHON)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# Google Test (only if tests enabled)
if(FDC_SCHEDULER_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Enable testing
    enable_testing()
    include(GoogleTest)
endif()

# Find Boost (required for graph algorithms)
find_package(Boost REQUIRED COMPONENTS graph)

# ============================================================================
# FDC_Scheduler Library - All sources included
# ============================================================================

set(SCHEDULER_SOURCES
    # Core data structures
    src/node.cpp
    src/edge.cpp
    src/train.cpp
    src/railway_network.cpp
    src/schedule.cpp
    
    # Conflict detection and resolution
    src/conflict_detector.cpp
    src/railway_ai_resolver.cpp
    
    # API layers
    src/json_api.cpp
    src/json_api_wrapper.cpp
    
    # Import/Export
    src/railml_parser.cpp
    src/railml_exporter.cpp
    
    # Optimization
    src/route_optimizer.cpp
)

# Create library (static by default, shared if option enabled)
if(FDC_SCHEDULER_BUILD_SHARED)
    add_library(fdc_scheduler SHARED ${SCHEDULER_SOURCES})
    set_target_properties(fdc_scheduler PROPERTIES 
        VERSION ${PROJECT_VERSION}
        SOVERSION 2
    )
else()
    add_library(fdc_scheduler STATIC ${SCHEDULER_SOURCES})
endif()

# Link dependencies
target_link_libraries(fdc_scheduler
    PUBLIC
        nlohmann_json::nlohmann_json
        Boost::graph
        pugixml::pugixml
)

# Include directories
target_include_directories(fdc_scheduler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Export compile definitions
target_compile_definitions(fdc_scheduler
    PUBLIC
        FDC_SCHEDULER_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FDC_SCHEDULER_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FDC_SCHEDULER_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

message(STATUS "FDC_Scheduler v${PROJECT_VERSION} - Standalone mode")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${FDC_SCHEDULER_BUILD_SHARED}")
message(STATUS "  Examples: ${FDC_SCHEDULER_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${FDC_SCHEDULER_BUILD_TESTS}")

# Examples
if(FDC_SCHEDULER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(FDC_SCHEDULER_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Python bindings
if(FDC_SCHEDULER_BUILD_PYTHON)
    add_subdirectory(python)
endif()

# Install rules
install(TARGETS fdc_scheduler
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/fdc_scheduler
    DESTINATION include
)
