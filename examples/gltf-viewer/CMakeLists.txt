include(examples-kit)

find_package(Boost CONFIG COMPONENTS program_options REQUIRED)

CreateMinireExample(
    gltf-viewer
    LIBRARIES
        Boost::program_options
)
