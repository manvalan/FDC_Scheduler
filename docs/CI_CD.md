# CI/CD Pipeline Documentation

## Overview

FDC_Scheduler uses GitHub Actions for comprehensive CI/CD automation. The pipeline includes building, testing, quality checks, Docker image publishing, and automated releases.

## Workflows

### 1. Build and Test (`ci.yml`)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`
- Manual workflow dispatch

**Jobs:**

#### Build Matrix
Builds on multiple platforms and compilers:
- **Linux**: Ubuntu 22.04
  - GCC 11, GCC 12
  - Clang 14, Clang 15
- **macOS**: macOS 13 (Intel), macOS 14 (ARM)
  - Apple Clang
- **Windows**: Windows Server 2022
  - MSVC 2022

**Steps:**
1. Checkout code
2. Install dependencies (Boost, CMake)
3. Configure with CMake
4. Build with parallel jobs
5. Run smoke tests (examples)
6. Upload build artifacts

**Artifacts:**
- `fdc_scheduler-{os}-{compiler}` - Build outputs
- Retention: 7 days

#### Docker Build
1. Build runtime image (production)
2. Build development image
3. Test Docker images
4. Cache optimization with GitHub Actions cache

#### Code Quality
1. Run cppcheck (static analysis)
2. Check code formatting (clang-format)
3. Upload quality reports

**Artifacts:**
- `quality-reports` - cppcheck results
- Retention: 30 days

### 2. Release (`release.yml`)

**Triggers:**
- Git tags matching `v*.*.*` (e.g., v2.0.0)
- Manual workflow dispatch

**Jobs:**

#### Create Release
1. Extract version from tag
2. Generate changelog from git commits
3. Create GitHub Release

#### Build Artifacts
Builds for all platforms:
- Linux x64 (tar.gz)
- macOS x64 (tar.gz)
- macOS ARM64 (tar.gz)
- Windows x64 (zip)

**Artifact Contents:**
- Static library (`libfdc_scheduler.*`)
- Header files (`include/fdc_scheduler/`)
- Example binaries (`bin/`)
- README and LICENSE

#### Docker Release
1. Build multi-architecture images (amd64, arm64)
2. Push to GitHub Container Registry
3. Tag with version and `latest`

**Image Tags:**
- `ghcr.io/manvalan/fdc_scheduler:latest`
- `ghcr.io/manvalan/fdc_scheduler:2.0.0`
- `ghcr.io/manvalan/fdc_scheduler:2.0`
- `ghcr.io/manvalan/fdc_scheduler:2`
- `ghcr.io/manvalan/fdc_scheduler:dev`

#### Documentation
Generates and uploads API documentation

### 3. Code Quality (`code-quality.yml`)

**Triggers:**
- Push to `main` or `develop`
- Pull requests
- Weekly schedule (Monday 00:00 UTC)
- Manual workflow dispatch

**Jobs:**

#### Static Analysis
- **cppcheck**: Comprehensive static analysis
- **clang-tidy**: Additional checks
- **Include analysis**: Dependency tracking

**Reports:**
- `cppcheck-report.xml`
- `includes-report.txt`
- Retention: 30 days

#### Code Formatting
- Check all C++ files with clang-format
- Fail if formatting issues found

#### Security Scanning
- **Trivy**: Vulnerability scanning
- Upload results to GitHub Security tab
- SARIF format for integration

#### Dependency Analysis
- Extract CMake dependencies
- Generate dependency report

#### Code Coverage
- Build with coverage flags
- Run examples for coverage data
- Generate lcov report
- Upload to Codecov

#### Documentation Check
- Verify README.md and LICENSE
- Check documentation structure
- Detect broken links

### 4. Docker (`docker.yml`)

**Triggers:**
- Push to `main` branch
- Git tags `v*.*.*`
- Pull requests
- Manual workflow dispatch

**Jobs:**

#### Build and Test
1. Build runtime image
2. Build development image
3. Test images
4. Push to GitHub Container Registry (on main/tags)

**Features:**
- Multi-platform builds (amd64, arm64)
- GitHub Actions cache for faster builds
- Automatic tagging based on branch/tag
- Pull request builds (without push)

#### Security Scan
- Trivy vulnerability scanner
- Upload results to GitHub Security
- SARIF format

#### Multi-architecture Build (releases only)
- Build for amd64, arm64, arm/v7
- Push with version tags

## Usage

### Running Workflows Manually

1. Go to **Actions** tab in GitHub
2. Select workflow (e.g., "Build and Test")
3. Click **Run workflow**
4. Select branch
5. Click **Run workflow** button

### Creating a Release

```bash
# Tag the release
git tag v2.1.0

# Push tag to trigger release workflow
git push origin v2.1.0

# Workflow will:
# 1. Create GitHub Release with changelog
# 2. Build artifacts for all platforms
# 3. Publish Docker images
# 4. Generate documentation
```

### Viewing Workflow Status

**In Repository:**
- Go to **Actions** tab
- View all workflow runs
- Click on run for detailed logs

**In README:**
- Status badges show current state
- Click badge to view workflow details

### Downloading Artifacts

**From Workflow Run:**
1. Go to Actions → Workflow run
2. Scroll to **Artifacts** section
3. Download desired artifact

**From Release:**
1. Go to **Releases** tab
2. Select release
3. Download platform-specific package

### Using Docker Images

```bash
# Pull latest
docker pull ghcr.io/manvalan/fdc_scheduler:latest

# Pull specific version
docker pull ghcr.io/manvalan/fdc_scheduler:v2.0.0

# Pull development image
docker pull ghcr.io/manvalan/fdc_scheduler:dev

# Run
docker run --rm ghcr.io/manvalan/fdc_scheduler:latest
```

## Configuration

### Required Secrets

**None required for public repositories!**

GitHub automatically provides:
- `GITHUB_TOKEN` - For GitHub API access
- Push permissions to GitHub Container Registry

### Optional Secrets

For advanced features:

**Codecov (Code Coverage):**
```
CODECOV_TOKEN - from codecov.io
```

**Docker Hub (if publishing there):**
```
DOCKER_USERNAME
DOCKER_PASSWORD
```

## Status Badges

Add to README.md:

```markdown
[![Build and Test](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/ci.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/ci.yml)
[![Code Quality](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/code-quality.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/code-quality.yml)
[![Docker](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/docker.yml/badge.svg)](https://github.com/Manvalan/FDC_Scheduler/actions/workflows/docker.yml)
```

See `.github/BADGES.md` for complete badge list.

## Workflow Files

### Directory Structure

```
.github/
├── workflows/
│   ├── ci.yml              # Build and test
│   ├── release.yml         # Automated releases
│   ├── code-quality.yml    # Static analysis
│   └── docker.yml          # Docker images
└── BADGES.md               # Badge reference
```

### Workflow Dependencies

```
ci.yml
├── Build (matrix)
├── Docker
├── Quality
└── Status (depends on all)

release.yml
├── Create Release
├── Build Artifacts (depends on release)
├── Docker Release (depends on release)
└── Documentation (depends on release)

code-quality.yml
├── Static Analysis
├── Formatting
├── Security
├── Dependencies
├── Coverage
└── Documentation

docker.yml
├── Build
├── Security Scan (depends on build)
└── Multi-arch (release tags only)
```

## Debugging Workflows

### View Detailed Logs

1. Go to Actions → Failed workflow
2. Click on failed job
3. Expand failed step
4. View complete output

### Common Issues

**Build Failure:**
```bash
# Test locally with same configuration
docker run --rm -v $(pwd):/workspace -w /workspace ubuntu:22.04 bash -c "
  apt-get update && apt-get install -y cmake g++ libboost-all-dev
  mkdir build && cd build
  cmake .. && make -j4
"
```

**Docker Build Failure:**
```bash
# Build locally
docker build --target runtime -t test .

# Check specific stage
docker build --target builder -t test .
```

**Dependency Issues:**
```bash
# Check CMake configuration
cmake -B build -LAH

# Verify dependencies
find /usr -name "boost*" 2>/dev/null
```

### Re-running Workflows

1. Go to failed workflow run
2. Click **Re-run jobs**
3. Select:
   - **Re-run failed jobs** - Only failed
   - **Re-run all jobs** - Complete re-run

## Performance Optimization

### Build Cache

Workflows use GitHub Actions cache:
- CMake build cache
- Docker layer cache
- Dependency cache

**Manual cache clear:**
1. Go to Actions → Caches
2. Delete old caches

### Parallel Builds

All builds use parallel compilation:
```bash
cmake --build build -j 4
make -j$(nproc)
```

### Matrix Strategy

`fail-fast: false` - Continue other builds if one fails

## Security

### Workflow Permissions

Minimal permissions requested:
- `contents: read` - Read repository
- `packages: write` - Push Docker images (release only)

### Secret Handling

- Never log secrets
- Use environment variables
- GitHub masks secrets in logs

### Dependency Scanning

- Trivy scans all images
- Results in Security tab
- Automated weekly scans

## Best Practices

1. **Test locally before pushing:**
   ```bash
   ./build.sh
   docker build -t test .
   ```

2. **Use draft releases for testing:**
   ```bash
   git tag v2.1.0-rc1
   git push origin v2.1.0-rc1
   ```

3. **Monitor workflow duration:**
   - CI should complete < 30 minutes
   - Release should complete < 1 hour

4. **Keep workflows DRY:**
   - Use composite actions for repeated steps
   - Share configuration via anchors

5. **Version lock critical actions:**
   ```yaml
   uses: actions/checkout@v4  # ✓ Good
   uses: actions/checkout@main # ✗ Risky
   ```

## Maintenance

### Updating Workflows

1. Edit `.github/workflows/*.yml`
2. Test with workflow_dispatch
3. Commit and push
4. Monitor first run

### Dependency Updates

Update in workflows:
- CMake version
- Compiler versions
- Action versions
- Base Docker images

### Scheduled Maintenance

Weekly (Monday 00:00 UTC):
- Code quality checks
- Security scans
- Dependency analysis

## Troubleshooting

### Build Matrix Failures

**Issue**: One platform fails consistently

**Solution**:
1. Check platform-specific code
2. Test locally with same OS/compiler
3. Add platform-specific workarounds

### Docker Build Timeouts

**Issue**: Docker build exceeds time limit

**Solution**:
1. Optimize Dockerfile layers
2. Use multi-stage builds (already done)
3. Improve .dockerignore
4. Enable BuildKit

### Rate Limits

**Issue**: GitHub API rate limits

**Solution**:
- Use `GITHUB_TOKEN` (included)
- Reduce API calls
- Add delays between retries

## Future Enhancements

Planned improvements:

1. **Test Coverage:**
   - Add Google Test
   - Integrate with Codecov
   - Aim for 80%+ coverage

2. **Performance Benchmarks:**
   - Automated benchmarking
   - Compare against baseline
   - Track performance over time

3. **Deployment:**
   - Kubernetes manifests
   - Helm charts
   - Automated deployment

4. **Notifications:**
   - Slack/Discord integration
   - Email notifications
   - Status webhooks

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [GitHub Container Registry](https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry)
- [Docker Build Push Action](https://github.com/docker/build-push-action)
- [Actions Cache](https://github.com/actions/cache)

## Summary

FDC_Scheduler CI/CD pipeline provides:

✅ **Multi-platform builds** - Linux, macOS, Windows  
✅ **Automated testing** - Every commit tested  
✅ **Code quality** - Static analysis, formatting, security  
✅ **Docker images** - Automatic builds and publishing  
✅ **Releases** - Automated with artifacts  
✅ **Documentation** - Always up to date  
✅ **Security** - Regular scanning  
✅ **Monitoring** - Status badges and reports

For questions or issues, see the [GitHub Discussions](https://github.com/Manvalan/FDC_Scheduler/discussions).
