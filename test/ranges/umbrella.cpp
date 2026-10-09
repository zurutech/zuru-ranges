// Single-include smoke test: everything is reachable through "zr/ranges.h", and a
// multi-view pipeline mixing factories, adaptors, std and Unreal containers works in
// both configurations.
#include "CoreMinimal.h"

#include "zr/ranges.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int square(int x) { return x * x; }
constexpr bool is_even(int x) { return x % 2 == 0; }
} // namespace

TEST(Umbrella, FactoryAdaptorPipeline) {
    TArray<int> a = views::iota(1, 6)          // 1,2,3,4,5
                    | views::transform(square) // 1,4,9,16,25
                    | views::filter(is_even)   // 4,16
                    | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(a.Num(), 2);
    EXPECT_EQ(a[0] + a[1], 20);
}

TEST(Umbrella, ConcatZipEnumerateMixed) {
    TArray<int> b{1, 2};
    std::vector<int> c{3, 4};

    int concat_sum = 0;
    for (int v : views::concat(b, c)) {
        concat_sum += v;
    }
    EXPECT_EQ(concat_sum, 10);

    int zip_sum = 0;
    for (auto&& [x, y] : views::zip(b, c)) {
        zip_sum += x * y;
    }
    EXPECT_EQ(zip_sum, 11); // 1*3 + 2*4

    int enum_sum = 0;
    for (auto&& [i, v] : views::enumerate(c)) {
        enum_sum += static_cast<int>(i) + v;
    }
    EXPECT_EQ(enum_sum, 8); // (0+3)+(1+4)

    int cart_sum = 0;
    for (auto&& [x, y] : views::cartesian_product(b, c)) {
        cart_sum += x * y;
    }
    EXPECT_EQ(cart_sum, 21); // 1*3 + 1*4 + 2*3 + 2*4
}

TEST(Umbrella, ReverseSingleReachable) {
    EXPECT_EQ(views::single(5)[0], 5);
    TArray<int> b{1, 2};
    TArray<int> r = b | views::reverse | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(r[0], 2);
}
