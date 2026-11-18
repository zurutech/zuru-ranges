// as_rvalue_view over std and Unreal bases, both build configurations.
#include "CoreMinimal.h"
#include "Containers/ChunkedArray.h"
#include "Containers/Deque.h"
#include "Containers/StridedView.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/as_input_view.h"
#include "zr/ranges/views/as_rvalue_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/take_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <concepts>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

// Move-only: a pipeline over it compiles only if every element is moved, and a moved-from
// element is null.
using ptr = std::unique_ptr<int>;
using vec = std::vector<ptr>;
using ints = std::vector<int>;

vec pointers(int n) {
    vec out;
    for (int i = 1; i <= n; ++i) {
        out.push_back(std::make_unique<int>(i));
    }
    return out;
}

TArray<ptr> pointer_array(int n) {
    TArray<ptr> out;
    for (int i = 1; i <= n; ++i) {
        out.Add(std::make_unique<int>(i));
    }
    return out;
}

// The pointees, 0 for a moved-from element.
template <class R> ints pointees(const R& r) {
    ints out;
    for (const ptr& p : r) {
        out.push_back(p ? *p : 0);
    }
    return out;
}

using vec_rvalue = decltype(std::declval<vec&>() | views::as_rvalue);
using owning_rvalue = decltype(std::declval<vec>() | views::as_rvalue);
using input_rvalue = decltype(std::declval<vec&>() | views::as_input | views::as_rvalue);
using arr_rvalue = decltype(std::declval<TArray<ptr>&>() | views::as_rvalue);
using deque_rvalue = decltype(std::declval<TDeque<ptr>&>() | views::as_rvalue);
using set_rvalue = decltype(std::declval<TSet<int>&>() | views::as_rvalue);

// [range.as.rvalue.overview]: the elements are rvalues; views::all(E) when they already are.
static_assert(std::same_as<vec_rvalue, as_rvalue_view<ref_view<vec>>>);
static_assert(std::same_as<range_reference_t<vec_rvalue>, ptr&&> && std::same_as<range_reference_t<arr_rvalue>, ptr&&>);
static_assert(std::same_as<range_reference_t<const owning_rvalue>, const ptr&&>);
static_assert(std::same_as<decltype(views::iota(0, 3) | views::as_rvalue), iota_view<int, int>>);
static_assert(std::same_as<decltype(std::declval<vec_rvalue>() | views::as_rvalue), vec_rvalue>);

// [range.as.rvalue.view]: move_iterator over the base's iterator, move_sentinel over its
// sentinel when the base is not common; sized and borrowed as the base is.
static_assert(std::same_as<iterator_t<vec_rvalue>, std::move_iterator<vec::iterator>>);
static_assert(std::same_as<sentinel_t<input_rvalue>, std::move_sentinel<vec::iterator>>);
static_assert(common_range<vec_rvalue> && common_range<arr_rvalue> && !common_range<input_rvalue>);
static_assert(sized_range<vec_rvalue> && sized_range<arr_rvalue> && sized_range<set_rvalue>);
static_assert(borrowed_range<vec_rvalue> && !borrowed_range<owning_rvalue>);
static_assert(std::default_initializable<owning_rvalue> && !std::default_initializable<vec_rvalue>);
static_assert(std::same_as<decltype(as_rvalue_view(std::declval<vec&>())), vec_rvalue>);
static_assert(std::convertible_to<iterator_t<owning_rvalue>, iterator_t<const owning_rvalue>>);

// range_traits: the base's category capped at random access, whatever iterator_t<TArray> is.
static_assert(random_access_range<vec_rvalue> && !contiguous_range<vec_rvalue>);
static_assert(random_access_range<arr_rvalue> && !contiguous_range<arr_rvalue>);
static_assert(random_access_range<deque_rvalue>);
static_assert(forward_range<set_rvalue> && !bidirectional_range<set_rvalue>);
static_assert(input_range<input_rvalue> && !forward_range<input_rvalue>);

// A view over it indexes it by position: random access and common in both configurations.
using arr_taken = decltype(std::declval<arr_rvalue>() | views::take(2));
static_assert(random_access_range<arr_taken> && common_range<arr_taken>);
static_assert(std::same_as<range_reference_t<arr_taken>, ptr&&>);

template <class T>
concept can_as_rvalue = requires { views::as_rvalue(std::declval<T>()); };
static_assert(can_as_rvalue<vec&> && !can_as_rvalue<int>);

} // namespace

TEST(AsRvalueView, MovesTheElements) {
    vec v = pointers(3);
    auto rv = v | views::as_rvalue;
    EXPECT_EQ(rv.size(), 3U);
    EXPECT_EQ(std::as_const(rv).size(), 3U);

    vec out = rv | to<vec>();
    EXPECT_EQ(pointees(out), (ints{1, 2, 3}));
    EXPECT_EQ(pointees(v), (ints{0, 0, 0}));
}

TEST(AsRvalueView, Base) {
    vec v = pointers(2);
    auto rv = v | views::as_rvalue;
    EXPECT_EQ(&rv.base().base(), &v);
    EXPECT_EQ(pointees((pointers(2) | views::as_rvalue).base().base()), (ints{1, 2})); // owning_view: base() && only
}

TEST(AsRvalueView, ReserveHintOfUnsizedBase) {
    // ranges::reserve_hint is ranges::size for a sized range: only an unsized base reaches the
    // members.
    auto rv = as_rvalue_view(views::iota(0) | views::take(3));
    static_assert(approximately_sized_range<decltype(rv)> && !sized_range<decltype(rv)>);
    EXPECT_EQ(zr::ranges::reserve_hint(rv), 3U);
    EXPECT_EQ(zr::ranges::reserve_hint(std::as_const(rv)), 3U);
}

TEST(AsRvalueView, UnrealBases) {
    TArray<ptr> a = pointer_array(3);
    EXPECT_EQ(pointees(a | views::as_rvalue | to<vec>()), (ints{1, 2, 3}));
    EXPECT_EQ(pointees(a), (ints{0, 0, 0}));

    // Into an Unreal container: Add(ElementType&&).
    vec v = pointers(2);
    EXPECT_EQ(pointees(v | views::as_rvalue | to<TArray>()), (ints{1, 2}));
    EXPECT_EQ(pointees(v), (ints{0, 0}));

    // TMap's iterator is forward, with != only.
    TMap<int, ptr> m;
    m.Add(1, std::make_unique<int>(10));
    auto pairs = m | views::as_rvalue | to<std::vector>();
    static_assert(std::same_as<decltype(pairs), std::vector<TPair<int, ptr>>>);
    ASSERT_EQ(pairs.size(), 1U);
    EXPECT_EQ(*pairs[0].Value, 10);
    EXPECT_EQ((*m.begin()).Value, nullptr);

    // operator* of TStridedView's iterator is non-const.
    int raw[4]{1, 0, 2, 0};
    TStridedView<int> strided(static_cast<int32>(2 * sizeof(int)), raw, 2);
    EXPECT_EQ(strided | views::as_rvalue | to<ints>(), (ints{1, 2}));
}

TEST(AsRvalueView, NonCommonUnrealBase) {
    // The sentinel is TArray's iterator: not semiregular in the debug configuration before UE 5.8.
    TArray<ptr> a = pointer_array(2);
    auto rv = a | views::as_input | views::as_rvalue;
    auto it = rv.begin();
    EXPECT_FALSE(it == rv.end());
    ++it;
    ++it;
    EXPECT_TRUE(it == rv.end());
    EXPECT_EQ(pointees(rv | to<vec>()), (ints{1, 2}));
}

TEST(AsRvalueView, AsymmetricInequalityOfUnrealSentinel) {
    // TChunkedArray's `!=` is `Count < Rhs.Count` (see unreal_quirks.cpp).
    TChunkedArray<int> arr;
    arr.AddElement(1);
    auto rv = arr | views::as_rvalue;
    auto it = rv.begin();
    EXPECT_FALSE(it == rv.end());
    EXPECT_FALSE(rv.end() == it);
    ++it;
    EXPECT_TRUE(it == rv.end());
    EXPECT_TRUE(rv.end() == it);
}

TEST(AsRvalueView, RandomAccessByPosition) {
    // TDeque's iterator only increments: the element is reached through element_at, as an rvalue.
    TDeque<ptr> d;
    for (int i = 1; i <= 3; ++i) {
        d.EmplaceLast(std::make_unique<int>(i));
    }
    auto rv = d | views::as_rvalue;
    static_assert(std::same_as<decltype(rv[1]), ptr&&>);
    ptr second = rv[1];
    EXPECT_EQ(*second, 2);
    EXPECT_EQ(d[1], nullptr);
    EXPECT_EQ(pointees(rv | views::reverse | to<vec>()), (ints{3, 0, 1}));
}

TEST(AsRvalueView, TupleElementsByPosition) {
    // zip's iter_move maps the tuple's elements, and so does element_at where zip's iterator is
    // not a std::random_access_iterator (C++20, or over TArray's debug iterator).
    vec keys = pointers(2);
    TArray<ptr> values = pointer_array(2);
    auto rv = views::zip(keys, values) | views::as_rvalue;
    static_assert(random_access_range<decltype(rv)>);
    static_assert(std::same_as<range_reference_t<decltype(rv)>, std::tuple<ptr&&, ptr&&>>);
    static_assert(std::same_as<decltype(rv[1]), std::tuple<ptr&&, ptr&&>>);

    std::tuple<ptr, ptr> second = rv[1];
    EXPECT_EQ(*std::get<0>(second) + *std::get<1>(second), 4);
    EXPECT_EQ(pointees(keys), (ints{1, 0}));
    EXPECT_EQ(pointees(values), (ints{1, 0}));
}
