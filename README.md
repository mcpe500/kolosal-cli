# Kolosal CLI

**Model Discovery** - Automatically fetches Kolosal models from Hugging Face
**Interactive Selection** - Navigate through models and files with keyboard controls
**Smart Search** - Real-time filtering with search functionality
**Quantization Info** - Detailed information about model quantization types
**Fast Navigation** - Efficient viewport handling for large model lists
**User-Friendly** - Clear visual indicators and helpful instructions
**Smart Caching** - Intelligent caching system reduces API calls and improves performance

A command-line interface for browsing and managing Kolosal language models from Hugging Face. Features an interactive console interface for selecting models and quantized .gguf files.

## Features

**Model Discovery** - Automatically fetches Kolosal models from Hugging Face
**Interactive Selection** - Navigate through models and files with keyboard controls
**Smart Search** - Real-time filtering with search functionality
**Quantization Info** - Detailed information about model quantization types
**Fast Navigation** - Efficient viewport handling for large model lists
**User-Friendly** - Clear visual indicators and helpful instructions

## Supported Quantization Types

The CLI automatically detects and provides descriptions for various quantization formats:

- **Q8_0** - 8-bit quantization, good balance of quality and size
- **Q6_K** - 6-bit quantization, high quality with smaller size  
- **Q5_K_M/S** - 5-bit quantization (medium/small variants)
- **Q4_K_M/S** - 4-bit quantization (medium/small variants)
- **Q3_K_L/M/S** - 3-bit quantization (large/medium/small variants)
- **Q2_K** - 2-bit quantization, extremely small but lower quality
- **F16/F32** - Floating point formats (16-bit/32-bit)

## Architecture

The project follows a clean, modular architecture with clear separation of concerns:

```
include/           # Header files
├── http_client.h         # HTTP communication with libcurl
├── model_file.h          # Model file data structures
├── hugging_face_client.h # Hugging Face API client
├── interactive_list.h    # Console UI component
├── cache_manager.h       # Smart caching system
└── kolosal_cli.h        # Main application logic

src/              # Implementation files
├── http_client.cpp
├── model_file.cpp
├── hugging_face_client.cpp
├── interactive_list.cpp
├── cache_manager.cpp
├── kolosal_cli.cpp
└── main.cpp             # Entry point
```

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed documentation.

## Smart Caching System

Kolosal CLI includes an intelligent caching mechanism that significantly improves performance:

### Features
- **Two-Level Cache**: In-memory and persistent disk storage
- **Automatic TTL**: Models (1 hour), Model Files (30 minutes)
- **Cache Management**: Interactive menu for cache operations
- **Cross-Platform**: Works on Windows, Linux, and macOS

### Benefits
- **Faster Startup**: Cached data loads instantly
- **Reduced API Calls**: Less network traffic to Hugging Face
- **Offline Capability**: Access recently cached data without internet
- **User Control**: Clear cache when needed

### Cache Management Menu
- Continue to Model Selection
- Clear Cache
- View Cache Status
- Exit Application

See [CACHING.md](docs/CACHING.md) for detailed information.

## Building from Source

### Prerequisites

- CMake 3.12 or higher
- C++17 compatible compiler
- libcurl (included in external/)
- nlohmann/json (included in external/)

### Build Instructions

1. Navigate to the project directory:
   ```bash
   cd kolosal-cli
   ```

2. Create and enter build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   # Windows
   cmake --build . --config Debug
   
   # Linux/macOS
   make
   ```

5. Run the application:
   ```bash
   # Windows
   .\Debug\kolosal-cli.exe
   
   # Linux/macOS
   ./kolosal-cli
   ```

## Usage

### Navigation Controls

- **↑/↓ Arrow Keys** - Navigate through items
- **Enter** - Select highlighted item  
- **Escape** - Cancel operation or exit search mode
- **/** - Enter search mode
- **Backspace** - Edit search query or clear search
- **Ctrl+C** - Exit application

### Basic Workflow

1. **Model Selection**: Browse available Kolosal models from Hugging Face
2. **File Selection**: Choose from available .gguf files for the selected model
3. **Information Display**: View details about the selected model and file
4. **Download URL**: Get the direct download link for the model file

### Search Functionality

- Press `/` to enter search mode
- Type to filter models/files in real-time
- Press `Backspace` to edit or clear search
- Press `Enter` or arrow keys to exit search mode

## Error Handling

The application gracefully handles various error conditions:

- **Network Issues**: Falls back to sample data when API requests fail
- **Empty Results**: Provides helpful messages and suggestions
- **API Errors**: Clear error reporting with debug information

## Dependencies

- [libcurl](https://curl.se/libcurl/) - HTTP client library
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing library
- Windows Console API - For interactive terminal features (Windows only)

## Future Features

- **File Download** - Direct downloading of selected model files
- **Configuration** - Customizable settings and preferences
- **Progress Bars** - Visual progress indicators for downloads
- **Model Management** - Local model organization and management
- **Cross-Platform UI** - Enhanced compatibility across operating systems

## Contributing

Contributions are welcome! The modular architecture makes it easy to add new features:

- Add new API endpoints in `HuggingFaceClient`
- Enhance UI components in `InteractiveList`
- Extend model file utilities in `ModelFileUtils`
- Improve error handling and user experience

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.