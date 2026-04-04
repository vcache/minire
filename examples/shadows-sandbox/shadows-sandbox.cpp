#include "../common/testbed.hpp"

#include <cassert>
#include <cstdlib> // for EXIT_SUCCESS
#include <cstring> // for std::memcpy
#include <initializer_list>
#include <random>

namespace
{
    // TODO: this is a placeholder for a sandbox to play with different
    //       shadow methods and parameters.
    //       Implement this once the features and API of models::shadow_params have stabilized.

    class ShadowsSandboxExample
        : public minire::examples::GuiTestbedApplication
    {
    public:
        using GuiTestbedApplication::GuiTestbedApplication;

        void onStart() override
        {
            GuiTestbedApplication::onStart();
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<ShadowsSandboxExample>("Shadows sandbox");
}
