# Duppy Conquerer (Base Loop)

Author: Karl-Johan Bailey

Minimal, dependency-free C++ game loop scaffold:
- Delta time + elapsed time
- Frame cap / target FPS
- Basic console input (type a command and press Enter)
- Debug display configuration (FPS, delta, frame index, last input)

## Build & Run (CMake)

```bash
cmake -S . -B build
cmake --build build
./build/duppy
```

## Build & Run (single command, no CMake)

macOS/Linux:
```bash
mkdir -p build &&
g++ -std=c++17 -O2 -pthread \
  src/main.cpp src/core/*.cpp \
  -o build/duppy &&
./build/duppy
```

Windows (Developer Command Prompt):
```bat
mkdir build
cl /std:c++17 /W4 /EHsc ^
  src\main.cpp src\core\*.cpp ^
  /Fe:build\duppy.exe
build\duppy.exe
```

## Controls

- Type `q` then press Enter to quit.
- Any other line is treated as a simple input event.

## Notes

- The console input implementation uses a background thread that blocks on `std::getline`. This keeps the main loop non-blocking without platform-specific APIs.
