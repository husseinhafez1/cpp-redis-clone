find_package(GTest REQUIRED)

add_executable(store_tests
    store_tests.cpp
)

add_executable(resp_tests
    resp_tests.cpp
)

target_link_libraries(store_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    store
)

target_link_libraries(resp_tests 
    PRIVATE 
    GTest::GTest 
    GTest::Main
    resp
)

target_include_directories(resp_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME store_tests COMMAND store_tests)
add_test(NAME resp_tests COMMAND resp_tests)

set_tests_properties(store_tests PROPERTIES
    ENVIRONMENT "GTEST_COLOR=1"
    TIMEOUT 5
)

set_tests_properties(resp_tests PROPERTIES
    ENVIRONMENT "GTEST_COLOR=1"
    TIMEOUT 5
)