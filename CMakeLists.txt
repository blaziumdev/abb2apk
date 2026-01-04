cmake_minimum_required(VERSION 3.15)
project(aab2apk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif()
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/aab_converter.cpp
    src/process_runner.cpp
    src/file_utils.cpp
    src/signing.cpp
    src/config.cpp
)

set(HEADERS
    include/aab_converter.h
    include/process_runner.h
    include/file_utils.h
    include/signing.h
    include/config.h
)

# Executable
add_executable(aab2apk ${SOURCES} ${HEADERS})

target_include_directories(aab2apk PRIVATE include)

# Platform-specific libraries
if(WIN32)
    # Windows doesn't need extra libraries for subprocess
elseif(UNIX AND NOT APPLE)
    # Linux
elseif(APPLE)
    # macOS
endif()

# Installation
install(TARGETS aab2apk
    RUNTIME DESTINATION bin
)

# Testing (optional)
enable_testing()

