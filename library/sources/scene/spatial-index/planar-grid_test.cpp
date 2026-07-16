#include <minire/scene/spatial-index/planar-grid.hpp>

#include <scene/spatial-handler.hpp>
#include <minire/utils/culling-test.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

namespace minire::scene::spatial_index::test
{
    namespace
    {
        // test frustums
        static utils::FrustumPlanes const kEmptyPlanes
        {
            glm::vec3(0), glm::vec3(0), {}
        };

        static utils::FrustumPlanes const kSmallFrustum
        {
            glm::vec3(-0.5f), glm::vec3(0.5f), {}
        };

        static utils::FrustumPlanes const kTrivialFrustum
        {
            glm::vec3(-1.0f), glm::vec3(1.0f), {}
        };

        static utils::FrustumPlanes const kLargeFrustum
        {
            glm::vec3(-10.0f), glm::vec3(10.0f), {}
        };

        // dummy payloads
        static std::string kPayload0 = "foo";
        static std::string kPayload1 = "buz";
        static std::string kPayload2 = "bar";

        // payload caster
        inline IndexPayload * mockPayload(std::string& s)
        {
            return reinterpret_cast<IndexPayload*>(&s);
        }
    }

    TEST(PlanarIndex, SmokeTest)
    {
        PlanarGrid planarGrid;
        std::vector<IndexPayload *> output;
        planarGrid.cull(kEmptyPlanes, 0, output);
        EXPECT_TRUE(output.empty());
    }

    TEST(PlanarIndex, TrivialCase)
    {
        PlanarGrid planarGrid;
        SpatialHandler spatialHandler(planarGrid, mockPayload(kPayload0), 0,
                                      utils::Aabb{-1, -1, -1, 1, 1, 1});

        for(auto const & frustumPlanes : {kSmallFrustum, kTrivialFrustum, kLargeFrustum})
        {
            std::vector<IndexPayload *> output;
            planarGrid.cull(frustumPlanes, 0, output);
            ASSERT_EQ(output.size(), 1);
            EXPECT_EQ(output[0], mockPayload(kPayload0));

            output.clear();
            planarGrid.cull(frustumPlanes, 42, output);
            EXPECT_TRUE(output.empty());
        }
    }

    TEST(PlanarIndex, DisjointElement)
    {
        PlanarGrid planarGrid;
        SpatialHandler spatialHandler(planarGrid, mockPayload(kPayload0), 0,
                                      utils::Aabb{100, 100, 100, 102, 102, 102});

        std::vector<IndexPayload *> output;
        planarGrid.cull(kTrivialFrustum, 0, output);
        EXPECT_TRUE(output.empty());
    }

    TEST(PlanarIndex, ElementUpdateCulling)
    {
        PlanarGrid planarGrid;
        SpatialHandler spatialHandler(planarGrid, mockPayload(kPayload0), 0,
                                      utils::Aabb{-0.5, -0.5, -0.5, 0.5, 0.5, 0.5});

        std::vector<IndexPayload *> output;
        
        // intersects initially
        planarGrid.cull(kTrivialFrustum, 0, output);
        ASSERT_EQ(output.size(), 1);

        // moved far away
        spatialHandler.update(utils::Aabb{100, 100, 100, 102, 102, 102});
        output.clear();
        planarGrid.cull(kTrivialFrustum, 0, output);
        EXPECT_TRUE(output.empty());

        // noved back
        spatialHandler.update(utils::Aabb{-0.5, -0.5, -0.5, 0.5, 0.5, 0.5});
        output.clear();
        planarGrid.cull(kTrivialFrustum, 0, output);
        ASSERT_EQ(output.size(), 1);
    }

    TEST(PlanarIndex, ElementErasure)
    {
        PlanarGrid planarGrid;
        std::vector<IndexPayload *> output;

        {
            SpatialHandler spatialHandler(planarGrid, mockPayload(kPayload0), 0,
                                          utils::Aabb{-1, -1, -1, 1, 1, 1});
            planarGrid.cull(kTrivialFrustum, 0, output);
            ASSERT_EQ(output.size(), 1);
        }

        output.clear();
        planarGrid.cull(kTrivialFrustum, 0, output);
        EXPECT_TRUE(output.empty());
    }

    TEST(PlanarIndex, MultipleElementsLifecycle)
    {
        PlanarGrid planarGrid(glm::vec2(1, 1));
        
        // is inside
        std::optional<SpatialHandler> h0(std::in_place, planarGrid, mockPayload(kPayload0), 0,
                                         utils::Aabb{-1, -1, -1, 1, 1, 1});

        // is outside
        std::optional<SpatialHandler> h1(std::in_place, planarGrid, mockPayload(kPayload1), 0,
                                         utils::Aabb{10, 10, 10, 12, 12, 12});

        std::vector<IndexPayload *> output;
        
        // initial intersection
        planarGrid.cull(kTrivialFrustum, 0, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload0));

        // swap their positions
        h0->update(utils::Aabb{10, 10, 10, 12, 12, 12});
        h1->update(utils::Aabb{-1, -1, -1, 1, 1, 1});

        output.clear();
        planarGrid.cull(kTrivialFrustum, 0, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload1));

        // erase the remaining visible element
        h1.reset(); 

        output.clear();
        planarGrid.cull(kTrivialFrustum, 0, output);
        EXPECT_TRUE(output.empty());
    }

    TEST(PlanarIndex, DeduplicationOfLargeElements)
    {
        // NOTES: tiles must be huge enough, otherwise
        //        linear searches will kill the performance
        PlanarGrid planarGrid(glm::vec2(50, 50));
        
        SpatialHandler giantElement(planarGrid, mockPayload(kPayload0), 0,
                                    utils::Aabb{-500, -500, -500, 500, 500, 500});

        std::vector<IndexPayload *> output;
        planarGrid.cull(kLargeFrustum, 0, output);
        
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload0));
    }

    TEST(PlanarIndex, MassiveOutOfBoundsClamp)
    {
        // NOTES: tiles must be huge enough, otherwise
        //        linear searches will kill the performance
        PlanarGrid planarGrid(glm::vec2(100, 100));
        
        SpatialHandler farNegative(planarGrid, mockPayload(kPayload0), 0,
                                   utils::Aabb{-100000, -100000, -100000, -99999, -99999, -99999});
        SpatialHandler farPositive(planarGrid, mockPayload(kPayload1), 0,
                                   utils::Aabb{100000, 100000, 100000, 100001, 100001, 100001});

        std::vector<IndexPayload *> output;
        planarGrid.cull(kTrivialFrustum, 0, output);
        
        EXPECT_TRUE(output.empty());
    }

    TEST(PlanarIndex, LayerSegregation)
    {
        PlanarGrid planarGrid;
        
        SpatialHandler mesh(planarGrid, mockPayload(kPayload0), 0, utils::Aabb{-1, -1, -1, 1, 1, 1});
        SpatialHandler light(planarGrid, mockPayload(kPayload1), 1, utils::Aabb{-1, -1, -1, 1, 1, 1});
        SpatialHandler bboard(planarGrid, mockPayload(kPayload2), 2, utils::Aabb{-1, -1, -1, 1, 1, 1});

        std::vector<IndexPayload *> output;

        planarGrid.cull(kTrivialFrustum, 0, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload0));

        output.clear();
        planarGrid.cull(kTrivialFrustum, 1, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload1));

        output.clear();
        planarGrid.cull(kTrivialFrustum, 2, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], mockPayload(kPayload2));
    }
}