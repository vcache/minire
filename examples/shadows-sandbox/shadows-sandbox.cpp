#include "../common/testbed.hpp"

#include <minire/errors.hpp>
#include <minire/gui/arranger.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/dropdown.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/components/listview.hpp>
#include <minire/gui/components/spinbox.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/gui/layouts/array.hpp>

#include <any>
#include <cassert>
#include <vector>

namespace
{
    using namespace minire::gui;
    using namespace minire::models;
    using namespace minire::text;

    static Format const kDefaultFormat = Format().foreground(glm::vec4(0, 0, 0, 1))
                                                 .background(glm::vec4(0, 0, 0, 0));

    static float const kLabelsWidth = 100.0f;
    static float const kSpacing = 7.0f;

    template<class... Ts>
    struct Overloaded : Ts... { using Ts::operator()...; };

    class ShadowsSandboxExample
        : public minire::examples::GuiTestbedApplication
    {
        enum class LightType { kPoint, kDirectional };

        static std::string toString(LightType lightType)
        {
            switch(lightType)
            {
                case LightType::kPoint: return "Point light";
                case LightType::kDirectional: return "Directional light";
            }

            MINIRE_THROW("unknown LightType: {}", static_cast<int>(lightType));
        }

        static std::string toString(shadow_params::Method const & method)
        {
            return std::visit(Overloaded
            {
                [](shadow_params::method::Standard const &) -> std::string
                {
                    return "Standard";
                },
                [](shadow_params::method::ESM const & esm) -> std::string
                {
                    return fmt::format("ESM, K = {:.2f}", esm._factor);
                },
                [](shadow_params::method::LogESM const & logEsm) -> std::string
                {
                    return fmt::format("Log-ESM, K = {:.2f}", logEsm._factor);
                },
            }, method);
        }

        static std::string toString(shadow_params::Filter const & filter)
        {
            return std::visit(Overloaded
            {
                [](std::monostate const &) -> std::string { return "No filtering"; },
                [](shadow_params::filter::PCF const &) -> std::string
                {
                    return "Percentage-Closer Filtering (PCF)";
                },
                [](shadow_params::filter::GaussianBlur const & gaussianBlur) -> std::string
                {
                    return fmt::format("GaussianBlur ({} pass)", gaussianBlur._iterations);
                },
            }, filter);
        }

        static std::string toString(shadow_params::Bias const & bias)
        {
            return std::visit(Overloaded
            {
                [](shadow_params::bias::Constant const & constant)
                {
                    return fmt::format("Constant: {:.6}",
                                       constant._biasBase);
                },
                [](shadow_params::bias::SlopScaled const & slopScaled)
                {
                    return fmt::format("Slop-Scaled: {:.6f} / {:.6f}",
                                       slopScaled._biasBase,
                                       slopScaled._maxBias);
                },
            }, bias);
        }

    private:
        template<typename ValueType, typename... ValueArgs>
        ValueType::Sptr addParameter(std::string const & title,
                                     Dimension const & valueDimension,
                                     ValueArgs && ... valueArgs)
        {
            assert(_base);

            auto row = _base->emplace<Component>("row-" + title);

            auto label = row->emplace<components::Text>("label",
                FormattedString(title + ":", kDefaultFormat));
            label->horizontal() = Arranger(position::Begin(), dimension::Content{});

            auto value = row->emplace<ValueType>("value", std::forward<ValueArgs>(valueArgs)...);

            auto layout = row->newLayout<layouts::Row>();
            layout->pushBack(label, dimension::Constant{kLabelsWidth});
            layout->pushBack(dimension::Constant{kSpacing});
            layout->pushBack(value, valueDimension);

            assert(_baseLayout);
            _baseLayout->pushBack(row, dimension::Constant{25});
            _baseLayout->pushBack(dimension::Constant{kSpacing});

            return value;
        }

        void rebuildLight()
        {
            ShadowParams shadowParams;

            assert(_lightType && _lightType->current());
            LightType lightType = std::any_cast<LightType>(*_lightType->current());

            assert(_mapSize && _mapSize->current());
            shadowParams._mapSize = std::any_cast<size_t>(*_mapSize->current());

            assert(_method && _method->current());
            shadowParams._method = std::any_cast<shadow_params::Method>(*_method->current());

            assert(_filtering && _filtering->current());
            shadowParams._filter = std::any_cast<shadow_params::Filter>(*_filtering->current());

            assert(_normalBias && _normalBias->current());
            shadowParams._normalBias = std::any_cast<shadow_params::Bias>(*_normalBias->current());

            assert(_depthBias && _depthBias->current());
            shadowParams._depthBias = std::any_cast<shadow_params::Bias>(*_normalBias->current());

            if (_lightNode)
            {
                _lightNode->detach();
                _lightNode.reset();
            }

            switch(lightType)
            {
                case LightType::kPoint:
                    _lightNode = scene().root().make("test-light", Node{Transform(glm::vec3(5.0f))});
                    _lightNode->make("bulb", PointLight(glm::vec4(1, 1, 1, 500), 2, shadowParams, true));
                    break;

                case LightType::kDirectional:
                    shadowParams._center = shadow_params::center::CameraYPlaneHitPoint{};
                    shadowParams._radiusMargin = shadow_params::margin::Absolute{10.0f};
                    _lightNode = scene().root().make("test-light",
                        Node{Transform(glm::vec3(0), lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0)))});
                    _lightNode->make("sun", DirectionalLight(glm::vec3(1, 1, 1), shadowParams, true));
                    break;
            }
        }

    public:
        explicit ShadowsSandboxExample(int width, int height,
                                       std::string const & title,
                                       minire::content::Manager & contentManager)
            : GuiTestbedApplication(width, height, title, contentManager, false)
        {}

        void onStart() override
        {
            GuiTestbedApplication::onStart();

            // add a model on a scene

            using namespace minire::content;
            scene().root().make("cube",
                Mesh
                {
                    ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = nullptr,
                    ._skin = std::nullopt,
                    ._visible = true,
                });

            // build GUI

            _base = guiRoot().emplace<components::Image>("base",
                theme().get<sprite::MaybeImage>(components::Button::kName, "bg-normal", {}));

            _base->horizontal() = Arranger(position::End{}, dimension::Constant{370}, 5, 5);
            _base->vertical() = Arranger(position::End{}, dimension::Constant{500}, 5, 5);
            _base->padding() = minire::utils::Rect(10);
            _baseLayout = _base->newLayout<layouts::Column>();

            _lightType = addParameter<components::Dropdown>("Light type", dimension::Fill{});
            {
                _lightType->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            return FormattedString(toString(std::any_cast<LightType>(value)),
                                                   kDefaultFormat);
                        }));
                *_lightType->contents() = std::vector<std::any>
                {
                    LightType::kDirectional, LightType::kPoint,
                };
                _lightType->select(0);
                _lightType->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            _mapSize = addParameter<components::Dropdown>("Shadow map size", dimension::Fill{});
            {
                _mapSize->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            return FormattedString(std::to_string(std::any_cast<size_t>(value)),
                                                   kDefaultFormat);
                        }));
                *_mapSize->contents() = std::vector<std::any>
                {
                    size_t(64),   size_t(128),  size_t(256),  size_t(512),
                    size_t(1024), size_t(2048), size_t(4096), size_t(8192),
                };
                _mapSize->select(4);
                _mapSize->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            _method = addParameter<components::Dropdown>("Shadow map method", dimension::Fill{});
            {
                _method->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            shadow_params::Method const & method = std::any_cast<shadow_params::Method>(value);
                            return FormattedString(toString(method), kDefaultFormat);
                        }));
                *_method->contents() = std::vector<std::any>
                {
                    shadow_params::Method{shadow_params::method::Standard{}},
                    shadow_params::Method{shadow_params::method::ESM{10}},
                    shadow_params::Method{shadow_params::method::ESM{32}},
                    shadow_params::Method{shadow_params::method::ESM{50}},
                    shadow_params::Method{shadow_params::method::ESM{75}},
                    shadow_params::Method{shadow_params::method::ESM{100}},
                    shadow_params::Method{shadow_params::method::ESM{300}},
                    shadow_params::Method{shadow_params::method::ESM{500}},
                    shadow_params::Method{shadow_params::method::ESM{1000}},
                    shadow_params::Method{shadow_params::method::ESM{2500}},
                    shadow_params::Method{shadow_params::method::ESM{5000}},
                    shadow_params::Method{shadow_params::method::ESM{7500}},
                    shadow_params::Method{shadow_params::method::ESM{10000}},
                    shadow_params::Method{shadow_params::method::LogESM{10}},
                    shadow_params::Method{shadow_params::method::LogESM{32}},
                    shadow_params::Method{shadow_params::method::LogESM{50}},
                    shadow_params::Method{shadow_params::method::LogESM{75}},
                    shadow_params::Method{shadow_params::method::LogESM{100}},
                    shadow_params::Method{shadow_params::method::LogESM{300}},
                    shadow_params::Method{shadow_params::method::LogESM{500}},
                    shadow_params::Method{shadow_params::method::LogESM{1000}},
                    shadow_params::Method{shadow_params::method::LogESM{2500}},
                    shadow_params::Method{shadow_params::method::LogESM{5000}},
                    shadow_params::Method{shadow_params::method::LogESM{7500}},
                    shadow_params::Method{shadow_params::method::LogESM{10000}},
                };
                _method->select(0);
                _method->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            _filtering = addParameter<components::Dropdown>("Filtering", dimension::Fill{});
            {
                _filtering->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            shadow_params::Filter const & filter = std::any_cast<shadow_params::Filter>(value);
                            return FormattedString(toString(filter), kDefaultFormat);
                        }));
                *_filtering->contents() = std::vector<std::any>
                {
                    shadow_params::Filter{std::monostate()},
                    shadow_params::Filter{shadow_params::filter::PCF{}},
                    shadow_params::Filter{shadow_params::filter::GaussianBlur{1}},
                    shadow_params::Filter{shadow_params::filter::GaussianBlur{2}},
                    shadow_params::Filter{shadow_params::filter::GaussianBlur{3}},
                    shadow_params::Filter{shadow_params::filter::GaussianBlur{4}},
                };
                _filtering->select(0);
                _filtering->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            _normalBias = addParameter<components::Dropdown>("Normal bias", dimension::Fill{});
            {
                _normalBias->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            shadow_params::Bias const & bias = std::any_cast<shadow_params::Bias>(value);
                            return FormattedString(toString(bias), kDefaultFormat);
                        }));
                *_normalBias->contents() = std::vector<std::any>
                {
                    shadow_params::Bias{shadow_params::bias::Constant(0.0f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.00001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.00005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.0001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.0005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.01f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.05f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.1f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.5f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.0001f, 0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.001f,  0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.01f,   0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.0001f, 0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.001f,  0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.01f,   0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.1f,    0.1f)},
                };
                _normalBias->select(0);
                _normalBias->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            _depthBias = addParameter<components::Dropdown>("Depth bias", dimension::Fill{});
            {
                _depthBias->setBaseItemBuilder(
                    components::listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            shadow_params::Bias const & bias = std::any_cast<shadow_params::Bias>(value);
                            return FormattedString(toString(bias), kDefaultFormat);
                        }));
                *_depthBias->contents() = std::vector<std::any>
                {
                    shadow_params::Bias{shadow_params::bias::Constant(0.0f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.00001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.00005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.0001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.0005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.001f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.005f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.01f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.05f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.1f)},
                    shadow_params::Bias{shadow_params::bias::Constant(0.5f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.0001f, 0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.001f,  0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.01f,   0.01f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.0001f, 0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.001f,  0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.01f,   0.1f)},
                    shadow_params::Bias{shadow_params::bias::SlopScaled(0.1f,    0.1f)},
                };
                _depthBias->select(0);
                _depthBias->setCallback(std::in_place_type<components::dropdown::OnSelectionChanged>, "foo",
                    [this](Component const &, components::dropdown::OnSelectionChanged const &)
                    { rebuildLight(); });
            }

            // erase the last one spacing
            _baseLayout->popBack();

            rebuildLight();
        }

    private:
        components::Image::Sptr    _base;
        layouts::Column::Sptr      _baseLayout;
        components::Dropdown::Sptr _lightType;
        components::Dropdown::Sptr _mapSize;
        components::Dropdown::Sptr _method;
        components::Dropdown::Sptr _filtering;
        components::Dropdown::Sptr _normalBias;
        components::Dropdown::Sptr _depthBias;
        minire::scene::Node::Sptr  _lightNode;
    };
}

int main(int, char **)
{
    return minire::examples::main<ShadowsSandboxExample>("Shadows sandbox");
}
