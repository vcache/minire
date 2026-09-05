# Building instructions:
#
# sudo apt install python3-glad
# glad --api="gl:core=4.6" c

cmake_minimum_required(VERSION 3.25)
project(glad VERSION 1.0.0 LANGUAGES C)

add_library(glad STATIC src/gl.c)

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

find_package(OpenGL REQUIRED)
target_link_libraries(glad PUBLIC OpenGL::GL)

if(NOT WIN32)
    target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
endif()
