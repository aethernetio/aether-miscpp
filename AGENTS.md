# Agent Guide: aether-miscpp

## Essential Commands
- **Build**: `ninja` in `build-clang/`
- **Test All**: `ctest` or `ninja test` in `build-clang/`
- **Test Single**: Run executable in `build-clang/tests/<suite>/run/` (e.g., `./build-clang/tests/test-types/run/ae-misc-test-types`)
- **Configure**: `cmake -G Ninja -S . -B build-clang -DCMAKE_CXX_COMPILER=clang++`
- **Format**: `clang-format -i <file>` (config in `.clang-format`)

## Architecture
- **Header-Only**: The `aether-miscpp` library is an `INTERFACE` library. All implementation is in headers under `src/aether-miscpp/`.
- **Namespaces**: Primarily `ae`, but some utilities use specific ones like `crc32`.
- **Dependencies**: Managed via **CPM**. CMake will download them (e.g., Unity for tests) during configuration.

## Testing
- **Framework**: [Unity](https://github.com/ThrowTheSwitch/Unity) (not to be confused with the game engine).
- **Structure**: Tests are organized in subdirectories under `tests/` (e.g., `test-types`, `test-format`).
- **Execution**: Each test suite has a `main.cpp` that calls individual test functions.

## Standards & Style
- **C++ Standard**: C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- **Copyright**: All files must include the Aethernet Inc. Apache 2.0 license header.
- **IWYU**: Some headers use `// IWYU pragma: begin_exports` / `end_exports`.

## Operational Gotchas
- **Cross-Platform**: Designed for desktop and IoT (e.g., `ESP_PLATFORM` checks in CMake).
- **Build Artifacts**: Ensure `build-clang` exists; it's the default environment in this setup.
