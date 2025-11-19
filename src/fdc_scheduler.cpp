#include "fdc_scheduler.hpp"

namespace fdc_scheduler {

std::string Version::to_string() {
    return std::to_string(MAJOR) + "." + 
           std::to_string(MINOR) + "." + 
           std::to_string(PATCH);
}

void initialize() {
    // Optional global initialization
    // Could be used for logging setup, etc.
}

void cleanup() {
    // Optional global cleanup
}

} // namespace fdc_scheduler
