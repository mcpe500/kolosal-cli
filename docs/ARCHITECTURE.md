# Kolosal CLI - Code Architecture Documentation

## Overview

The Kolosal CLI has been refactored into a clean, maintainable architecture that separates concerns and follows modern C++ best practices. The codebase is now organized into header files in the `include/` directory and implementation files in the `src/` directory.

## Project Structure

```
kolosal-cli/
├── include/           # Header files (.h)
│   ├── http_client.h
│   ├── model_file.h
│   ├── hugging_face_client.h
│   ├── interactive_list.h
│   └── kolosal_cli.h
├── src/              # Implementation files (.cpp)
│   ├── http_client.cpp
│   ├── model_file.cpp
│   ├── hugging_face_client.cpp
│   ├── interactive_list.cpp
│   ├── kolosal_cli.cpp
│   └── main.cpp
├── external/         # External dependencies
├── build/           # Build output directory
└── CMakeLists.txt   # Build configuration
```

## Component Architecture

### 1. HTTP Client (`http_client.h/.cpp`)

**Purpose**: Low-level HTTP communication using libcurl

**Key Features**:
- Global curl initialization and cleanup
- Reusable HTTP GET request functionality
- Error handling for network operations
- Response data collection

**Classes**:
- `HttpResponse`: Struct to hold HTTP response data
- `HttpClient`: Static class providing HTTP operations

### 2. Model File (`model_file.h/.cpp`)

**Purpose**: Data structures and utilities for model files and quantization

**Key Features**:
- Quantization type detection from filenames
- Model file representation with metadata
- Sorting utilities for prioritizing quantization types

**Classes**:
- `QuantizationInfo`: Struct containing quantization metadata
- `ModelFile`: Represents a model file with quantization info
- `ModelFileUtils`: Utility functions for model file operations

### 3. Hugging Face Client (`hugging_face_client.h/.cpp`)

**Purpose**: High-level API client for Hugging Face services

**Key Features**:
- Fetches Kolosal models from Hugging Face API
- Retrieves model files with quantization information
- JSON parsing and error handling
- API endpoint management

**Classes**:
- `HuggingFaceClient`: Static class providing API operations

### 4. Interactive List (`interactive_list.h/.cpp`)

**Purpose**: Console-based interactive UI component

**Key Features**:
- Keyboard navigation (arrow keys, search, etc.)
- Real-time search filtering
- Viewport management for large lists
- Windows console API integration
- Visual highlighting and indicators

**Classes**:
- `InteractiveList`: Interactive console list widget

### 5. Kolosal CLI (`kolosal_cli.h/.cpp`)

**Purpose**: Main application logic and user flow

**Key Features**:
- Application initialization and cleanup
- User workflow orchestration
- Fallback sample data when API fails
- Result presentation

**Classes**:
- `KolosalCLI`: Main application class

### 6. Main Entry Point (`main.cpp`)

**Purpose**: Application entry point

**Key Features**:
- Simple, clean entry point
- Application lifecycle management

## Design Principles

### 1. Separation of Concerns
Each component has a single, well-defined responsibility:
- HTTP operations are isolated in `HttpClient`
- UI logic is contained in `InteractiveList`
- Business logic is in `KolosalCLI`
- Data structures are in `ModelFile`

### 2. Dependency Inversion
- High-level modules don't depend on low-level modules
- `KolosalCLI` uses `HuggingFaceClient` through a clean interface
- `HuggingFaceClient` uses `HttpClient` for network operations

### 3. Single Responsibility Principle
- Each class has one reason to change
- Clear interfaces between components
- Minimal coupling between modules

### 4. Error Handling
- Graceful degradation with fallback data
- Clear error messages for users
- Proper resource cleanup

## Benefits of the Refactored Architecture

### 1. Maintainability
- **Modular Design**: Changes to one component don't affect others
- **Clear Interfaces**: Well-defined boundaries between components
- **Documentation**: Comprehensive inline documentation and comments

### 2. Testability
- **Isolated Components**: Each component can be unit tested independently
- **Mock-friendly**: Interfaces allow for easy mocking in tests
- **Pure Functions**: Many utility functions are stateless and pure

### 3. Extensibility
- **New Features**: Easy to add new functionality without modifying existing code
- **API Changes**: HTTP client can be easily swapped or extended
- **UI Enhancements**: Interactive list can be enhanced without affecting business logic

### 4. Code Reusability
- **Generic Components**: `InteractiveList` can be reused for other menus
- **HTTP Client**: Can be used for other API integrations
- **Model Utilities**: Quantization detection can be used elsewhere

## Building the Project

The CMakeLists.txt has been updated to automatically discover all source and header files:

```bash
# Navigate to build directory
cd build

# Build the project
cmake --build . --config Debug
```

## Adding New Features

### To add a new API endpoint:
1. Add the method to `HuggingFaceClient`
2. Use existing `HttpClient` for network operations
3. Handle JSON parsing and errors

### To add a new UI component:
1. Create new header in `include/`
2. Implement in `src/`
3. Follow the same patterns as `InteractiveList`

### To modify business logic:
1. Update `KolosalCLI` class methods
2. Maintain separation between UI and business logic
3. Update documentation as needed

## Future Improvements

1. **Configuration Management**: Add config file support
2. **Logging System**: Implement structured logging
3. **Unit Tests**: Add comprehensive test suite
4. **Cross-platform UI**: Abstract console operations for better portability
5. **Download Manager**: Implement actual file downloading functionality
6. **Progress Indicators**: Add progress bars for long operations

This architecture provides a solid foundation for future enhancements while maintaining clean, readable, and maintainable code.
