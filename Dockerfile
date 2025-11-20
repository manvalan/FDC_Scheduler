# =============================================================================
# FDC_Scheduler Docker Image - Multi-stage Build
# =============================================================================
# Stage 1: Builder - Compile the application
# Stage 2: Runtime - Minimal production image
# =============================================================================

# -----------------------------------------------------------------------------
# Stage 1: Builder
# -----------------------------------------------------------------------------
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Create build directory and compile
# - Release build for optimization
# - Build examples for demonstration
# - Skip Python bindings (can be enabled with -DFDC_SCHEDULER_BUILD_PYTHON=ON)
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DFDC_SCHEDULER_BUILD_EXAMPLES=ON \
    -DFDC_SCHEDULER_BUILD_TESTS=OFF \
    -DFDC_SCHEDULER_BUILD_PYTHON=OFF && \
    make -j$(nproc)

# -----------------------------------------------------------------------------
# Stage 2: Runtime (Production)
# -----------------------------------------------------------------------------
FROM ubuntu:22.04 AS runtime

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies (much smaller than build deps)
RUN apt-get update && apt-get install -y \
    libboost-graph1.74.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create application user (non-root for security)
RUN useradd -m -u 1000 -s /bin/bash fdc && \
    mkdir -p /app /data /logs && \
    chown -R fdc:fdc /app /data /logs

# Set working directory
WORKDIR /app

# Copy compiled binaries and libraries from builder
COPY --from=builder --chown=fdc:fdc /build/build/libfdc_scheduler.a ./lib/
COPY --from=builder --chown=fdc:fdc /build/build/examples/* ./bin/
COPY --from=builder --chown=fdc:fdc /build/include ./include/

# Copy configuration files
COPY --chown=fdc:fdc examples/test_network.json /data/

# Set environment variables
ENV FDC_DATA_DIR=/data
ENV FDC_LOG_DIR=/logs
ENV FDC_LOG_LEVEL=INFO

# Expose ports for REST API (if using rest_api_demo)
EXPOSE 8080

# Switch to non-root user
USER fdc

# Default command: run the simple example
# Override with docker run fdc_scheduler <command>
CMD ["./bin/simple_example"]

# -----------------------------------------------------------------------------
# Stage 3: Development (Optional)
# -----------------------------------------------------------------------------
FROM builder AS development

# Install additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    vim \
    nano \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy source code
COPY . .

# Keep container running for interactive development
CMD ["/bin/bash"]

# -----------------------------------------------------------------------------
# Usage Examples:
# -----------------------------------------------------------------------------
# Build production image:
#   docker build --target runtime -t fdc_scheduler:latest .
#
# Build development image:
#   docker build --target development -t fdc_scheduler:dev .
#
# Run simple example:
#   docker run --rm fdc_scheduler:latest
#
# Run specific demo:
#   docker run --rm fdc_scheduler:latest ./bin/config_demo
#
# Run REST API server:
#   docker run --rm -p 8080:8080 fdc_scheduler:latest ./bin/rest_api_demo
#
# Interactive development:
#   docker run --rm -it -v $(pwd):/workspace fdc_scheduler:dev
#
# Run with custom data:
#   docker run --rm -v $(pwd)/data:/data fdc_scheduler:latest
#
# Run with persistent logs:
#   docker run --rm -v $(pwd)/logs:/logs fdc_scheduler:latest
# -----------------------------------------------------------------------------
