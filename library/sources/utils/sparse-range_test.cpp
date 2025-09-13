#include <utils/sparse-range.hpp>

#include <gtest/gtest.h>

namespace minire::utils::test
{
    // TODO: cover corner-cases

    TEST(SparseRange, SmokeTest)
    {
        SparseRange<float> sparseRange;

        sparseRange.insert(10, 13);
        sparseRange.insert(13, 14);
        sparseRange.insert(15, 16);

        auto const & result = sparseRange.tighten();

        ASSERT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], std::make_pair(10.0f, 14.0f));
        EXPECT_EQ(result[1], std::make_pair(15.0f, 16.0f));
    }
}
