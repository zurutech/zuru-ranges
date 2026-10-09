// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/const_iterator.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"

#include <iterator>
#include <type_traits>
#include <utility>

/**
 * @file const_access.h
 *
 * @brief [range.access] cbegin, cend, crbegin, crend; [range.prim.cdata] cdata;
 *        [range.refinements] constant_range; [ranges.syn] const_iterator_t, const_sentinel_t,
 *        range_const_reference_t.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [ranges.syn] possibly-const-range
template <input_range R> constexpr auto& possibly_const_range(R& r) noexcept {
    if constexpr (input_range<const R>) {
        return const_cast<const R&>(r);
    } else {
        return r;
    }
}

// [range.prim.cdata] as-const-pointer
template <class T> constexpr auto as_const_pointer(const T* p) noexcept { return p; }

// [range.access.cbegin]
struct cbegin_impl final {
    template <class T>
        requires maybe_borrowed_range<T> && requires(T& t) { const_iterator<decltype(zr::ranges::begin(possibly_const_range(t)))>(zr::ranges::begin(possibly_const_range(t))); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::begin(possibly_const_range(t)))) {
        using U = decltype(zr::ranges::begin(possibly_const_range(t)));
        return const_iterator<U>(zr::ranges::begin(possibly_const_range(t)));
    }
};

// [range.access.cend]
struct cend_impl final {
    template <class T>
        requires maybe_borrowed_range<T> && requires(T& t) { const_sentinel<decltype(zr::ranges::end(possibly_const_range(t)))>(zr::ranges::end(possibly_const_range(t))); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::end(possibly_const_range(t)))) {
        using U = decltype(zr::ranges::end(possibly_const_range(t)));
        return const_sentinel<U>(zr::ranges::end(possibly_const_range(t)));
    }
};

// [range.access.crbegin]
struct crbegin_impl final {
    template <class T>
        requires maybe_borrowed_range<T> && requires(T& t) { const_iterator<decltype(zr::ranges::rbegin(possibly_const_range(t)))>(zr::ranges::rbegin(possibly_const_range(t))); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::rbegin(possibly_const_range(t)))) {
        using U = decltype(zr::ranges::rbegin(possibly_const_range(t)));
        return const_iterator<U>(zr::ranges::rbegin(possibly_const_range(t)));
    }
};

// [range.access.crend]
struct crend_impl final {
    template <class T>
        requires maybe_borrowed_range<T> && requires(T& t) { const_sentinel<decltype(zr::ranges::rend(possibly_const_range(t)))>(zr::ranges::rend(possibly_const_range(t))); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::rend(possibly_const_range(t)))) {
        using U = decltype(zr::ranges::rend(possibly_const_range(t)));
        return const_sentinel<U>(zr::ranges::rend(possibly_const_range(t)));
    }
};

// [range.prim.cdata]
struct cdata_impl final {
    template <class T>
        requires maybe_borrowed_range<T> && requires(T& t) { as_const_pointer(zr::ranges::data(possibly_const_range(t))); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::data(possibly_const_range(t)))) {
        return as_const_pointer(zr::ranges::data(possibly_const_range(t)));
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::cbegin_impl cbegin{};
inline constexpr detail::cend_impl cend{};
inline constexpr detail::crbegin_impl crbegin{};
inline constexpr detail::crend_impl crend{};
inline constexpr detail::cdata_impl cdata{};
} // namespace cpo

// [ranges.syn]
template <range R> using const_iterator_t = decltype(zr::ranges::cbegin(std::declval<R&>()));
template <range R> using const_sentinel_t = decltype(zr::ranges::cend(std::declval<R&>()));
template <range R> using range_const_reference_t = iter_const_reference_t<iterator_t<R>>;

// [range.refinements]
template <class T>
concept constant_range = input_range<T> && detail::constant_iterator<iterator_t<T>>;

ZR_END_NAMESPACE_RANGES
