cmake_minimum_required(VERSION 3.20)
project(MatrixBreakthrough VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force UTF-8 source encoding (MSVC defaults to system codepage 936/GBK)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---- Optional SFML GUI (off by default — terminal-only for Level 1) ----
option(ENABLE_GUI "Build with SFML graphical interface" OFF)

# ---- Collect all source files ----
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# ---- Executable ----
add_executable(matrix_breakthrough ${SOURCES})

# ---- Include directories ----
target_include_directories(matrix_breakthrough PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/model
    ${CMAKE_SOURCE_DIR}/include/controller
    ${CMAKE_SOURCE_DIR}/include/view
)

# ---- SFML linkage (only when ENABLE_GUI=ON) ----
if(ENABLE_GUI)
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
    target_link_libraries(matrix_breakthrough PRIVATE
        sfml-graphics sfml-window sfml-system
    )
    target_compile_definitions(matrix_breakthrough PRIVATE HAS_GUI)
endif()

# ---- Platform-specific: Windows console ----
if(WIN32)
    if(NOT ENABLE_GUI)
        # Force console subsystem so std::cout / _getch work
        set_target_properties(matrix_breakthrough PROPERTIES
            WIN32_EXECUTABLE FALSE
        )
    endif()
endif()
