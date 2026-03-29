#include "../common/testbed.hpp"

#include <cmath>

namespace
{
    class CustomCursor
        : public minire::examples::TestbedApplication
    {
        auto makeImage()
        {
            auto lease = contentManager().borrow("cursor.png");
            assert(lease);
            return lease->as<minire::models::Image::Sptr>();
        }

    public:
        template<typename... Args>
        explicit CustomCursor(Args && ... args)
            : TestbedApplication(std::forward<Args>(args)...)
            , _colorCursor(std::make_shared<ColorCursor>(makeImage(), 31, 31))
        {}

        bool handle(minire::application::OnKeyDown const & e) override
        {
            if (TestbedApplication::handle(e))
                return true;

            if (e._key == SDLK_TAB)
            {
                MINIRE_INFO("Change cursor");
                _cursor = (_cursor + 1) % 3;

                if (0 == _cursor)
                {
                    setSystemCursor(minire::models::SystemCursor::kArrow);
                }
                else if (1 == _cursor)
                {
                    setSystemCursor(minire::models::SystemCursor::kIbeam);
                }
                else if (2 == _cursor)
                {
                    setColorCursor(_colorCursor);
                }
            }

            return true;
        }

    private:
        int               _cursor = 0;
        ColorCursor::Sptr _colorCursor;
    };
}

int main(int, char **)
{
    return minire::examples::main<CustomCursor>("Custom Cursor");
}
