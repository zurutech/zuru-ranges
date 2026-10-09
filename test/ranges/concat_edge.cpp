// Edge cases of concat_view: empty ranges in every position, runs of empties,
// single-range, many-range, all-empty, and iter_move. Both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
template <class It> concept can_step_back = requires(It it) { --it; };

// view is iterated as an lvalue; the forwarding ref only lets it bind rvalue views too.
template <class View> std::vector<int> flatten(View&& view) { // NOLINT(cppcoreguidelines-missing-std-forward)
    std::vector<int> out;
    for (auto&& x : view) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

// Mirror of flatten walking backwards with operator--; needs a common, bidirectional concat.
template <class View> std::vector<int> backward(View&& view) { // NOLINT(cppcoreguidelines-missing-std-forward)
    std::vector<int> out;
    const auto first = zr::ranges::begin(view);
    for (auto it = zr::ranges::end(view); it != first;) {
        --it;
        out.push_back(static_cast<int>(*it));
    }
    return out;
}
} // namespace

TEST(ConcatEdge, EmptyFirst) {
    TArray<int> a{};
    std::vector<int> b{1, 2};
    TArray<int> c{3};
    EXPECT_EQ(flatten(views::concat(a, b, c)), (std::vector<int>{1, 2, 3}));
}

TEST(ConcatEdge, EmptyMiddle) {
    TArray<int> a{1};
    TArray<int> b{};
    std::vector<int> c{2};
    EXPECT_EQ(flatten(views::concat(a, b, c)), (std::vector<int>{1, 2}));
}

TEST(ConcatEdge, EmptyLast) {
    std::vector<int> a{1, 2};
    TArray<int> b{};
    EXPECT_EQ(flatten(views::concat(a, b)), (std::vector<int>{1, 2}));
}

TEST(ConcatEdge, AllEmptyYieldsEmpty) {
    TArray<int> a{};
    std::vector<int> b{};
    TArray<int> c{};
    auto v = views::concat(a, b, c);
    EXPECT_TRUE(flatten(v).empty());
    EXPECT_FALSE(zr::ranges::begin(v) != zr::ranges::end(v)); // begin == end immediately
}

TEST(ConcatEdge, RunsOfConsecutiveEmpties) {
    TArray<int> e0{}, e1{}, e2{}, e3{};
    TArray<int> one{1};
    std::vector<int> two{2};
    // empty, empty, {1}, empty, {2}, empty, empty
    EXPECT_EQ(flatten(views::concat(e0, e1, one, e2, two, e3)), (std::vector<int>{1, 2}));
}

TEST(ConcatEdge, SingleElementEach) {
    TArray<int> a{1};
    std::vector<int> b{2};
    TArray<int> c{3};
    std::vector<int> d{4};
    EXPECT_EQ(flatten(views::concat(a, b, c, d)), (std::vector<int>{1, 2, 3, 4}));
}

TEST(ConcatEdge, ForwardSetInterleaved) {
    TArray<int> a{1, 2};
    TSet<int> s{3};
    std::vector<int> b{4, 5};
    auto v = views::concat(a, s, b);
    static_assert(forward_range<decltype(v)>);
    EXPECT_EQ(flatten(v), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(ConcatEdge, SizeIsSumWhenAllSized) {
    TArray<int> a{1, 2};
    std::vector<int> b{3, 4, 5};
    EXPECT_EQ(views::concat(a, b).size(), 5u);
}

TEST(ConcatEdge, IterMoveYieldsCommonRvalueReference) {
    TArray<int> a{10};
    std::vector<int> b{20};
    auto v = views::concat(a, b);
    auto it = zr::ranges::begin(v);
    static_assert(std::is_same_v<decltype(iter_move(it)), int&&>);
    // noexcept ([range.concat.iterator]) tracks the element ops: the std leg derefs noexcept,
    // but the Unreal *checked* iterator's operator* is not, so concat's iter_move is noexcept
    // only in the raw-pointer configuration.
#if ZR_FAKE_UE_RANGED_FOR_CHECKS
    static_assert(!noexcept(iter_move(it)));
#else
    static_assert(noexcept(iter_move(it)));
#endif
    EXPECT_EQ(iter_move(it), 10);
}

TEST(ConcatBidirectional, BackwardAcrossRanges) {
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5};
    auto v = views::concat(a, b);
    static_assert(bidirectional_range<decltype(v)>);
    EXPECT_EQ(backward(v), (std::vector<int>{5, 4, 3, 2, 1}));
}

TEST(ConcatBidirectional, BackwardSkipsEmptyRanges) {
    std::vector<int> a{1};
    std::vector<int> e{};
    std::vector<int> b{2};
    // prev() must step over the empty middle range when crossing the boundary backwards.
    EXPECT_EQ(backward(views::concat(a, e, b)), (std::vector<int>{2, 1}));
}

TEST(ConcatBidirectional, IncrementDecrementRoundTrip) {
    std::vector<int> a{1, 2};
    std::vector<int> b{3, 4};
    auto v = views::concat(a, b);
    auto it = zr::ranges::begin(v);
    ++it; ++it; ++it; // step across the boundary into b -> 4
    EXPECT_EQ(*it, 4);
    --it; // back across the boundary -> 3
    EXPECT_EQ(*it, 3);
    --it; // -> 2 (back into a)
    EXPECT_EQ(*it, 2);
    ++it; // -> 3 again
    EXPECT_EQ(*it, 3);
}

TEST(ConcatBidirectional, ForwardOnlyBaseIsNotBidirectional) {
    // A forward-only base (TSet) keeps concat forward; operator-- is not available.
    TArray<int> a{1, 2};
    TSet<int> s{3};
    auto v = views::concat(a, s);
    static_assert(forward_range<decltype(v)>);
    static_assert(!bidirectional_range<decltype(v)>);
    static_assert(!can_step_back<decltype(zr::ranges::begin(v))>);
    EXPECT_EQ(flatten(v), (std::vector<int>{1, 2, 3}));
}
