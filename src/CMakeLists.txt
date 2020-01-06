add_library(core STATIC
    fdedup.cpp
    fs_accessor.cpp
)
add_library(fdedup::core ALIAS core)

target_include_directories(core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if (CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(core stdc++fs)    # filesystem is considered experimental in GCC
endif()


add_executable(fdedup main.cpp)
target_link_libraries(fdedup
    fdedup::core
    Boost::program_options
)

install(TARGETS fdedup
    RUNTIME DESTINATION bin
)
