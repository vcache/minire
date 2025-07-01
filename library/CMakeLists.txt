cmake_minimum_required(VERSION 3.25)

# Configure threads library
find_package(Threads REQUIRED)

# Configure boost library
# TODO find_package(Boost COMPONENTS system filesystem program_options REQUIRED)

# Configure OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Configure SDL
find_package(SDL2 REQUIRED)

# Configure PNG
find_package(PNG REQUIRED)

# Configure fmt::format (TODO: remove it after moving to C++20)
find_package(fmt REQUIRED)

# TODO config GLM
#set(BUILD_SHARED_LIBS OFF)
#set(GLM_TEST_ENABLE OFF)
#add_subdirectory(3rd-party/glm-0.9.9.5)

# TODO config inja
#set(BUILD_TESTING OFF)
#set(BUILD_BENCHMARK OFF)
#set(INJA_INSTALL OFF)
#set(INJA_EXPORT ON)
#add_subdirectory(3rd-party/inja)

# Setup defines

add_definitions(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Build the library

file(GLOB_RECURSE MINIRE_SRCS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp")
list(FILTER MINIRE_SRCS EXCLUDE REGEX "_test.cpp$")

file(GLOB_RECURSE MINIRE_TESTS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*_test.cpp")

add_library(minire ${MINIRE_SRCS})

target_link_libraries(
    minire

    "${CMAKE_THREAD_LIBS_INIT}"
#    "${Boost_SYSTEM_LIBRARY}"
#    "${Boost_FILESYSTEM_LIBRARY}"
#    "${Boost_PROGRAM_OPTIONS_LIBRARY}"

#    glm
#    inja
    fmt::fmt

    "${SDL2_LIBRARIES}"
    "${OPENGL_LIBRARIES}"
    "${PNG_LIBRARIES}"
)

target_compile_features(minire PUBLIC cxx_std_20)

target_include_directories(minire PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SDL2_INCLUDE_DIRS}"
    "${OPENGL_INCLUDE_DIRS}" # TODO: Added in version 3.29.
    "${PNG_INCLUDE_DIRS}"
)

target_compile_options(minire
    PRIVATE -Wall -Wextra -pedantic -Werror
)
