cmake_minimum_required(VERSION 3.20)
project(jettison_state_rx_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Security and hardening flags
add_compile_definitions(
    _FORTIFY_SOURCE=2
    _GLIBCXX_ASSERTIONS
)

add_compile_options(
    -fstack-protector-strong
    -fstack-clash-protection
    -fcf-protection
)

add_link_options(
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
)

# Comprehensive warning flags (all warnings as errors)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Weverything
        -Wconversion
        -Wsign-conversion
        -Wcast-align
        -Wformat=2
        -Wformat-security
        -Wuninitialized
        -Wnull-dereference
        -Wdouble-promotion
        -Wimplicit-fallthrough
        -Wshadow
        -Wundef
        -Wstrict-overflow=5
        -Wwrite-strings
        -Wpointer-arith
        -Wcast-qual
        -Wswitch-default
        -Wunreachable-code
        -Wfloat-equal
        # Disable some overly pedantic warnings from -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-padded
        -Wno-weak-vtables
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-switch-enum
        -Wno-deprecated-redundant-constexpr-static-def
        -Wno-unsafe-buffer-usage
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wsign-conversion
        -Wcast-align
        -Wformat=2
        -Wformat-security
        -Wuninitialized
        -Wnull-dereference
        -Wdouble-promotion
        -Wimplicit-fallthrough
        -Wshadow
        -Wundef
        -Wstrict-overflow=5
        -Wwrite-strings
        -Wpointer-arith
        -Wcast-qual
        -Wswitch-default
        -Wlogical-op
        -Wduplicated-cond
        -Wduplicated-branches
        -Wfloat-equal
    )
endif()

# Find required packages
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Use pkg-config for libwebsockets
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)

# nlohmann_json for JSON output
find_package(nlohmann_json 3.2.0 REQUIRED)

# Path to jettison proto C++ files (git submodule)
set(JETTISON_PROTO_CPP_DIR "${CMAKE_SOURCE_DIR}/jettison_proto_cpp" CACHE PATH "Path to jettison_proto_cpp")

if(NOT EXISTS "${JETTISON_PROTO_CPP_DIR}")
    message(FATAL_ERROR "jettison_proto_cpp directory not found at: ${JETTISON_PROTO_CPP_DIR}\n"
                        "Did you forget to initialize the git submodule?\n"
                        "Run: git submodule update --init --recursive")
endif()

# Collect all proto .cc files
file(GLOB PROTO_SRCS "${JETTISON_PROTO_CPP_DIR}/*.pb.cc")
file(GLOB PROTO_HDRS "${JETTISON_PROTO_CPP_DIR}/*.pb.h")

if(NOT PROTO_SRCS)
    message(FATAL_ERROR "No proto .cc files found in ${JETTISON_PROTO_CPP_DIR}")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/websocket_client.cpp
    src/proto_validator.cpp
    src/dump_manager.cpp
    src/json_converter.cpp
    ${PROTO_SRCS}
)

# Create executable
add_executable(jettison_state_rx ${SOURCES})

# Include directories
target_include_directories(jettison_state_rx PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${JETTISON_PROTO_CPP_DIR}
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(jettison_state_rx PRIVATE
    ${LIBWEBSOCKETS_LIBRARIES}
    ${Protobuf_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Add compile definitions for libwebsockets
target_compile_definitions(jettison_state_rx PRIVATE ${LIBWEBSOCKETS_CFLAGS_OTHER})

# Installation
install(TARGETS jettison_state_rx DESTINATION bin)

# ==============================================================================
# Code Quality Targets
# ==============================================================================

# Find clang-format and clang-tidy
find_program(CLANG_FORMAT clang-format)
find_program(CLANG_TIDY clang-tidy)

# Collect source files for formatting and linting
file(GLOB_RECURSE ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

# Format target - run clang-format on all source files
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format on all source files..."
        VERBATIM
    )
else()
    message(WARNING "clang-format not found - format target will not be available")
endif()

# Lint target - run clang-tidy on all source files
if(CLANG_TIDY)
    add_custom_target(lint
        COMMAND ${CLANG_TIDY}
            ${ALL_SOURCE_FILES}
            --
            -std=c++20
            -I${CMAKE_SOURCE_DIR}/src
            -I${JETTISON_PROTO_CPP_DIR}
            ${LIBWEBSOCKETS_CFLAGS}
            ${Protobuf_INCLUDE_DIRS}
        COMMENT "Running clang-tidy on all source files..."
        VERBATIM
    )

    # Add a strict lint target that treats warnings as errors
    add_custom_target(lint-strict
        COMMAND ${CLANG_TIDY}
            ${ALL_SOURCE_FILES}
            --warnings-as-errors='*'
            --
            -std=c++20
            -I${CMAKE_SOURCE_DIR}/src
            -I${JETTISON_PROTO_CPP_DIR}
            ${LIBWEBSOCKETS_CFLAGS}
            ${Protobuf_INCLUDE_DIRS}
        COMMENT "Running clang-tidy (strict mode - warnings as errors)..."
        VERBATIM
    )
else()
    message(WARNING "clang-tidy not found - lint targets will not be available")
endif()

# Check target - run format, then lint-strict
add_custom_target(check
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target format
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lint-strict
    COMMENT "Running format and lint checks..."
    VERBATIM
)

# Make the main build depend on check if ENFORCE_CHECKS is set
option(ENFORCE_CHECKS "Enforce format and lint checks before building" ON)
if(ENFORCE_CHECKS AND CLANG_FORMAT AND CLANG_TIDY)
    add_dependencies(jettison_state_rx check)
    message(STATUS "Code quality checks are ENFORCED before building")
    message(STATUS "To disable: cmake -DENFORCE_CHECKS=OFF")
else()
    message(STATUS "Code quality checks are NOT enforced")
    message(STATUS "To enable: cmake -DENFORCE_CHECKS=ON")
endif()
