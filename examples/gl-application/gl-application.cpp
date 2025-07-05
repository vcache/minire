#include <minire/sdl/gl-application.hpp>

#include <cstdlib> // for EXIT_SUCCESS

int main()
{
    minire::sdl::GlApplication application(1024, 1024, "Example");

    application.run();

    return EXIT_SUCCESS;
}
