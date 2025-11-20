# RailML Support in FDC_Scheduler

## Overview

FDC_Scheduler v2.0 provides comprehensive support for importing and exporting railway network data in **RailML** format. RailML is an XML-based standard for exchanging railway infrastructure and timetable data.

**Supported Versions:**
- RailML 2.x (classic format, widely adopted)
- RailML 3.x (new standard with enhanced structure)

## Features

### Export Capabilities

✅ **Infrastructure Export**
- Operational points (stations, junctions)
- Track connections with properties
- Distance and speed information
- Track types (single, double, high-speed)

✅ **Timetable Export**
- Train schedules with stops
- Arrival and departure times
- Platform assignments
- Train identifiers and codes

✅ **Metadata**
- Creator information
- Creation timestamps
- Custom IDs for infrastructure/timetable

✅ **Flexible Options**
- Pretty-printed or compact XML
- Selective export (infrastructure/timetable)
- Custom element IDs

### Import Capabilities

⚠️ **Partial Implementation**

The RailML parser is structurally complete with:
- XML parsing using pugixml library
- Version auto-detection (2.x vs 3.x)
- Parsing framework for both versions
- Error handling and statistics

**To complete full import support**, detailed parsing logic needs to be implemented for:
- Operational points → Network nodes
- Tracks → Network edges  
- Train parts → Train schedules
- Schema-specific differences between 2.x and 3.x

## Usage

### Basic Export Example

```cpp
#include "fdc_scheduler/railway_network.hpp"
#include "fdc_scheduler/railml_exporter.hpp"

// Create a railway network
auto network = std::make_shared<RailwayNetwork>();
network->add_node(Node("A", "Station A", NodeType::STATION, 10));
network->add_node(Node("B", "Station B", NodeType::STATION, 8));
network->add_edge(Edge("A", "B", 25.0, TrackType::DOUBLE, 160.0));

// Create train schedules
std::vector<std::shared_ptr<TrainSchedule>> schedules;
// ... add schedules

// Export to RailML 3.x
RailMLExporter exporter;
RailMLExportOptions options;
options.pretty_print = true;
options.export_infrastructure = true;
options.export_timetable = true;

bool success = exporter.export_to_file(
    "network.railml",
    *network,
    schedules,
    RailMLExportVersion::VERSION_3,
    options
);

if (success) {
    auto stats = exporter.get_statistics();
    std::cout << "Exported " << stats["stations"] << " stations\n";
    std::cout << "Exported " << stats["tracks"] << " tracks\n";
    std::cout << "Exported " << stats["trains"] << " trains\n";
} else {
    std::cerr << "Error: " << exporter.get_last_error() << "\n";
}
```

### Export to String

```cpp
// Export to XML string instead of file
std::string xml = exporter.export_to_string(
    *network,
    schedules,
    RailMLExportVersion::VERSION_2,
    options
);

// Use xml string for API transmission, validation, etc.
```

### Convenience Functions

```cpp
// Export network only (no timetable)
export_railml_network(
    "infrastructure.railml",
    *network,
    RailMLExportVersion::VERSION_3
);

// Export timetable with associated network
export_railml_schedules(
    "timetable.railml",
    schedules,
    *network,
    RailMLExportVersion::VERSION_2
);
```

### Import Example (Framework)

```cpp
#include "fdc_scheduler/railml_parser.hpp"

// Parse RailML file
RailMLParser parser;
bool success = parser.parse_file("network.railml");

if (success) {
    auto network = parser.get_network();
    auto schedules = parser.get_schedules();
    auto stats = parser.get_statistics();
    
    // Use imported network and schedules
} else {
    std::cerr << "Parse error: " << parser.get_last_error() << "\n";
}
```

## Export Options

### RailMLExportOptions Structure

```cpp
struct RailMLExportOptions {
    bool pretty_print = true;           // Format with indentation
    bool include_metadata = true;       // Include creator/timestamp
    bool export_infrastructure = true;  // Export network structure
    bool export_timetable = true;       // Export train schedules
    bool export_rolling_stock = false;  // Export train definitions (future)
    std::string infrastructure_id = ""; // Custom infrastructure ID
    std::string timetable_id = "";      // Custom timetable ID
};
```

### Common Configurations

**Infrastructure Only:**
```cpp
RailMLExportOptions options;
options.export_infrastructure = true;
options.export_timetable = false;
```

**Timetable Only:**
```cpp
RailMLExportOptions options;
options.export_infrastructure = false;
options.export_timetable = true;
```

**Compact Format (no pretty print):**
```cpp
RailMLExportOptions options;
options.pretty_print = false;
```

## RailML Version Differences

### RailML 2.x Structure

```xml
<railml version="2.4">
  <infrastructure>
    <operationalPoints>
      <ocp id="station_1" name="Station A">
        <propOperational operationalType="station"/>
      </ocp>
    </operationalPoints>
    <tracks>
      <track id="track_1">
        <trackBegin ref="station_1"/>
        <trackEnd ref="station_2"/>
        <trackTopology>
          <trackElements>
            <line length="25000"/>
          </trackElements>
        </trackTopology>
      </track>
    </tracks>
  </infrastructure>
  <timetable>
    <trainParts>
      <trainPart id="train_1" code="IC123">
        <ocpsTT>
          <ocpTT ocpRef="station_1">
            <times scope="scheduled" arrival="10:00:00" departure="10:05:00"/>
          </ocpTT>
        </ocpsTT>
      </trainPart>
    </trainParts>
  </timetable>
</railml>
```

### RailML 3.x Structure

```xml
<railml version="3.1">
  <infrastructure>
    <functionalInfrastructure>
      <operationalPoints>
        <operationalPoint id="op_1" name="Station A"/>
      </operationalPoints>
    </functionalInfrastructure>
    <topology>
      <netElements>
        <netElement id="ne_1">
          <relation ref="ne_2"/>
        </netElement>
      </netElements>
    </topology>
  </infrastructure>
  <timetable>
    <trainParts>
      <trainPart id="tp_1">
        <operationalTrainNumber>IC123</operationalTrainNumber>
        <trainPartSequence sequence="1">
          <course>
            <ocpRef ref="op_1">
              <times scope="scheduled" arrival="10:00:00" departure="10:05:00"/>
            </ocpRef>
          </course>
        </trainPartSequence>
      </trainPart>
    </trainParts>
  </timetable>
</railml>
```

**Key Differences:**
1. RailML 3.x separates functional infrastructure from topology
2. RailML 3.x uses `<operationalPoint>` vs 2.x `<ocp>`
3. RailML 3.x has `<netElements>` for topology vs 2.x `<tracks>`
4. RailML 3.x has more structured train part sequences

## Data Mapping

### Network → RailML

| FDC_Scheduler | RailML 2.x | RailML 3.x |
|---------------|------------|------------|
| Node (Station) | `<ocp>` with `operationalType="station"` | `<operationalPoint>` |
| Edge (Track) | `<track>` with `<trackBegin>` and `<trackEnd>` | `<netElement>` with `<relation>` |
| Distance | `<line length="...">` (meters) | Associated positioning system |
| TrackType | Inferred from properties | Topology metadata |
| Max Speed | Track attributes | Speed profile |

### Schedule → RailML

| FDC_Scheduler | RailML 2.x | RailML 3.x |
|---------------|------------|------------|
| TrainSchedule | `<trainPart>` | `<trainPart>` with `<trainPartSequence>` |
| ScheduleStop | `<ocpTT>` | `<ocpRef>` in `<course>` |
| Arrival/Departure | `<times>` attributes | `<times>` attributes |
| Platform | Track assignment (if specified) | Resource allocation |

## Validation

Generated RailML files can be validated using official RailML tools:

**RailML Validation:**
- Online validator: https://www.railml.org/en/public-relations/validation.html
- Schema files: https://www.railml.org/en/download/schemas.html

**Common Validation Issues:**
- Missing required attributes
- Invalid time formats (should be HH:MM:SS)
- Invalid references between elements
- Schema version mismatches

## Advanced Usage

### Custom Infrastructure IDs

```cpp
RailMLExportOptions options;
options.infrastructure_id = "network_italy_north";
options.timetable_id = "timetable_2024_winter";
```

### Export Statistics

```cpp
auto stats = exporter.get_statistics();
std::cout << "Stations: " << stats["stations"] << "\n";
std::cout << "Tracks: " << stats["tracks"] << "\n";
std::cout << "Trains: " << stats["trains"] << "\n";
```

### Error Handling

```cpp
if (!exporter.export_to_file(...)) {
    std::string error = exporter.get_last_error();
    // Handle specific error cases
    if (error.find("Cannot create file") != std::string::npos) {
        // File system error
    } else if (error.find("Invalid network") != std::string::npos) {
        // Data validation error
    }
}
```

## Integration with FDC_Scheduler

### Complete Workflow Example

```cpp
// 1. Create network
auto network = std::make_shared<RailwayNetwork>();
network->add_node(Node("Milan", "Milano Centrale", NodeType::STATION, 24));
network->add_node(Node("Rome", "Roma Termini", NodeType::STATION, 32));
network->add_edge(Edge("Milan", "Rome", 477.0, TrackType::HIGH_SPEED, 300.0));

// 2. Create schedules
auto schedule = std::make_shared<TrainSchedule>("FR_9600");
schedule->add_stop(ScheduleStop("Milan", 
    std::chrono::system_clock::now() + std::chrono::hours(10),
    std::chrono::system_clock::now() + std::chrono::hours(10) + std::chrono::minutes(5),
    true));
schedule->add_stop(ScheduleStop("Rome",
    std::chrono::system_clock::now() + std::chrono::hours(13),
    std::chrono::system_clock::now() + std::chrono::hours(13) + std::chrono::minutes(5),
    true));

// 3. Detect conflicts
ConflictDetector detector(*network);
auto conflicts = detector.detect_conflicts({schedule});

// 4. Resolve conflicts with RailwayAI
RailwayAIResolver resolver(*network);
auto result = resolver.resolve_conflicts(conflicts, {schedule});

// 5. Export final solution to RailML
RailMLExporter exporter;
exporter.export_to_file("solution.railml", *network, 
                        result.modified_trains,
                        RailMLExportVersion::VERSION_3);
```

## Limitations and Future Work

### Current Limitations

1. **Import:** Partial implementation - framework ready but detailed parsing needed
2. **Rolling Stock:** Not yet supported (planned for future version)
3. **Signals:** Not included in current export
4. **Routes:** Route information not explicitly exported
5. **Track Occupancy:** Real-time occupancy not tracked

### Planned Enhancements

- [ ] Complete RailML 2.x/3.x import with full schema support
- [ ] Rolling stock definitions (train compositions)
- [ ] Signal and interlocking data
- [ ] Route and path information
- [ ] Extended metadata (operating companies, line numbers)
- [ ] ETCS and ATP system data
- [ ] Infrastructure manager information
- [ ] Performance characteristics

## Examples

The repository includes a complete example demonstrating RailML functionality:

```bash
# Build and run RailML example
cd build/examples
./railml_example
```

This example:
1. Creates a test railway network
2. Generates train schedules
3. Exports to RailML 2.x and 3.x
4. Displays export statistics
5. Shows XML preview

**Output Files:**
- `test_network_v2.railml` - RailML 2.4 format
- `test_network_v3.railml` - RailML 3.1 format

## References

- **RailML Official Site:** https://www.railml.org/
- **RailML 2.x Documentation:** https://wiki2.railml.org/
- **RailML 3.x Documentation:** https://wiki3.railml.org/
- **pugixml Library:** https://pugixml.org/

## License

RailML support in FDC_Scheduler follows the same license as the main project.
RailML is a registered trademark of railML.org e.V.
