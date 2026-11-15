// as_input_view over std and Unreal bases, both build configurations.
#include "CoreMinimal.h"
#include "Containers/ChunkedArray.h"
#include "Containers/StridedView.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/as_input_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/take_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <concepts>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

template <class R> std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

using vec = std::vector<int>;
using vec_input = decltype(std::declval<vec&>() | views::as_input);
using owning_input = decltype(std::declval<vec>() | views::as_input);
using arr_input = decltype(std::declval<TArray<int>&>() | views::as_input);
using istream_range = subrange<std::istream_iterator<int>, std::default_sentinel_t>;

// [range.as.input.overview]: input only and non-common whatever the base; sized and borrowed
// as the base is.
static_assert(std::same_as<vec_input, as_input_view<ref_view<vec>>>);
static_assert(input_range<vec_input> && !forward_range<vec_input> && !common_range<vec_input>);
static_assert(input_range<const vec_input> && !forward_range<const vec_input>);
static_assert(input_range<arr_input> && !forward_range<arr_input> && !common_range<arr_input>);
static_assert(sized_range<vec_input> && sized_range<arr_input>);
static_assert(borrowed_range<vec_input> && !borrowed_range<owning_input>);

// views::all(E) for an input range that is neither common nor forward already.
static_assert(std::same_as<decltype(std::declval<istream_range>() | views::as_input), istream_range>);
static_assert(std::same_as<decltype(std::declval<vec_input>() | views::as_input), vec_input>);

// [range.as.input.view]: default-constructible as V is; as_input_view(R&&) deduces
// views::all_t<R>; base() const& requires copy_constructible<V>.
static_assert(std::default_initializable<owning_input> && !std::default_initializable<vec_input>);
static_assert(std::same_as<decltype(as_input_view(std::declval<vec&>())), vec_input>);
template <class T>
concept has_base = requires(T&& t) { std::forward<T>(t).base(); };
static_assert(has_base<vec_input&> && has_base<owning_input> && !has_base<owning_input&>);

// [range.as.input.iterator]: move-only; iterator<false> converts to iterator<true> only;
// postfix ++ returns void.
static_assert(std::movable<iterator_t<vec_input>> && !std::copyable<iterator_t<vec_input>>);
static_assert(std::default_initializable<iterator_t<vec_input>>);
static_assert(std::is_void_v<decltype(std::declval<iterator_t<vec_input>&>()++)>);
static_assert(std::convertible_to<iterator_t<owning_input>, iterator_t<const owning_input>>);
static_assert(!std::convertible_to<iterator_t<const owning_input>, iterator_t<owning_input>>);

template <class T>
concept can_as_input = requires { views::as_input(std::declval<T>()); };
static_assert(can_as_input<vec&> && !can_as_input<int>);

} // namespace

TEST(AsInputView, YieldsBaseElements) {
    vec v{1, 2, 3};
    auto in = v | views::as_input;
    EXPECT_EQ(collect(in), v);
    EXPECT_EQ(in.size(), 3U);
    EXPECT_EQ(std::as_const(in).size(), 3U);

    for (int& x : in) {
        x *= 10;
    }
    EXPECT_EQ(v, (vec{10, 20, 30}));
}

TEST(AsInputView, Base) {
    vec v{1, 2};
    auto in = v | views::as_input;
    EXPECT_EQ(&in.base().base(), &v);
    EXPECT_EQ((vec{1, 2} | views::as_input).base().base(), v); // owning_view: base() && only
}

TEST(AsInputView, ReserveHintOfUnsizedBase) {
    // ranges::reserve_hint is ranges::size for a sized range: only an unsized base reaches the
    // members.
    auto in = views::iota(0) | views::take(3) | views::as_input;
    static_assert(approximately_sized_range<decltype(in)> && !sized_range<decltype(in)>);
    EXPECT_EQ(zr::ranges::reserve_hint(in), 3U);
    EXPECT_EQ(zr::ranges::reserve_hint(std::as_const(in)), 3U);
}

TEST(AsInputView, InputOnlyBasePassesThrough) {
    std::istringstream in("1 2 3");
    EXPECT_EQ(collect(istream_range(std::istream_iterator<int>(in), std::default_sentinel) | views::as_input), (vec{1, 2, 3}));
}

TEST(AsInputView, ConstIteratorConvertsFromMutable) {
    auto in = vec{5, 6} | views::as_input;
    iterator_t<const owning_input> cit = in.begin();
    EXPECT_EQ(*cit, 5);
    EXPECT_FALSE(cit == std::as_const(in).end());
}

TEST(AsInputView, SizedSentinel) {
    vec v{1, 2, 3};
    auto in = v | views::as_input;
    static_assert(std::sized_sentinel_for<sentinel_t<vec_input>, iterator_t<vec_input>>);
    auto it = in.begin();
    it++;
    EXPECT_EQ(in.end() - it, 2);
    EXPECT_EQ(it - in.end(), -2);
    EXPECT_EQ(it.base(), v.begin() + 1);
    EXPECT_EQ(std::move(it).base(), v.begin() + 1);
}

TEST(AsInputView, IterMoveAndIterSwap) {
    std::vector<std::string> names{"a", "b"};
    auto in = names | views::as_input;
    auto first = in.begin();
    auto second = in.begin();
    ++second;
    std::ranges::iter_swap(first, second);
    EXPECT_EQ(names, (std::vector<std::string>{"b", "a"}));

    static_assert(std::same_as<decltype(std::ranges::iter_move(first)), std::string&&>);
    std::string taken = std::ranges::iter_move(first);
    EXPECT_EQ(taken, "b");
}

TEST(AsInputView, UnrealBases) {
    TArray<int> a{1, 2, 3};
    auto in = a | views::as_input;
    EXPECT_EQ(collect(in), (vec{1, 2, 3}));
    EXPECT_EQ(in.size(), 3);

    TSet<int> s{4};
    EXPECT_EQ(collect(s | views::as_input), (vec{4}));

    // operator* of TStridedView's iterator is non-const.
    int raw[4]{1, 0, 2, 0};
    TStridedView<int> strided(static_cast<int32>(2 * sizeof(int)), raw, 2);
    EXPECT_EQ(collect(strided | views::as_input), (vec{1, 2}));
}

TEST(AsInputView, AsymmetricInequalityOfUnrealSentinel) {
    // TChunkedArray's `!=` is `Count < Rhs.Count` (see unreal_quirks.cpp).
    TChunkedArray<int> arr;
    arr.AddElement(1);
    auto in = arr | views::as_input;
    auto it = in.begin();
    EXPECT_FALSE(it == in.end());
    EXPECT_FALSE(in.end() == it);
    ++it;
    EXPECT_TRUE(it == in.end());
    EXPECT_TRUE(in.end() == it);
}
