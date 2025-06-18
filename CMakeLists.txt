cmake_minimum_required(VERSION 3.12)

project(kolosal-cli
  LANGUAGES CXX
  VERSION 1.0.0
  DESCRIPTION "A simple CLI application using FTXUI"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add FTXUI subdirectory
set(FTXUI_BUILD_EXAMPLES OFF)
set(FTXUI_BUILD_DOCS OFF)
set(FTXUI_BUILD_TESTS OFF)
add_subdirectory(external/FTXUI)

# Create src directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add executable
add_executable(kolosal-cli src/main.cpp)

# Link FTXUI libraries
target_link_libraries(kolosal-cli
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# Include directories
target_include_directories(kolosal-cli PRIVATE external/FTXUI/include)