include(examples-kit)

find_package(Boost COMPONENTS program_options REQUIRED)

CreateMinireExample(
    gltf-viewer
    LIBRARIES
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
