# Docker Deployment Guide

## Overview

FDC_Scheduler provides complete Docker support for easy deployment and development. This guide covers all Docker-related operations.

## Quick Start

### Build and Run

```bash
# Build production image
docker build -t fdc_scheduler:latest .

# Run simple example
docker run --rm fdc_scheduler:latest

# Run with volume mounts
docker run --rm \
  -v $(pwd)/data:/data \
  -v $(pwd)/logs:/logs \
  fdc_scheduler:latest
```

### Using Docker Compose

```bash
# Start all services
docker-compose up

# Start specific service
docker-compose up fdc_api

# Start in background
docker-compose up -d

# View logs
docker-compose logs -f fdc_api

# Stop services
docker-compose down
```

### Using Helper Script

```bash
# Make executable (first time only)
chmod +x docker-helper.sh

# Build image
./docker-helper.sh build

# Run demo
./docker-helper.sh run logging_demo

# Start API server
./docker-helper.sh api 8080

# Development environment
./docker-helper.sh dev

# Run all demos
./docker-helper.sh demo-all

# Clean everything
./docker-helper.sh clean
```

## Docker Images

### Production Image (runtime)

**Size**: ~200 MB (minimal Ubuntu with only runtime dependencies)

**Features**:
- Optimized Release build
- Only runtime libraries (Boost Graph)
- Non-root user for security
- Health checks ready
- All compiled examples included

**Build**:
```bash
docker build --target runtime -t fdc_scheduler:latest .
```

**What's included**:
- `libfdc_scheduler.a` - Static library
- All example binaries in `/app/bin/`
- Header files in `/app/include/`
- Sample data in `/data/`

### Development Image (development)

**Size**: ~800 MB (includes build tools, debugger, editors)

**Features**:
- Full build environment
- CMake, GCC, GDB, Valgrind
- Interactive shell
- Volume mounting for live editing
- Build cache persistence

**Build**:
```bash
docker build --target development -t fdc_scheduler:dev .
```

**Usage**:
```bash
# Interactive development
docker run --rm -it \
  -v $(pwd):/workspace \
  fdc_scheduler:dev

# Inside container
cd /workspace/build
cmake ..
make -j$(nproc)
./examples/simple_example
```

## Docker Compose Services

### fdc_app
Main application container running simple_example.

```bash
docker-compose up fdc_app
```

### fdc_api
REST API server on port 8080 with health checks.

```bash
docker-compose up fdc_api

# Test health
curl http://localhost:8080/health
```

**Environment variables**:
- `FDC_API_PORT`: Port number (default: 8080)
- `FDC_JWT_SECRET`: JWT secret key
- `FDC_LOG_LEVEL`: Log level (DEBUG, INFO, etc.)

### fdc_dev
Development environment with interactive shell.

```bash
docker-compose run --rm fdc_dev

# Inside container
mkdir -p build && cd build
cmake .. && make
```

### fdc_config_demo
Run configuration management demo.

```bash
docker-compose run --rm fdc_config_demo
```

### fdc_logging_demo
Run logging framework demo.

```bash
docker-compose run --rm fdc_logging_demo
```

### fdc_realtime_demo
Run real-time optimization demo.

```bash
docker-compose run --rm fdc_realtime_demo
```

## Volume Mounts

### Data Volume (`/data`)
Mount point for railway network data, configurations, etc.

```bash
docker run --rm \
  -v $(pwd)/data:/data \
  fdc_scheduler:latest ./bin/simple_example
```

### Logs Volume (`/logs`)
Mount point for application logs.

```bash
docker run --rm \
  -v $(pwd)/logs:/logs \
  fdc_scheduler:latest ./bin/logging_demo
```

### Workspace Volume (dev only)
Mount source code for live editing.

```bash
docker run --rm -it \
  -v $(pwd):/workspace \
  fdc_scheduler:dev
```

## Environment Variables

### Common Variables
- `FDC_DATA_DIR`: Data directory (default: `/data`)
- `FDC_LOG_DIR`: Log directory (default: `/logs`)
- `FDC_LOG_LEVEL`: Log level (TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL)

### API Variables
- `FDC_API_HOST`: API host (default: `0.0.0.0`)
- `FDC_API_PORT`: API port (default: `8080`)
- `FDC_JWT_SECRET`: JWT secret for authentication

### Example
```bash
docker run --rm \
  -e FDC_LOG_LEVEL=DEBUG \
  -e FDC_API_PORT=9000 \
  -e FDC_JWT_SECRET=my-secret-key \
  -p 9000:9000 \
  fdc_scheduler:latest ./bin/rest_api_demo
```

## Production Deployment

### Single Container

```bash
# Build production image
docker build --target runtime -t fdc_scheduler:prod .

# Run with production settings
docker run -d \
  --name fdc_scheduler \
  --restart unless-stopped \
  -p 8080:8080 \
  -v /opt/fdc/data:/data \
  -v /opt/fdc/logs:/logs \
  -e FDC_LOG_LEVEL=INFO \
  -e FDC_JWT_SECRET=$(cat /opt/fdc/jwt_secret) \
  fdc_scheduler:prod ./bin/rest_api_demo
```

### Docker Compose Production

```bash
# Set environment variables
export JWT_SECRET=$(openssl rand -hex 32)

# Start services
docker-compose -f docker-compose.yml up -d fdc_api

# View logs
docker-compose logs -f fdc_api

# Health check
curl http://localhost:8080/health
```

### With Reverse Proxy (nginx)

```yaml
# docker-compose.prod.yml
version: '3.8'

services:
  fdc_api:
    image: fdc_scheduler:latest
    environment:
      FDC_JWT_SECRET: ${JWT_SECRET}
    volumes:
      - ./data:/data
      - ./logs:/logs
    networks:
      - internal

  nginx:
    image: nginx:alpine
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf
      - ./ssl:/etc/nginx/ssl
    depends_on:
      - fdc_api
    networks:
      - internal

networks:
  internal:
    driver: bridge
```

## Multi-Architecture Builds

Build for multiple platforms:

```bash
# Enable buildx
docker buildx create --use

# Build for multiple architectures
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --target runtime \
  -t fdc_scheduler:latest \
  --push .
```

## Health Checks

### Built-in Health Check

The `fdc_api` service includes a health check:

```yaml
healthcheck:
  test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
  interval: 30s
  timeout: 10s
  retries: 3
  start_period: 40s
```

### Manual Health Check

```bash
# Check container health
docker inspect --format='{{.State.Health.Status}}' fdc_scheduler_api

# Test endpoint
curl http://localhost:8080/health
```

## Debugging

### View Logs

```bash
# Container logs
docker logs -f fdc_scheduler_api

# Compose logs
docker-compose logs -f fdc_api

# Application logs (if mounted)
tail -f logs/fdc_scheduler.log
```

### Interactive Shell

```bash
# Enter running container
docker exec -it fdc_scheduler_api /bin/bash

# Or use helper script
./docker-helper.sh shell fdc_scheduler_api
```

### Debug with Development Image

```bash
# Start dev container
docker-compose run --rm fdc_dev

# Inside container
cd /workspace/build
gdb ./examples/simple_example
```

## Performance Tuning

### Resource Limits

```yaml
# docker-compose.yml
services:
  fdc_api:
    # ... other config ...
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 2G
        reservations:
          cpus: '1'
          memory: 1G
```

### Build Cache

```bash
# Use BuildKit for better caching
DOCKER_BUILDKIT=1 docker build -t fdc_scheduler:latest .

# Cache mount for dependencies
docker build \
  --cache-from fdc_scheduler:latest \
  -t fdc_scheduler:latest .
```

## Troubleshooting

### Build Issues

```bash
# Clean build (no cache)
docker build --no-cache -t fdc_scheduler:latest .

# Check build context size
du -sh .
# Should be < 100 MB (thanks to .dockerignore)

# View build layers
docker history fdc_scheduler:latest
```

### Runtime Issues

```bash
# Check container status
docker ps -a

# View detailed logs
docker logs --tail 100 fdc_scheduler_api

# Inspect container
docker inspect fdc_scheduler_api

# Check resource usage
docker stats fdc_scheduler_api
```

### Network Issues

```bash
# List networks
docker network ls

# Inspect network
docker network inspect fdc_network

# Test connectivity
docker exec fdc_scheduler_api ping -c 3 google.com
```

## CI/CD Integration

### GitHub Actions Example

```yaml
# .github/workflows/docker.yml
name: Docker Build

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build Docker image
        run: docker build -t fdc_scheduler:latest .
      
      - name: Test image
        run: docker run --rm fdc_scheduler:latest ./bin/simple_example
      
      - name: Push to registry
        if: github.ref == 'refs/heads/main'
        run: |
          echo ${{ secrets.DOCKER_PASSWORD }} | docker login -u ${{ secrets.DOCKER_USERNAME }} --password-stdin
          docker tag fdc_scheduler:latest your-registry/fdc_scheduler:latest
          docker push your-registry/fdc_scheduler:latest
```

## Best Practices

1. **Security**:
   - Always run as non-root user (already configured)
   - Use secrets management for JWT_SECRET
   - Regularly update base images
   - Scan images for vulnerabilities

2. **Performance**:
   - Use multi-stage builds (already implemented)
   - Minimize layer count
   - Use .dockerignore (already configured)
   - Cache dependencies appropriately

3. **Monitoring**:
   - Implement health checks (already configured)
   - Mount log volumes for persistent logging
   - Use Docker stats for resource monitoring
   - Set up alerts for container failures

4. **Development**:
   - Use development image for interactive work
   - Mount source code as volumes
   - Persist build cache
   - Use docker-compose for consistency

## Examples

### Complete Production Setup

```bash
# 1. Build production image
docker build --target runtime -t fdc_scheduler:prod .

# 2. Create directories
mkdir -p /opt/fdc/{data,logs}

# 3. Generate JWT secret
openssl rand -hex 32 > /opt/fdc/jwt_secret

# 4. Run with systemd (create service file)
cat > /etc/systemd/system/fdc-scheduler.service << EOF
[Unit]
Description=FDC Scheduler API
After=docker.service
Requires=docker.service

[Service]
ExecStart=/usr/bin/docker run --rm \
  --name fdc_scheduler \
  -p 8080:8080 \
  -v /opt/fdc/data:/data \
  -v /opt/fdc/logs:/logs \
  -e FDC_JWT_SECRET=$(cat /opt/fdc/jwt_secret) \
  fdc_scheduler:prod ./bin/rest_api_demo
ExecStop=/usr/bin/docker stop fdc_scheduler
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# 5. Start service
systemctl enable fdc-scheduler
systemctl start fdc-scheduler
```

### Development Workflow

```bash
# 1. Build dev image
./docker-helper.sh build dev

# 2. Start interactive shell
./docker-helper.sh dev

# 3. Inside container - build and test
mkdir -p build && cd build
cmake .. && make -j$(nproc)
./examples/logging_demo

# 4. Exit and commit changes
exit

# Changes persist in mounted volume
```

## Summary

FDC_Scheduler provides comprehensive Docker support:

✅ **Multi-stage builds** - Optimized production images  
✅ **Docker Compose** - Multiple service configurations  
✅ **Helper script** - Simplified common operations  
✅ **Development environment** - Full build tools included  
✅ **Production ready** - Health checks, non-root user, security  
✅ **Documentation** - Complete deployment guide  
✅ **Best practices** - Security, performance, monitoring

For more information, see:
- [README.md](../README.md) - Project overview
- [ARCHITECTURE.md](../ARCHITECTURE.md) - System architecture
- [docker-compose.yml](../docker-compose.yml) - Service definitions
