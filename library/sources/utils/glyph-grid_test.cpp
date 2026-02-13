#include <minire/utils/glyph-grid.hpp>

#include <gtest/gtest.h>

namespace minire::utils::test
{
    TEST(TextLayout, Trivial)
    {
        TextLayout textLayout;
        EXPECT_TRUE(textLayout.empty());
        EXPECT_EQ(textLayout.indexOf(0, 0), std::nullopt);
        EXPECT_EQ(textLayout.indexOf(10, 10), std::nullopt);

        EXPECT_THROW(textLayout.layoutOf(0), std::exception);
    }

    TEST(TextLayout, Single)
    {
        TextLayout textLayout({utils::Rect(6, 6, 9, 9)});

        EXPECT_FALSE(textLayout.empty());

        EXPECT_EQ(textLayout.indexOf(0.0, 0.0), std::nullopt);
        EXPECT_EQ(textLayout.indexOf(5.0, 5.0), std::nullopt);
        EXPECT_EQ(textLayout.indexOf(5.9, 5.9), std::nullopt);
        EXPECT_EQ(textLayout.indexOf(9.1, 9.1), std::nullopt);

        EXPECT_EQ(textLayout.indexOf(6.0, 6.0), 0);
        EXPECT_EQ(textLayout.indexOf(7.0, 7.0), 0);
        EXPECT_EQ(textLayout.indexOf(8.0, 8.0), 0);
        EXPECT_EQ(textLayout.indexOf(9.0, 9.0), 0);

        EXPECT_EQ(textLayout.layoutOf(0), utils::Rect(6, 6, 9, 9));
        EXPECT_THROW(textLayout.layoutOf(1), std::exception);
    }

    TEST(TextLayout, Separated)
    {
        TextLayout textLayout(
        {
            utils::Rect(6, 6, 9, 9),
            utils::Rect(10, 10, 15, 15),
        });

        EXPECT_FALSE(textLayout.empty());

        EXPECT_EQ(textLayout.indexOf(8.0,  8.0), 0);
        EXPECT_EQ(textLayout.indexOf(11.0, 11.0), 1);

        EXPECT_EQ(textLayout.layoutOf(0), utils::Rect(6, 6, 9, 9));
        EXPECT_EQ(textLayout.layoutOf(1), utils::Rect(10, 10, 15, 15));
        EXPECT_THROW(textLayout.layoutOf(2), std::exception);
    }

    TEST(TextLayout, Intersected)
    {
        TextLayout textLayout(
        {
            utils::Rect(6, 6, 9, 9),
            utils::Rect(7, 7, 15, 15),
        });

        EXPECT_FALSE(textLayout.empty());

        EXPECT_EQ(textLayout.indexOf(8.0,  8.0), 0);
        EXPECT_EQ(textLayout.indexOf(11.0, 11.0), 1);

        EXPECT_EQ(textLayout.layoutOf(0), utils::Rect(6, 6, 9, 9));
        EXPECT_EQ(textLayout.layoutOf(1), utils::Rect(7, 7, 15, 15));
        EXPECT_THROW(textLayout.layoutOf(2), std::exception);
    }

    TEST(TextLayout, Nested)
    {
        TextLayout textLayout(
        {
            utils::Rect(6, 6, 9, 9),
            utils::Rect(7, 7, 8, 8),
        });

        EXPECT_FALSE(textLayout.empty());

        EXPECT_EQ(textLayout.indexOf(6.5, 6.5), 0);
        EXPECT_EQ(textLayout.indexOf(7.5, 7.5), 0);

        EXPECT_EQ(textLayout.layoutOf(0), utils::Rect(6, 6, 9, 9));
        EXPECT_EQ(textLayout.layoutOf(1), utils::Rect(7, 7, 8, 8));
        EXPECT_THROW(textLayout.layoutOf(2), std::exception);
    }
}