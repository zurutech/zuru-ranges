// ranges::fold_left / fold_left_first / fold_right / fold_right_last / fold_left_with_iter /
// fold_left_first_with_iter over std and Unreal containers, both build configurations.
#include "CoreMinimal.h"
#include "Containers/ChunkedArray.h"
#include "Containers/Deque.h"
#include "Containers/List.h"

#include "zr/algorithm.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/take_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

constexpr int add(int a, int b) { return a + b; }

// Not commutative: the order of the fold shows in the result.
std::string left_concat(std::string acc, int x) { return std::move(acc) + std::to_string(x); }
std::string right_concat(int x, std::string acc) { return std::to_string(x) + std::move(acc); }

using plus_t = std::plus<>;

// [alg.fold]: U is decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>; the range overloads of
// the _with_iter forms return borrowed_iterator_t<R> (dangling for a non-borrowed rvalue).
static_assert(std::is_same_v<decltype(fold_left(std::declval<TArray<int>&>(), 0.5, plus_t{})), double>);
static_assert(std::is_same_v<decltype(fold_left(std::declval<TArray<int>&>(), 0, plus_t{})), int>);
static_assert(std::is_same_v<decltype(fold_left_with_iter(TArray<int>{}, 0, plus_t{})), fold_left_with_iter_result<dangling, int>>);
static_assert(std::is_same_v<decltype(fold_left_with_iter(std::declval<TArray<int>&>(), 0, plus_t{})),
                             fold_left_with_iter_result<iterator_t<TArray<int>&>, int>>);
static_assert(std::is_same_v<decltype(fold_left_first(std::declval<TArray<int>&>(), plus_t{})), std::optional<int>>);
static_assert(std::is_same_v<decltype(fold_right_last(std::declval<TArray<int>&>(), plus_t{})), std::optional<int>>);
static_assert(std::is_same_v<decltype(fold_left_first_with_iter(TSet<int>{}, plus_t{})),
                             fold_left_first_with_iter_result<dangling, std::optional<int>>>);

// The right folds need a decrementing iterator or random access by position: not a forward TSet.
template <class R>
concept right_foldable = requires(R& r) {
    fold_right(r, 0, plus_t{});
    fold_right_last(r, plus_t{});
};
static_assert(right_foldable<TArray<int>> && right_foldable<TDoubleLinkedList<int>> && right_foldable<TDeque<int>>);
static_assert(right_foldable<TChunkedArray<int>> && right_foldable<std::list<int>>);
#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
static_assert(right_foldable<TBitArray<>>);
#endif
static_assert(!right_foldable<TSet<int>> && !right_foldable<TMap<int, int>> && !right_foldable<std::forward_list<int>>);

// [alg.fold]: the iterator overloads take a bidirectional_iterator; a std forward iterator with a
// decrement is not one. TDeque's iterator declares `--` without a Decrement in its base.
struct forward_with_decrement {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::forward_iterator_tag;
    int* p = nullptr;
    int& operator*() const { return *p; }
    forward_with_decrement& operator++() {
        ++p;
        return *this;
    }
    forward_with_decrement operator++(int) {
        auto copy = *this;
        ++p;
        return copy;
    }
    forward_with_decrement& operator--() {
        --p;
        return *this;
    }
    bool operator==(const forward_with_decrement&) const = default;
};
static_assert(std::forward_iterator<forward_with_decrement> && !std::bidirectional_iterator<forward_with_decrement>);
template <class I>
concept right_foldable_iterator = requires(I i) {
    fold_right(i, i, 0, plus_t{});
    fold_right_last(i, i, plus_t{});
};
static_assert(!right_foldable_iterator<forward_with_decrement> && right_foldable_iterator<std::list<int>::iterator>);
static_assert(!right_foldable_iterator<iterator_t<TDeque<int>>> && right_foldable_iterator<iterator_t<TDoubleLinkedList<int>>>);

#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
// TBitArray's value type is bool: an accumulator returning the proxy does not fold (as vector<bool>).
template <class R, class F>
concept left_first_foldable = requires(R& r, F f) { fold_left_first(r, f); };
struct keep_last {
    template <class T, class U> U operator()(T&&, U x) const { return x; }
};
static_assert(!left_first_foldable<TBitArray<>, keep_last> && !left_first_foldable<std::vector<bool>, keep_last>);
static_assert(std::is_same_v<decltype(fold_left_first(std::declval<TBitArray<>&>(), std::logical_or<>{})), std::optional<bool>>);
#endif

} // namespace

TEST(Fold, FoldLeft) {
    TArray<int> a{1, 2, 3};
    EXPECT_EQ(fold_left(a, 10, add), 16);
    EXPECT_EQ(fold_left(a.begin(), a.end(), 0, plus_t{}), 6);
    EXPECT_DOUBLE_EQ(fold_left(a, 0.5, plus_t{}), 6.5);
    EXPECT_EQ(fold_left(a, std::string{}, left_concat), "123");
    TSet<int> s{4, 5};
    EXPECT_EQ(fold_left(s, 0, add), 9);
    EXPECT_EQ(fold_left(TArray<int>{}, 7, add), 7);
    EXPECT_EQ(fold_left(views::iota(1) | views::take(4), 0, add), 10); // counted iterator and default_sentinel
    EXPECT_EQ(fold_left(a | views::filter([](int x) { return x != 2; }), 0, add), 4);
    std::vector<int> v{1, 2, 3, 4};
    EXPECT_EQ(fold_left(v, 1, std::multiplies<>{}), 24);
}

TEST(Fold, FoldLeftFirst) {
    TArray<int> a{1, 2, 3};
    EXPECT_EQ(fold_left_first(a, add), std::optional<int>(6));
    EXPECT_EQ(fold_left_first(a.begin(), a.end(), std::multiplies<>{}), std::optional<int>(6));
    EXPECT_EQ(fold_left_first(TArray<int>{}, add), std::nullopt);
    TMap<int, int> m{{1, 10}, {2, 20}};
    auto sum_values = [](TPair<int, int> acc, const TPair<int, int>& p) {
        acc.Value += p.Value;
        return acc;
    };
    EXPECT_EQ(fold_left_first(m, sum_values)->Value, 30);
}

TEST(Fold, FoldRight) {
    TArray<int> a{1, 2, 3};
    EXPECT_EQ(fold_right(a, std::string{}, right_concat), "123");
    EXPECT_EQ(fold_right(a.begin(), a.end(), 10, add), 16);
    EXPECT_EQ(fold_right(TArray<int>{}, 7, add), 7);
    TDoubleLinkedList<int> dl;
    for (int x : {1, 2, 3}) {
        dl.AddTail(x);
    }
    EXPECT_EQ(fold_right(dl, std::string{}, right_concat), "123");
    TDeque<int> dq{1, 2, 3};
    EXPECT_EQ(fold_right(dq, std::string{}, right_concat), "123");
    std::list<int> l{1, 2, 3};
    EXPECT_EQ(fold_right(l, std::string{}, right_concat), "123");

    // By position: these iterators do not decrement.
    TChunkedArray<int> c;
    for (int x : {1, 2, 3}) {
        c.AddElement(x);
    }
    EXPECT_EQ(fold_right(c, std::string{}, right_concat), "123");
    EXPECT_EQ(fold_right(TChunkedArray<int>{}, 7, add), 7);
#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
    TBitArray<> bits(false, 3);
    zr::ranges::element_at(bits, 2) = true;
    EXPECT_EQ(fold_right(bits, 0, [](bool b, int acc) { return acc * 2 + (b ? 1 : 0); }), 4); // bits 2, 1, 0: 1, 0, 0
    EXPECT_EQ(fold_left_first(bits, [](bool, bool b) { return b; }), std::optional<bool>(true));
    EXPECT_FALSE(zr::ranges::element_at(bits, 0)); // the accumulator is a bool, not a proxy to bit 0
#endif
}

TEST(Fold, FoldRightLast) {
    TArray<int> a{1, 2, 3};
    EXPECT_EQ(fold_right_last(a, std::minus<>{}), std::optional<int>(1 - (2 - 3)));
    EXPECT_EQ(fold_right_last(a.begin(), a.end(), add), std::optional<int>(6));
    EXPECT_EQ(fold_right_last(TArray<int>{}, add), std::nullopt);
    TChunkedArray<int> c;
    for (int x : {1, 2, 3}) {
        c.AddElement(x);
    }
    EXPECT_EQ(fold_right_last(c, std::minus<>{}), std::optional<int>(1 - (2 - 3)));
    EXPECT_EQ(fold_right_last(TChunkedArray<int>{}, add), std::nullopt);
    TDoubleLinkedList<int> dl;
    dl.AddTail(5);
    EXPECT_EQ(fold_right_last(dl, add), std::optional<int>(5));
}

TEST(Fold, FoldLeftWithIter) {
    TArray<int> a{1, 2, 3};
    auto [it, value] = fold_left_with_iter(a, 0, add);
    EXPECT_FALSE(it != a.end()); // the debug iterator has `!=` only
    EXPECT_EQ(value, 6);
    auto r = fold_left_with_iter(a.begin(), a.end(), std::string{}, left_concat);
    EXPECT_EQ(r.value, "123");
    EXPECT_FALSE(r.in != a.end());
    auto no_elements = fold_left_with_iter(TArray<int>{}, 3, add);
    EXPECT_EQ(no_elements.value, 3);
    // A sentinel end: the result's iterator is the counted iterator at the end.
    auto t = views::iota(1) | views::take(3);
    auto rt = fold_left_with_iter(t, 0, add);
    EXPECT_EQ(rt.value, 6);
    EXPECT_TRUE(rt.in == t.end());
    // [algorithms.results]: in_value_result converts to a compatible result.
    fold_left_with_iter_result<dangling, long> converted = fold_left_with_iter(a.begin(), a.end(), 0, add);
    EXPECT_EQ(converted.value, 6);
}

TEST(Fold, FoldLeftFirstWithIter) {
    TArray<int> a{1, 2, 3};
    auto r = fold_left_first_with_iter(a, add);
    EXPECT_FALSE(r.in != a.end());
    EXPECT_EQ(r.value, std::optional<int>(6));
    TArray<int> none;
    auto e = fold_left_first_with_iter(none, add);
    EXPECT_FALSE(e.in != none.end());
    EXPECT_EQ(e.value, std::nullopt);
    TSet<int> s{2, 3};
    EXPECT_EQ(fold_left_first_with_iter(s.begin(), s.end(), std::multiplies<>{}).value, std::optional<int>(6));
}
