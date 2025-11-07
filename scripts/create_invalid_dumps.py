#!/usr/bin/env python3
"""
Create test dumps with buf.validate constraint violations.
Requires: pip install protobuf==5.29.2
"""

import sys
import os

# Add jettison_proto_python to path
proto_path = "/home/jare/git/jettison_proto_python"
if os.path.exists(proto_path):
    sys.path.insert(0, proto_path)
else:
    print(f"Error: {proto_path} not found. Install jettison_proto_python first.")
    sys.exit(1)

try:
    from jon_shared_data_pb2 import JonGUIState
except ImportError as e:
    print(f"Error importing protobuf: {e}")
    print("Install Python proto bindings: pip install protobuf==5.29.2")
    sys.exit(1)


def create_compass_azimuth_too_high():
    """Create dump with compass.azimuth >= 360 (violates: lt: 360)"""
    with open("dumps/state_0001.bin", "rb") as f:
        data = f.read()

    state = JonGUIState()
    state.ParseFromString(data)

    # Violate: azimuth should be [0, 360), set to 400
    state.compass.azimuth = 400.0

    with open("test_dumps/compass_azimuth_too_high.bin", "wb") as f:
        f.write(state.SerializeToString())

    print("✓ Created: compass_azimuth_too_high.bin (azimuth=400, valid range: [0, 360))")


def create_compass_azimuth_negative():
    """Create dump with compass.azimuth < 0 (violates: gte: 0)"""
    with open("dumps/state_0001.bin", "rb") as f:
        data = f.read()

    state = JonGUIState()
    state.ParseFromString(data)

    # Violate: azimuth should be >= 0, set to -45
    state.compass.azimuth = -45.0

    with open("test_dumps/compass_azimuth_negative.bin", "wb") as f:
        f.write(state.SerializeToString())

    print("✓ Created: compass_azimuth_negative.bin (azimuth=-45, valid range: [0, 360))")


def create_compass_elevation_too_high():
    """Create dump with compass.elevation > 90 (violates: lte: 90)"""
    with open("dumps/state_0001.bin", "rb") as f:
        data = f.read()

    state = JonGUIState()
    state.ParseFromString(data)

    # Violate: elevation should be [-90, 90], set to 120
    state.compass.elevation = 120.0

    with open("test_dumps/compass_elevation_too_high.bin", "wb") as f:
        f.write(state.SerializeToString())

    print("✓ Created: compass_elevation_too_high.bin (elevation=120, valid range: [-90, 90])")


def create_compass_bank_too_low():
    """Create dump with compass.bank < -180 (violates: gte: -180)"""
    with open("dumps/state_0001.bin", "rb") as f:
        data = f.read()

    state = JonGUIState()
    state.ParseFromString(data)

    # Violate: bank should be [-180, 180), set to -200
    state.compass.bank = -200.0

    with open("test_dumps/compass_bank_too_low.bin", "wb") as f:
        f.write(state.SerializeToString())

    print("✓ Created: compass_bank_too_low.bin (bank=-200, valid range: [-180, 180))")


def create_multiple_violations():
    """Create dump with multiple validation violations"""
    with open("dumps/state_0001.bin", "rb") as f:
        data = f.read()

    state = JonGUIState()
    state.ParseFromString(data)

    # Multiple violations
    state.compass.azimuth = 500.0       # > 360
    state.compass.elevation = -100.0    # < -90
    state.compass.bank = 200.0          # >= 180

    with open("test_dumps/multiple_violations.bin", "wb") as f:
        f.write(state.SerializeToString())

    print("✓ Created: multiple_violations.bin (azimuth=500, elevation=-100, bank=200)")


def main():
    os.makedirs("test_dumps", exist_ok=True)

    print("Creating test dumps with buf.validate violations...\n")

    create_compass_azimuth_too_high()
    create_compass_azimuth_negative()
    create_compass_elevation_too_high()
    create_compass_bank_too_low()
    create_multiple_violations()

    print(f"\n✓ All test dumps created in test_dumps/")
    print(f"  Total files: {len(os.listdir('test_dumps'))}")


if __name__ == "__main__":
    main()
