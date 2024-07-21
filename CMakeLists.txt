cmake_minimum_required(VERSION 3.25.1)

project(Bulk VERSION 0.0.1)

add_executable(bulk main.cpp)

target_include_directories(
    bulk
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(bulk PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set(CMAKE_BUILD_TYPE Debug)


