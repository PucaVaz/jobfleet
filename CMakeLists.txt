cmake_minimum_required(VERSION 3.20)
project(jobfleet VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -O2")

# Optional ThreadSanitizer build
option(TSAN "Build with ThreadSanitizer" OFF)
if(TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -g")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=thread")
endif()

# libtslog static library
add_library(libtslog STATIC
    src/libtslog/tslog.cpp
    src/libtslog/tslog.h
)

target_include_directories(libtslog PUBLIC src)

# Find required libraries
find_package(Threads REQUIRED)

# Link threading library
target_link_libraries(libtslog PUBLIC Threads::Threads)

# logtest executable
add_executable(logtest tools/logtest.cpp)
target_link_libraries(logtest PRIVATE libtslog)

# Set output directory for executables
set_target_properties(logtest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)