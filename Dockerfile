# SPDX-License-Identifier: GPL-3.0-or-later
# Multi-stage Dockerfile for Jettison State RX

# Stage 1: Builder
FROM ubuntu:24.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    clang \
    clang-format \
    clang-tidy \
    git \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler \
    libwebsockets-dev \
    libssl-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . /build

# Initialize submodules
RUN git submodule update --init --recursive || true

# Workaround: Remove incompatible includes for older protobuf
# The proto files were generated with protobuf 32+ but Ubuntu 24.04 has protobuf 23
# Also remove buf/validate includes since we're not using runtime validation
RUN find jettison_proto_cpp -name "*.pb.h" -exec sed -i \
    -e '/#include "google\/protobuf\/runtime_version.h"/d' \
    -e '/#include "buf\/validate\/validate.pb.h"/d' \
    {} \;

# Build the project (use GCC to avoid clang strictness with proto files)
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_C_COMPILER=gcc \
          -DCMAKE_CXX_COMPILER=g++ \
          -DENFORCE_CHECKS=OFF \
          .. && \
    make -j$(nproc)

# Stage 2: Runtime (for creating AppImage)
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libprotobuf-lite27t64 \
    libwebsockets19 \
    libssl3t64 \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=builder /build/build/jettison_state_rx /usr/local/bin/

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/jettison_state_rx"]
CMD ["--help"]

# Stage 3: AppImage builder
FROM ubuntu:24.04 AS appimage-builder

ENV DEBIAN_FRONTEND=noninteractive

# Install AppImage tools and dependencies
RUN apt-get update && apt-get install -y \
    wget \
    file \
    patchelf \
    desktop-file-utils \
    libfuse2t64 \
    libprotobuf-lite27t64 \
    libwebsockets19 \
    libssl3t64 \
    libstdc++6 \
    libgcc-s1 \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /appimage

# Copy binary from builder
COPY --from=builder /build/build/jettison_state_rx /appimage/

# Download AppImage tools
RUN wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
    wget -q https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage && \
    chmod +x *.AppImage

# This stage will be used by GitHub Actions to create the AppImage
CMD ["/bin/bash"]
