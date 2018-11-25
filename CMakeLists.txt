cmake_minimum_required(VERSION 3.9)
project(fdedup)

include(CTest)
enable_testing()

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(list_utils)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT UNIX)
    message(FATAL_ERROR "Non-UNIX systems aren't supported yet (fdedup uses inodes)")
endif()

find_package(Boost 1.65 REQUIRED
    COMPONENTS
        program_options
        unit_test_framework
)

if (CMAKE_COMPILER_IS_GNUCXX)
    CompileString(cxx_flags " "
        -Wall
        -Wextra
        -Wpedantic
        -Weffc++

        -Werror=return-type
        -Werror=uninitialized

        -Wformat
        -Wmissing-braces
        -Wmissing-noreturn
        -Wswitch-unreachable

        -Wno-undef
    )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${cxx_flags}")
endif()

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()
