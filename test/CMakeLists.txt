add_executable(ut_fdedup ut_fdedup.cpp)

target_compile_definitions(
    ut_fdedup
        PRIVATE BOOST_TEST_DYN_LINK
)

target_link_libraries(ut_fdedup
    fdedup::core
    Boost::unit_test_framework
)

add_test(
    NAME fdedup
    COMMAND $<TARGET_FILE:ut_fdedup>
)
