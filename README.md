# FDC_Scheduler

**Complete Standalone Railway Network Scheduling Library**

[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](https://github.com/yourusername/FDC_Scheduler)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

FDC_Scheduler is a **complete and autonomous** C++ library for railway network management, train scheduling, conflict detection, and AI-powered resolution. Version 2.0 includes all core functionality with no external dependencies (except Boost Graph and nlohmann/json).

## ✨ What's New in v2.0

- 🎯 **Standalone library** - no external FDC dependencies
- 🤖 **RailwayAI integration** - intelligent conflict resolution
- 🚄 **Complete implementation** - all core features included
- 📦 **Simple build** - single CMake project
- 🎨 **Clean API** - modern C++17 interface

## Features

- 🚂 **Complete Railway Network Management**
  - Nodes (stations, junctions, halts)
  - Edges (track sections with capacity, speed limits)
  - Multi-platform station support
  
- 📅 **Advanced Train Scheduling**
  - Train timetables with precise timing
  - Platform assignments
  - Dwell time management
  - Multi-stop route planning

- ⚠️ **Conflict Detection System**
  - Section conflicts (single/double track)
  - Platform conflicts at stations
  - Head-on collision detection
  - Time buffer validation (configurable)

- 🤖 **RailwayAI Conflict Resolution**
  - Automatic resolution of double track conflicts
  - Single track meet planning and collision avoidance
  - Intelligent platform reassignment at stations
  - Priority-based delay distribution
  - Quality scoring for resolution strategies
  - JSON API for external optimization engines
  - RailwayAI v2.0 compatible

- 📄 **Data Import/Export**
  - RailML 2.x/3.x export (full support)
  - RailML 2.x/3.x import (framework ready)
  - Native JSON format
  - XML parsing with pugixml

- 🔧 **Flexible Architecture**
  - Header-only option available
  - Modular design
  - No GUI dependencies
  - Cross-platform (Linux, macOS, Windows)

## Quick Start

### Requirements

**Runtime:**
- Boost >= 1.70 (Graph library)
- C++17 compliant compiler (GCC 7.3+, Clang 6.0+, MSVC 2017+)

**Build-time:**
- CMake >= 3.15
- nlohmann/json (auto-downloaded via FetchContent)

**No external dependencies from other projects!**

### Build

```bash
git clone https://github.com/YOUR_USERNAME/FDC_Scheduler.git
cd FDC_Scheduler

# Quick build
./build.sh

# Or manual
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run demo
./examples/railway_ai_integration_example
```

### Install

```bash
cd build
sudo make install
# Installs to /usr/local/lib and /usr/local/include
```

### Basic Usage

```cpp
#include <fdc_scheduler/network.hpp>
#include <fdc_scheduler/schedule.hpp>
#include <fdc_scheduler/conflict_detector.hpp>

using namespace fdc_scheduler;

// Create network
RailwayNetwork network;
network.add_station("MILANO", "Milano Centrale", 12); // 12 platforms
network.add_station("ROMA", "Roma Termini", 24);
network.add_track_section("MILANO", "ROMA", 480.0, 250.0, TrackType::HIGH_SPEED);

// Create train schedule
TrainSchedule train("FR9615");
train.add_stop("MILANO", "08:00", "08:00", 1);
train.add_stop("ROMA", "11:00", "11:00", 5);

// Detect conflicts
ConflictDetector detector(network);
auto conflicts = detector.detect_all(schedules);

// Resolve conflicts with RailwayAI
if (!conflicts.empty()) {
    RailwayAIResolver resolver(network);
    auto result = resolver.resolve_conflicts(schedules, conflicts);
    
    std::cout << "Resolved " << conflicts.size() << " conflicts\n";
    std::cout << "Quality score: " << result.quality_score << "\n";
}
```

### RailwayAI Conflict Resolution

```cpp
#include <fdc_scheduler/railway_ai_resolver.hpp>

// Configure AI resolver
RailwayAIConfig config;
config.allow_platform_reassignment = true;
config.allow_single_track_meets = true;
config.min_headway_seconds = 120;

RailwayAIResolver resolver(network, config);

// Resolve specific conflict types
for (const auto& conflict : conflicts) {
    ResolutionResult result;
    
    switch (conflict.type) {
        case ConflictType::SECTION_OVERLAP:
            // Handles both double and single track
            result = resolver.resolve_single_conflict(conflict, schedules);
            break;
            
        case ConflictType::PLATFORM_CONFLICT:
            // Reassigns platforms or applies delays
            result = resolver.resolve_station_conflict(conflict, schedules);
            break;
    }
    
    std::cout << "Strategy: " << strategy_to_string(result.strategy_used) << "\n";
    std::cout << "Total delay: " << result.total_delay.count() << "s\n";
}
```

## JSON API

FDC_Scheduler provides a complete JSON API for integration with external systems:

```cpp
#include <fdc_scheduler/json_api.hpp>

JsonApi api;

// Load network from RailML
std::string result = api.load_network("network.railml");

// Add train from JSON
std::string train_json = R"({
  "train_id": "IC101",
  "stops": [
    {"node_id": "MILANO", "arrival": "08:00", "departure": "08:05", "platform": 3}
  ]
})";
api.add_train(train_json);

// Detect conflicts
std::string conflicts_json = api.detect_conflicts();
```

## Examples

Run the RailwayAI integration demo:

```bash
cd build/examples
./railway_ai_integration_example
```

This demonstrates:
- ✅ Double track conflict resolution with headway management
- ✅ Single track conflict resolution with meeting point planning
- ✅ Station platform conflict resolution with reassignment
- ✅ Complex multi-train scenarios with mixed track types

## Architecture

```
fdc_scheduler/
├── include/fdc_scheduler/
│   ├── railway_network.hpp     # Railway network (nodes, edges)
│   ├── schedule.hpp            # Train schedules and timetables
│   ├── train.hpp               # Train properties
│   ├── conflict_detector.hpp   # Conflict detection algorithms
│   ├── railway_ai_resolver.hpp # AI-powered conflict resolution
│   ├── railml_parser.hpp       # RailML 2.x/3.x import
│   ├── railml_exporter.hpp     # RailML 2.x/3.x export
│   └── json_api.hpp            # JSON REST API interface
├── src/                        # Implementation files
├── tests/                      # Unit tests
├── examples/                   # Usage examples
│   └── railway_ai_integration_example.cpp
└── docs/                       # Documentation
    └── RAILWAY_AI_RESOLUTION.md
```

## RailML Compatibility

### RailML 3.x Support
- Full infrastructure import/export
- Timetable import/export
- Rolling stock definitions

### RailML 2.x Support
- Legacy format compatibility
- Automatic conversion to RailML 3.x

## Documentation

- [API Reference](docs/api_reference.md)
- [RailML Integration Guide](docs/railml_guide.md)
- [Conflict Detection Algorithms](docs/conflict_detection.md)
- [JSON API Specification](docs/json_api.md)

## Testing

```bash
cd build
ctest --output-on-failure
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

MIT License - See [LICENSE](LICENSE) for details.

## Roadmap

- [x] Core network management
- [x] Basic conflict detection
- [x] JSON API
- [ ] RailML 2.x parser
- [ ] RailML 3.x parser
- [ ] Advanced optimization algorithms
- [ ] Python bindings
- [ ] REST API server

## Credits

Developed as part of the FDC (Ferrovie Digitali Controllate) project.

**Migrated from**: [FDC Railway Manager](https://github.com/manvalan/FDC)
