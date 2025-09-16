#include <rasterizer/resources.hpp>

#include <gtest/gtest.h>

namespace minire::rasterizer::test
{
    TEST(Resources, SmokeTest)
    {
        Resources resources;

        Resources::Key key = textures::Id{"foo", models::Sampler(), false};

        // Empty storage
        {
            auto result = resources.find(key);
            ASSERT_FALSE(result.has_value());
        }

        // Insert a new one
        {
            resources.insert(key, std::make_shared<std::string>("bar"));
            auto result = resources.find(key);
            ASSERT_TRUE(result.has_value());
            EXPECT_THROW(std::any_cast<std::string>(result),
                         std::bad_any_cast);
            auto value = std::any_cast<std::shared_ptr<std::string>>(result);
            ASSERT_TRUE(value);
            EXPECT_EQ(*value, "bar");
        }

        // Erase the key
        {
            EXPECT_NO_THROW(resources.erase(key));
            auto result = resources.find(key);
            EXPECT_FALSE(result.has_value());
        }
    }

    TEST(Resources, LayerDisposal)
    {
        Resources resources;

        Resources::Key key = textures::Id{"foo", models::Sampler(), false};

        resources.insert(key, std::string("foo-data"));
        ASSERT_TRUE(resources.find(key).has_value());

        resources.disposeLayer(resources.current());
        EXPECT_FALSE(resources.find(key).has_value());
    }

    TEST(Resources, LayerSwitch)
    {
        Resources resources;

        Resources::Key keyFoo = textures::Id{"foo", models::Sampler(), false};
        Resources::Key keyBar = textures::Id{"bar", models::Sampler(), false};

        resources.insert(keyFoo, std::string("foo-data"));
        ASSERT_TRUE(resources.find(keyFoo).has_value());

        resources.newLayer("another");

        resources.insert(keyBar, std::string("bar-data"));
        ASSERT_TRUE(resources.find(keyBar).has_value());

        resources.disposeLayer("another");
        EXPECT_FALSE(resources.find(keyBar).has_value());
        EXPECT_TRUE(resources.find(keyFoo).has_value());
    }

    TEST(Resources, Override)
    {
        Resources resources;

        Resources::Key key = textures::Id{"foo", models::Sampler(), false};

        resources.insert(key, std::string("foo-data"));
        ASSERT_TRUE(resources.find(key).has_value());

        EXPECT_THROW(resources.insert(key, std::string("new-data")),
                     std::exception);
        std::any oldValue = resources.find(key);
        ASSERT_TRUE(oldValue.has_value());
        EXPECT_EQ(std::any_cast<std::string>(oldValue), "foo-data");

        EXPECT_NO_THROW(resources.insert(key, std::string("new-data"), true));
        std::any newValue = resources.find(key);
        ASSERT_TRUE(newValue.has_value());
        EXPECT_EQ(std::any_cast<std::string>(newValue), "new-data");
    }

    TEST(Resources, OverrideAndKeepLayer)
    {
        Resources resources;

        Resources::Key key = textures::Id{"foo", models::Sampler(), false};

        resources.insert(key, std::string("foo-data"));
        resources.newLayer("new-layer");
        resources.insert(key, std::string("new-data"), true);
        resources.disposeLayer("new-layer");

        std::any newValue = resources.find(key);
        ASSERT_TRUE(newValue.has_value());
        EXPECT_EQ(std::any_cast<std::string>(newValue), "new-data");
    }
}
