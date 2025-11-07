# Jettison State RX

A C++ WebSocket client for receiving and validating Jettison GUI state messages with **runtime buf.validate constraint validation** using [protovalidate-cc](https://github.com/bufbuild/protovalidate-cc).

## Features

- **WebSocket Streaming**: Connect to Jettison state servers via WebSocket (ws:// or wss://)
- **Runtime Validation**: Validates all received messages using [buf.validate](https://github.com/bufbuild/protovalidate) constraints with full CEL expression support
- **Data Integrity Protection**: Detects corrupted or out-of-range values with detailed error reporting
- **JSON Output**: Converts Protocol Buffer messages to JSON for easy consumption
- **Dump Mode**: Save raw binary payloads for offline analysis
- **Read Dumps**: Validate and analyze previously saved dumps
- **Portable AppImage**: Single executable with all dependencies bundled

## Security Warning

**⚠️ IMPORTANT: This application handles sensitive data**

- State messages may contain **GPS coordinates**, **system information**, and other **sensitive operational data**
- Dump files (`dumps/*.bin`) contain raw binary payloads that may include this sensitive data
- Log output may also contain sensitive information
- SSL certificate validation is **disabled** for convenience with self-signed certificates

**Best practices:**
- Do not share dump files or logs without reviewing their contents
- Store dumps in secure locations
- Delete dumps when no longer needed
- Use this tool only on trusted networks
- Be aware that SSL certificate errors are ignored (intended for local/development use only)

## Installation

### Quick Start: Download AppImage (Recommended)

The easiest way to use Jettison State RX is to download the pre-built AppImage from the [Releases](https://github.com/YOUR_USERNAME/demo_jettison_state_rx_cpp/releases) page:

```bash
# Download the AppImage
wget https://github.com/YOUR_USERNAME/demo_jettison_state_rx_cpp/releases/latest/download/Jettison_State_RX-x86_64.AppImage

# Make it executable
chmod +x Jettison_State_RX-x86_64.AppImage

# Run it
./Jettison_State_RX-x86_64.AppImage --help
```

**Benefits of AppImage:**
- No installation required - single executable file
- No dependencies to install - everything bundled
- Works on any Linux with glibc 2.35+ (Ubuntu 22.04+)
- Completely portable - run from any directory

## Dependencies (for building from source)

- C++20 compatible compiler (clang recommended)
- CMake 3.20+
- Protocol Buffers (libprotobuf)
- libwebsockets
- OpenSSL
- nlohmann_json
- Docker (optional, for containerized builds)

## Building

### Quick Build (Recommended)

```bash
# Install dependencies (Arch Linux example)
sudo pacman -S cmake clang clang-tools-extra protobuf libwebsockets openssl nlohmann-json

# Clone with submodules
git clone --recursive git@github.com:YOUR_USERNAME/demo_jettison_state_rx_cpp.git
cd demo_jettison_state_rx_cpp

# Build (automatically runs format and lint checks)
scripts/build.sh
```

The build script will:
1. ✓ Check for required tools
2. ✓ Initialize git submodules (if needed)
3. ✓ Format code with `clang-format`
4. ✓ Lint code with `clang-tidy` (warnings treated as errors)
5. ✓ Build with full security hardening flags
6. ✓ Enable all warnings as errors

### Build Script Options

```bash
scripts/build.sh --clean        # Clean build directory first
scripts/build.sh --no-checks    # Skip format and lint checks
scripts/build.sh -j 8           # Use 8 parallel build jobs
scripts/build.sh --help         # Show all options
```

### Manual Build (Advanced)

```bash
# Initialize submodules
git submodule update --init --recursive

# Configure and build manually
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run format and lint manually
make format        # Format all code
make lint          # Run linter (informational)
make lint-strict   # Run linter (warnings as errors)
make check         # Run format + lint-strict
```

### Docker Build (Fully Reproducible)

Build in a clean, isolated Docker environment - no system dependencies required:

```bash
# Clone with submodules
git clone --recursive git@github.com:YOUR_USERNAME/demo_jettison_state_rx_cpp.git
cd demo_jettison_state_rx_cpp

# Build Docker image
docker build -t jettison-state-rx:latest --target runtime .

# Run the application
docker run --rm jettison-state-rx:latest sych.local

# Or run interactively
docker run --rm -it jettison-state-rx:latest /bin/bash
```

**Build AppImage in Docker:**

```bash
# Build the AppImage builder stage
docker build -t jettison-appimage-builder --target appimage-builder .

# Create AppImage
docker run --rm -v $(pwd)/output:/output jettison-appimage-builder bash -c "
  mkdir -p AppDir/usr/bin AppDir/usr/lib AppDir/usr/share/applications AppDir/usr/share/icons/hicolor/scalable/apps
  cp /appimage/jettison_state_rx AppDir/usr/bin/
  cp /.github/appimage-assets/jettison-state-rx.desktop AppDir/usr/share/applications/
  cp /.github/appimage-assets/jettison-state-rx.svg AppDir/usr/share/icons/hicolor/scalable/apps/
  cp /.github/appimage-assets/jettison-state-rx-launcher.sh AppDir/AppRun
  chmod +x AppDir/AppRun
  ./linuxdeploy-x86_64.AppImage --appdir AppDir --executable AppDir/usr/bin/jettison_state_rx
  ./appimagetool-x86_64.AppImage AppDir /output/jettison-state-rx.AppImage
"
```

**Docker Stages:**
- `builder` - Compiles the application with all dependencies
- `runtime` - Minimal runtime image with just the binary (~50MB)
- `appimage-builder` - Tools for creating AppImages

### Build Configuration

The build system enforces strict code quality by default:

- **Security Hardening**: `_FORTIFY_SOURCE=2`, stack protector, RELRO, NX
- **Warnings**: All warnings enabled and treated as errors (`-Werror`)
- **Format Check**: Code must match GNU style (`.clang-format`)
- **Lint Check**: Must pass `clang-tidy` with zero warnings

To disable quality checks (not recommended):
```bash
cmake -DENFORCE_CHECKS=OFF ..
```

**Note:** The project uses `jettison_proto_cpp` as a git submodule. The build script will automatically initialize it, or run `git submodule update --init --recursive` manually.

## Usage

### Display Help

```bash
./Jettison_State_RX-x86_64.AppImage --help
```

### Live Streaming Mode

Connect to a WebSocket server and receive state updates:

```bash
./Jettison_State_RX-x86_64.AppImage sych.local
```

**Usage:**
```
./Jettison_State_RX-x86_64.AppImage <host>              # Stream state messages
./Jettison_State_RX-x86_64.AppImage <host> --dump N    # Capture N dumps and exit
./Jettison_State_RX-x86_64.AppImage --read-dump <file> # Validate a dump file
```

Press `Ctrl+C` to stop streaming.

### Dump Mode

Capture N raw binary payloads to the `dumps/` directory:

```bash
./Jettison_State_RX-x86_64.AppImage sych.local --dump 10
```

This will save 10 state messages as:
- `dumps/state_0001.bin`
- `dumps/state_0002.bin`
- ...
- `dumps/state_0010.bin`

The dumps directory is automatically created if it doesn't exist.

### Read Dump Mode

Validate and display a previously captured dump file:

```bash
./Jettison_State_RX-x86_64.AppImage --read-dump dumps/state_0001.bin --json-stdout
```

## Validation Examples

### Valid Message

When a message passes all buf.validate constraints:

```
Reading dump file: dumps/state_0001.bin
Read 546 bytes
Validation: PASSED

JSON Output:
{
  "protocol_version": 1,
  "system_monotonic_time_us": "123456789",
  "gps": {
    "latitude": xx.xx,
    "longitude": xx.xx,
    "manual_latitude": xx.xx,
    "manual_longitude": xx.xx
  },
  "compass": {
    "azimuth": 162.39,
    "elevation": 2.48,
    "bank": 0.45
  },
  "rotary": {
    "azimuth": 316.18,
    "platform_azimuth": 316.18,
    "platform_elevation": -1.22
  },
  ...
}
```

### Invalid Message (Corrupted Data)

When validation detects out-of-range values:

```
Reading dump file: dumps/state_corrupted.bin
Read 546 bytes
INVALID MESSAGE
Parse errors:
  - Field 'gps.longitude': value must be greater than or equal to -180 and less than or equal to 180 (rule: double.gte_lte)
  - Field 'gps.latitude': value must be greater than or equal to -90 and less than or equal to 90 (rule: double.gte_lte)
  - Field 'compass.azimuth': value must be greater than or equal to 0 and less than 360 (rule: double.gte_lt)
  - Field 'compass.elevation': value must be greater than or equal to -90 and less than or equal to 90 (rule: double.gte_lte)
  - Field 'rotary.azimuth': value must be greater than or equal to 0 and less than 360 (rule: double.gte_lt)
  - Field 'camera_day.focus_pos': value must be greater than or equal to 0 and less than or equal to 1 (rule: double.gte_lte)
  - Field 'camera_day.horizontal_fov_degrees': value must be greater than 0 and less than 360 (rule: double.gt_lt)
  ...
```

The validator enforces constraints such as:
- **Geographic coordinates**: latitude ∈ [-90, 90], longitude ∈ [-180, 180]
- **Angles**: azimuth ∈ [0, 360), elevation ∈ [-90, 90], bank ∈ [-180, 180)
- **Normalized values**: camera controls ∈ [0, 1]
- **Field of view**: positive values < 360°
- **Required fields**: ensures all critical subsystem data is present

### Testing Validation with Corrupted Data

The `scripts/corrupt_dump.py` utility is provided for testing buf.validate constraint validation:

```bash
# Corrupt a valid dump by replacing double values with 999.0
python3 scripts/corrupt_dump.py dumps/state_0001.bin dumps/state_corrupted.bin

# Test validation on the corrupted dump
./Jettison_State_RX-x86_64.AppImage --read-dump dumps/state_corrupted.bin
```

The script systematically corrupts all double-precision floating-point values in the protobuf binary, replacing them with out-of-range values (999.0). This is useful for:
- Verifying buf.validate constraints are enforced
- Testing error handling and reporting
- Demonstrating data integrity protection
- Regression testing validation logic

## Building

### AppImage (Recommended)

Build a portable Linux AppImage with all dependencies bundled:

```bash
docker build --target export -t jettison-state-rx:appimage .
docker run --rm -v "$(pwd)":/output <IMAGE_ID> sh -c "cp /appimage/*.AppImage /output/"
```

The resulting `Jettison_State_RX-x86_64.AppImage` runs on any Linux system with glibc 2.35+ (Ubuntu 22.04+).

### Running the AppImage

```bash
chmod +x Jettison_State_RX-x86_64.AppImage
./Jettison_State_RX-x86_64.AppImage --help
```

## Dependencies

The application uses the following libraries:

- **[Protobuf 29.2](https://github.com/protocolbuffers/protobuf)**: Protocol buffer serialization
- **[protovalidate-cc v1.0.0-rc.2](https://github.com/bufbuild/protovalidate-cc)**: Runtime buf.validate constraint validation
- **[Abseil 20240722.0](https://github.com/abseil/abseil-cpp)**: Google's C++ common libraries
- **[libwebsockets 4.3.3](https://github.com/warmcat/libwebsockets)**: WebSocket client
- **[RE2](https://github.com/google/re2)**: Regular expression library (used by CEL-C++)
- **[CEL-C++](https://github.com/google/cel-cpp)**: Common Expression Language (vendored by protovalidate-cc)

All dependencies are statically or dynamically linked and bundled in the AppImage.

## Protocol

The application receives binary Protocol Buffer messages of type `ser.JonGUIState` via WebSocket. Each message contains sensor data from various subsystems:

- System status
- GPS coordinates
- Compass orientation
- Camera settings (day/thermal)
- Rotary platform position
- Environmental sensors
- Recording status

All fields are validated against their buf.validate constraints defined in the `.proto` files.

## Development

### Code Quality Workflow

The project enforces strict code quality standards:

1. **Format code** (GNU style via `clang-format`)
2. **Lint code** (strict checks via `clang-tidy`)
3. **Build** (all warnings as errors + security hardening)

This workflow is **automatically enforced** by default. Every build runs:
```bash
./build.sh  # Runs format -> lint -> build
```

### Code Style

This project uses the **GNU** coding style, enforced via `.clang-format`:

```bash
# Format all source files (run automatically in build.sh)
make format

# Or manually
find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

### Static Analysis

Strict linting with `clang-tidy` (run automatically in build.sh):

```bash
make lint         # Informational warnings only
make lint-strict  # Warnings treated as errors (default in build.sh)
make check        # Run format + lint-strict

# Or manually
clang-tidy src/*.cpp -- -std=c++20 -Ijettison_proto_cpp
```

### Security Hardening

The build includes comprehensive security flags:

- **Stack Protection**: `-fstack-protector-strong`, `-fstack-clash-protection`
- **Fortify**: `_FORTIFY_SOURCE=2`, `_GLIBCXX_ASSERTIONS`
- **Control Flow**: `-fcf-protection`
- **Linker**: RELRO, BIND_NOW, NX stack (`-Wl,-z,relro,-z,now,-z,noexecstack`)
- **Warnings**: `-Werror -Weverything` (with pragmatic exclusions)

### Validation

The application performs basic validation of protobuf messages:

- Checks that all required fields are present
- Validates that `protocol_version > 0`
- Reports any parsing errors

**Note**: Full buf.validate constraint validation (including CEL expressions) would require the `protovalidate-cc` library. The current implementation provides basic structural validation.

## Connection Details

- **Protocol**: WebSocket Secure (WSS)
- **Port**: 443 (HTTPS)
- **Endpoint**: `/ws/ws_state`
- **Message Format**: Binary Protocol Buffers
- **Message Type**: `ser.JonGUIState` (from `jon_shared_data.proto`)

## Project Structure

```
.
├── CMakeLists.txt              # Build configuration
├── CMakeLists.txt.dynamic      # Dynamic build config (for AppImage)
├── Dockerfile                  # AppImage build (Ubuntu 22.04)
├── VERSION                     # Version number
├── LICENSE                     # GPL-3.0 license
├── README.md                   # This file
│
├── .clang-format               # Code formatting rules (GNU style)
├── .clang-tidy                 # Static analysis configuration
├── .dockerignore               # Docker build exclusions
├── .gitignore                  # Git ignore patterns
├── .gitmodules                 # Git submodule configuration
│
├── src/                        # Application source code
│   ├── main.cpp                # Entry point and CLI argument handling
│   ├── websocket_client.*      # WebSocket client implementation
│   ├── proto_validator.*       # Protobuf parsing and validation
│   ├── json_converter.*        # JSON serialization
│   └── dump_manager.*          # File dump/read operations
│
├── scripts/                    # Utility scripts
│   ├── README.md               # Scripts documentation
│   ├── build.sh                # Manual build script with quality checks
│   ├── corrupt_dump.py         # Corruption testing utility
│   ├── create_invalid_dumps.py # Targeted test case generator
│   └── test_all_dumps.sh       # Validation test runner
│
├── dumps/                      # Binary dump files (gitignored)
│   └── state_*.bin             # Captured state messages from live connections
│
├── test_dumps/                 # Test dumps with violations (gitignored)
│   └── *.bin                   # Generated test cases with specific constraint violations
│
├── jettison_proto_cpp/         # Proto files (git submodule)
│   ├── jon_shared_data.pb.h    # Generated protobuf headers
│   └── jon_shared_data.pb.cc   # Generated protobuf implementations
│
└── .github/                    # GitHub configuration
    └── workflows/              # CI/CD automation
        └── build-appimage.yml  # AppImage build and release workflow
```

## Protocol Definition

This project uses the **Jettison Protocol** defined in Protocol Buffers with [buf.validate](https://github.com/bufbuild/protovalidate) constraints.

### Source Repositories

**Protocol Source:**
- [jettison_protogen](https://github.com/lpportorino/jettison_protogen.git) - Source repository containing:
  - Protocol Buffer definitions (`.proto` files in `./proto`)
  - buf.validate constraint annotations
  - Code generation scripts
  - GitHub Actions automation for generating C++/Python/etc. code

**Generated Code (Git Submodule):**
- [jettison_proto_cpp](https://github.com/lpportorino/jettison_proto_cpp.git) - **Auto-generated** by jettison_protogen CI/CD
  - Pre-compiled C++ headers (`.pb.h`) and implementations (`.pb.cc`)
  - Generated from Protocol Buffers 29.2
  - Automatically updated when proto files change
  - Used as a git submodule in this project (`jettison_proto_cpp/`)

**Important:** Do not manually edit files in `jettison_proto_cpp/` - they are automatically generated and pushed by the jettison_protogen GitHub Actions workflow.

### Directory Descriptions

**Source Code:**
- **`src/`** - Application source code implementing WebSocket client, validation, and JSON conversion
- **`jettison_proto_cpp/`** - Git submodule containing pre-compiled protobuf files (Protobuf 29.2)

**Build & Configuration:**
- **`CMakeLists.txt`** - Static build configuration for manual builds
- **`CMakeLists.txt.dynamic`** - Dynamic linking configuration used by AppImage builds
- **`Dockerfile`** - Multi-stage Docker build for creating portable AppImage (Ubuntu 22.04 base)
- **`.clang-format`** - Code formatting rules enforcing GNU style
- **`.clang-tidy`** - Static analysis configuration for code quality checks

**Scripts & Tools:**
- **`scripts/`** - Utility scripts for building, testing, and validation
  - `build.sh` - Automated build with format/lint checks
  - `corrupt_dump.py` - Corrupts dumps to test buf.validate constraints
  - `create_invalid_dumps.py` - Generates specific violation test cases
  - `test_all_dumps.sh` - Comprehensive validation test runner

**Data Directories (gitignored):**
- **`dumps/`** - Binary state messages captured from live WebSocket connections
  - Contains sensitive data (GPS coordinates, system info)
  - Files named `state_NNNN.bin`
  - Created automatically by `--dump` mode
- **`test_dumps/`** - Generated test cases with buf.validate constraint violations
  - Used for regression testing validation logic
  - Regenerated using `scripts/create_invalid_dumps.py`

**CI/CD:**
- **`.github/workflows/`** - GitHub Actions automation
  - `build-appimage.yml` - Builds AppImage on push/PR and creates releases on tags

## License

This project is licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later).

See the [LICENSE](LICENSE) file for details.

## Contributing

When contributing:

1. Follow the GNU code style (enforced by `.clang-format`)
2. Run `clang-tidy` before submitting changes
3. Ensure code compiles without warnings with `-Wall -Wextra -Wpedantic`
4. Test with both stream and dump modes

## Troubleshooting

### Connection Refused

- Verify the Jettison system is reachable: `ping sych.local`
- Check that the WebSocket service is running on port 443
- Ensure firewall rules allow connections

### SSL/TLS Errors

- The application ignores certificate validation errors by design
- If you still see SSL errors, check OpenSSL installation and version

### Parse Errors

- Verify the Jettison system is running a compatible protocol version
- Check that the endpoint is `/ws/ws_state` and not another WebSocket endpoint
- Use dump mode to capture problematic payloads for analysis

### Build Errors

- Ensure all dependencies are installed
- Make sure git submodules are initialized: `git submodule update --init --recursive`
- Check that `jettison_proto_cpp/` directory contains `.pb.cc` and `.pb.h` files
- Verify CMake can find libwebsockets: `pkg-config --cflags libwebsockets`
