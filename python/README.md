# FDC_Scheduler Python Bindings

Python bindings for FDC_Scheduler using pybind11.

## Features

Complete Python interface to:
- ✅ Railway network management (graph operations)
- ✅ Train scheduling and timetables
- ✅ Conflict detection
- ✅ AI-powered conflict resolution
- ✅ RailML 2.x/3.x import/export
- ✅ Pathfinding algorithms

## Building

### Requirements

- Python 3.7+
- CMake 3.15+
- C++17 compiler
- pybind11 (auto-downloaded)

### Build Instructions

```bash
# Configure with Python bindings enabled
cmake -B build -DFDC_SCHEDULER_BUILD_PYTHON=ON

# Build
cmake --build build

# The module will be in build/python/pyfdc_scheduler.*
```

### Install (Optional)

```bash
# Install to Python site-packages
cmake --install build
```

## Usage

### Basic Example

```python
import pyfdc_scheduler as fdc

# Create network
network = fdc.RailwayNetwork()

# Add stations
network.add_node(fdc.Node("A", "Station A", fdc.NodeType.STATION, 10))
network.add_node(fdc.Node("B", "Station B", fdc.NodeType.STATION, 8))

# Add track
network.add_edge(fdc.Edge("A", "B", 25.0, fdc.TrackType.DOUBLE, 160.0))

# Create schedule
import datetime
base_time = datetime.datetime.now()

schedule = fdc.TrainSchedule("IC_100")
schedule.add_stop(fdc.ScheduleStop(
    "A",
    base_time,
    base_time + datetime.timedelta(minutes=5),
    True
))
schedule.add_stop(fdc.ScheduleStop(
    "B",
    base_time + datetime.timedelta(minutes=30),
    base_time + datetime.timedelta(minutes=35),
    True
))

# Detect conflicts
detector = fdc.ConflictDetector(network)
conflicts = detector.detect_conflicts([schedule])

# Resolve with AI
resolver = fdc.RailwayAIResolver(network)
result = resolver.resolve_conflicts(conflicts, [schedule])

print(f"Success: {result.success}")
print(f"Quality: {result.quality_score}")
```

### RailML Export

```python
import pyfdc_scheduler as fdc

# ... create network and schedules ...

exporter = fdc.RailMLExporter()
options = fdc.RailMLExportOptions()
options.pretty_print = True

success = exporter.export_to_file(
    "network.railml",
    network,
    schedules,
    fdc.RailMLExportVersion.VERSION_3,
    options
)

if success:
    stats = exporter.get_statistics()
    print(f"Exported {stats['trains']} trains")
```

### Pathfinding

```python
path = network.find_shortest_path("Milano", "Roma")
print(f"Route: {' -> '.join(path)}")
```

## API Reference

### Classes

#### RailwayNetwork
- `add_node(node: Node) -> bool`
- `add_edge(edge: Edge) -> bool`
- `find_shortest_path(start: str, end: str) -> list[str]`
- `get_node_count() -> int`
- `get_edge_count() -> int`

#### Node
- `__init__(id: str, name: str, type: NodeType, platforms: int)`
- `get_id() -> str`
- `get_name() -> str`
- `get_platforms() -> int`

#### Edge
- `__init__(from_node: str, to_node: str, distance: float, track_type: TrackType, max_speed: float)`
- `get_distance() -> float`
- `get_max_speed() -> float`

#### TrainSchedule
- `__init__(train_id: str)`
- `add_stop(stop: ScheduleStop)`
- `get_stops() -> list[ScheduleStop]`

#### ScheduleStop
- `__init__(node_id: str, arrival: datetime, departure: datetime, is_stop: bool)`
- Properties: `node_id`, `arrival`, `departure`, `platform`, `is_stop`

#### ConflictDetector
- `__init__(network: RailwayNetwork)`
- `detect_conflicts(schedules: list[TrainSchedule]) -> list[Conflict]`
- `set_buffer_time(seconds: int)`

#### RailwayAIResolver
- `__init__(network: RailwayNetwork, config: RailwayAIConfig = None)`
- `resolve_conflicts(conflicts: list[Conflict], schedules: list[TrainSchedule]) -> ResolutionResult`

### Enumerations

#### NodeType
- `STATION` - Major station
- `JUNCTION` - Track junction
- `HALT` - Small halt
- `DEPOT` - Depot/maintenance

#### TrackType
- `SINGLE` - Single track (bidirectional)
- `DOUBLE` - Double track
- `HIGH_SPEED` - High-speed line
- `URBAN` - Urban/metro

#### TrainType
- `REGIONAL`
- `INTERCITY`
- `HIGH_SPEED`
- `FREIGHT`
- `SUBURBAN`

#### ConflictType
- `SECTION_CONFLICT` - Track section occupied
- `PLATFORM_CONFLICT` - Platform conflict at station
- `HEAD_ON_CONFLICT` - Head-on collision risk

#### ResolutionStrategy
- `DELAY_TRAIN` - Temporal separation
- `CHANGE_PLATFORM` - Spatial separation
- `ADD_OVERTAKING_POINT` - Single track meets
- `ADJUST_SPEED` - Dynamic timing
- `PRIORITY_BASED` - Hierarchical resolution

## Examples

See `python/example.py` for a comprehensive example demonstrating:
- Network creation (Milano-Roma high-speed line)
- Train scheduling
- Conflict detection
- AI-powered resolution
- RailML export
- Pathfinding

Run it with:
```bash
python python/example.py
```

## Type Hints

The bindings include full type information for Python IDEs:

```python
def add_node(self, node: Node) -> bool: ...
def find_shortest_path(self, start: str, end: str) -> list[str]: ...
```

## Performance

Python bindings have minimal overhead:
- Network operations: < 1μs overhead
- Conflict detection: Same as C++ (O(N²))
- Object creation: ~100ns overhead

Large datasets (1000+ trains) are handled efficiently through C++ backend.

## Platform Support

Tested on:
- ✅ Linux (Ubuntu 20.04+, x86_64, ARM64)
- ✅ macOS (10.15+, x86_64, Apple Silicon)
- ✅ Windows (10+, x86_64)

## License

Same license as FDC_Scheduler (see main LICENSE file).

## Contributing

For issues or contributions, see the main FDC_Scheduler repository.
