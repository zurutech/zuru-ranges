// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_traits.h"

#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

/**
 * @file range_concepts.h
 *
 * @brief [range.req] range concepts and [ranges.syn] associated types.
 *
 * DEVIATION: forward_range ... contiguous_range read the category from range_traits, not from
 * iterator_t<T> ([range.refinements]): Unreal iterators carry no category, and the iterator
 * type of TArray/TSortedMap differs between Shipping (T*) and debug. The std adapter in
 * range_traits.h derives the trait from the iterator, so a std range gets the std answer.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [range.range]
template <class T>
concept range = requires(T& t) {
    zr::ranges::begin(t);
    zr::ranges::end(t);
};

// [range.range] borrowed_range
template <class T>
concept borrowed_range = range<T> && (std::is_lvalue_reference_v<T> || enable_borrowed_range<std::remove_cvref_t<T>>);

// [ranges.syn] associated types.
template <class T> using iterator_t = decltype(zr::ranges::begin(std::declval<T&>()));
template <range R> using sentinel_t = decltype(zr::ranges::end(std::declval<R&>()));
template <range R> using range_difference_t = iter_difference_t<iterator_t<R>>;
template <range R> using range_value_t = iter_value_t<iterator_t<R>>;
template <range R> using range_reference_t = iter_reference_t<iterator_t<R>>;
template <range R> using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<R>>;
template <range R> using range_common_reference_t = iter_common_reference_t<iterator_t<R>>;

// [range.approximately.sized]
template <class T>
concept approximately_sized_range = range<T> && requires(T& t) { zr::ranges::reserve_hint(t); };

// [range.sized]
template <class T>
concept sized_range = approximately_sized_range<T> && requires(T& t) { zr::ranges::size(t); };

template <sized_range R> using range_size_t = decltype(zr::ranges::size(std::declval<R&>()));

// [range.refinements]
// DEVIATION: input_range reads detail::relaxed_input_iterator; output_range accepts an
// iterator that is not a std::input_or_output_iterator (Unreal) with `*i = t` valid.
template <class R, class T>
concept output_range = range<R> && (std::output_iterator<iterator_t<R>, T> ||
                                    (!std::input_or_output_iterator<iterator_t<R>> &&
                                     requires(iterator_t<R>& i, T&& t) { *i = std::forward<T>(t); }));

template <class T>
concept input_range = range<T> && detail::relaxed_input_iterator<iterator_t<T>>;

template <class T>
concept forward_range = input_range<T> && forward_via_traits<std::remove_reference_t<T>>;

template <class T>
concept bidirectional_range = forward_range<T> && bidirectional_via_traits<std::remove_reference_t<T>>;

template <class T>
concept random_access_range = bidirectional_range<T> && random_access_via_traits<std::remove_reference_t<T>>;

template <class T>
concept contiguous_range = random_access_range<T> && contiguous_via_traits<std::remove_reference_t<T>> && requires(T& t) {
    { zr::ranges::data(t) } -> std::same_as<std::add_pointer_t<range_reference_t<T>>>;
};

template <class T>
concept common_range = range<T> && std::same_as<iterator_t<T>, sentinel_t<T>>;

namespace detail {

// [range.refinements] is-initializer-list
template <class R> inline constexpr bool is_initializer_list_v = false;
template <class E> inline constexpr bool is_initializer_list_v<std::initializer_list<E>> = true;
template <class R> inline constexpr bool is_initializer_list = is_initializer_list_v<std::remove_cvref_t<R>>;

} // namespace detail

ZR_END_NAMESPACE_RANGES
