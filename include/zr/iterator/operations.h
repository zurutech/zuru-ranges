// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"

#include <iterator>
#include <type_traits>

/**
 * @file operations.h
 *
 * @brief [range.iter.ops] ranges::distance; detail::advance and detail::next for std iterators.
 *
 * DEVIATION: I and S model the relaxed input_or_output_iterator and sentinel_for
 * (iterator/concepts.h); `first == last` is detail::iter_equal.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [range.iter.op.advance] ranges::advance(i, n) and ranges::advance(i, n, bound), for std
// iterators; zr's own: libc++ 15 ships std::ranges::advance/next/prev as experimental.
template <std::input_or_output_iterator I> constexpr void advance(I& i, std::iter_difference_t<I> n) {
    if constexpr (std::random_access_iterator<I>) {
        i += n;
    } else {
        for (; n > 0; --n) {
            ++i;
        }
        if constexpr (std::bidirectional_iterator<I>) {
            for (; n < 0; ++n) {
                --i;
            }
        }
    }
}

template <std::input_or_output_iterator I, std::sentinel_for<I> S>
constexpr std::iter_difference_t<I> advance(I& i, std::iter_difference_t<I> n, S bound) {
    if constexpr (std::sized_sentinel_for<S, I>) {
        const auto d = bound - i;
        if ((n < 0 && d <= n) || (n > 0 && d >= n) || n == 0) {
            detail::advance(i, n);
            return 0;
        }
        if constexpr (std::assignable_from<I&, S>) {
            i = std::move(bound);
        } else {
            detail::advance(i, d);
        }
        return n - d;
    } else {
        for (; n > 0 && !(i == bound); --n) {
            ++i;
        }
        if constexpr (std::bidirectional_iterator<I> && std::same_as<I, S>) {
            for (; n < 0 && !(i == bound); ++n) {
                --i;
            }
        }
        return n;
    }
}

// [range.iter.op.next] ranges::next(i, bound).
template <std::input_or_output_iterator I, std::sentinel_for<I> S> constexpr I next(I i, S bound) {
    if constexpr (std::assignable_from<I&, S>) {
        i = std::move(bound);
    } else if constexpr (std::sized_sentinel_for<S, I>) {
        detail::advance(i, bound - i);
    } else {
        while (!(i == bound)) {
            ++i;
        }
    }
    return i;
}

} // namespace detail

namespace detail {

// std::sized_sentinel_for's `s - i` and `i - s` for an iterator that is not a
// std::input_or_output_iterator (detail::relaxed_counted_iterator, default_sentinel_t).
template <class S, class I>
concept relaxed_sized_sentinel_for = !std::input_or_output_iterator<I> && requires(const I& i, const S& s) {
    { s - i } -> std::same_as<iter_difference_t<I>>;
    { i - s } -> std::same_as<iter_difference_t<I>>;
};

} // namespace detail

// [range.iter.op.distance]
struct distance_fn {
    // DEVIATION: `last - first` under detail::relaxed_sized_sentinel_for ([range.iter.ops.general]:
    // the most efficient implementation).
    template <class I, sentinel_for<I> S>
        requires(!std::sized_sentinel_for<S, I>)
    constexpr iter_difference_t<I> operator()(I first, S last) const {
        if constexpr (detail::relaxed_sized_sentinel_for<S, I>) {
            return last - first;
        } else {
            iter_difference_t<I> n = 0;
            for (; !detail::iter_equal(first, last); ++first) {
                ++n;
            }
            return n;
        }
    }

    template <class I, std::sized_sentinel_for<std::decay_t<I>> S>
    constexpr iter_difference_t<std::decay_t<I>> operator()(I&& first, S last) const {
        if constexpr (!std::is_array_v<std::remove_reference_t<I>>) {
            return last - first;
        } else {
            return last - static_cast<std::decay_t<I>>(first);
        }
    }

    template <range R>
    constexpr range_difference_t<R> operator()(R&& r) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        if constexpr (sized_range<R>) {
            return static_cast<range_difference_t<R>>(zr::ranges::size(r));
        } else {
            return (*this)(zr::ranges::begin(r), zr::ranges::end(r));
        }
    }
};

inline constexpr distance_fn distance{};

ZR_END_NAMESPACE_RANGES
