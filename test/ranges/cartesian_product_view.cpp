// views::cartesian_product over heterogeneous containers, both build configurations. Covers the
// odometer order, restarting a tail base (for TArray in debug, a non-assignable checked
// iterator), and the empty-base arrangements, where a wrong end() would run ++ off an empty
// range.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/cartesian_product_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/unreal/ue_range_traits.h"

#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

using cart_arr_vec = decltype(views::cartesian_product(std::declval<TArray<int>&>(), std::declval<std::vector<char>&>()));
using cart_arr_set = decltype(views::cartesian_product(std::declval<TArray<int>&>(), std::declval<TSet<int>&>()));
using cart_set_arr = decltype(views::cartesian_product(std::declval<TSet<int>&>(), std::declval<TArray<int>&>()));

// All bases random-access and the tail sized -> random_access (never contiguous, the
// elements are synthesized tuples), sized, and common (the first base is common).
static_assert(random_access_range<cart_arr_vec> && !contiguous_range<cart_arr_vec>);
static_assert(sized_range<cart_arr_vec> && common_range<cart_arr_vec>);
// A forward tail base has no element_at, so the whole product drops to forward. It is
// still sized, and still common, since commonality depends on the first base alone.
static_assert(forward_range<cart_arr_set> && !random_access_range<cart_arr_set>);
static_assert(sized_range<cart_arr_set> && common_range<cart_arr_set>);
// A forward first base likewise caps the category, even though every tail base indexes.
static_assert(forward_range<cart_set_arr> && !random_access_range<cart_set_arr>);

// [range.cartesian.iterator]: decrement and arithmetic iff the base iterators provide them
// (TArray's is a T* in Shipping only).
static_assert(prefix_decrementable<iterator_t<cart_arr_vec>> == prefix_decrementable<iterator_t<TArray<int>>>);

// A non-common first base leaves the product non-common (end() is default_sentinel).
using cart_iota_vec = decltype(views::cartesian_product(views::iota(0), std::declval<std::vector<char>&>()));
static_assert(!common_range<cart_iota_vec>);

// The iterators are not assignable in the debug configuration: copy and ++ only.
template <class It> It advanced(It it, int n) {
    while (n-- > 0) {
        ++it;
    }
    return it;
}

// Collects the product as a flat string, so both the elements and their ORDER are asserted.
template <class View> std::string spell(View&& view) {
    std::string out;
    for (auto&& [i, c] : std::forward<View>(view)) {
        out += std::to_string(i);
        out += c;
    }
    return out;
}

} // namespace

TEST(CartesianProductView, LastRangeVariesFastest) {
    TArray<int> a{1, 2, 3};
    std::vector<char> b{'x', 'y'};
    EXPECT_EQ(spell(views::cartesian_product(a, b)), "1x1y2x2y3x3y");
}

TEST(CartesianProductView, RestartsANonAssignableTailIterator) {
    // TArray is the TAIL base, so its iterator is the one restarted at every carry. In
    // the debug configuration that iterator is TCheckedPointerIterator, which is not
    // copy-assignable; the movable_box reconstruct is what makes this compile and run.
    std::vector<char> b{'x', 'y'};
    TArray<int> a{1, 2, 3};
    std::string out;
    for (auto&& [c, i] : views::cartesian_product(b, a)) {
        out += c;
        out += std::to_string(i);
    }
    EXPECT_EQ(out, "x1x2x3y1y2y3");
}

TEST(CartesianProductView, ThreeBasesVisitEveryCombinationOnce) {
    TArray<int> a{1, 2};
    std::vector<char> b{'x', 'y', 'z'};
    TSet<int> s{7, 8};
    int count = 0;
    int sum = 0;
    for (auto&& [i, c, k] : views::cartesian_product(a, b, s)) {
        ++count;
        sum += i + static_cast<int>(c - 'x') + k;
    }
    EXPECT_EQ(count, 12);
    // Each base contributes its own sum, repeated once per combination of the others:
    // (1+2)*6 + (0+1+2)*4 + (7+8)*6 = 18 + 12 + 90.
    EXPECT_EQ(sum, 120);
}

TEST(CartesianProductView, SizeIsTheProductAndIndexingIsMixedRadix) {
    TArray<int> a{1, 2, 3};
    std::vector<char> b{'x', 'y'};
    auto v = views::cartesian_product(a, b);
    EXPECT_EQ(static_cast<int>(v.size()), 6);

    // Flat index 3 with radices (3, 2): digit_1 = 3 % 2 = 1, digit_0 = 3 / 2 = 1.
    auto [x, y] = v[3];
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 'y');

    auto [first_i, first_c] = v[0];
    EXPECT_EQ(first_i, 1);
    EXPECT_EQ(first_c, 'x');
    auto [last_i, last_c] = v[5];
    EXPECT_EQ(last_i, 3);
    EXPECT_EQ(last_c, 'y');
}

TEST(CartesianProductView, IndexingAgreesWithIteration) {
    TArray<int> a{4, 5, 6};
    std::vector<char> b{'p', 'q'};
    auto v = views::cartesian_product(a, b);
    std::ptrdiff_t index = 0;
    for (auto&& [i, c] : v) {
        auto [xi, xc] = v[index];
        EXPECT_EQ(i, xi);
        EXPECT_EQ(c, xc);
        ++index;
    }
    EXPECT_EQ(index, static_cast<std::ptrdiff_t>(v.size()));
}

// --- the empty-base arrangements: every one must yield an empty product ---------------
// A wrong end() here is UB, not a compile error: ++ would step past an empty base looking
// for a boundary it can never reach.

TEST(CartesianProductView, EmptyFirstBaseYieldsNothing) {
    TArray<int> no_ints;
    std::vector<char> b{'x', 'y'};
    int count = 0;
    for (auto&& t : views::cartesian_product(no_ints, b)) {
        (void)t;
        ++count;
    }
    EXPECT_EQ(count, 0);
    EXPECT_TRUE(views::cartesian_product(no_ints, b).empty());
}

TEST(CartesianProductView, EmptyLastBaseYieldsNothing) {
    TArray<int> a{1, 2, 3};
    std::vector<char> no_chars;
    int count = 0;
    for (auto&& t : views::cartesian_product(a, no_chars)) {
        (void)t;
        ++count;
    }
    EXPECT_EQ(count, 0);
    EXPECT_TRUE(views::cartesian_product(a, no_chars).empty());
}

TEST(CartesianProductView, EmptyMiddleBaseYieldsNothing) {
    TArray<int> a{1, 2, 3};
    std::vector<char> no_chars;
    TSet<int> s{7, 8};
    int count = 0;
    for (auto&& t : views::cartesian_product(a, no_chars, s)) {
        (void)t;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(CartesianProductView, SeveralEmptyBasesYieldNothing) {
    TArray<int> no_ints;
    std::vector<char> no_chars;
    int count = 0;
    for (auto&& t : views::cartesian_product(no_ints, no_chars)) {
        (void)t;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

// --- degenerate arities ---------------------------------------------------------------

TEST(CartesianProductView, SingleRangeIsAViewOfOneTuples) {
    TArray<int> a{1, 2, 3};
    int sum = 0;
    int count = 0;
    for (auto&& t : views::cartesian_product(a)) {
        sum += std::get<0>(t);
        ++count;
    }
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(static_cast<int>(views::cartesian_product(a).size()), 3);
}

TEST(CartesianProductView, NoRangesIsTheSingleEmptyTuple) {
    // [range.cartesian.overview]: views::cartesian_product() is views::single(tuple()),
    // the identity of the operation: one element, not zero.
    int count = 0;
    for (auto&& t : views::cartesian_product()) {
        static_assert(std::tuple_size_v<std::remove_cvref_t<decltype(t)>> == 0);
        ++count;
    }
    EXPECT_EQ(count, 1);
}

// --- const iteration, sentinel end, mutation ------------------------------------------

TEST(CartesianProductView, ConstIterationYieldsTheSameSequence) {
    TArray<int> a{1, 2};
    std::vector<char> b{'x', 'y'};
    const auto v = views::cartesian_product(a, b);
    EXPECT_EQ(spell(v), "1x1y2x2y");
    EXPECT_EQ(static_cast<int>(v.size()), 4);
}

TEST(CartesianProductView, NonCommonFirstBaseEndsAtDefaultSentinel) {
    // A join first base: iteration covers every combination.
    std::vector<std::vector<int>> nested{{1, 2}, {3}};
    std::vector<char> b{'x', 'y'};
    EXPECT_EQ(spell(views::cartesian_product(views::join(nested), b)), "1x1y2x2y3x3y");
}

namespace {
// Non-sized sentinel: subrange(begin, end_at) is forward, not common, not sized.
struct end_at {
    std::vector<int>::const_iterator e;
    friend bool operator==(std::vector<int>::const_iterator it, end_at s) { return it == s.e; }
};
} // namespace

TEST(CartesianProductView, NonSizedTailBaseStillIteratesAndTerminates) {
    // A common first base (so end() is an iterator, not default_sentinel) paired with a
    // non-common, non-sized tail: end()'s emptiness check falls back to
    // `!(begin != end)` instead of `size() == 0`, and next() compares the tail's iterator
    // against a sentinel rather than an iterator.
    std::vector<int> first{1, 2};
    const std::vector<int> values{10, 20, 30};
    auto tail = subrange(values.begin(), end_at{values.end()});
    using tail_t = decltype(tail);
    static_assert(forward_range<tail_t> && !common_range<tail_t> && !sized_range<tail_t>);

    auto v = views::cartesian_product(first, tail);
    static_assert(common_range<decltype(v)> && !sized_range<decltype(v)>);

    std::string out;
    for (auto&& [a, b] : v) {
        out += std::to_string(a);
        out += ':';
        out += std::to_string(b);
        out += ' ';
    }
    EXPECT_EQ(out, "1:10 1:20 1:30 2:10 2:20 2:30 ");
}

TEST(CartesianProductView, EmptyNonSizedTailBaseYieldsNothing) {
    // The same path with an empty tail: the begin-or-first-end adjustment has to detect
    // the emptiness without a size().
    std::vector<int> first{1, 2};
    std::vector<std::vector<int>> all_empty{{}, {}};
    int count = 0;
    for (auto&& t : views::cartesian_product(first, views::join(all_empty))) {
        (void)t;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(CartesianProductView, MutatesThroughTheYieldedReferences) {
    TArray<int> a{1, 2};
    std::vector<int> b{10, 20};
    // Every element of either base is visited once per combination of the other base,
    // i.e. twice here, so each is incremented twice.
    for (auto&& [x, y] : views::cartesian_product(a, b)) {
        x += 1;
        y += 1;
    }
    EXPECT_EQ(a[0], 3); // 1 + 2
    EXPECT_EQ(a[1], 4); // 2 + 2
    EXPECT_EQ(b[0], 12); // 10 + 2
    EXPECT_EQ(b[1], 22); // 20 + 2
}

TEST(CartesianProductView, IterMoveYieldsElementWiseRvalueReferences) {
    std::vector<std::string> left{"a", "b"};
    std::vector<std::string> right{"c"};
    auto v = views::cartesian_product(left, right);
    auto it = zr::ranges::begin(v);
    // tuple<string&&, string&&>, not the value-decayed tuple a defaulted iter_move gives.
    static_assert(std::is_same_v<decltype(std::ranges::iter_move(it)), std::tuple<std::string&&, std::string&&>>);
    std::string taken = std::get<0>(std::ranges::iter_move(it));
    EXPECT_EQ(taken, "a"); // the element was moved out of left[0], not copied
}

TEST(CartesianProductView, IterSwapSwapsEveryComponent) {
    std::vector<std::string> left{"a", "b"};
    std::vector<std::string> right{"c", "d"};
    auto v = views::cartesian_product(left, right);

    // element 0 is (left[0], right[0]) and element 3 is (left[1], right[1]), so the two
    // positions share no component and every swap is a real one.
    std::ranges::iter_swap(zr::ranges::begin(v), advanced(zr::ranges::begin(v), 3));

    EXPECT_EQ(left[0], "b");
    EXPECT_EQ(left[1], "a");
    EXPECT_EQ(right[0], "d");
    EXPECT_EQ(right[1], "c");
}
