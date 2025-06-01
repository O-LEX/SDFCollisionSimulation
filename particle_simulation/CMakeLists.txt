cmake_minimum_required(VERSION 3.28)
project(particle_simulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(particle_simulation main.cpp)

target_link_libraries(particle_simulation PRIVATE
    simulation_lib
)
