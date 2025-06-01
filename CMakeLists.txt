cmake_minimum_required(VERSION 3.28)
if(WIN32)
  set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
project(sdf_simulation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(SOURCES
    src/mesh.cpp
    src/sdf.cpp
    src/bvh.cpp
    src/renderer.cpp
    src/particle.cpp
    src/simulation.cpp
    src/collision_object.cpp
)

add_library(simulation_lib STATIC ${SOURCES})

target_include_directories(simulation_lib PUBLIC
    src/
)

target_link_libraries(simulation_lib PUBLIC
    GLEW::GLEW
    glm::glm
    glfw
)

add_subdirectory(particle_simulation)
add_subdirectory(collision_simulation)