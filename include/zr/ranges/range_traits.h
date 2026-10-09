// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"

#include <concepts>
#include <ranges>
#include <type_traits>

/**
 * @file range_traits.h
 *
 * @brief Not in the standard: range_traits, the range category of a type.
 *
 * Unreal iterators carry no category, and TArray's/TSortedMap's iterator type differs between
 * Shipping (T*) and debug: each Unreal container specializes range_traits_impl; a std range
 * gets the std adapter below.
 */

ZR_BEGIN_NAMESPACE_RANGES

// The iterator categories of [iterator.concepts], ordered.
enum class range_category {
    input,
    forward,
    bidirectional,
    random_access,
    contiguous,
};

// Primary template: empty, so an unspecialized non-std type is not a range.
template <class T> struct range_traits_impl {};

// cv-qualified T: range_traits_impl<T> unless T is a std range (then the std adapter below
// reads const T itself).
template <class T> struct range_traits : range_traits_impl<std::remove_reference_t<T>> {};

// [range.range] enable_borrowed_range. The std trait itself: zr views specialize
// std::ranges::enable_borrowed_range next to their range_traits_impl.
using std::ranges::enable_borrowed_range;

template <class T>
concept has_element_type = requires { typename T::ElementType; };

// DEVIATION: the std adapter applies to a std::ranges::range without UE's ElementType;
// Unreal containers get explicit specializations.
template <class T>
concept standard_range = std::ranges::range<T> && !has_element_type<std::remove_cv_t<T>>;

namespace detail {

// Category of a std input range from its iterator alone. Never through std::ranges::
// contiguous_range: ranges::data may reach view_interface::data of a user view, whose
// constraint reads this trait.
template <class Range> constexpr range_category deduce_std_category() {
    using I = std::ranges::iterator_t<Range>;
    if constexpr (std::contiguous_iterator<I>) {
        return range_category::contiguous;
    } else if constexpr (std::random_access_iterator<I>) {
        return range_category::random_access;
    } else if constexpr (std::bidirectional_iterator<I>) {
        return range_category::bidirectional;
    } else if constexpr (std::forward_iterator<I>) {
        return range_category::forward;
    } else {
        return range_category::input;
    }
}

} // namespace detail

// Adapter lifting a std input range (cv-qualified or not) into the category vocabulary.
template <standard_range Range>
    requires std::input_iterator<std::ranges::iterator_t<Range>>
struct range_traits_impl<Range> {
    static constexpr bool std_adapter = true;
    static constexpr range_category category = detail::deduce_std_category<Range>();
};

namespace detail {

template <class T>
concept std_adapted = requires { requires range_traits_impl<T>::std_adapter; };

} // namespace detail

// const T: T's traits, unless T itself is std-adapted (then the adapter reads const T).
template <class T>
    requires(!detail::std_adapted<T>)
struct range_traits_impl<const T> : range_traits_impl<T> {};

// A trait declares at least a category.
template <class T>
concept has_range_traits = requires {
    { range_traits<T>::category } -> std::convertible_to<range_category>;
};

namespace detail {

// [range.refinements]: a range declaring no trait is an input range.
template <class T> constexpr range_category category_of() {
    if constexpr (has_range_traits<T>) {
        return range_traits<T>::category;
    } else {
        return range_category::input;
    }
}

} // namespace detail

// Category predicates over the trait; range_concepts.h builds the range concepts on them.
// Size is not a trait predicate: ranges::size reads range_traits<T>::size (range_access.h).

template <class T>
concept forward_via_traits = has_range_traits<T> && range_traits<T>::category >= range_category::forward;

template <class T>
concept bidirectional_via_traits = has_range_traits<T> && range_traits<T>::category >= range_category::bidirectional;

template <class T>
concept random_access_via_traits = has_range_traits<T> && range_traits<T>::category >= range_category::random_access;

template <class T>
concept contiguous_via_traits = has_range_traits<T> && range_traits<T>::category >= range_category::contiguous;

ZR_END_NAMESPACE_RANGES
