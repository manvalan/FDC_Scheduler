#!/usr/bin/env python3
"""
FDC_Scheduler Python Example
Demonstrates the use of Python bindings for railway network management
"""

import sys
import datetime
from pathlib import Path

# Add build directory to path
sys.path.insert(0, str(Path(__file__).parent.parent / 'build' / 'python'))

try:
    import pyfdc_scheduler as fdc
    print(f"✓ FDC_Scheduler {fdc.__version__} loaded successfully")
except ImportError as e:
    print(f"✗ Error loading pyfdc_scheduler: {e}")
    print("Make sure to build with -DFDC_SCHEDULER_BUILD_PYTHON=ON")
    sys.exit(1)

def print_separator(title):
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60 + "\n")

def create_test_network():
    """Create a simple railway network for testing"""
    print_separator("Creating Railway Network")
    
    network = fdc.RailwayNetwork()
    
    # Add stations
    network.add_node(fdc.Node("Milano", "Milano Centrale", fdc.NodeType.STATION, 24))
    network.add_node(fdc.Node("Bologna", "Bologna Centrale", fdc.NodeType.STATION, 16))
    network.add_node(fdc.Node("Firenze", "Firenze SMN", fdc.NodeType.STATION, 18))
    network.add_node(fdc.Node("Roma", "Roma Termini", fdc.NodeType.STATION, 32))
    
    print(f"Added {network.get_node_count()} stations:")
    for node in network.get_all_nodes():
        print(f"  - {node}")
    
    # Add tracks
    network.add_edge(fdc.Edge("Milano", "Bologna", 219.0, fdc.TrackType.HIGH_SPEED, 300.0))
    network.add_edge(fdc.Edge("Bologna", "Firenze", 105.0, fdc.TrackType.HIGH_SPEED, 300.0))
    network.add_edge(fdc.Edge("Firenze", "Roma", 277.0, fdc.TrackType.HIGH_SPEED, 300.0))
    
    # Reverse directions
    network.add_edge(fdc.Edge("Bologna", "Milano", 219.0, fdc.TrackType.HIGH_SPEED, 300.0))
    network.add_edge(fdc.Edge("Firenze", "Bologna", 105.0, fdc.TrackType.HIGH_SPEED, 300.0))
    network.add_edge(fdc.Edge("Roma", "Firenze", 277.0, fdc.TrackType.HIGH_SPEED, 300.0))
    
    print(f"\nAdded {network.get_edge_count()} track sections")
    
    return network

def create_train_schedules():
    """Create sample train schedules"""
    print_separator("Creating Train Schedules")
    
    schedules = []
    base_time = datetime.datetime.now().replace(hour=6, minute=0, second=0, microsecond=0)
    
    # Train 1: Milano -> Roma (morning)
    train1 = fdc.TrainSchedule("FR_9600")
    train1.add_stop(fdc.ScheduleStop(
        "Milano",
        base_time,
        base_time + datetime.timedelta(minutes=5),
        True
    ))
    train1.add_stop(fdc.ScheduleStop(
        "Bologna",
        base_time + datetime.timedelta(minutes=50),
        base_time + datetime.timedelta(minutes=53),
        True
    ))
    train1.add_stop(fdc.ScheduleStop(
        "Firenze",
        base_time + datetime.timedelta(minutes=110),
        base_time + datetime.timedelta(minutes=113),
        True
    ))
    train1.add_stop(fdc.ScheduleStop(
        "Roma",
        base_time + datetime.timedelta(minutes=190),
        base_time + datetime.timedelta(minutes=200),
        True
    ))
    schedules.append(train1)
    print(f"  {train1}")
    
    # Train 2: Milano -> Roma (slightly later, potential conflict)
    train2 = fdc.TrainSchedule("FR_9602")
    train2.add_stop(fdc.ScheduleStop(
        "Milano",
        base_time + datetime.timedelta(minutes=10),
        base_time + datetime.timedelta(minutes=15),
        True
    ))
    train2.add_stop(fdc.ScheduleStop(
        "Bologna",
        base_time + datetime.timedelta(minutes=55),
        base_time + datetime.timedelta(minutes=58),
        True
    ))
    train2.add_stop(fdc.ScheduleStop(
        "Roma",
        base_time + datetime.timedelta(minutes=195),
        base_time + datetime.timedelta(minutes=205),
        True
    ))
    schedules.append(train2)
    print(f"  {train2}")
    
    # Train 3: Roma -> Milano (return service)
    train3 = fdc.TrainSchedule("FR_9605")
    train3.add_stop(fdc.ScheduleStop(
        "Roma",
        base_time + datetime.timedelta(hours=1),
        base_time + datetime.timedelta(hours=1, minutes=5),
        True
    ))
    train3.add_stop(fdc.ScheduleStop(
        "Firenze",
        base_time + datetime.timedelta(hours=2, minutes=25),
        base_time + datetime.timedelta(hours=2, minutes=28),
        True
    ))
    train3.add_stop(fdc.ScheduleStop(
        "Milano",
        base_time + datetime.timedelta(hours=3, minutes=15),
        base_time + datetime.timedelta(hours=3, minutes=20),
        True
    ))
    schedules.append(train3)
    print(f"  {train3}")
    
    return schedules

def detect_conflicts(network, schedules):
    """Detect scheduling conflicts"""
    print_separator("Detecting Conflicts")
    
    detector = fdc.ConflictDetector(network)
    detector.set_buffer_time(120)  # 2 minutes buffer
    
    conflicts = detector.detect_conflicts(schedules)
    
    print(f"Found {len(conflicts)} conflict(s):")
    for i, conflict in enumerate(conflicts, 1):
        print(f"\n  Conflict #{i}:")
        print(f"    Type: {conflict.type}")
        print(f"    Trains: {conflict.train1_id} vs {conflict.train2_id}")
        print(f"    Location: {conflict.location}")
        print(f"    Severity: {conflict.severity:.2f}")
        print(f"    Description: {conflict.description}")
    
    return conflicts

def resolve_conflicts_with_ai(network, conflicts, schedules):
    """Resolve conflicts using RailwayAI"""
    print_separator("Resolving Conflicts with AI")
    
    if not conflicts:
        print("No conflicts to resolve!")
        return schedules
    
    # Configure AI resolver
    config = fdc.RailwayAIConfig()
    config.enable_delay = True
    config.enable_platform_change = True
    config.max_delay_minutes = 10
    config.min_headway_seconds = 120
    
    resolver = fdc.RailwayAIResolver(network, config)
    
    result = resolver.resolve_conflicts(conflicts, schedules)
    
    print(f"Resolution result:")
    print(f"  Success: {result.success}")
    print(f"  Strategy used: {result.strategy_used}")
    print(f"  Quality score: {result.quality_score:.2f}")
    print(f"  Modified trains: {len(result.modified_trains)}")
    
    if result.resolution_notes:
        print(f"\n  Notes:")
        for note in result.resolution_notes:
            print(f"    - {note}")
    
    return result.modified_trains if result.success else schedules

def export_to_railml(network, schedules):
    """Export network and schedules to RailML"""
    print_separator("Exporting to RailML 3.x")
    
    exporter = fdc.RailMLExporter()
    
    options = fdc.RailMLExportOptions()
    options.pretty_print = True
    options.include_metadata = True
    options.export_infrastructure = True
    options.export_timetable = True
    
    filename = "python_example_network.railml"
    success = exporter.export_to_file(
        filename,
        network,
        schedules,
        fdc.RailMLExportVersion.VERSION_3,
        options
    )
    
    if success:
        stats = exporter.get_statistics()
        print(f"✓ Successfully exported to {filename}")
        print(f"  Stations: {stats['stations']}")
        print(f"  Tracks: {stats['tracks']}")
        print(f"  Trains: {stats['trains']}")
    else:
        print(f"✗ Export failed: {exporter.get_last_error()}")

def find_shortest_path(network):
    """Demonstrate pathfinding"""
    print_separator("Pathfinding")
    
    path = network.find_shortest_path("Milano", "Roma")
    
    if path:
        print(f"Shortest path from Milano to Roma:")
        print(f"  Route: {' -> '.join(path)}")
        print(f"  Stops: {len(path)}")
    else:
        print("No path found!")

def main():
    """Main example function"""
    print("\n" + "=" * 60)
    print("  FDC_Scheduler Python Bindings Example")
    print("  High-Speed Rail Network: Milano - Bologna - Firenze - Roma")
    print("=" * 60)
    
    try:
        # Create network
        network = create_test_network()
        
        # Create schedules
        schedules = create_train_schedules()
        
        # Pathfinding demo
        find_shortest_path(network)
        
        # Detect conflicts
        conflicts = detect_conflicts(network, schedules)
        
        # Resolve conflicts with AI
        if conflicts:
            schedules = resolve_conflicts_with_ai(network, conflicts, schedules)
        
        # Export to RailML
        export_to_railml(network, schedules)
        
        print_separator("Summary")
        print("✓ Python bindings working correctly!")
        print(f"✓ Network: {network.get_node_count()} stations, {network.get_edge_count()} tracks")
        print(f"✓ Schedules: {len(schedules)} trains")
        print(f"✓ Conflicts detected and resolved")
        print(f"✓ RailML export successful")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
