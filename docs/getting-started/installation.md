# Installation

## Requirements
- C++23 compiler (GCC/Clang/MSVC)
- CMake 3.22+ (recommended)

## Add as subproject
```cmake
add_subdirectory(external/umath)
target_link_libraries(your_target PRIVATE usub::umath)
```

## Install package
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```
