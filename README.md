# Aether miscpp library

Collection of miscelnaus utils widely used in most aether c++ libraries.

## Usage

Use [CPM](https://github.com/cpm-cmake/CPM.cmake) to integrate it with your project:

```cmake
include(cmake/CPM.cmake)

CPMAddPackage("gh:aethernetio/aether-miscpp#main")
...

target_link_library(${target_name} PRIVATE aether::miscpp)
```

## Build and test

Configure, build and test Cmake project as usual:

```sh
mkdir build
cd build
cmake ..
cmake --build .
ctest .
```

./CMakeLists.txt automatically detects if this project build on its own or as part of another project.
In the first case it also automatically enable tests and installation.
In the second case you are free to enable it by options `AE_BUILD_TESTS` and `AE_INSTALL`.
