# CMake integration

## find_package
```cmake
find_package(umath REQUIRED)
target_link_libraries(your_target PRIVATE usub::umath)
```

## Subproject
```cmake
add_subdirectory(umath)
target_link_libraries(your_target PRIVATE usub::umath)
```
