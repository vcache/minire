#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/progress-bar.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdlib> // for EXIT_SUCCESS

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    class GuiProgressBar
        : public minire::GuiApplication
    {
    protected:
        using GuiApplication::GuiApplication;

        void onStart() override
        {
            GuiApplication::onStart();

            auto layout = std::make_shared<layouts::Grid>(2, 2);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;
            container->horizontal() = Arranger::fill();
            container->vertical() = Arranger::fill();

            auto makeProgressBar = [this, container, layout]
                (size_t row, size_t col, ProgressBar::Direction dir, std::string name)
                {
                    auto progressBar = container->emplace<ProgressBar>(name, dir);

                    bool const isHorizontal = ProgressBar::Direction::kLeftToRight == dir ||
                                              ProgressBar::Direction::kRightToLeft == dir;

                    if (isHorizontal)
                    {
                        progressBar->horizontal() = Arranger(position::Center{}, dimension::Constant{250});
                        progressBar->vertical() = Arranger(position::Center{}, dimension::Constant{24});
                    }
                    else
                    {
                        progressBar->horizontal() = Arranger(position::Center{}, dimension::Constant{24});
                        progressBar->vertical() = Arranger(position::Center{}, dimension::Constant{250});
                    }

                    layout->set(row, col, progressBar->id());

                    _progressBars.emplace_back(progressBar);
                };

            makeProgressBar(0, 0, ProgressBar::Direction::kLeftToRight, "kLeftToRight");
            makeProgressBar(0, 1, ProgressBar::Direction::kRightToLeft, "kRightToLeft");
            makeProgressBar(1, 0, ProgressBar::Direction::kTopToBottom, "kTopToBottom");
            makeProgressBar(1, 1, ProgressBar::Direction::kBottomToTop, "kBottomToTop");
        }

        bool onStep() override
        {
            float value = (1.0f + std::sin(_phase)) / 2.0f;

            for(auto const & wprogressBar : _progressBars)
            {
                if (auto progressBar = wprogressBar.lock();
                    progressBar)
                {
                    progressBar->value() = value;
                }
            }

            _phase += frameTime();

            return GuiApplication::onStep();
        }

    private:
        double                         _phase = 0;
        std::vector<ProgressBar::Wptr> _progressBars;
    };
}

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        GuiProgressBar application(1280, 720, "GUI ProgressBar", manager);
        application.setGlDebug(false);
        application.setVsync(true);

        // Main loop
        application.run();

        // Finish
        return EXIT_SUCCESS;
    }
    catch(std::exception const & e)
    {
        MINIRE_ERROR("Fatal error:\n{}", e.what());
    }
    catch(...)
    {
        MINIRE_ERROR("Fatal error: (unknown error)");
    }

    return EXIT_FAILURE;
}
