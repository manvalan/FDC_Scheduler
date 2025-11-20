#!/bin/bash

# ============================================================================
# FDC_Scheduler v2.0 - Standalone Build Script
# ============================================================================
# Usage: ./build.sh [options]
#
# Options:
#   --clean         Clean build directory
#   --release       Build in Release mode (default: Debug)
#   --tests         Build tests (default: OFF)
#   --examples      Build examples (default: ON)
#   --shared        Build shared library (default: static)
#   --install       Install after build
#   --help          Show this help message
#
# Examples:
#   ./build.sh                    # Debug build with examples
#   ./build.sh --release --tests  # Release build with tests
#   ./build.sh --clean --release  # Clean release build
#
# Requirements:
#   - CMake >= 3.15
#   - C++17 compiler
#   - Boost >= 1.70
# ============================================================================

set -e

# Default options
BUILD_TYPE="Debug"
BUILD_TESTS="OFF"
BUILD_EXAMPLES="ON"
BUILD_SHARED="OFF"
CLEAN_BUILD=false
INSTALL_AFTER_BUILD=false

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
        --shared)
            BUILD_SHARED="ON"
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD=true
            shift
            ;;
        --help|-h)
            echo "FDC_Scheduler v2.0 Build Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --clean         Clean build directory before building"
            echo "  --release       Build in Release mode (default: Debug)"
            echo "  --tests         Enable building tests"
            echo "  --no-tests      Disable building tests (default)"
            echo "  --examples      Enable building examples (default)"
            echo "  --no-examples   Disable building examples"
            echo "  --shared        Build shared library (default: static)"
            echo "  --install       Install after successful build"
            echo "  --help, -h      Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                      # Quick debug build"
            echo "  $0 --release            # Release build"
            echo "  $0 --clean --release    # Clean release build"
            echo "  $0 --tests --examples   # Build with all features"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "============================================"
echo "  FDC_Scheduler v2.0 - Standalone Build"
echo "============================================"
echo "Build Type:     $BUILD_TYPE"
echo "Library Type:   $([ "$BUILD_SHARED" = "ON" ] && echo "Shared" || echo "Static")"
echo "Build Tests:    $BUILD_TESTS"
echo "Build Examples: $BUILD_EXAMPLES"
echo "============================================"

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Check for Boost
echo ""
echo "Checking dependencies..."
if ! cmake --find-package -DNAME=Boost -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST >/dev/null 2>&1; then
    echo "⚠️  Warning: Boost library may not be found"
    echo "   Install with: sudo apt install libboost-graph-dev (Debian/Ubuntu)"
    echo "                 brew install boost (macOS)"
fi

# Configure
echo ""
echo "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DFDC_SCHEDULER_BUILD_TESTS=$BUILD_TESTS \
    -DFDC_SCHEDULER_BUILD_EXAMPLES=$BUILD_EXAMPLES \
    -DFDC_SCHEDULER_BUILD_SHARED=$BUILD_SHARED \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo "Building..."
cmake --build . -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo ""
echo "✓ Build completed successfully!"

# Run tests if built
if [ "$BUILD_TESTS" = "ON" ]; then
    echo ""
    echo "Running tests..."
    ctest --output-on-failure
fi

# Install if requested
if [ "$INSTALL_AFTER_BUILD" = true ]; then
    echo ""
    echo "Installing..."
    sudo cmake --install .
    echo "✓ Installation completed!"
fi

echo ""
echo "============================================"
echo "  Build Summary"
echo "============================================"
echo "Build directory: $(pwd)"
echo ""
if [ "$BUILD_SHARED" = "ON" ]; then
    echo "Library:  libfdc_scheduler.so"
else
    echo "Library:  libfdc_scheduler.a"
fi
echo ""
if [ "$BUILD_EXAMPLES" = "ON" ]; then
    echo "Examples built:"
    ls -1 examples/ 2>/dev/null | grep -v "\.json\|CMake\|cmake" | sed 's/^/  - /'
    echo ""
    echo "Run demo:"
    echo "  cd examples"
    echo "  ./railway_ai_integration_example"
fi
echo "============================================"
echo ""
echo "✓ FDC_Scheduler v2.0 ready!"
echo ""
