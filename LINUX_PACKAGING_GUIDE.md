# Linux Packaging Guide for Kolosal CLI

## Overview
The Linux packaging system creates native packages for various Linux distributions, supporting both DEB (Debian/Ubuntu) and RPM (Red Hat/Fedora/CentOS) package formats. The packages include all necessary dependencies and provide a complete installation with systemd service integration and proper system-wide deployment.

## Package Structure

### DEB Package Contents (Debian/Ubuntu)
```
/usr/
├── bin/
│   ├── kolosal                       # Main CLI executable
│   ├── kolosal-server               # Server executable
└── lib/
    ├── libkolosal_server.so         # Main server library
    ├── libllama-*.so                # Inference engine libraries (CPU/CUDA/Vulkan)

/etc/
├── kolosal/
│   └── config.yaml                  # Server configuration (from config.linux.yaml)
└── systemd/system/
    └── kolosal-server.service       # Systemd service file

/var/lib/kolosal/                    # Data directory (created during install)
├── models/                          # Model storage directory
└── (user data)                      # Runtime data and caches
```

### RPM Package Contents (Red Hat/Fedora/CentOS)
The RPM package follows the same structure as DEB but uses RPM-specific file paths and conventions. All files are placed in standard FHS-compliant locations.

## Package Variants

### Architecture Support
- **amd64/x86_64**: Primary 64-bit Intel/AMD architecture
- **arm64/aarch64**: ARM 64-bit architecture (Apple Silicon, ARM servers)
- **armhf/armv7**: 32-bit ARM architecture (Raspberry Pi, embedded systems)

### Acceleration Support
The packages include support for multiple inference acceleration backends:

1. **CPU-only** (default): Uses optimized CPU inference
2. **CUDA**: GPU acceleration for NVIDIA graphics cards
3. **Vulkan**: Cross-platform GPU acceleration
4. **Metal**: Apple Silicon acceleration (when cross-compiling)

## Dependencies

### Essential Runtime Dependencies
- **glibc >= 2.17**: Standard C library
- **libgcc**: GCC runtime library
- **libstdc++**: C++ standard library
- **libcurl**: HTTP client library for API calls
- **libgomp**: OpenMP runtime for parallel processing
- **openssl-libs**: SSL/TLS support for secure connections
- **zlib**: Compression library

### Optional Acceleration Dependencies
- **cuda-runtime**: NVIDIA CUDA runtime (for CUDA builds)
- **vulkan-loader**: Vulkan API loader (for Vulkan builds)

## Building Linux Packages

### Prerequisites

**Important**: This project requires **CMake 3.23 or higher** due to PoDoFo dependency requirements.

#### Check CMake Version
```bash
cmake --version
# Should show version 3.23.0 or higher
```

#### Install Dependencies with Modern CMake

**Ubuntu 22.04 LTS and newer:**
```bash
sudo apt update
sudo apt install build-essential cmake git curl \
                 libcurl4-openssl-dev libssl-dev \
                 pkg-config ninja-build
```

**Ubuntu 20.04 LTS and 22.04 LTS (requires CMake upgrade):**
```bash
# Remove old CMake
sudo apt remove cmake

# Option 1: Install CMake via Snap (recommended for Ubuntu 22.04+)
sudo snap install cmake --classic

# Option 2: Build CMake from source (more reliable for dependency issues)
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7.tar.gz
tar -xzf cmake-3.27.7.tar.gz
cd cmake-3.27.7
./bootstrap --prefix=/usr/local
make -j$(nproc)
sudo make install
# Add to PATH
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# Option 3: Kitware repository (may have libssl dependency issues on Ubuntu 22.04+)
# Only use if Options 1 or 2 don't work for your specific case
# wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
# echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
# sudo apt update
# sudo apt install cmake

# Install other dependencies
sudo apt install build-essential git curl \
                 libcurl4-openssl-dev libssl-dev \
                 pkg-config ninja-build
```

**Fedora/RHEL 9/CentOS Stream 9:**
```bash
sudo dnf install gcc gcc-c++ cmake git curl-devel \
                 openssl-devel pkgconfig ninja-build
```

**RHEL 8/CentOS 8 (requires CMake upgrade):**
```bash
# Enable EPEL repository
sudo dnf install epel-release

# Install newer CMake from EPEL or build from source
sudo dnf install gcc gcc-c++ git curl-devel openssl-devel pkgconfig ninja-build

# For CMake 3.23+, you may need to build from source:
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7.tar.gz
tar -xzf cmake-3.27.7.tar.gz
cd cmake-3.27.7
./bootstrap --prefix=/usr/local
make -j$(nproc)
sudo make install
```

**Alternative: Install CMake via Snap (Universal - Recommended for Ubuntu 22.04+)**
```bash
# This method avoids dependency conflicts and always provides the latest CMake
sudo snap install cmake --classic

# Verify version
cmake --version

# If you get "command not found", add snap bin to PATH
echo 'export PATH="/snap/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### Build Configuration

#### Pre-Build CMake Version Check
```bash
# Verify CMake version before building
cmake --version
# Must be 3.23.0 or higher

# If version is too old, see Prerequisites section for upgrade instructions
```

#### Standard Build (CPU-only)
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
```

#### CUDA-Enabled Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr \
         -DUSE_CUDA=ON
make -j$(nproc)
```

#### Vulkan-Enabled Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr \
         -DUSE_VULKAN=ON
make -j$(nproc)
```

### Package Generation

#### DEB Package (Ubuntu/Debian)
```bash
# Generate the DEB package
cpack -G DEB

# The resulting package will be named: kolosal_1.0.0_amd64.deb
```

#### RPM Package (RHEL/Fedora/CentOS)
```bash
# Generate the RPM package
cpack -G RPM

# The resulting package will be named: kolosal-cli-1.0.0-1.x86_64.rpm
```

#### Automatic Package Format Detection
The build system automatically detects the target distribution:
- Detects `/etc/redhat-release` or `/etc/fedora-release` → generates RPM
- Otherwise → generates DEB

## Installation

### DEB Package Installation
```bash
# Install the package
sudo dpkg -i kolosal_1.0.0_amd64.deb

# Install any missing dependencies
sudo apt install -f

# Alternative: Use apt for automatic dependency resolution
sudo apt install ./kolosal_1.0.0_amd64.deb
```

### RPM Package Installation
```bash
# Install the package
sudo rpm -i kolosal-cli-1.0.0-1.x86_64.rpm

# Or use dnf for automatic dependency resolution
sudo dnf install ./kolosal-cli-1.0.0-1.x86_64.rpm

# On older systems with yum
sudo yum localinstall kolosal-cli-1.0.0-1.x86_64.rpm
```

## Post-Installation Setup

### Automatic Setup
The package automatically performs the following setup tasks:

1. **User and Group Creation**: Creates a `kolosal` system user and group
2. **Directory Setup**: Creates `/var/lib/kolosal` and `/var/lib/kolosal/models`
3. **Permissions**: Sets appropriate ownership and permissions
4. **Service Integration**: Enables and starts the `kolosal-server` systemd service
5. **Library Registration**: Updates system library cache with `ldconfig`

### User Access Configuration
```bash
# Add your user to the kolosal group for CLI access
sudo usermod -a -G kolosal $USER

# Refresh group membership (logout/login or use newgrp)
newgrp kolosal
```

### Service Management
```bash
# Check service status
sudo systemctl status kolosal-server

# Start/stop the service
sudo systemctl start kolosal-server
sudo systemctl stop kolosal-server

# Enable/disable auto-start
sudo systemctl enable kolosal-server
sudo systemctl disable kolosal-server

# View service logs
sudo journalctl -u kolosal-server -f
```

## Configuration

### System Configuration
- **Location**: `/etc/kolosal/config.yaml`
- **Format**: YAML configuration file
- **Permissions**: Read-only for security

### User Data Directory
- **Location**: `/var/lib/kolosal/`
- **Owner**: `kolosal:kolosal`
- **Permissions**: Group writable for CLI access

### Environment Variables
Override configuration with environment variables:
```bash
export KOLOSAL_PORT=8080
export KOLOSAL_HOST=localhost
export KOLOSAL_LOG_LEVEL=DEBUG
export KOLOSAL_API_KEY=your_api_key_here
```

## Usage Examples

### CLI Usage
```bash
# Get help
kolosal --help
kolosal-server --help

# Basic operations
kolosal model list
kolosal model download llama2
kolosal chat

# Server operations
kolosal server start
kolosal server status
kolosal server stop
```

### Service Verification
```bash
# Test server connectivity
curl http://localhost:8080/v1/health

# Check available models
curl http://localhost:8080/v1/models
```

## Uninstallation

### DEB Package Removal
```bash
# Remove package but keep configuration
sudo apt remove kolosal

# Remove package and configuration files
sudo apt purge kolosal

# Complete cleanup (removes user data)
sudo rm -rf /var/lib/kolosal /etc/kolosal
sudo userdel kolosal
```

### RPM Package Removal
```bash
# Remove package
sudo rpm -e kolosal-cli
# Or using dnf/yum
sudo dnf remove kolosal-cli

# Complete cleanup (removes user data)
sudo rm -rf /var/lib/kolosal /etc/kolosal
sudo userdel kolosal
```

## Distribution-Specific Notes

### Ubuntu/Debian
- Uses `libcurl4-openssl-dev` for CURL support
- Service logs available via `journalctl`
- Package dependencies automatically resolved with `apt`

### Red Hat/Fedora/CentOS
- Uses `libcurl-devel` for CURL support
- SELinux may require additional configuration for custom ports
- Firewall rules may need adjustment for network access

### Arch Linux
While not officially packaged, the software can be built from source:
```bash
# Install dependencies
sudo pacman -S base-devel cmake curl openssl

# Build from source following the standard build process
```

## Troubleshooting

### Common Issues

#### CMake Version Too Old
```bash
# Error: "CMake 3.23 or higher is required. You are running version X.X.X"

# Solution 1: Use Snap (recommended - avoids dependency conflicts)
sudo snap install cmake --classic
cmake --version  # Verify it shows 3.23+

# Solution 2: Build from source (most reliable)
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7.tar.gz
tar -xzf cmake-3.27.7.tar.gz
cd cmake-3.27.7
./bootstrap --prefix=/usr/local
make -j$(nproc)
sudo make install
# Ensure /usr/local/bin is in PATH
export PATH="/usr/local/bin:$PATH"
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc

# Solution 3: Upgrade via package manager (newer distributions only)
# Ubuntu 22.04+
sudo apt update && sudo apt install cmake

# Fedora/RHEL 9+
sudo dnf update cmake
```

#### CMake Installation Dependency Issues
```bash
# Error: "cmake : Depends: libssl1.1 (>= 1.1.1) but it is not installable"
# This happens when using Kitware repository on Ubuntu 22.04+ (libssl3 vs libssl1.1 conflict)

# Solution 1: Use Snap instead (bypasses system package dependencies)
sudo apt remove cmake  # Remove conflicting package
sudo snap install cmake --classic

# Solution 2: Build CMake from source
sudo apt remove cmake
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7.tar.gz
tar -xzf cmake-3.27.7.tar.gz
cd cmake-3.27.7
./bootstrap --prefix=/usr/local
make -j$(nproc)
sudo make install

# Solution 3: Use AppImage (portable)
wget https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-x86_64.tar.gz
tar -xzf cmake-3.27.7-linux-x86_64.tar.gz
sudo mv cmake-3.27.7-linux-x86_64 /opt/cmake
sudo ln -sf /opt/cmake/bin/cmake /usr/local/bin/cmake

# Verify installation
cmake --version
which cmake
```

#### Permission Denied
```bash
# Ensure user is in kolosal group
groups $USER
sudo usermod -a -G kolosal $USER
```

#### Service Won't Start
```bash
# Check service logs
sudo journalctl -u kolosal-server -n 50

# Verify configuration
sudo kolosal-server --config /etc/kolosal/config.yaml --validate
```

#### Missing Dependencies
```bash
# Check library dependencies
ldd /usr/bin/kolosal
ldd /usr/bin/kolosal-server

# Install missing packages
sudo apt install -f  # Debian/Ubuntu
sudo dnf install missing-package  # Fedora/RHEL
```

#### Port Conflicts
```bash
# Check what's using port 8080
sudo netstat -tlnp | grep 8080
sudo ss -tlnp | grep 8080

# Change port in configuration
sudo nano /etc/kolosal/config.yaml
```

## Security Considerations

### Default Security Settings
- Server binds to `localhost` by default (not publicly accessible)
- No default API keys (authentication disabled by default)
- Service runs as dedicated `kolosal` user (not root)
- Configuration files have restrictive permissions

### Hardening Recommendations
```bash
# Enable authentication
sudo nano /etc/kolosal/config.yaml
# Set: auth.require_api_key: true
# Add: api_keys with secure random keys

# Restrict network access
# Set: server.host: "127.0.0.1"  # localhost only
# Or: server.allow_public_access: true  # LAN access

# Enable firewall rules if needed
sudo ufw allow 8080/tcp  # Ubuntu
sudo firewall-cmd --add-port=8080/tcp --permanent  # RHEL/Fedora
```

## Advanced Configuration

### Multi-GPU Setup
For systems with multiple GPUs, configure GPU selection in the config file:
```yaml
inference:
  gpu_device: 0  # Select specific GPU
  gpu_layers: 32  # Number of layers to offload to GPU
```

### Performance Tuning
```yaml
server:
  threads: 8           # Number of processing threads
  batch_size: 512      # Batch size for processing
  context_length: 4096 # Maximum context window
```

### Custom Model Paths
```yaml
models:
  storage_path: "/var/lib/kolosal/models"
  cache_path: "/var/lib/kolosal/cache"
```

This guide provides comprehensive instructions for building, packaging, installing, and managing Kolosal CLI on Linux systems using native package managers and following Linux distribution best practices.
