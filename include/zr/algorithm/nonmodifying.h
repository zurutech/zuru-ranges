// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/indirect_requirements.h"
#include "zr/ranges/dangling.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"

#include <functional>
#include <utility>

/**
 * @file nonmodifying.h
 *
 * @brief [alg.nonmodifying] all_of, any_of, none_of, find, find_if, find_if_not, count,
 *        count_if.
 *
 * Algorithm function objects ([alg.func.obj]) with the iterator/sentinel and range overloads.
 * For several ranges, concatenate: `any_of(views::concat(a, b), pred)`.
 */

ZR_BEGIN_NAMESPACE_RANGES

// DEVIATION (all algorithms): I models detail::relaxed_input_iterator and S the relaxed
// sentinel_for (iterator/concepts.h); `first == last` is detail::iter_equal: Unreal iterators are not std::input_iterator and
// may have only `!=`.

// [alg.all.of]
struct all_of_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
        for (; !detail::iter_equal(first, last); ++first) {
            if (!std::invoke(pred, std::invoke(proj, *first))) {
                return false;
            }
        }
        return true;
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr all_of_fn all_of{};

// [alg.any.of]
struct any_of_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
        for (; !detail::iter_equal(first, last); ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                return true;
            }
        }
        return false;
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr any_of_fn any_of{};

// [alg.none.of]
struct none_of_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
        return !any_of(std::move(first), std::move(last), std::move(pred), std::move(proj));
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return !any_of(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr none_of_fn none_of{};

// [alg.find]
struct find_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class T = detail::projected_value_t<I, Proj>>
        requires detail::indirectly_equality_comparable<I, Proj, T>
    constexpr I operator()(I first, S last, const T& value, Proj proj = {}) const {
        for (; !detail::iter_equal(first, last); ++first) {
            if (std::invoke(proj, *first) == value) {
                break;
            }
        }
        return first;
    }

    template <input_range R, class Proj = std::identity, class T = detail::projected_value_t<iterator_t<R>, Proj>>
        requires detail::indirectly_equality_comparable<iterator_t<R>, Proj, T>
    constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), value, std::move(proj));
    }
};

inline constexpr find_fn find{};

struct find_if_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const {
        for (; !detail::iter_equal(first, last); ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                break;
            }
        }
        return first;
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr find_if_fn find_if{};

struct find_if_not_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const {
        for (; !detail::iter_equal(first, last); ++first) {
            if (!std::invoke(pred, std::invoke(proj, *first))) {
                break;
            }
        }
        return first;
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr find_if_not_fn find_if_not{};

// [alg.count]
struct count_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class T = detail::projected_value_t<I, Proj>>
        requires detail::indirectly_equality_comparable<I, Proj, T>
    constexpr iter_difference_t<I> operator()(I first, S last, const T& value, Proj proj = {}) const {
        iter_difference_t<I> counter = 0;
        for (; !detail::iter_equal(first, last); ++first) {
            if (std::invoke(proj, *first) == value) {
                ++counter;
            }
        }
        return counter;
    }

    template <input_range R, class Proj = std::identity, class T = detail::projected_value_t<iterator_t<R>, Proj>>
        requires detail::indirectly_equality_comparable<iterator_t<R>, Proj, T>
    constexpr range_difference_t<R> operator()(R&& r, const T& value, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), value, std::move(proj));
    }
};

inline constexpr count_fn count{};

struct count_if_fn {
    template <detail::relaxed_input_iterator I, sentinel_for<I> S, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, I, Proj>
    constexpr iter_difference_t<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
        iter_difference_t<I> counter = 0;
        for (; !detail::iter_equal(first, last); ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                ++counter;
            }
        }
        return counter;
    }

    template <input_range R, class Proj = std::identity, class Pred>
        requires detail::indirect_unary_predicate_projected<Pred, iterator_t<R>, Proj>
    constexpr range_difference_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const { // NOLINT(cppcoreguidelines-missing-std-forward)
        return (*this)(zr::ranges::begin(r), zr::ranges::end(r), std::move(pred), std::move(proj));
    }
};

inline constexpr count_if_fn count_if{};

ZR_END_NAMESPACE_RANGES
