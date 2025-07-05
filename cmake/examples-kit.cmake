include_guard(GLOBAL)

macro(CreateMinireExample name)

    # Log the start

    message("-- Minire: * Creating example: '${name}'")

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
    )
    target_link_libraries(${name} minire)
    target_include_directories(
        ${name} 
        PUBLIC
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/3rd-party"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_features(${name} PUBLIC cxx_std_20)

endmacro()
