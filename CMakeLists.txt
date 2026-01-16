cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0144 NEW)
project(umath VERSION 0.1.0 LANGUAGES CXX)

include_directories(${umath_SOURCE_DIR})

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0" CACHE STRING "Debug flags" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-g -O0" CACHE STRING "Debug flags" FORCE)

file(GLOB_RECURSE UMATH_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/umath/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/umath/*.hpp
)
file(GLOB_RECURSE UMATH_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/umath/*.cpp
)
list(FILTER UMATH_HEADERS EXCLUDE REGEX ".*/tests\\.*$")
list(FILTER UMATH_SOURCES EXCLUDE REGEX ".*/tests\\.*$")

add_library(umath STATIC ${UMATH_SOURCES} ${UMATH_HEADERS})
add_library(usub::umath ALIAS umath)

target_include_directories(umath
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/umath>
)

set_target_properties(umath PROPERTIES
        EXPORT_NAME umath
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

option(UMATH_BUILD_EXAMPLES "Build UMATH example executables" OFF)
if (UMATH_BUILD_EXAMPLES)
    add_executable(umath_example examples/main.cpp)
    target_link_libraries(umath_example PRIVATE umath)
    target_compile_definitions(umath_example PRIVATE DEV_STAGE=${DEV_STAGE})
endif ()

option(UMATH_BUILD_TESTS "Build UMATH tests" OFF)

if (UMATH_BUILD_TESTS)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(umath_tests_int128 tests/test_int128.cpp)
    target_link_libraries(umath_tests_int128 PRIVATE umath GTest::gtest_main)
    FetchContent_MakeAvailable(googletest)

    add_executable(umath_tests_int256 tests/test_int256.cpp)
    target_link_libraries(umath_tests_int256 PRIVATE umath GTest::gtest_main)

    add_executable(umath_tests_numerics tests/test_numerics.cpp)
    target_link_libraries(umath_tests_numerics PRIVATE umath GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(umath_tests_int128 DISCOVERY_MODE PRE_TEST)
    gtest_discover_tests(umath_tests_int256 DISCOVERY_MODE PRE_TEST)
    gtest_discover_tests(umath_tests_numerics DISCOVERY_MODE PRE_TEST)
endif ()


install(TARGETS umath
        EXPORT umathTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/umath
)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/umath
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
)

configure_package_config_file(
        cmake/umathConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/umathConfig.cmake
        INSTALL_DESTINATION lib/cmake/umath
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/umathConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/umathConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/umathConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umath
)

install(EXPORT umathTargets
        NAMESPACE umath::
        FILE umathTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umath
)