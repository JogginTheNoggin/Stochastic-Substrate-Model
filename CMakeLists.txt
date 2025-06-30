cmake_minimum_required(VERSION 3.14)
project(Athena)

include(CTest)
include(FetchContent)

# Set source directories
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests/unit_tests)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Allow setting build type (default: Debug)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -----------------------------------
# Libsodium (Secure Randomness)
# -----------------------------------

FetchContent_Declare(
  Sodium
  GIT_REPOSITORY https://github.com/robinlinden/libsodium-cmake.git
  GIT_TAG e5b985ad0dd235d8c4307ea3a385b45e76c74c6a #lastest commit hash of wrapper
)

set(SODIUM_DISABLE_TESTS ON)
FetchContent_MakeAvailable(Sodium)

# -----------------------------------
# Core Library (AthenaLib)
# -----------------------------------
file(GLOB_RECURSE CORE_SOURCES 
    LIST_DIRECTORIES false 
    "${SOURCE_DIR}/general/*.cpp"
)

message(STATUS "Core source files found: ${CORE_SOURCES}")

if(CORE_SOURCES)
    add_library(AthenaLib ${CORE_SOURCES})
    target_include_directories(AthenaLib PUBLIC "${SOURCE_DIR}/headers")
    target_link_libraries(AthenaLib PUBLIC sodium)
else()
    message(WARNING "No core source files found. Skipping AthenaLib creation.")
endif()

# -----------------------------------
# Main Executable (Athena)
# -----------------------------------
file(GLOB_RECURSE MAIN_SOURCES 
    LIST_DIRECTORIES false 
    "${SOURCE_DIR}/main.cpp"
)

if(MAIN_SOURCES)
    add_executable(Athena ${MAIN_SOURCES})
    target_link_libraries(Athena AthenaLib)
else()
    message(WARNING "No main source file found. Skipping Athena executable.")
endif()

# -----------------------------------
# Unit Tests (separate binary per folder)
# -----------------------------------

enable_testing()

# Add GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Get all directories inside TEST_DIR (recursively)
file(GLOB_RECURSE ALL_TEST_DIRS RELATIVE ${TEST_DIR} LIST_DIRECTORIES true "${TEST_DIR}/*")

foreach(SUBDIR ${ALL_TEST_DIRS})
    set(FULL_PATH "${TEST_DIR}/${SUBDIR}")

    # Skip if not a directory
    if(NOT IS_DIRECTORY "${FULL_PATH}")
        continue()
    endif()

    # Get .cpp files directly in this folder (not subfolders)
    file(GLOB SUBDIR_SOURCES 
        LIST_DIRECTORIES false 
        "${FULL_PATH}/*.cpp"
    )

    if(SUBDIR_SOURCES)
        message(STATUS "Found test folder: ${SUBDIR}") # list found directories for tests
        string(REPLACE "/" "_" TEST_NAME ${SUBDIR})
        set(TEST_EXECUTABLE "${TEST_NAME}")

        add_executable(${TEST_EXECUTABLE} ${SUBDIR_SOURCES})
        set_target_properties(${TEST_EXECUTABLE} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/AthenaTests"
        )
        target_include_directories(${TEST_EXECUTABLE} PRIVATE 
            ${SOURCE_DIR}
            ${TEST_DIR}
        )
        target_link_libraries(${TEST_EXECUTABLE} PRIVATE AthenaLib gtest gtest_main)

        add_test(NAME ${TEST_EXECUTABLE} COMMAND ${CMAKE_BINARY_DIR}/AthenaTests/${TEST_EXECUTABLE})
    endif()
endforeach()

