cmake_minimum_required(VERSION 3.25)

# TODO: avoid global add_definitions and use target_compile_definitions instead

# Common options
set(MINIRE_CXX_STD cxx_std_20)
set(MINIRE_COMP_OPTS -Wall -Wextra -pedantic -Werror)

# Configure threads library
find_package(Threads REQUIRED)

# Configure boost library
find_package(Boost CONFIG COMPONENTS headers REQUIRED)

# Configure OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Configure SDL
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Configure fmt::format (TODO: remove it after moving to C++20)
find_package(fmt REQUIRED)

# Configure GLM
find_package(glm REQUIRED)

# Configure GTest
find_package(GTest)

# Inject GLAD
add_subdirectory(../3rd-party/glad 3rd-party/glad)

# Configure PhysicsFS
option(USE_PHYSFS "Enable PhysicsFS support" ON)
if(USE_PHYSFS)
    find_package(physfs CONFIG QUIET)
    if(NOT physfs_FOUND)
        find_package(PhysFS QUIET)
    endif()

    if(physfs_FOUND OR PhysFS_FOUND)
        if(TARGET physfs)
            set(PHYSFS_LINK_TARGET "physfs")
        elseif(TARGET PhysFS::PhysFS)
            set(PHYSFS_LINK_TARGET "PhysFS::PhysFS")
        else()
            add_library(PhysFS::PhysFS INTERFACE IMPORTED)
            target_include_directories(PhysFS::PhysFS INTERFACE "${PHYSFS_INCLUDE_DIR}")
            target_link_libraries(PhysFS::PhysFS INTERFACE "${PHYSFS_LIBRARY}")
            set(PHYSFS_LINK_TARGET "PhysFS::PhysFS")
        endif()
    else()
        message(WARNING "PhysicsFS not found! Building without it.")
        set(USE_PHYSFS OFF)
    endif()
endif()

# Setup defines
add_definitions(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# TODO: is it really the best place for such definitions?
# TODO: can't use GLM_FORCE_INLINE because of string_casts
add_definitions(-DGLM_FORCE_CXX17 -DGLM_FORCE_RADIANS)

add_definitions(
    -DSTBI_NO_BMP
    -DSTBI_NO_GIF
    -DSTBI_NO_HDR
    -DSTBI_NO_PIC
    -DSTBI_NO_PNM
    -DSTBI_NO_PSD
    -DSTBI_WINDOWS_UTF8)

# Build data directory

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp
    COMMAND xxd -c 20 -n kBuiltinGuiAtlas -i ${CMAKE_CURRENT_SOURCE_DIR}/data/gui-atlas.png > ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp
    COMMAND xxd -c 20 -n kBuiltinGuiFontFace6x13 -i ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13.bdf >> ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp
    COMMAND xxd -c 20 -n kBuiltinGuiFontFace6x13B -i ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13B.bdf >> ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp
    COMMAND xxd -c 20 -n kBuiltinGuiFontFace6x13O -i ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13O.bdf >> ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/data/gui-atlas.png
            ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13.bdf
            ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13B.bdf
            ${CMAKE_CURRENT_SOURCE_DIR}/data/6x13O.bdf
    COMMENT "Embedding data into source dir"
)

# Build the library

file(GLOB_RECURSE MINIRE_SRCS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp")
list(FILTER MINIRE_SRCS EXCLUDE REGEX "_test.cpp$")

# Exclude OS-specific source files
if(WIN32)
    list(FILTER MINIRE_SRCS EXCLUDE REGEX "-linux\\.cpp$")
else()
    list(FILTER MINIRE_SRCS EXCLUDE REGEX "-windows\\.cpp$")
endif()

file(GLOB_RECURSE MINIRE_TESTS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*_test.cpp")

add_library(minire ${MINIRE_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/minire-data.cpp)

if(USE_PHYSFS)
    target_compile_definitions(minire PUBLIC MINIRE_HAS_PHYSFS)
endif()

target_link_libraries(minire
    "${CMAKE_THREAD_LIBS_INIT}"

    glm::glm
    fmt::fmt
    Boost::headers

    "${SDL2_LIBRARIES}"
    "${OPENGL_LIBRARIES}"
    SDL2_mixer::SDL2_mixer

    ${PHYSFS_LINK_TARGET}
    glad
)

target_compile_features(minire PUBLIC "${MINIRE_CXX_STD}")

# TODO: make 3rd-party optional (take em' from the system or
#       from a common build dir (when used as a submodule))

target_include_directories(minire PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party"
    "${SDL2_INCLUDE_DIRS}"
    "${OPENGL_INCLUDE_DIRS}" # TODO: Added in version 3.29.
    ${PHYSFS_INCLUDE_DIR}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sources"
)

target_compile_options(minire PRIVATE ${MINIRE_COMP_OPTS})

# Build unit tests

# TODO: cannot build under WIN32 because of a conflict between gtest's main() and SDL_main()
if (GTest_FOUND AND NOT WIN32)
    include(GoogleTest)
    file(GLOB_RECURSE MINIRE_TESTS
         LIST_DIRECTORIES FALSE
         "${CMAKE_CURRENT_SOURCE_DIR}/sources/*_test.cpp")  # TODO: why underscore instead hyphen?
    add_executable(minire-tests ${MINIRE_TESTS})
    target_compile_features(minire-tests PUBLIC "${MINIRE_CXX_STD}")
    target_link_libraries(minire-tests minire GTest::gtest GTest::gtest_main)
    target_include_directories(minire-tests PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/sources")
    target_compile_options(minire-tests PRIVATE ${MINIRE_COMP_OPTS})
    gtest_add_tests(TARGET minire-tests)
else()
    message("GTest library not found, thus, unit-test won't be built.")
endif(GTest_FOUND)
