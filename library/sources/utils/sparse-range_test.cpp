#include <utils/sparse-range.hpp>

#include <gtest/gtest.h>

namespace minire::utils::test
{
    struct SparseRangeCase
    {
        std::vector<std::pair<int, int>> _input;
        std::vector<std::pair<int, int>> _expected;        
    };

    class SparseRangeTest
        : public testing::TestWithParam<SparseRangeCase>
    {};

    TEST_P(SparseRangeTest, SmokeTest)
    {
        SparseRangeCase param = GetParam();
        SparseRange<int> sparseRange;

        for(auto [begin, end] : param._input)
        {
            sparseRange.insert(begin, end);
        }

        EXPECT_EQ(sparseRange.tighten(),
                  param._expected);
    }

    INSTANTIATE_TEST_SUITE_P(
        SparseRangeTestGroup,
        SparseRangeTest,
        testing::Values(
            SparseRangeCase{{}, {}}
           ,SparseRangeCase{{ {15, 20} }, { {15, 20} }}
           ,SparseRangeCase{{ {15, 20}, {17, 25} }, { {15, 25} }}
           ,SparseRangeCase{{ {15, 20}, {21, 25} }, { {15, 20}, {21, 25} }}
           ,SparseRangeCase{{ {15, 20}, {15, 20} }, { {15, 20} }}
           ,SparseRangeCase{{ {15, 25}, {17, 20} }, { {15, 25} }}
           ,SparseRangeCase{{ {15, 20}, {20, 25} }, { {15, 25} }}
           ,SparseRangeCase{{ {15, 20}, {15, 18} }, { {15, 20} }}
           ,SparseRangeCase{{ {15, 20}, {17, 20} }, { {15, 20} }}
           ,SparseRangeCase{{ {10, 13}, {13, 14}, {15, 16} }, { {10, 14}, {15, 16} }}
           ,SparseRangeCase{{ {15, 16}, {13, 14}, {10, 13} }, { {10, 14}, {15, 16} }}
        )
    );
}

