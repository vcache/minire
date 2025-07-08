include(examples-kit)

find_package(Boost COMPONENTS program_options REQUIRED)

CreateMinireExample(
    rotating-cube
    LIBRARIES
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
