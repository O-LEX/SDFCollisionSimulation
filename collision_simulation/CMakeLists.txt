cmake_minimum_required(VERSION 3.28)
project(collision_simulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(collision_simulation main.cpp)

target_link_libraries(collision_simulation PRIVATE
    simulation_lib
)
