cmake_minimum_required(VERSION 3.15)
project(FDC_Scheduler VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(FDC_SCHEDULER_BUILD_TESTS "Build unit tests" ON)
option(FDC_SCHEDULER_BUILD_EXAMPLES "Build examples" ON)
option(FDC_SCHEDULER_BUILD_DOCS "Build documentation" OFF)
option(FDC_SCHEDULER_HEADER_ONLY "Header-only library" OFF)

# Library type
if(FDC_SCHEDULER_HEADER_ONLY)
    set(FDC_SCHEDULER_LIB_TYPE INTERFACE)
else()
    set(FDC_SCHEDULER_LIB_TYPE STATIC)
endif()

# Dependencies
include(FetchContent)

# nlohmann/json for JSON parsing
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Main library
if(FDC_SCHEDULER_HEADER_ONLY)
    add_library(fdc_scheduler INTERFACE)
    target_include_directories(fdc_scheduler INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_compile_definitions(fdc_scheduler INTERFACE FDC_SCHEDULER_HEADER_ONLY)
else()
    file(GLOB_RECURSE FDC_SCHEDULER_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    )
    
    add_library(fdc_scheduler ${FDC_SCHEDULER_LIB_TYPE} ${FDC_SCHEDULER_SOURCES})
    
    target_include_directories(fdc_scheduler 
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

# Link dependencies
target_link_libraries(fdc_scheduler ${FDC_SCHEDULER_LIB_TYPE} nlohmann_json::nlohmann_json)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fdc_scheduler ${FDC_SCHEDULER_LIB_TYPE} 
        -Wall -Wextra -Wpedantic -Werror
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(fdc_scheduler ${FDC_SCHEDULER_LIB_TYPE} 
        /W4 /WX
    )
endif()

# Tests
if(FDC_SCHEDULER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(FDC_SCHEDULER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
if(FDC_SCHEDULER_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS fdc_scheduler
    EXPORT fdc_scheduler-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT fdc_scheduler-targets
    FILE fdc_scheduler-targets.cmake
    NAMESPACE fdc_scheduler::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdc_scheduler
)

# Package config
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fdc_scheduler-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fdc_scheduler-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdc_scheduler
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/fdc_scheduler-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/fdc_scheduler-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fdc_scheduler-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fdc_scheduler
)

# Print configuration summary
message(STATUS "")
message(STATUS "FDC_Scheduler Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Header-only: ${FDC_SCHEDULER_HEADER_ONLY}")
message(STATUS "  Build Tests: ${FDC_SCHEDULER_BUILD_TESTS}")
message(STATUS "  Build Examples: ${FDC_SCHEDULER_BUILD_EXAMPLES}")
message(STATUS "  Build Docs: ${FDC_SCHEDULER_BUILD_DOCS}")
message(STATUS "")
