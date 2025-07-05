cmake_minimum_required(VERSION 3.25)

project(minire)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(library)
add_subdirectory(examples) # TODO: make it optional
