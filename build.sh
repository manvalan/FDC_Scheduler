#!/bin/bash

# FDC_Scheduler Build Script
# Usage: ./build.sh [options]
#
# Options:
#   --clean         Clean build directory
#   --release       Build in Release mode (default: Debug)
#   --tests         Build tests (default: ON)
#   --examples      Build examples (default: ON)
#   --docs          Build documentation
#   --install       Install after build
#   --header-only   Build as header-only library

set -e

# Default options
BUILD_TYPE="Debug"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
BUILD_DOCS="OFF"
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false
HEADER_ONLY="OFF"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --tests)
            BUILD_TESTS="ON"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --examples)
            BUILD_EXAMPLES="ON"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --docs)
            BUILD_DOCS="ON"
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD=true
            shift
            ;;
        --header-only)
            HEADER_ONLY="ON"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--clean] [--release] [--tests] [--no-tests] [--examples] [--no-examples] [--docs] [--install] [--header-only]"
            exit 1
            ;;
    esac
done

echo "======================================"
echo "FDC_Scheduler Build Script"
echo "======================================"
echo "Build Type: $BUILD_TYPE"
echo "Build Tests: $BUILD_TESTS"
echo "Build Examples: $BUILD_EXAMPLES"
echo "Build Docs: $BUILD_DOCS"
echo "Header-only: $HEADER_ONLY"
echo "======================================"

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo "Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DFDC_SCHEDULER_BUILD_TESTS=$BUILD_TESTS \
    -DFDC_SCHEDULER_BUILD_EXAMPLES=$BUILD_EXAMPLES \
    -DFDC_SCHEDULER_BUILD_DOCS=$BUILD_DOCS \
    -DFDC_SCHEDULER_HEADER_ONLY=$HEADER_ONLY

# Build
echo "Building..."
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo "Build completed successfully!"

# Run tests if built
if [ "$BUILD_TESTS" = "ON" ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi

# Install if requested
if [ "$INSTALL_AFTER_BUILD" = true ]; then
    echo "Installing..."
    sudo cmake --install .
    echo "Installation completed!"
fi

echo "======================================"
echo "Build Summary:"
echo "  Build directory: $(pwd)"
if [ "$HEADER_ONLY" = "OFF" ]; then
    echo "  Library: $(pwd)/libfdc_scheduler.a"
fi
if [ "$BUILD_EXAMPLES" = "ON" ]; then
    echo "  Examples: $(pwd)/examples/"
fi
echo "======================================"
