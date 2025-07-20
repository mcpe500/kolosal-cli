# Kolosal CLI

A cross-platform command-line interface for discovering, downloading, and running language models from Hugging Face.

![Kolosal CLI Demo](https://raw.githubusercontent.com/KolosalAI/kolosal-cli/main/docs/kolosal-cli-demo.gif)

## Key Features

- **Model Discovery**: Fetch and browse Kolosal models directly from Hugging Face.
- **Interactive TUI**: Navigate, search, and select models with an easy-to-use terminal interface.
- **Download & Run**: Download GGUF models and run them locally with a built-in inference server.
- **Chat Interface**: Interact directly with loaded models through a command-line chat.
- **Smart Caching**: Reduces API calls and improves performance with intelligent caching.
- **Cross-Platform**: Fully supported on Windows, macOS, and Linux.

## Getting Started

The easiest way to get started is to download a pre-built binary for your operating system from the [**Releases**](https://github.com/KolosalAI/kolosal-cli/releases) page.

### Quick Start

Once installed, you can run the application from your terminal:

```bash
# Launch the interactive model browser
kolosal-cli

# Stop the background inference server
kolosal-cli --stop-server
```

## Building from Source

If you prefer to build from source, ensure you have CMake (3.14+) and a C++17 compiler installed.

### 1. Clone the Repository

```bash
git clone https://github.com/KolosalAI/kolosal-cli.git
cd kolosal-cli
```

### 2. Build the Project

- **Linux / macOS**:
  ```bash
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  make -j$(nproc) # Linux
  # make -j$(sysctl -n hw.ncpu) # macOS
  ```

- **Windows**:
  ```powershell
  mkdir build; cd build
  cmake ..
  cmake --build . --config Release
  ```

### 3. Run the Application

- **Linux / macOS**:
  ```bash
  ./kolosal-cli
  ```

- **Windows**:
  ```powershell
  .\Release\kolosal-cli.exe
  ```