# =============================================================================
# GitHub Actions Status Badges
# =============================================================================
# Copy these badges to your README.md
# =============================================================================

# CI/CD Status
[![Build and Test](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/ci.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/ci.yml)
[![Code Quality](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/code-quality.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/code-quality.yml)
[![Docker](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/docker.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/docker.yml)

# Release Status
[![Release](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/release.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/release.yml)
[![GitHub release](https://img.shields.io/github/v/release/Manvalan/FDC_Scheduler)](https://github.com/Manvalan/FDC_Scheduler/releases)

# Code Quality Metrics
[![codecov](https://codecov.io/gh/Manvalan/FDC_Scheduler/branch/main/graph/badge.svg)](https://codecov.io/gh/Manvalan/FDC_Scheduler)

# Container Registry
[![Docker Image](https://ghcr-badge.deta.dev/manvalan/fdc_scheduler/latest_tag?trim=major&label=latest)](https://github.com/Manvalan/FDC_Scheduler/pkgs/container/fdc_scheduler)
[![Docker Image Size](https://ghcr-badge.deta.dev/manvalan/fdc_scheduler/size)](https://github.com/Manvalan/FDC_Scheduler/pkgs/container/fdc_scheduler)

# Project Info
[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](https://github.com/Manvalan/FDC_Scheduler)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com/Manvalan/FDC_Scheduler)

# =============================================================================
# Workflow Files Overview
# =============================================================================

## ci.yml - Build and Test
- Matrix builds: Linux (GCC/Clang), macOS, Windows (MSVC)
- Multiple compiler versions
- Automated testing with examples
- Docker build verification
- Code quality checks (cppcheck, clang-tidy)
- Artifact uploads

## release.yml - Automated Releases
- Triggered by version tags (v*.*.*)
- Creates GitHub releases with changelog
- Builds artifacts for Linux, macOS, Windows
- Publishes Docker images to GHCR
- Multi-architecture support (amd64, arm64)
- Documentation publishing

## code-quality.yml - Static Analysis
- cppcheck for static analysis
- clang-format for code formatting
- Trivy security scanning
- Dependency analysis
- Code coverage reporting
- Documentation checks
- Weekly scheduled runs

## docker.yml - Docker Images
- Builds on every push to main
- Multi-platform builds (amd64, arm64, arm/v7)
- Publishes to GitHub Container Registry
- Security scanning with Trivy
- Automatic tagging (latest, version, branch)
- Cache optimization

# =============================================================================
# Required Secrets (for private repos or advanced features)
# =============================================================================

# GitHub Personal Access Token (for releases)
# GITHUB_TOKEN - automatically provided by GitHub Actions

# Codecov (optional, for code coverage)
# CODECOV_TOKEN - from codecov.io

# Docker Hub (optional, if publishing to Docker Hub)
# DOCKER_USERNAME
# DOCKER_PASSWORD

# =============================================================================
# Usage Examples
# =============================================================================

# Trigger CI manually
# Go to Actions tab → Build and Test → Run workflow

# Create a release
# git tag v2.1.0
# git push origin v2.1.0
# Release workflow will automatically create GitHub release and publish Docker images

# View workflow status
# https://github.com/Manvalan/FDC_Scheduler/actions

# Pull Docker image
# docker pull ghcr.io/manvalan/fdc_scheduler:latest
# docker pull ghcr.io/manvalan/fdc_scheduler:dev
# docker pull ghcr.io/manvalan/fdc_scheduler:v2.0.0
