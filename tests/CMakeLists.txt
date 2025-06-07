find_package(GTest REQUIRED)

add_executable(store_tests
    store_tests.cpp
)

target_link_libraries(store_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    store
)

add_test(NAME store_tests COMMAND store_tests)