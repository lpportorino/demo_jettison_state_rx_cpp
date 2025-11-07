# Utility Scripts

This directory contains utility scripts for building, testing, and validating the Jettison State RX application.

## Scripts

### build.sh

Traditional build script for manual compilation (alternative to Docker build).

**Usage:**
```bash
scripts/build.sh [OPTIONS]

Options:
  --clean         Clean build directory before building
  --no-checks     Skip format and lint checks
  -j, --jobs N    Number of parallel build jobs (default: nproc)
  -h, --help      Show help message
```

**Prerequisites:**
- CMake 3.20+
- clang, clang-format, clang-tidy
- libprotobuf, libwebsockets, openssl, nlohmann-json

**Example:**
```bash
# Full clean build with quality checks
scripts/build.sh --clean

# Fast build without linting
scripts/build.sh --no-checks -j 8
```

### corrupt_dump.py

Testing utility that corrupts protobuf dump files for validation testing.

**Purpose:**
- Tests buf.validate constraint enforcement
- Systematically corrupts double values to 999.0 (out of valid ranges)
- Verifies error detection and reporting

**Usage:**
```bash
python3 scripts/corrupt_dump.py <input.bin> <output.bin>
```

**Example:**
```bash
# First, capture a valid dump
./Jettison_State_RX-x86_64.AppImage --uri wss://sych.local:443/ws/ws_state --insecure --dump 1

# Corrupt the dump
python3 scripts/corrupt_dump.py dumps/state_0001.bin dumps/state_corrupted.bin

# Verify validation detects the corruption
./Jettison_State_RX-x86_64.AppImage --read-dump dumps/state_corrupted.bin
```

**How it works:**
- Parses protobuf wire format (tag + wire type + value)
- Identifies wire type 1 (64-bit double fields)
- Replaces all double values with 999.0
- Reports number of corruptions made

### create_invalid_dumps.py

Creates specific test cases with targeted buf.validate violations.

**Purpose:**
- Generates test dumps with known constraint violations
- Useful for regression testing validation logic
- Creates specific edge cases (negative values, out-of-range, multiple violations)

**Prerequisites:**
- Python protobuf library: `pip install protobuf==5.29.2`
- jettison_proto_python installed at `/home/jare/git/jettison_proto_python`
- Valid dump file at `dumps/state_0001.bin`

**Usage:**
```bash
python3 scripts/create_invalid_dumps.py
```

**Creates:**
- `test_dumps/compass_azimuth_too_high.bin` - azimuth >= 360
- `test_dumps/compass_azimuth_negative.bin` - azimuth < 0
- `test_dumps/compass_elevation_too_high.bin` - elevation > 90
- `test_dumps/compass_bank_too_low.bin` - bank < -180
- `test_dumps/multiple_violations.bin` - multiple violations

**Note:** The `test_dumps/` directory is gitignored. These files are generated artifacts and can be recreated anytime by running this script.

### test_all_dumps.sh

Comprehensive test runner for dump validation.

**Purpose:**
- Tests valid dumps (should pass validation)
- Tests corrupted dumps (should fail parsing)
- Tests range violations (should detect constraint violations)

**Prerequisites:**
- `jettison_state_rx` binary built and in current directory
- Valid dumps in `dumps/`
- Test dumps in `test_dumps/`

**Usage:**
```bash
scripts/test_all_dumps.sh
```

**Output:**
- Summary of validation results for each category
- Identifies which dumps pass/fail validation
- Shows validation error messages for failures

## Directory Structure

```
scripts/
├── README.md                   # This file
├── build.sh                    # Manual build script
├── corrupt_dump.py             # Generic corruption utility
├── create_invalid_dumps.py     # Targeted test case generator
└── test_all_dumps.sh           # Test runner
```

## Testing Workflow

1. **Capture live data:**
   ```bash
   ./Jettison_State_RX-x86_64.AppImage --uri wss://sych.local:443/ws/ws_state --insecure --dump 5
   ```

2. **Create specific test cases:**
   ```bash
   python3 scripts/create_invalid_dumps.py
   ```

3. **Create generic corrupted dumps:**
   ```bash
   python3 scripts/corrupt_dump.py dumps/state_0001.bin dumps/state_corrupted.bin
   ```

4. **Run validation tests:**
   ```bash
   scripts/test_all_dumps.sh
   ```

5. **Verify results:**
   - Valid dumps: "Validation: PASSED"
   - Corrupted dumps: Parse errors or "INVALID MESSAGE"
   - Constraint violations: Detailed error messages with field names and violated constraints
