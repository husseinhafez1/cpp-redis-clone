cmake_minimum_required(VERSION 3.10)
project(cpp-redis-clone)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

# Create RESP library
add_library(resp
    src/server/resp.cpp
)

target_include_directories(resp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add executable
add_executable(redis-server 
    src/main.cpp
    src/server/server.cpp
    src/store/store.cpp
)

# Include directories
target_include_directories(redis-server PRIVATE 
    include
    ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(redis-server PRIVATE 
    ${Boost_LIBRARIES}
    pthread
    resp
)

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