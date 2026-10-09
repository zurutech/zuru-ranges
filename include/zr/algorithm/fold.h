// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/algorithm/results.h"
#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/dangling.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"

#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @file fold.h
 *
 * @brief [alg.fold] fold_left, fold_left_first, fold_right, fold_right_last, fold_left_with_iter,
 *        fold_left_first_with_iter (P2322R6), with [algorithm.syn]'s flipped,
 *        indirectly-binary-left-foldable and indirectly-binary-right-foldable.
 *
 * Algorithm function objects ([alg.func.obj]) with the iterator/sentinel and range overloads.
 *
 * DEVIATION (all): I models detail::relaxed_input_iterator and S the relaxed sentinel_for
 * (iterator/concepts.h); `first == last` is detail::iter_equal; the foldable concepts read
 * dereferenceable<I> for indirectly_readable<I>, as [alg.nonmodifying] does: Unreal iterators are
 * not std iterators.
 *
 * DEVIATION (fold_right, fold_right_last): bidirectional_iterator<I> is
 * detail::relaxed_bidirectional_iterator (also an Unreal iterator with a prefix decrement), and the
 * range overloads fold a detail::index_iterable range by position through ranges::element_at:
 * iterators of Unreal random-access containers mostly do not decrement (TChunkedArray, TBitArray,
 * TDeque, ...).
 */

ZR_BEGIN_NAMESPACE_RANGES

// [algorithm.syn]
template <class I, class T> using fold_left_with_iter_result = in_value_result<I, T>;
template <class I, class T> using fold_left_first_with_iter_result = in_value_result<I, T>;

namespace detail {

// [algorithm.syn] flipped
template <class F> class flipped {
    F f;

public:
    template <class T, class U>
        requires std::invocable<F&, U, T>
    std::invoke_result_t<F&, U, T> operator()(T&&, U&&);
};

// [algorithm.syn] indirectly-binary-left-foldable-impl
template <class F, class T, class I, class U>
concept indirectly_binary_left_foldable_impl = std::movable<T> && std::movable<U> && std::convertible_to<T, U> &&
                                               std::invocable<F&, U, iter_reference_t<I>> &&
                                               std::assignable_from<U&, std::invoke_result_t<F&, U, iter_reference_t<I>>>;

// [algorithm.syn] indirectly-binary-left-foldable (see file note).
template <class F, class T, class I>
concept indirectly_binary_left_foldable =
    std::copy_constructible<F> && dereferenceable<I> && std::invocable<F&, T, iter_reference_t<I>> &&
    std::convertible_to<std::invoke_result_t<F&, T, iter_reference_t<I>>, std::decay_t<std::invoke_result_t<F&, T, iter_reference_t<I>>>> &&
    indirectly_binary_left_foldable_impl<F, T, I, std::decay_t<std::invoke_result_t<F&, T, iter_reference_t<I>>>>;

// [algorithm.syn] indirectly-binary-right-foldable
template <class F, class T, class I>
concept indirectly_binary_right_foldable = indirectly_binary_left_foldable<flipped<F>, T, I>;

template <class I>
concept declares_bidirectional_iterator_concept = requires {
    typename I::iterator_concept;
    requires std::derived_from<typename I::iterator_concept, std::bidirectional_iterator_tag>;
};

// [iterator.concept.bidir] bidirectional_iterator (see file note): a std iterator models it, or
// declares it (a view's iterator over an Unreal base, or a C++20 tuple proxy); an Unreal iterator
// decrements.
template <class I>
concept relaxed_bidirectional_iterator =
    relaxed_input_iterator<I> && prefix_decrementable<I> &&
    (std::bidirectional_iterator<I> || !std::input_or_output_iterator<I> || declares_bidirectional_iterator_concept<I>);

// A bidirectional_range the right folds accept: by position, or through its iterator.
template <class R>
concept right_foldable_range = index_iterable<R> || (bidirectional_range<R> && relaxed_bidirectional_iterator<iterator_t<R>>);

// ranges::next(first, last) ([range.iter.op.next]) over a relaxed iterator: by increments.
template <class I, class S> constexpr I next_to(I first, S last) {
    if constexpr (std::input_or_output_iterator<I> && std::sentinel_for<S, I>) {
        return detail::next(std::move(first), std::move(last));
    } else {
        while (!detail::iter_equal(first, last)) {
            ++first;
        }
        return first;
    }
}

// The right fold of positions [0, n) of r into init, by element_at.
template <class U, class R, class T, class F> constexpr U fold_right_by_position(R& r, range_difference_t<R> n, T init, F& f) {
    if (n == 0) {
        return U(std::move(init));
    }
    U accum = std::invoke(f, zr::ranges::element_at(r, --n), std::move(init));
    while (n > 0) {
        accum = std::invoke(f, zr::ranges::element_at(r, --n), std::move(accum));
    }
    return accum;
}

} // namespace detail

// [alg.fold] fold_left_with_iter
struct fold_left_with_iter_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class T = iter_value_t<I>, detail::indirectly_binary_left_foldable<T, I> F>
    constexpr auto operator()(I first, S last, T init, F f) const {
        using U = std::decay_t<std::invoke_result_t<F&, T, iter_reference_t<I>>>;
        using result = fold_left_with_iter_result<I, U>;
        if (detail::iter_equal(first, last)) {
            return result{std::move(first), U(std::move(init))};
        }
        U accum = std::invoke(f, std::move(init), *first);
        for (++first; !detail::iter_equal(first, last); ++first) {
            accum = std::invoke(f, std::move(accum), *first);
        }
        return result{std::move(first), std::move(accum)};
    }

    template <input_range R, class T = range_value_t<R>, detail::indirectly_binary_left_foldable<T, iterator_t<R>> F>
    constexpr fold_left_with_iter_result<borrowed_iterator_t<R>, std::decay_t<std::invoke_result_t<F&, T, range_reference_t<R>>>>
    operator()(R&& r, T init, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(init), std::move(f));
    }
};

inline constexpr fold_left_with_iter_fn fold_left_with_iter{};

// [alg.fold] fold_left
struct fold_left_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class T = iter_value_t<I>, detail::indirectly_binary_left_foldable<T, I> F>
    constexpr auto operator()(I first, S last, T init, F f) const {
        return fold_left_with_iter(std::move(first), last, std::move(init), f).value;
    }

    template <input_range R, class T = range_value_t<R>, detail::indirectly_binary_left_foldable<T, iterator_t<R>> F>
    constexpr auto operator()(R&& r, T init, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return fold_left_with_iter(zr::ranges::begin(r), zr::ranges::end(r), std::move(init), f).value;
    }
};

inline constexpr fold_left_fn fold_left{};

// [alg.fold] fold_left_first_with_iter
struct fold_left_first_with_iter_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, detail::indirectly_binary_left_foldable<iter_value_t<I>, I> F>
        requires std::constructible_from<iter_value_t<I>, iter_reference_t<I>>
    constexpr auto operator()(I first, S last, F f) const {
        using U = decltype(fold_left(std::move(first), last, iter_value_t<I>(*first), f));
        using result = fold_left_first_with_iter_result<I, std::optional<U>>;
        if (detail::iter_equal(first, last)) {
            return result{std::move(first), std::optional<U>()};
        }
        std::optional<U> init(std::in_place, *first);
        for (++first; !detail::iter_equal(first, last); ++first) {
            *init = std::invoke(f, std::move(*init), *first);
        }
        return result{std::move(first), std::move(init)};
    }

    template <input_range R, detail::indirectly_binary_left_foldable<range_value_t<R>, iterator_t<R>> F>
        requires std::constructible_from<range_value_t<R>, range_reference_t<R>>
    constexpr fold_left_first_with_iter_result<borrowed_iterator_t<R>,
                                               std::optional<decltype(fold_left(std::declval<R&>(), std::declval<range_value_t<R>>(), std::declval<F&>()))>>
    operator()(R&& r, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(f));
    }
};

inline constexpr fold_left_first_with_iter_fn fold_left_first_with_iter{};

// [alg.fold] fold_left_first
struct fold_left_first_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, detail::indirectly_binary_left_foldable<iter_value_t<I>, I> F>
        requires std::constructible_from<iter_value_t<I>, iter_reference_t<I>>
    constexpr auto operator()(I first, S last, F f) const {
        return fold_left_first_with_iter(std::move(first), last, f).value;
    }

    template <input_range R, detail::indirectly_binary_left_foldable<range_value_t<R>, iterator_t<R>> F>
        requires std::constructible_from<range_value_t<R>, range_reference_t<R>>
    constexpr auto operator()(R&& r, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return fold_left_first_with_iter(zr::ranges::begin(r), zr::ranges::end(r), f).value;
    }
};

inline constexpr fold_left_first_fn fold_left_first{};

// [alg.fold] fold_right
struct fold_right_fn {
    template <detail::relaxed_bidirectional_iterator I, sentinel_for<I> S, class T = iter_value_t<I>, detail::indirectly_binary_right_foldable<T, I> F>
    constexpr auto operator()(I first, S last, T init, F f) const {
        using U = std::decay_t<std::invoke_result_t<F&, iter_reference_t<I>, T>>;
        if (detail::iter_equal(first, last)) {
            return U(std::move(init));
        }
        I tail = detail::next_to(first, last);
        U accum = std::invoke(f, *--tail, std::move(init));
        while (!detail::iter_equal(first, tail)) {
            accum = std::invoke(f, *--tail, std::move(accum));
        }
        return accum;
    }

    template <detail::right_foldable_range R, class T = range_value_t<R>, detail::indirectly_binary_right_foldable<T, iterator_t<R>> F>
    constexpr auto operator()(R&& r, T init, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        using U = std::decay_t<std::invoke_result_t<F&, range_reference_t<R>, T>>;
        if constexpr (!detail::index_iterable<R>) {
            return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(init), std::move(f));
        } else {
            return detail::fold_right_by_position<U>(r, static_cast<range_difference_t<R>>(zr::ranges::size(r)), std::move(init), f);
        }
    }
};

inline constexpr fold_right_fn fold_right{};

// [alg.fold] fold_right_last
struct fold_right_last_fn {
    template <detail::relaxed_bidirectional_iterator I, sentinel_for<I> S, detail::indirectly_binary_right_foldable<iter_value_t<I>, I> F>
        requires std::constructible_from<iter_value_t<I>, iter_reference_t<I>>
    constexpr auto operator()(I first, S last, F f) const {
        using U = decltype(fold_right(first, last, iter_value_t<I>(*first), f));
        if (detail::iter_equal(first, last)) {
            return std::optional<U>();
        }
        I tail = detail::next_to(first, std::move(last));
        --tail;
        return std::optional<U>(std::in_place, fold_right(std::move(first), tail, iter_value_t<I>(*tail), std::move(f)));
    }

    template <detail::right_foldable_range R, detail::indirectly_binary_right_foldable<range_value_t<R>, iterator_t<R>> F>
        requires std::constructible_from<range_value_t<R>, range_reference_t<R>>
    constexpr auto operator()(R&& r, F f) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        using U = decltype(fold_right(r, std::declval<range_value_t<R>>(), f));
        if constexpr (!detail::index_iterable<R>) {
            return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(f));
        } else {
            auto n = static_cast<range_difference_t<R>>(zr::ranges::size(r));
            if (n == 0) {
                return std::optional<U>();
            }
            --n;
            return std::optional<U>(std::in_place, detail::fold_right_by_position<U>(r, n, range_value_t<R>(zr::ranges::element_at(r, n)), f));
        }
    }
};

inline constexpr fold_right_last_fn fold_right_last{};

ZR_END_NAMESPACE_RANGES
