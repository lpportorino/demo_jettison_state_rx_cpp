# SPDX-License-Identifier: GPL-3.0-or-later
# Ubuntu 22.04 dynamic build + AppImage packaging
#
# Strategy: Let protovalidate-cc handle CEL-C++ vendoring internally
# We only build: libwebsockets → Abseil → Protobuf 29.2 → protovalidate-cc (vendored) → app

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# ==============================================================================
# Install build dependencies
# ==============================================================================
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    git \
    ca-certificates \
    wget \
    curl \
    pkg-config \
    python3 \
    file \
    patchelf \
    desktop-file-utils \
    imagemagick \
    # Java for ANTLR4 (required by CEL-C++ which protovalidate-cc will build)
    default-jdk \
    # Dynamic libraries
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# ==============================================================================
# Upgrade CMake to 3.24+ (protovalidate-cc requires 3.24+, Ubuntu 22.04 has 3.22)
# ==============================================================================
RUN cd /tmp && \
    wget https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz && \
    tar -xzf cmake-3.28.3-linux-x86_64.tar.gz && \
    cp -r cmake-3.28.3-linux-x86_64/bin/* /usr/local/bin/ && \
    cp -r cmake-3.28.3-linux-x86_64/share/* /usr/local/share/ && \
    rm -rf /tmp/cmake-3.28.3*

# ==============================================================================
# Build libwebsockets dynamically
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch v4.3.3 https://github.com/warmcat/libwebsockets.git && \
    cd libwebsockets && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DLWS_WITH_STATIC=OFF \
          -DLWS_WITH_SHARED=ON \
          -DLWS_WITHOUT_TESTAPPS=ON \
          -DLWS_WITHOUT_TEST_SERVER=ON \
          -DLWS_WITHOUT_TEST_PING=ON \
          -DLWS_WITHOUT_TEST_CLIENT=ON \
          -DLWS_WITH_SSL=ON \
          -DLWS_OPENSSL_SUPPORT=ON \
          -DLWS_WITH_HTTP2=OFF \
          -DLWS_IPV6=ON \
          -DLWS_UNIX_SOCK=OFF \
          -DLWS_WITH_LIBUV=OFF \
          -DLWS_WITH_LIBEVENT=OFF \
          .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/libwebsockets

# ==============================================================================
# Build Abseil (required by Protobuf 29.2)
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch 20240722.0 https://github.com/abseil/abseil-cpp.git && \
    cd abseil-cpp && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DABSL_BUILD_TESTING=OFF \
          -DABSL_PROPAGATE_CXX_STD=ON \
          -DBUILD_SHARED_LIBS=ON && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/abseil-cpp

# ==============================================================================
# Build Protobuf 29.2 dynamically (matches our pre-compiled protos)
# ==============================================================================
RUN cd /tmp && \
    wget https://github.com/protocolbuffers/protobuf/releases/download/v29.2/protobuf-29.2.tar.gz && \
    tar -xzf protobuf-29.2.tar.gz && \
    cd protobuf-29.2 && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -Dprotobuf_BUILD_TESTS=OFF \
          -Dprotobuf_BUILD_SHARED_LIBS=ON \
          -Dprotobuf_ABSL_PROVIDER=package && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/protobuf-29.2*

# ==============================================================================
# Build RE2 (required by protovalidate-cc and CEL-C++)
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch 2024-07-02 https://github.com/google/re2.git && \
    cd re2 && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DBUILD_SHARED_LIBS=ON \
          -DRE2_BUILD_TESTING=OFF && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/re2

# ==============================================================================
# Build protovalidate-cc with VENDORING (will vendor CEL-C++ internally)
# Now that Abseil, Protobuf, and RE2 are installed, only CEL-C++ will be vendored
# NOTE: Cannot use ENABLE_INSTALL=ON with VENDORING due to CMake export bugs
# Manually install the built files instead
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch v1.0.0-rc.2 https://github.com/bufbuild/protovalidate-cc.git && \
    cd protovalidate-cc && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_CXX_FLAGS="-fPIC" \
          -DBUILD_SHARED_LIBS=ON \
          -DPROTOVALIDATE_CC_ENABLE_TESTS=OFF \
          -DPROTOVALIDATE_CC_ENABLE_CONFORMANCE=OFF \
          -DPROTOVALIDATE_CC_ENABLE_INSTALL=OFF \
          -DPROTOVALIDATE_CC_ENABLE_VENDORING=ON && \
    cmake --build build -j$(nproc) && \
    cp -v build/libprotovalidate_cc.so /usr/local/lib/ && \
    find build/_deps -name "*.so" -o -name "*.so.*" | xargs -I {} cp -v {} /usr/local/lib/ && \
    mkdir -p /usr/local/include && \
    cp -rv buf /usr/local/include/ && \
    cp -rv build/gen/proto_cc_protovalidate/buf/validate/*.pb.h /usr/local/include/buf/validate/ && \
    cp -rv build/_deps/cel_cpp-src/common /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/eval /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/base /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/internal /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/runtime /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/extensions /usr/local/include/ && \
    find build/_deps/cel_cpp-build -type d -name "cel" -exec cp -rv {} /usr/local/include/ \; && \
    ldconfig && \
    cd / && rm -rf /tmp/protovalidate-cc

# ==============================================================================
# Setup workspace and build our application
# ==============================================================================
WORKDIR /build

# Copy source code
COPY CMakeLists.txt.dynamic CMakeLists.txt
COPY src ./src
COPY jettison_proto_cpp ./jettison_proto_cpp

# Build our application
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          .. && \
    make -j$(nproc) VERBOSE=1 && \
    make install

# Verify the binary is dynamically linked
RUN ldd /usr/local/bin/jettison_state_rx && \
    /usr/local/bin/jettison_state_rx --help

# ==============================================================================
# Create AppImage
# ==============================================================================

# Install linuxdeploy
RUN cd /tmp && \
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
    chmod +x linuxdeploy-x86_64.AppImage && \
    ./linuxdeploy-x86_64.AppImage --appimage-extract && \
    mv squashfs-root /opt/linuxdeploy && \
    ln -s /opt/linuxdeploy/AppRun /usr/local/bin/linuxdeploy && \
    cd /

# Create AppDir structure
RUN mkdir -p /appimage/AppDir/usr/bin

# Copy binary
RUN cp /usr/local/bin/jettison_state_rx /appimage/AppDir/usr/bin/

# Create desktop file (outside AppDir, let linuxdeploy deploy it)
RUN echo "[Desktop Entry]" > /appimage/jettison_state_rx.desktop && \
    echo "Type=Application" >> /appimage/jettison_state_rx.desktop && \
    echo "Name=Jettison State RX" >> /appimage/jettison_state_rx.desktop && \
    echo "Comment=Jettison State Receiver with buf.validate" >> /appimage/jettison_state_rx.desktop && \
    echo "Exec=jettison_state_rx" >> /appimage/jettison_state_rx.desktop && \
    echo "Icon=jettison_state_rx" >> /appimage/jettison_state_rx.desktop && \
    echo "Categories=Utility;" >> /appimage/jettison_state_rx.desktop && \
    echo "Terminal=true" >> /appimage/jettison_state_rx.desktop && \
    cat /appimage/jettison_state_rx.desktop

# Create icon using ImageMagick (256x256 gray PNG)
RUN convert -size 256x256 xc:gray /appimage/jettison_state_rx.png

# Package AppImage - let linuxdeploy handle both desktop file and icon
WORKDIR /appimage
RUN linuxdeploy --appdir AppDir \
    --desktop-file=jettison_state_rx.desktop \
    --icon-file=jettison_state_rx.png \
    --output appimage && \
    ls -lh *.AppImage

# ==============================================================================
# Export stage - just the AppImage
# ==============================================================================
FROM scratch AS export
COPY --from=0 /appimage/*.AppImage /jettison_state_rx.AppImage
