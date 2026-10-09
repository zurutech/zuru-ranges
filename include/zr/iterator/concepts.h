// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

/**
 * @file concepts.h
 *
 * @brief [iterator.concepts] and [iterator.traits], relaxed where Unreal iterators require it.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [iterator.synopsis] exposition-only with-reference / can-reference.
template <class T> using with_reference = T&;

template <class T>
concept can_reference = requires { typename with_reference<T>; };

template <typename T>
concept noexcept_movable = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

// [range.utility.helpers] different-from (also used by [const.iterators])
template <class T, class U>
concept different_from = !std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

// [iterator.concept.winc] is-integer-like: no integer-class types, so the integral types
// other than bool.
template <class T>
concept is_integer_like = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

template <class T>
concept is_signed_integer_like = is_integer_like<T> && std::signed_integral<T>;

// [ranges.syn] to-unsigned-like, make-unsigned-like-t, make-signed-like-t.
template <class T> using make_unsigned_like_t = std::make_unsigned_t<T>;
template <class T> using make_signed_like_t = std::make_signed_t<T>;
template <is_integer_like T> constexpr auto to_unsigned_like(T value) noexcept { return static_cast<make_unsigned_like_t<T>>(value); }

// [concept.booleantestable]
template <class T>
concept boolean_testable_impl = std::convertible_to<T, bool>;

template <class T>
concept boolean_testable = boolean_testable_impl<T> && requires(T&& t) {
    { !std::forward<T>(t) } -> boolean_testable_impl;
};

} // namespace detail

// [iterator.synopsis] exposition-only dereferenceable
template <class T>
concept dereferenceable = requires(T& t) {
    { *t } -> detail::can_reference;
};

namespace detail {

// `*i` of a const iterator.
// DEVIATION: through a copy when *i is invalid on a const iterator (TStridedView's operator*
// is non-const); the reference designates the element, not the copy.
// Named concepts throughout, not requires-expressions in `if constexpr`: MSVC < 19.44 evaluates
// those to false.
template <class I>
concept const_dereferenceable = requires(const I& it) { *it; };

template <class I> constexpr bool nothrow_deref() {
    if constexpr (const_dereferenceable<I>) {
        return noexcept(*std::declval<const I&>());
    } else {
        return std::is_nothrow_copy_constructible_v<I> && noexcept(*std::declval<I&>());
    }
}

template <dereferenceable I> constexpr decltype(auto) deref(const I& it) noexcept(nothrow_deref<I>()) {
    if constexpr (const_dereferenceable<I>) {
        return *it;
    } else {
        I copy(it);
        return *copy;
    }
}

} // namespace detail

// [iterator.concept.winc]
// DEVIATION: `++t` need not yield T& (TSparseArray's yields its base, TChunkedArray's void).
template <class T>
concept prefix_incrementable = requires(T t) { ++t; };

template <class T>
concept postfix_incrementable = requires(T t) {
    { t++ } -> std::same_as<T>;
};

namespace detail {

// Non-standard: true for an iterator whose `--` is declared but ill-formed once instantiated, so
// that no expression check can see it (UE's TIteratorAdapter over a base without Decrement:
// TDeque's iterator). Specialized in unreal/ue_range_traits.h.
template <class I> inline constexpr bool decrement_disabled = false;

} // namespace detail

template <class T>
concept prefix_decrementable = !detail::decrement_disabled<std::remove_cv_t<T>> && requires(T t) {
    { --t } -> std::same_as<T&>;
};

template <class T>
concept postfix_decrementable = !detail::decrement_disabled<std::remove_cv_t<T>> && requires(T t) {
    { t-- } -> std::same_as<T>;
};

// [iterator.concept.iterator]
// DEVIATION: dereferenceable and prefix_incrementable only; no weakly_incrementable (Unreal
// iterators are not assignable and have no difference_type).
template <class T>
concept input_or_output_iterator = dereferenceable<T> && prefix_incrementable<T>;

template <class T> using const_ref = std::remove_reference_t<T>&;

// DEVIATION: replaces weakly-equality-comparable-with ([concept.equalitycomparable]) in
// sentinel_for: TCheckedPointerIterator (TArray, TArrayView, FString, TSortedMap with
// TARRAY_RANGED_FOR_CHECKS) has only `!=` before UE 5.7.
template <class Sentinel, class Iter>
concept weakly_inequality_testable_with = requires(const_ref<Sentinel> s, const_ref<Iter> i) {
    { s != i } -> std::convertible_to<bool>;
    { i != s } -> std::convertible_to<bool>;
};

template<class T>
concept inequality_comparable = weakly_inequality_testable_with<T, T>;

namespace detail {

// [concept.equalitycomparable] weakly-equality-comparable-with, relaxed: `==` or `!=`.
template <class T, class U>
concept relaxed_equality_comparable_with = requires(const_ref<T> t, const_ref<U> u) {
    { t == u } -> boolean_testable;
} || weakly_inequality_testable_with<T, U>;

template <class T>
concept relaxed_equality_comparable = relaxed_equality_comparable_with<T, T>;

// `bool(t == u)`.
// DEVIATION: `!(t != u) && !(u != t)` when `t == u` is ill-formed (Unreal iterators through
// UE 5.6, see weakly_inequality_testable_with). Both orders: [concept.equalitycomparable]
// requires `!=` to be symmetric, and TChunkedArray's is `Count < Rhs.Count` (only ranged-for's
// `it != end` is meant to compile; `end != it` is always false), so either order alone answers
// `end == it` wrongly. Every view's iterator/sentinel `==` routes through here.
template <class T, class U>
    requires requires(const T& t, const U& u) { bool(t == u); }
constexpr bool iter_equal(const T& t, const U& u) noexcept(noexcept(bool(t == u))) {
    return bool(t == u);
}

template <class T, class U>
    requires weakly_inequality_testable_with<T, U> && (!requires(const T& t, const U& u) { bool(t == u); })
constexpr bool iter_equal(const T& t, const U& u) noexcept(noexcept(bool(t != u)) && noexcept(bool(u != t))) {
    return !bool(t != u) && !bool(u != t);
}

} // namespace detail

// [iterator.concept.sentinel]
// DEVIATION: no semiregular<S>, `!=` for `==` (see above). Uniform for std and Unreal
// iterators, so zip/concat over both compose.
template <class Sentinel, class Iter>
concept sentinel_for = input_or_output_iterator<Iter> && weakly_inequality_testable_with<Sentinel, Iter>;

// std::sentinel_for; for static_assert diagnostics only, never a constraint.
template <class Sentinel, class Iter>
concept strict_sentinel_for = std::sentinel_for<Sentinel, Iter>;

// [iterator.traits] iter_reference_t.
template <dereferenceable T> using iter_reference_t = decltype(*std::declval<T&>());

namespace detail {

// Non-standard: the value type of a proxy reference type that is its own value type otherwise
// (UE's FBitReference and FConstBitReference: bool, as vector<bool>'s). Specialized in
// unreal/ue_range_traits.h.
template <class Reference> struct proxy_value {
    using type = Reference;
};

template <class T> struct iter_value {
    using type = typename proxy_value<std::remove_cvref_t<iter_reference_t<T>>>::type;
};

template <class T>
    requires requires { typename std::iter_value_t<T>; }
struct iter_value<T> {
    using type = std::iter_value_t<T>;
};

} // namespace detail

// [readable.traits] iter_value_t.
// DEVIATION: remove_cvref_t<iter_reference_t<T>>, or its proxy_value, when std::iter_value_t<T>
// is ill-formed (Unreal iterators declare no value_type).
template <class T> using iter_value_t = typename detail::iter_value<std::remove_cvref_t<T>>::type;

namespace detail {

// DEVIATION: std::ptrdiff_t when std::iter_difference_t<T> is ill-formed (Unreal iterators
// have no difference_type). Not the container's SizeType: TArray's Shipping iterator is a T*,
// whose difference type is ptrdiff_t, so the type is the same in both configurations.
template <class T> struct iter_difference {
    using type = std::ptrdiff_t;
};

template <class T>
    requires requires { typename std::iter_difference_t<T>; }
struct iter_difference<T> {
    using type = std::iter_difference_t<T>;
};

} // namespace detail

// [incrementable.traits] iter_difference_t.
template <class T> using iter_difference_t = typename detail::iter_difference<T>::type;

// [iterator.synopsis] iter_rvalue_reference_t. ranges::iter_move needs no indirectly_readable.
template <dereferenceable T> using iter_rvalue_reference_t = decltype(std::ranges::iter_move(std::declval<T&>()));

namespace detail {

// ranges::iter_move(it) of a const iterator. Not named iter_move: see relaxed_iter_swap.
// DEVIATION: through a copy when *it is invalid on a const iterator (TStridedView, see deref).
template <class I>
concept const_iter_movable = requires(const I& it) { std::ranges::iter_move(it); };

template <class I> constexpr bool nothrow_relaxed_iter_move() {
    if constexpr (const_iter_movable<I>) {
        return noexcept(std::ranges::iter_move(std::declval<const I&>()));
    } else {
        return std::is_nothrow_copy_constructible_v<I> && noexcept(std::ranges::iter_move(std::declval<I&>()));
    }
}

template <dereferenceable I> constexpr decltype(auto) relaxed_iter_move(const I& it) noexcept(nothrow_relaxed_iter_move<I>()) {
    if constexpr (const_iter_movable<I>) {
        return std::ranges::iter_move(it);
    } else {
        I copy(it);
        return std::ranges::iter_move(copy);
    }
}

} // namespace detail

namespace detail {

template <class T> inline constexpr bool is_tuple_or_pair = false;
template <class... Ts> inline constexpr bool is_tuple_or_pair<std::tuple<Ts...>> = true;
template <class A, class B> inline constexpr bool is_tuple_or_pair<std::pair<A, B>> = true;

// C++20 std::tuple and std::pair have no basic_common_reference (P2321R2, C++23).
#if defined(__cpp_lib_ranges_zip)
template <class A, class B> inline constexpr bool cxx20_tuple_pair = false;
#else
template <class A, class B>
inline constexpr bool cxx20_tuple_pair = is_tuple_or_pair<std::remove_cvref_t<A>> && is_tuple_or_pair<std::remove_cvref_t<B>>;
#endif

// [concept.commonref] common_reference_with.
// DEVIATION (toolchain): true for two tuples or pairs in C++20, where std::tuple lacks the
// basic_common_reference that makes the views' tuple-of-references proxies readable.
template <class A, class B>
concept relaxed_common_reference_with = std::common_reference_with<A, B> || cxx20_tuple_pair<A, B>;

template <class I>
concept declares_input_iterator_concept = requires {
    typename I::iterator_concept;
    requires std::derived_from<typename I::iterator_concept, std::input_iterator_tag>;
};

// [iterator.concept.input] input_iterator, relaxed.
// DEVIATION: also an iterator that is not a std::input_or_output_iterator (Unreal), and, in
// C++20, a std::input_or_output_iterator with an input iterator_concept, a value type and a
// tuple or pair reference (a view's proxy; see relaxed_common_reference_with).
template <class I>
concept relaxed_input_iterator =
    input_or_output_iterator<I> &&
    (std::input_iterator<I> || !std::input_or_output_iterator<I> ||
     (declares_input_iterator_concept<I> && requires { typename std::iter_value_t<I>; } &&
      cxx20_tuple_pair<iter_reference_t<I>, std::iter_value_t<I>>));

} // namespace detail

// [iterator.synopsis] iter_common_reference_t.
template <dereferenceable T> using iter_common_reference_t = std::common_reference_t<iter_reference_t<T>, iter_value_t<T>&>;

namespace detail {

// [alg.req.ind.swap] indirectly_swappable.
// DEVIATION: or ranges::swap(*a, *b) valid: indirectly_swappable requires indirectly_readable,
// which Unreal iterators do not model.
template <class I1, class I2 = I1>
concept swappable_iters =
    std::indirectly_swappable<I1, I2> || requires(const I1& a, const I2& b) { std::ranges::swap(*a, *b); };

// noexcept of the branch relaxed_iter_swap takes.
template <class I1, class I2> constexpr bool nothrow_swappable_iters() {
    if constexpr (std::indirectly_swappable<I1, I2>) {
        return std::is_nothrow_invocable_v<decltype(std::ranges::iter_swap), const I1&, const I2&>;
    } else {
        return noexcept(std::ranges::swap(*std::declval<const I1&>(), *std::declval<const I2&>()));
    }
}

// Not named iter_swap: the views' iterators derive from detail::iterator_category_base, so
// this namespace is in their ADL set, and an iter_swap here would be a candidate of
// std::ranges::iter_swap whose constraint depends on itself.
template <class I1, class I2>
    requires swappable_iters<I1, I2>
constexpr void relaxed_iter_swap(const I1& lhs, const I2& rhs) noexcept(nothrow_swappable_iters<I1, I2>()) {
    if constexpr (std::indirectly_swappable<I1, I2>) {
        std::ranges::iter_swap(lhs, rhs);
    } else {
        std::ranges::swap(*lhs, *rhs);
    }
}

} // namespace detail

ZR_END_NAMESPACE_RANGES
