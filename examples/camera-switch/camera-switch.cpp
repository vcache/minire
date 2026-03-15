#include "../common/testbed.hpp"

namespace
{
    class CameraSwitch
        : public minire::examples::TestbedApplication
    {
    public:
        using TestbedApplication::TestbedApplication;

        void onStart() override
        {
            TestbedApplication::onStart();

            scene().root().make("cube",
                minire::models::Mesh
                {
                    ._source = minire::content::mkPath("Box.glb",
                                                       minire::content::path::Special::kMeshes,
                                                       minire::content::path::Index(0)),
                    ._defaultMaterial = nullptr,
                    ._skin = std::nullopt,
                    ._visible = true,
                });
        }
    };
}

int main(int, char **)
{
    return minire::examples::main<CameraSwitch>("Camera switch");
}
