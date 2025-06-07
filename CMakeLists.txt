cmake_minimum_required(VERSION 3.15)
project(cpp_redis_clone VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(store
    src/store/store.cpp
)

target_include_directories(store PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

enable_testing()

add_subdirectory(tests)