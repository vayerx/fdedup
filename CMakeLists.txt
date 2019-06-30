cmake_minimum_required(VERSION 3.14)
project(fdedup)

include(CTest)
enable_testing()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT UNIX)
    message(FATAL_ERROR "Non-UNIX systems aren't supported yet (fdedup uses inodes)")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release." FORCE)
endif()

find_package(Boost 1.65 REQUIRED
    COMPONENTS
        program_options
        unit_test_framework
)

if (CMAKE_COMPILER_IS_GNUCXX)
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 8)
        message(FATAL_ERROR "GNU C++ compiler is too old: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()

    add_compile_options(
        $<$<CONFIG:Debug>:-O0$<SEMICOLON>-g>
        $<$<CONFIG:Release>:-O2>
        -Wall
        -Wextra
        -Wpedantic
        -pedantic
        -Weffc++

        -Wcast-align
        -Wcast-qual
        -Wcomment
        -Wconversion
        -Werror=return-type
        -Werror=uninitialized
        -Wformat
        -Winit-self
        -Wmissing-braces
        -Wmissing-noreturn
        -Wno-undef
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wstrict-null-sentinel
        -Wswitch-unreachable
    )
endif()

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()
