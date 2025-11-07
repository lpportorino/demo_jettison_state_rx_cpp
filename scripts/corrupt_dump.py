#!/usr/bin/env python3
"""
Corrupt a protobuf dump by setting values out of their valid ranges.
This tests whether buf.validate constraints catch corrupted data.
"""

import sys
import struct

def corrupt_protobuf(input_file, output_file):
    """
    Read a protobuf binary, corrupt some double values to be out of range.

    Strategy: Find double fields (wire type 1, 8 bytes) and replace them
    with extreme values like 999.0 or -999.0 to violate range constraints.
    """
    with open(input_file, 'rb') as f:
        data = bytearray(f.read())

    print(f"Original size: {len(data)} bytes")

    # Protobuf wire format:
    # - Field tag: (field_number << 3) | wire_type
    # - Wire type 1 = 64-bit (double)
    # - Wire type 0 = varint (int32, int64, uint32, uint64, bool, enum)
    # - Wire type 2 = length-delimited (string, bytes, embedded messages)

    corruptions = 0
    i = 0
    while i < len(data):
        if i + 9 > len(data):
            break

        tag = data[i]
        wire_type = tag & 0x07
        field_number = tag >> 3

        # Wire type 1 = 64-bit (double)
        if wire_type == 1:
            # Read the original double value
            original_bytes = bytes(data[i+1:i+9])
            original_value = struct.unpack('<d', original_bytes)[0]

            # Corrupt it with an out-of-range value
            # For angles: valid range is typically -360 to 360 or 0 to 360
            # For temperature: typically -100 to 100
            # We'll use 999.0 to violate most constraints
            corrupt_value = 999.0
            corrupt_bytes = struct.pack('<d', corrupt_value)

            # Replace the 8 bytes
            data[i+1:i+9] = corrupt_bytes

            print(f"  Field {field_number} (double): {original_value:.2f} -> {corrupt_value:.2f}")
            corruptions += 1

            i += 9  # tag + 8 bytes
        elif wire_type == 0:
            # Varint - skip it
            i += 1
            while i < len(data) and (data[i] & 0x80):
                i += 1
            i += 1
        elif wire_type == 2:
            # Length-delimited - skip it
            i += 1
            if i >= len(data):
                break
            # Read length varint
            length = 0
            shift = 0
            while i < len(data):
                byte = data[i]
                i += 1
                length |= (byte & 0x7f) << shift
                if not (byte & 0x80):
                    break
                shift += 7
            i += length
        else:
            # Unknown wire type, skip 1 byte
            i += 1

    print(f"\nTotal corruptions: {corruptions}")

    with open(output_file, 'wb') as f:
        f.write(data)

    print(f"Corrupted dump saved to: {output_file}")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.bin> <output.bin>")
        sys.exit(1)

    corrupt_protobuf(sys.argv[1], sys.argv[2])
