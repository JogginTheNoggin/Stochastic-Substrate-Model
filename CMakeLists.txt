cmake_minimum_required(VERSION 3.10)
project(Athena)

include(CTest)

# Set source directories
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests/unit_tests)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Allow setting build type (default: Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -----------------------------------
# Core Library (AthenaLib)
# -----------------------------------
file(GLOB_RECURSE CORE_SOURCES 
    LIST_DIRECTORIES false 
    "${SOURCE_DIR}/general/*.cpp"
)

# Debugging message
message(STATUS "Core source files found: ${CORE_SOURCES}")

if(CORE_SOURCES)
    add_library(AthenaLib ${CORE_SOURCES})
    target_include_directories(AthenaLib PUBLIC "${SOURCE_DIR}/headers")
else()
    message(WARNING "No core source files found. Skipping AthenaLib creation.")
endif()

# -----------------------------------
# Main Executable (Athena)
# -----------------------------------
file(GLOB_RECURSE MAIN_SOURCES 
    LIST_DIRECTORIES false 
    "${SOURCE_DIR}/main.cpp"  # adjust if your entry point is elsewhere
)

if(MAIN_SOURCES)
    add_executable(Athena ${MAIN_SOURCES})
    target_link_libraries(Athena AthenaLib)
else()
    message(WARNING "No main source file found. Skipping Athena executable.")
endif()

# -----------------------------------
# Unit Tests (AthenaTests)
# -----------------------------------

enable_testing()

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    v1.14.0
)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES 
    LIST_DIRECTORIES false 
    "${TEST_DIR}/*.cpp"
)

if(TEST_SOURCES)
    add_executable(AthenaTests ${TEST_SOURCES})
    target_include_directories(AthenaTests PRIVATE "${SOURCE_DIR}") # Corrected include directory
    target_link_libraries(AthenaTests AthenaLib gtest gtest_main)
    add_test(NAME AthenaTests COMMAND AthenaTests)
else()
    message(WARNING "No test source files found. Skipping AthenaTests executable.")
endif()
