# FDC_Scheduler

**Railway Network Scheduling Library with RailML Compatibility**

FDC_Scheduler is a comprehensive C++ library for railway network management, train scheduling, conflict detection, and timetable optimization. Designed for interoperability with the RailML standard and providing JSON API integration.

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

- 🤖 **AI Integration Ready**
  - JSON API for external optimization engines
  - RailwayAI v2.0 compatible
  - Constraint-based problem formulation

- 📄 **RailML Support**
  - Import/Export RailML 2.x
  - Import/Export RailML 3.x
  - Native FDC JSON format

- 🔧 **Flexible Architecture**
  - Header-only option available
  - Modular design
  - No GUI dependencies
  - Cross-platform (Linux, macOS, Windows)

## Quick Start

### Requirements

- C++17 or later
- CMake 3.15+
- nlohmann/json (included via FetchContent)

### Build

```bash
git clone https://github.com/YOUR_USERNAME/FDC_Scheduler.git
cd FDC_Scheduler
mkdir build && cd build
cmake ..
make -j$(nproc)
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

std::cout << "Found " << conflicts.size() << " conflicts\n";
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

## Architecture

```
fdc_scheduler/
├── include/fdc_scheduler/
│   ├── network.hpp          # Railway network (nodes, edges)
│   ├── schedule.hpp         # Train schedules and timetables
│   ├── train.hpp            # Train properties
│   ├── conflict_detector.hpp # Conflict detection algorithms
│   ├── railml_parser.hpp    # RailML 2.x/3.x import
│   ├── railml_exporter.hpp  # RailML 2.x/3.x export
│   └── json_api.hpp         # JSON REST API interface
├── src/                     # Implementation files
├── tests/                   # Unit tests
├── examples/                # Usage examples
└── docs/                    # Documentation
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
