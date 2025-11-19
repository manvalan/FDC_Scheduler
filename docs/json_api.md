# FDC_Scheduler JSON API Reference

Complete reference for the FDC_Scheduler JSON API.

## Table of Contents

- [Network Operations](#network-operations)
- [Schedule Operations](#schedule-operations)
- [Conflict Detection](#conflict-detection)
- [Configuration](#configuration)

---

## Network Operations

### Load Network

Load a railway network from file (JSON, RailML 2.x, or RailML 3.x).

**Endpoint**: `load_network(path)`

**Request**:
```cpp
std::string result = api.load_network("network.json");
```

**Response**:
```json
{
  "success": true,
  "message": "Network loaded successfully",
  "network": {
    "nodes": 15,
    "edges": 23,
    "total_length_km": 450.5
  }
}
```

### Add Station

Add a new station to the network.

**Endpoint**: `add_station(node_json)`

**Request**:
```json
{
  "id": "MILANO",
  "name": "Milano Centrale",
  "type": "station",
  "platforms": 12,
  "latitude": 45.4865,
  "longitude": 9.2041
}
```

**Response**:
```json
{
  "success": true,
  "message": "Station added successfully",
  "node_id": "MILANO"
}
```

### Add Track Section

Add a track section between two stations.

**Endpoint**: `add_track_section(edge_json)`

**Request**:
```json
{
  "from": "MILANO",
  "to": "COMO",
  "distance": 45.0,
  "max_speed": 140.0,
  "track_type": "double",
  "bidirectional": true
}
```

**Response**:
```json
{
  "success": true,
  "message": "Track section added successfully"
}
```

---

## Schedule Operations

### Add Train

Add a new train schedule.

**Endpoint**: `add_train(train_json)`

**Request**:
```json
{
  "train_id": "IC101",
  "train_type": "InterCity",
  "stops": [
    {
      "node_id": "MILANO",
      "arrival": "08:00",
      "departure": "08:05",
      "platform": 3
    },
    {
      "node_id": "COMO",
      "arrival": "08:30",
      "departure": "08:30",
      "platform": 1
    }
  ]
}
```

**Response**:
```json
{
  "success": true,
  "message": "Train added successfully",
  "train_id": "IC101"
}
```

### Get All Trains

Retrieve all train schedules.

**Endpoint**: `get_all_trains()`

**Response**:
```json
{
  "trains": [
    {
      "train_id": "IC101",
      "train_type": "InterCity",
      "stops": [...],
      "total_distance_km": 45.0,
      "total_time_minutes": 30
    }
  ],
  "total": 1
}
```

### Update Train

Update an existing train schedule.

**Endpoint**: `update_train(train_id, updates_json)`

**Request**:
```json
{
  "stops": [
    {
      "node_id": "MILANO",
      "departure": "08:10"
    }
  ]
}
```

**Response**:
```json
{
  "success": true,
  "message": "Train updated successfully"
}
```

---

## Conflict Detection

### Detect All Conflicts

Detect conflicts across all schedules.

**Endpoint**: `detect_conflicts()`

**Response**:
```json
{
  "conflicts": [
    {
      "type": "platform_conflict",
      "train1": "IC101",
      "train2": "R205",
      "location": "Como San Giovanni",
      "platform": 1,
      "time": "08:25",
      "severity": 8,
      "description": "Platform conflict: both trains at platform 1"
    },
    {
      "type": "section_overlap",
      "train1": "IC101",
      "train2": "R203",
      "location": "Milano Centrale → Monza",
      "time": "08:08",
      "severity": 6,
      "description": "Trains on same section with insufficient buffer"
    }
  ],
  "total": 2,
  "by_type": {
    "platform_conflict": 1,
    "section_overlap": 1,
    "head_on_collision": 0,
    "timing_violation": 0
  }
}
```

### Detect Conflicts for Train

Detect conflicts involving a specific train.

**Endpoint**: `detect_conflicts_for_train(train_id)`

**Response**:
```json
{
  "train_id": "IC101",
  "conflicts": [...],
  "total": 1
}
```

### Validate Schedule

Validate entire schedule or specific train.

**Endpoint**: `validate_schedule(train_id = "")`

**Response**:
```json
{
  "valid": false,
  "conflicts": [...],
  "violations": [
    {
      "type": "insufficient_dwell_time",
      "train_id": "IC101",
      "location": "MONZA",
      "details": "Dwell time too short: 2 min (minimum: 3 min)"
    }
  ],
  "total_issues": 3
}
```

---

## AI Integration

### Resolve Conflicts with AI

Format conflicts for external AI optimizer.

**Endpoint**: `resolve_conflicts_with_ai(conflicts_json, constraints_json = "")`

**Request** (conflicts):
```json
{
  "conflicts": [
    {
      "type": "platform_conflict",
      "train1": "IC101",
      "train2": "R205",
      "location": "COMO",
      "platform": 1
    }
  ]
}
```

**Request** (constraints - optional):
```json
{
  "constraints": [
    {
      "train_id": "IC101",
      "type": "max_delay",
      "value": 5
    }
  ]
}
```

**Response**:
```json
{
  "success": true,
  "request_formatted": true,
  "formatted_request": {
    "network": {...},
    "trains": [...],
    "conflicts": [...]
  },
  "note": "Send this to your AI optimization engine"
}
```

### Apply Modifications

Apply modifications from AI optimizer.

**Endpoint**: `apply_modifications(modifications_json)`

**Request**:
```json
{
  "modifications": [
    {
      "train_id": "IC101",
      "type": "platform_change",
      "details": {
        "node_id": "COMO",
        "old_platform": 1,
        "new_platform": 2
      }
    },
    {
      "train_id": "R205",
      "type": "departure_delay",
      "details": {
        "node_id": "COMO",
        "delay_minutes": 5
      }
    }
  ]
}
```

**Response**:
```json
{
  "success": true,
  "applied": 2,
  "failed": 0,
  "details": [
    {
      "train_id": "IC101",
      "modification": "platform_change",
      "status": "applied"
    }
  ]
}
```

---

## Configuration

### Get Configuration

Retrieve current conflict detector configuration.

**Endpoint**: `get_config()`

**Response**:
```json
{
  "section_buffer_seconds": 119,
  "platform_buffer_seconds": 300,
  "head_on_buffer_seconds": 600,
  "detect_section_conflicts": true,
  "detect_platform_conflicts": true,
  "detect_head_on_collisions": true,
  "detect_timing_violations": true
}
```

### Update Configuration

Update conflict detector settings.

**Endpoint**: `set_config(config_json)`

**Request**:
```json
{
  "platform_buffer_seconds": 240,
  "detect_timing_violations": false
}
```

**Response**:
```json
{
  "success": true,
  "message": "Configuration updated successfully"
}
```

### Get Statistics

Get library usage statistics.

**Endpoint**: `get_statistics()`

**Response**:
```json
{
  "total_checks": 156,
  "section_conflicts_found": 3,
  "platform_conflicts_found": 1,
  "head_on_collisions_found": 0,
  "timing_violations_found": 2,
  "trains_loaded": 12,
  "nodes_loaded": 8,
  "edges_loaded": 10
}
```

---

## Error Responses

All endpoints return error responses in this format:

```json
{
  "success": false,
  "error": "Detailed error message",
  "error_code": "ERROR_TRAIN_NOT_FOUND"
}
```

Common error codes:
- `ERROR_NETWORK_NOT_LOADED`
- `ERROR_TRAIN_NOT_FOUND`
- `ERROR_INVALID_JSON`
- `ERROR_INVALID_PARAMETERS`
- `ERROR_FILE_NOT_FOUND`
- `ERROR_PARSING_FAILED`
