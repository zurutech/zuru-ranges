// Runtime: the zr::ranges CPOs drive iteration, size and empty over both std and
// Unreal containers. (Concept classification is checked at compile time in
// test/compile/range_concepts_checks.cpp.)
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

TEST(RangeConceptsMixed, CposDriveUnrealContainer) {
    TArray<int> a{1, 2, 3};
    int sum = 0;
    for (auto it = zr::ranges::begin(a); it != zr::ranges::end(a); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(static_cast<int>(zr::ranges::size(a)), 3);
    EXPECT_FALSE(zr::ranges::empty(a));
}

TEST(RangeConceptsMixed, CposDriveStdContainer) {
    std::vector<int> v{4, 5};
    EXPECT_EQ(static_cast<int>(zr::ranges::size(v)), 2);
    EXPECT_FALSE(zr::ranges::empty(v));
}

TEST(RangeConceptsMixed, EmptyContainerIsEmpty) {
    TArray<int> a{};
    EXPECT_TRUE(zr::ranges::empty(a));
    EXPECT_EQ(static_cast<int>(zr::ranges::size(a)), 0);
}
