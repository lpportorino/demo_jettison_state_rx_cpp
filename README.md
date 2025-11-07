# Jettison State Receiver (C++)

A C++ console application for receiving, validating, and displaying state updates from Jettison systems over WebSocket Secure (WSS) connections.

## Features

- Connect to Jettison state WebSocket endpoint (`/ws/ws_state`)
- Receive binary protobuf state messages
- Validate messages using buf.validate constraints
- Convert and display state as JSON
- Dump raw binary payloads for offline analysis
- Read and validate dumped payloads

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
wget https://github.com/YOUR_USERNAME/demo_jettison_state_rx_cpp/releases/latest/download/jettison-state-rx-*-linux-x86_64.AppImage

# Make it executable
chmod +x jettison-state-rx-*.AppImage

# Run it
./jettison-state-rx-*.AppImage sych.local
```

**Benefits of AppImage:**
- No installation required
- No dependencies to install
- Works on all major Linux distributions
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
./build.sh
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
./build.sh --clean        # Clean build directory first
./build.sh --no-checks    # Skip format and lint checks
./build.sh -j 8           # Use 8 parallel build jobs
./build.sh --help         # Show all options
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
./jettison_state_rx
```

### Stream State Messages

Connect to a Jettison system and display state updates in real-time:

```bash
./jettison_state_rx sych.local
```

Press `Ctrl+C` to stop streaming.

### Dump Mode

Capture N raw binary payloads to the `dumps/` directory:

```bash
./jettison_state_rx sych.local --dump 10
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
./jettison_state_rx --read-dump dumps/state_0001.bin
```

## Output Format

For each received message, the application displays:

1. **Message number and size**
2. **Validation status**: PASSED or FAILED
3. **Validation errors** (if any)
4. **Validation warnings** (if any)
5. **JSON representation** of the state message (when not in dump mode)

### Example Output

```
Connecting to wss://sych.local:443/ws/ws_state
Connected successfully

=== Message #1 (size: 1234 bytes) ===
Validation: PASSED

JSON Output:
{
  "protocol_version": 1,
  "system_monotonic_time_us": "1234567890",
  "system": {
    "voltage_v": 24.5,
    ...
  },
  ...
}
```

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
├── CMakeLists.txt          # Build configuration
├── src/
│   ├── main.cpp            # Entry point and CLI argument handling
│   ├── websocket_client.*  # WebSocket client implementation
│   ├── proto_validator.*   # Protobuf parsing and validation
│   ├── json_converter.*    # JSON serialization
│   └── dump_manager.*      # File dump/read operations
├── dumps/                  # Binary dump files (git-ignored)
├── .clang-format           # Code formatting rules
├── .clang-tidy             # Static analysis configuration
├── .gitignore              # Git ignore patterns
├── LICENSE                 # GPL-3.0 license
└── README.md               # This file
```

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
