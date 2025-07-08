cmake_minimum_required(VERSION 3.25)

# Configure threads library
find_package(Threads REQUIRED)

# Configure boost library
find_package(Boost COMPONENTS headers REQUIRED)

# Configure OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Configure SDL
find_package(SDL2 REQUIRED)

# Configure PNG
find_package(PNG REQUIRED)

# Configure fmt::format (TODO: remove it after moving to C++20)
find_package(fmt REQUIRED)

# Configure GLM
find_package(glm REQUIRED)

# Setup defines

add_definitions(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# TODO: is it really the best place for such definitions?
# TODO: can't use GLM_FORCE_INLINE because of string_casts
add_definitions(-DGLM_FORCE_CXX17 -DGLM_FORCE_RADIANS)

# Build the library

file(GLOB_RECURSE MINIRE_SRCS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp")
list(FILTER MINIRE_SRCS EXCLUDE REGEX "_test.cpp$")

file(GLOB_RECURSE MINIRE_TESTS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*_test.cpp")

add_library(minire ${MINIRE_SRCS})

target_link_libraries(minire
    "${CMAKE_THREAD_LIBS_INIT}"

    glm::glm
    fmt::fmt
    Boost::headers

    "${SDL2_LIBRARIES}"
    "${OPENGL_LIBRARIES}"
    "${PNG_LIBRARIES}"
)

target_compile_features(minire PUBLIC cxx_std_20)

target_include_directories(minire PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/3rd-party"
    "${SDL2_INCLUDE_DIRS}"
    "${OPENGL_INCLUDE_DIRS}" # TODO: Added in version 3.29.
    "${PNG_INCLUDE_DIRS}"
    "${Boost_INCLUDE_DIRS}"
)

target_compile_options(minire
    PRIVATE -Wall -Wextra -pedantic -Werror
)
