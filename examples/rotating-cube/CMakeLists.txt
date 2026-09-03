include(examples-kit)

find_package(Boost CONFIG COMPONENTS program_options REQUIRED)

CreateMinireExample(
    rotating-cube
    LIBRARIES
        Boost::program_options
)
