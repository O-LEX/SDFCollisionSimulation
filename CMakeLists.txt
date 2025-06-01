cmake_minimum_required(VERSION 3.28)
project(sdf_simulation)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(SOURCES
    src/main.cpp
    src/mesh.cpp
    src/sdf.cpp
    src/bvh.cpp
    src/renderer.cpp
    src/particle.cpp
    src/simulation.cpp
    src/collision_object.cpp
)

add_executable(sdf_simulation ${SOURCES})

target_link_libraries(sdf_simulation 
    OpenGL::GL
    GLEW::GLEW
    glfw
)

target_include_directories(sdf_simulation PRIVATE 
    ${GLFW3_INCLUDE_DIRS}
    src/
)

add_subdirectory(particle_simulation)
