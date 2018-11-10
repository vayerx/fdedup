cmake_minimum_required(VERSION 3.9)
project(fdedup)

set(CMAKE_CXX_STANDARD 17)

if (NOT UNIX)
    message(FATAL_ERROR "Non-UNIX systems aren't supported yet (fdedup uses inodes)")
endif()

find_package(Boost 1.60 REQUIRED
    COMPONENTS
        program_options
)

# GCC-specific: filesystem is considered experimental ATM
if (CMAKE_COMPILER_IS_GNUCXX)
    set(filesystem_lib  stdc++fs)
endif()

add_executable(fdedup main.cpp)
target_link_libraries(fdedup
    ${filesystem_lib}
    Boost::program_options
)

install(TARGETS fdedup RUNTIME DESTINATION bin)
