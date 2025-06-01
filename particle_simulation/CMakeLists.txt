cmake_minimum_required(VERSION 3.28)
project(particle_simulation)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Use source files from parent src directory
set(SOURCES
    main.cpp
    ../src/renderer.cpp
    ../src/particle.cpp
    ../src/simulation.cpp
    ../src/collision_object.cpp
    ../src/mesh.cpp
    ../src/sdf.cpp
    ../src/bvh.cpp
)

add_executable(particle_simulation ${SOURCES})

target_link_libraries(particle_simulation 
    OpenGL::GL
    GLEW::GLEW
    glfw
)

target_include_directories(particle_simulation PRIVATE 
    ${GLFW3_INCLUDE_DIRS}
    ../src/
)