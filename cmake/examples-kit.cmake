include_guard(GLOBAL)

macro(CreateMinireExample name)

    # Log the start

    message("-- Minire: * Creating example: '${name}'")

    # Parse arguments

    set(minire-example-${name}-options)
    set(minire-example-${name}-one-value-args)
    set(minire-example-${name}-multi-value-args COMPILE_OPTIONS
                                                LIBRARIES
                                                INCLUDE_DIRS)
    cmake_parse_arguments(
        minire-example-${name}
        "${minire-example-${name}-options}"
        "${minire-example-${name}-one-value-args}"
        "${minire-example-${name}-multi-value-args}"
        ${ARGN}
    )

    # Find sources

    file(GLOB_RECURSE minire-example-${name}-sources
         LIST_DIRECTORIES FALSE
         ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
    list(FILTER minire-example-${name}-sources EXCLUDE REGEX "_test.cpp$")

    # Log the sources

    message("           | Source of '${name}': ${minire-example-${name}-sources}")

    # Add service binary

    add_executable(
        ${name}
        ${minire-example-${name}-sources}
    )
    target_compile_options(
        ${name}
        PRIVATE
            -Wall -Wextra -pedantic -Werror -DMINIRE_EXAMPLE_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}"
            ${minire-example-${name}_COMPILE_OPTIONS}
    )
    target_link_libraries(${name} minire ${minire-example-${name}_LIBRARIES})
    target_include_directories(
        ${name} 
        PUBLIC
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/3rd-party"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            ${minire-example-i${name}_INCLUDE_DIRS}
    )

    target_compile_features(${name} PUBLIC cxx_std_20)

endmacro()
