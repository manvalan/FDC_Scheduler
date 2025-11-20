#!/bin/bash
# =============================================================================
# FDC_Scheduler Docker Helper Script
# =============================================================================
# Simplified commands for common Docker operations
# Usage: ./docker-helper.sh <command> [options]
# =============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
IMAGE_NAME="fdc_scheduler"
IMAGE_TAG="latest"
CONTAINER_NAME="fdc_scheduler_app"

# =============================================================================
# Helper Functions
# =============================================================================

print_header() {
    echo -e "\n${BLUE}===================================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}===================================================================${NC}\n"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# =============================================================================
# Commands
# =============================================================================

cmd_build() {
    print_header "Building FDC_Scheduler Docker Image"
    
    TARGET=${1:-runtime}
    
    if [ "$TARGET" = "dev" ]; then
        TARGET="development"
        IMAGE_TAG="dev"
    fi
    
    print_info "Building target: $TARGET"
    print_info "Image tag: ${IMAGE_NAME}:${IMAGE_TAG}"
    
    docker build \
        --target "$TARGET" \
        -t "${IMAGE_NAME}:${IMAGE_TAG}" \
        -f Dockerfile \
        .
    
    print_success "Build complete!"
    print_info "Image: ${IMAGE_NAME}:${IMAGE_TAG}"
}

cmd_run() {
    print_header "Running FDC_Scheduler"
    
    DEMO=${1:-simple_example}
    
    print_info "Running demo: $DEMO"
    
    docker run --rm \
        -v "$(pwd)/data:/data" \
        -v "$(pwd)/logs:/logs" \
        "${IMAGE_NAME}:${IMAGE_TAG}" \
        "./bin/$DEMO"
}

cmd_api() {
    print_header "Starting REST API Server"
    
    PORT=${1:-8080}
    JWT_SECRET=${2:-dev-secret-change-in-production}
    
    print_info "Port: $PORT"
    print_info "JWT Secret: ${JWT_SECRET:0:10}..."
    
    docker run --rm \
        -p "${PORT}:8080" \
        -v "$(pwd)/data:/data" \
        -v "$(pwd)/logs:/logs" \
        -e FDC_JWT_SECRET="$JWT_SECRET" \
        -e FDC_LOG_LEVEL=INFO \
        --name fdc_api \
        "${IMAGE_NAME}:${IMAGE_TAG}" \
        "./bin/rest_api_demo"
    
    print_success "API server running on http://localhost:${PORT}"
}

cmd_dev() {
    print_header "Starting Development Environment"
    
    if ! docker images | grep -q "${IMAGE_NAME}.*dev"; then
        print_warning "Development image not found. Building..."
        cmd_build dev
    fi
    
    print_info "Starting interactive shell..."
    
    docker run --rm -it \
        -v "$(pwd):/workspace" \
        -v "$(pwd)/data:/data" \
        -v "$(pwd)/logs:/logs" \
        -w /workspace \
        "${IMAGE_NAME}:dev" \
        /bin/bash
}

cmd_compose_up() {
    print_header "Starting Docker Compose Services"
    
    SERVICE=${1:-}
    
    if [ -z "$SERVICE" ]; then
        print_info "Starting all services..."
        docker-compose up
    else
        print_info "Starting service: $SERVICE"
        docker-compose up "$SERVICE"
    fi
}

cmd_compose_down() {
    print_header "Stopping Docker Compose Services"
    
    docker-compose down
    print_success "All services stopped"
}

cmd_logs() {
    print_header "Viewing Container Logs"
    
    SERVICE=${1:-fdc_api}
    
    print_info "Service: $SERVICE"
    docker-compose logs -f "$SERVICE"
}

cmd_shell() {
    print_header "Opening Shell in Running Container"
    
    CONTAINER=${1:-$CONTAINER_NAME}
    
    if ! docker ps | grep -q "$CONTAINER"; then
        print_error "Container $CONTAINER is not running"
        exit 1
    fi
    
    docker exec -it "$CONTAINER" /bin/bash
}

cmd_clean() {
    print_header "Cleaning Docker Resources"
    
    print_info "Stopping containers..."
    docker-compose down 2>/dev/null || true
    
    print_info "Removing images..."
    docker rmi "${IMAGE_NAME}:${IMAGE_TAG}" 2>/dev/null || true
    docker rmi "${IMAGE_NAME}:dev" 2>/dev/null || true
    
    print_info "Removing dangling images..."
    docker image prune -f
    
    print_success "Cleanup complete"
}

cmd_test() {
    print_header "Running Tests in Docker"
    
    print_info "Building test image..."
    docker build \
        --target builder \
        -t "${IMAGE_NAME}:test" \
        .
    
    print_info "Running tests..."
    docker run --rm "${IMAGE_NAME}:test" \
        /bin/bash -c "cd build && ctest --output-on-failure"
    
    print_success "Tests complete"
}

cmd_demo_all() {
    print_header "Running All Demos"
    
    DEMOS=(
        "simple_example"
        "config_demo"
        "logging_demo"
        "rest_api_demo"
    )
    
    for demo in "${DEMOS[@]}"; do
        print_info "Running $demo..."
        docker run --rm \
            -v "$(pwd)/data:/data" \
            -v "$(pwd)/logs:/logs" \
            "${IMAGE_NAME}:${IMAGE_TAG}" \
            "./bin/$demo" || print_warning "$demo failed or requires interaction"
        echo ""
    done
    
    print_success "All demos completed"
}

cmd_help() {
    cat << EOF
FDC_Scheduler Docker Helper Script

Usage: $0 <command> [options]

Commands:
    build [target]         Build Docker image (target: runtime|dev)
    run [demo]            Run a specific demo (default: simple_example)
    api [port] [secret]   Start REST API server (default: 8080)
    dev                   Start development environment
    compose-up [service]  Start docker-compose services
    compose-down          Stop docker-compose services
    logs [service]        View logs (default: fdc_api)
    shell [container]     Open shell in running container
    clean                 Clean Docker resources
    test                  Run tests in Docker
    demo-all              Run all demos
    help                  Show this help message

Examples:
    $0 build              # Build production image
    $0 build dev          # Build development image
    $0 run simple_example # Run simple example
    $0 run logging_demo   # Run logging demo
    $0 api 8080           # Start API on port 8080
    $0 dev                # Start interactive dev shell
    $0 compose-up         # Start all services
    $0 compose-up fdc_api # Start only API service
    $0 logs fdc_api       # View API logs
    $0 clean              # Clean all Docker resources
    $0 demo-all           # Run all demos

Environment Variables:
    JWT_SECRET            JWT secret for API authentication
    FDC_LOG_LEVEL         Log level (TRACE|DEBUG|INFO|WARN|ERROR|CRITICAL)
    FDC_DATA_DIR          Data directory path
    FDC_LOG_DIR           Logs directory path

EOF
}

# =============================================================================
# Main
# =============================================================================

main() {
    if [ $# -eq 0 ]; then
        cmd_help
        exit 0
    fi
    
    COMMAND=$1
    shift
    
    case $COMMAND in
        build)
            cmd_build "$@"
            ;;
        run)
            cmd_run "$@"
            ;;
        api)
            cmd_api "$@"
            ;;
        dev)
            cmd_dev "$@"
            ;;
        compose-up)
            cmd_compose_up "$@"
            ;;
        compose-down)
            cmd_compose_down "$@"
            ;;
        logs)
            cmd_logs "$@"
            ;;
        shell)
            cmd_shell "$@"
            ;;
        clean)
            cmd_clean "$@"
            ;;
        test)
            cmd_test "$@"
            ;;
        demo-all)
            cmd_demo_all "$@"
            ;;
        help|--help|-h)
            cmd_help
            ;;
        *)
            print_error "Unknown command: $COMMAND"
            echo ""
            cmd_help
            exit 1
            ;;
    esac
}

main "$@"
