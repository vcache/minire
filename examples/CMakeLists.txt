cmake_minimum_required(VERSION 3.25)

add_executable(gl-application gl-application.cpp)
target_compile_features(gl-application PUBLIC cxx_std_20)
target_link_libraries(gl-application minire)
target_include_directories(gl-application PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(gl-application PRIVATE -Wall -Wextra -pedantic -Werror)
