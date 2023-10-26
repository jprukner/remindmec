set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

cmake_minimum_required (VERSION 2.6)
project (opengl_learning)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(learn_opengl
    window.c
    shader.c
    texture.c
    matrix.c
    glad/src/glad.c
)

include_directories(
    glad/include/
    .
)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw3
    wayland-client
    m
    GL
    dl
)

target_link_libraries(learn_opengl
    ${ALL_LIBS}
)
