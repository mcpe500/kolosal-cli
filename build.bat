@echo off
echo Building Kolosal CLI...

if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

echo Configuring with CMake...
cmake ..
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful! 
echo Executable location: build\Release\kolosal-cli.exe
echo.
echo Run the application with:
echo   cd build\Release
echo   kolosal-cli.exe
echo.
pause
