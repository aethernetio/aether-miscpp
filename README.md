# Æthernet C++ Utilities

Header-only C++20 utilities shared by Æthernet C++ projects. This is an internal building block published separately for reuse and testing; it is not an Æthernet network client.

## Add with CPM.cmake

```cmake
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME aether-miscpp
  GITHUB_REPOSITORY aethernetio/aether-miscpp
  GIT_TAG main
)

target_link_libraries(your_target PRIVATE aether::miscpp)
```

For reproducible builds, pin `GIT_TAG` to a reviewed commit SHA.

## Requirements

- CMake 3.16 or newer;
- a C++20 compiler.

## Build and test

```bash
git clone https://github.com/aethernetio/aether-miscpp.git
cd aether-miscpp

cmake -S . -B build -DAE_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

When built as the root project, tests and installation are enabled by default. When included by another CMake project, use `AE_BUILD_TESTS` and `AE_INSTALL` to control them.

## CMake target

```cmake
aether::miscpp
```

## License

Apache License 2.0. See [LICENSE](LICENSE).
