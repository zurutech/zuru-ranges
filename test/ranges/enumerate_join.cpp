// enumerate_view and join_view, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }
// enumerate over a contiguous base: random_access + sized. join: forward, unsized.
static_assert(random_access_range<decltype(views::enumerate(std::declval<TArray<int>&>()))>);
static_assert(sized_range<decltype(views::enumerate(std::declval<TArray<int>&>()))>);
static_assert(forward_range<decltype(views::join(std::declval<TArray<TArray<int>>&>()))>);
static_assert(!sized_range<decltype(views::join(std::declval<TArray<TArray<int>>&>()))>);
} // namespace

// The sentinel and the const-conversion are member templates: nothing instantiates them
// unless a test does, so these two cases are what actually compiles that code.
TEST(EnumerateView, NonCommonBaseUsesSentinel) {
    // join's end() is always default_sentinel, so enumerate must build a sentinel rather
    // than an end iterator ([range.enumerate.sentinel]).
    std::vector<std::vector<int>> nested{{1, 2}, {}, {3}};
    auto en = views::enumerate(views::join(nested));
    static_assert(!std::is_same_v<decltype(en.begin()), decltype(en.end())>, "expected a distinct sentinel type");

    std::vector<int> indices;
    std::vector<int> values;
    for (auto&& [i, v] : en) {
        indices.push_back(static_cast<int>(i));
        values.push_back(v);
    }
    EXPECT_EQ(indices, (std::vector<int>{0, 1, 2}));
    EXPECT_EQ(values, (std::vector<int>{1, 2, 3}));
}

TEST(EnumerateView, ConstIteratorConvertsFromMutable) {
    std::vector<int> v{5, 6, 7};
    auto en = views::enumerate(v);

    const auto& cen = en;
    auto cit = cen.begin(); // iterator<true>
    cit = en.begin();       // iterator<false> -> iterator<true>, the converting ctor
    EXPECT_EQ(std::get<1>(*cit), 5);
    EXPECT_EQ(std::get<0>(*cit), 0);

    // Same conversion for transform, whose ctor is otherwise never instantiated either.
    auto tr = views::transform(v, doubler);
    const auto& ctr = tr;
    auto tcit = ctr.begin();
    tcit = tr.begin();
    EXPECT_EQ(*tcit, 10);
}

TEST(EnumerateView, PairsIndexWithElement) {
    TArray<int> a{10, 20, 30};
    int sum = 0;
    for (auto&& [i, v] : views::enumerate(a)) {
        sum += static_cast<int>(i) * 100 + v;
    }
    EXPECT_EQ(sum, 360); // (0*100+10)+(1*100+20)+(2*100+30)
}

TEST(EnumerateView, SizedAndIndexable) {
    TArray<int> a{10, 20, 30};
    auto e = views::enumerate(a);
    EXPECT_EQ(static_cast<int>(e.size()), 3);
    auto [idx, val] = e[2];
    EXPECT_EQ(static_cast<int>(idx), 2);
    EXPECT_EQ(val, 30);
}

TEST(JoinView, FlattensSkippingEmptyInner) {
    TArray<TArray<int>> nested{TArray<int>{1, 2}, TArray<int>{}, TArray<int>{3, 4, 5}};
    int sum = 0;
    for (int v : views::join(nested)) {
        sum += v;
    }
    EXPECT_EQ(sum, 15);
}

TEST(JoinView, ComposesWithTransformAndTo) {
    std::vector<std::vector<int>> sv{{1}, {2, 3}};
    TArray<int> out = sv | views::join | views::transform(doubler) | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(out.Num(), 3);
    EXPECT_EQ(out[0] + out[1] + out[2], 12); // (1+2+3)*2
}
