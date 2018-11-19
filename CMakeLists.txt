cmake_minimum_required(VERSION 3.9)
project(fdedup)

set(CMAKE_CXX_STANDARD 17)

if (NOT UNIX)
    message(FATAL_ERROR "Non-UNIX systems aren't supported yet (fdedup uses inodes)")
endif()

find_package(Boost 1.60 REQUIRED
    COMPONENTS
        program_options
        unit_test_framework
)

# GCC-specific: filesystem is considered experimental ATM
if (CMAKE_COMPILER_IS_GNUCXX)
    set(filesystem_lib  stdc++fs)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat -Werror=return-type -Werror=uninitialized")
endif()

add_library(fdedup_lib
    fdedup.cpp
    fs_accessor.cpp
)
target_link_libraries(fdedup_lib
    ${filesystem_lib}
)

add_executable(fdedup main.cpp)
target_link_libraries(fdedup
    fdedup_lib
    Boost::program_options
)

add_executable(ut_fdedup ut_fdedup.cpp)
target_compile_definitions(
    ut_fdedup
        PRIVATE BOOST_TEST_DYN_LINK
)
target_link_libraries(ut_fdedup
    fdedup_lib
    Boost::unit_test_framework
)

add_test(
    NAME fdedup
    COMMAND $<TARGET_FILE:ut_fdedup>
)

install(TARGETS fdedup RUNTIME DESTINATION bin)
