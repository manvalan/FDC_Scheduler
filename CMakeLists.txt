cmake_minimum_required(VERSION 3.15)
project(FDC_Scheduler VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(FDC_SCHEDULER_BUILD_EXAMPLES "Build examples" ON)
option(FDC_SCHEDULER_BUILD_TESTS "Build tests" OFF)

# Path to FDC core library
set(FDC_CORE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../FDC/cpp" CACHE PATH "Path to FDC core project")

# Include FDC headers
include_directories(
    ${FDC_CORE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find FDC core library
find_library(FDC_CORE_LIB
    NAMES fdc_core
    PATHS "${FDC_CORE_PATH}/build/lib"
    REQUIRED
)

message(STATUS "Found FDC core library: ${FDC_CORE_LIB}")

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Find Boost (required by FDC core)
find_package(Boost REQUIRED COMPONENTS graph)

# FDC_Scheduler library sources (only wrappers and new functionality)
set(SCHEDULER_SOURCES
    src/json_api_wrapper.cpp
    src/railml_parser.cpp
    src/railml_exporter.cpp
)

# Create library
add_library(fdc_scheduler STATIC ${SCHEDULER_SOURCES})

target_link_libraries(fdc_scheduler
    PUBLIC
        ${FDC_CORE_LIB}
        nlohmann_json::nlohmann_json
        Boost::graph
)

target_include_directories(fdc_scheduler
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${FDC_CORE_PATH}/include>
        $<INSTALL_INTERFACE:include>
)

# Examples
if(FDC_SCHEDULER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(FDC_SCHEDULER_BUILD_TESTS)
    add_subdirectory(tests)
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
