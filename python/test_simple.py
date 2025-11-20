#!/usr/bin/env python3
"""
Simple test script for FDC_Scheduler Python bindings
Tests basic functionality to ensure the bindings work correctly.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../build/python'))

import pyfdc_scheduler as fdc

def main():
    """Run basic tests"""
    print("=" * 60)
    print("FDC_Scheduler Python Bindings - Basic Test Suite")
    print("=" * 60)
    
    # Test 1: Enums
    print("\n✓ Testing enums...")
    assert hasattr(fdc, 'NodeType')
    assert hasattr(fdc, 'TrackType')
    assert hasattr(fdc, 'TrainType')
    assert hasattr(fdc, 'ConflictType')
    print("  All enums accessible")
    
    # Test 2: Network creation
    print("\n✓ Testing network...")
    network = fdc.RailwayNetwork()
    print(f"  Created: {network}")
    
    # Test 3: Node creation
    print("\n✓ Testing nodes...")
    node = fdc.Node("MILANO", "Milano Centrale", fdc.NodeType.STATION)
    print(f"  Created: {node}")
    assert network.add_node(node)
    print(f"  Added to network. Nodes: {network.num_nodes()}")
    
    # Test 4: Edge creation
    print("\n✓ Testing edges...")
    roma = fdc.Node("ROMA", "Roma Termini", fdc.NodeType.STATION)
    network.add_node(roma)
    edge = fdc.Edge("MILANO", "ROMA", 480.0, fdc.TrackType.HIGH_SPEED)
    print(f"  Created: {edge}")
    assert network.add_edge(edge)
    print(f"  Added to network. Edges: {network.num_edges()}")
    
    # Test 5: Train creation
    print("\n✓ Testing trains...")
    train = fdc.Train("IC100", "InterCity 100", fdc.TrainType.INTERCITY, 200.0)
    print(f"  Created: {train}")
    print(f"  Properties: speed={train.get_max_speed()}, accel={train.get_acceleration()}")
    
    # Test 6: Travel time
    print("\n✓ Testing travel time calculation...")
    time_hours = train.calculate_travel_time(100.0, 200.0)
    print(f"  100km @ 200km/h max: {time_hours:.3f} hours ({time_hours*60:.1f} minutes)")
    
    # Test 7: Conflict detector
    print("\n✓ Testing conflict detector...")
    detector = fdc.ConflictDetector(network)
    print(f"  Created detector")
    
    # Test 8: AI Config
    print("\n✓ Testing AI config...")
    config = fdc.RailwayAIConfig()
    print(f"  Default: max_delay={config.max_delay_minutes}min, headway={config.min_headway_seconds}s")
    config.delay_weight = 1.5
    print(f"  Modified delay_weight to {config.delay_weight}")
    
    # Test 9: RailML
    print("\n✓ Testing RailML support...")
    assert hasattr(fdc, 'RailMLVersion')
    assert hasattr(fdc, 'RailMLExporter')
    assert hasattr(fdc, 'RailMLParser')
    print(f"  RailML classes available")
    
    print("\n" + "=" * 60)
    print("✓✓✓ ALL BASIC TESTS PASSED! ✓✓✓")
    print("=" * 60)
    print(f"\nPython bindings version: {fdc.__version__}")
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"\n✗ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
