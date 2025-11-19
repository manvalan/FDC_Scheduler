#pragma once

/**
 * @file fdc_scheduler.hpp
 * @brief Main header file for FDC_Scheduler library
 * 
 * Include this single header to access all FDC_Scheduler functionality.
 */

// Core components
#include "fdc_scheduler/node.hpp"
#include "fdc_scheduler/edge.hpp"
#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/train.hpp"
#include "fdc_scheduler/schedule.hpp"

// Conflict detection
#include "fdc_scheduler/conflict_detector.hpp"

// RailML support
#include "fdc_scheduler/railml_parser.hpp"
#include "fdc_scheduler/railml_exporter.hpp"

// JSON API
#include "fdc_scheduler/json_api.hpp"

/**
 * @namespace fdc_scheduler
 * @brief Main namespace for FDC_Scheduler library
 */
namespace fdc_scheduler {

/**
 * @brief Library version information
 */
struct Version {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    
    /**
     * @brief Get version as string (e.g., "1.0.0")
     */
    static std::string to_string() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
};

/**
 * @brief Initialize FDC_Scheduler library
 * 
 * Call this before using the library (optional, but recommended).
 * Performs any necessary global initialization.
 */
inline void initialize() {
    // Reserved for future use (logging setup, resource allocation, etc.)
}

/**
 * @brief Cleanup FDC_Scheduler library
 * 
 * Call this when done using the library (optional).
 * Performs any necessary global cleanup.
 */
inline void cleanup() {
    // Reserved for future use (resource deallocation, etc.)
}

} // namespace fdc_scheduler
