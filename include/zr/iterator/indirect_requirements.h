// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"

#include <concepts>
#include <functional>
#include <type_traits>

/**
 * @file indirect_requirements.h
 *
 * @brief [projected], [indirectcallable], [alg.req]: the indirect callable and common algorithm
 *        requirements, relaxed for Unreal iterators.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [projected] projected<I, Proj>: reference, indirect-value-t, value_type
// ([iterator.synopsis] projected_value_t).
template <class I, class Proj> using projected_reference_t = std::invoke_result_t<Proj&, iter_reference_t<I>>;
template <class I, class Proj> using projected_indirect_value_t = std::invoke_result_t<Proj&, iter_value_t<I>&>;
template <class I, class Proj> using projected_value_t = std::remove_cvref_t<projected_indirect_value_t<I, Proj>>;

// [indirectcallable.indirectinvocable] indirectly_regular_unary_invocable<Proj, I>.
// DEVIATION: no indirectly_readable<I> (Unreal iterators have no value_type); common_reference_with
// is relaxed_common_reference_with (C++20 tuple proxies).
template <class Proj, class I>
concept indirectly_regular_unary_invocable_relaxed =
    dereferenceable<I> && std::copy_constructible<Proj> && std::regular_invocable<Proj&, iter_value_t<I>&> &&
    std::regular_invocable<Proj&, iter_reference_t<I>> &&
    relaxed_common_reference_with<projected_indirect_value_t<I, Proj>, projected_reference_t<I, Proj>>;

// [indirectcallable.indirectinvocable] indirect_unary_predicate<Pred, projected<I, Proj>>,
// relaxed as above.
template <class Pred, class I, class Proj>
concept indirect_unary_predicate_projected = indirectly_regular_unary_invocable_relaxed<Proj, I> && std::copy_constructible<Pred> &&
                                             std::predicate<Pred&, projected_indirect_value_t<I, Proj>> &&
                                             std::predicate<Pred&, projected_reference_t<I, Proj>>;

// [alg.req.ind.cmp] indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, const T*>,
// relaxed as above. indirect-value-t<const T*> is T&, iter_reference_t<const T*> const T&.
template <class I, class Proj, class T>
concept indirectly_equality_comparable = indirectly_regular_unary_invocable_relaxed<Proj, I> &&
                                         std::predicate<std::ranges::equal_to, projected_indirect_value_t<I, Proj>, T&> &&
                                         std::predicate<std::ranges::equal_to, projected_indirect_value_t<I, Proj>, const T&> &&
                                         std::predicate<std::ranges::equal_to, projected_reference_t<I, Proj>, T&> &&
                                         std::predicate<std::ranges::equal_to, projected_reference_t<I, Proj>, const T&>;

// [indirectcallable.indirectinvocable] indirect_unary_predicate; indirect-value-t<I> is
// iter_value_t<I>&.
// DEVIATION: no indirectly_readable<I> (Unreal iterators have no value_type); zr's iter_value_t.
template <class F, class I>
concept indirect_unary_predicate =
    dereferenceable<I> && std::copy_constructible<F> && std::predicate<F&, iter_value_t<I>&> && std::predicate<F&, iter_reference_t<I>>;

// [indirectcallable.indirectinvocable] indirect_binary_predicate.
// DEVIATION: as indirect_unary_predicate.
template <class F, class I1, class I2>
concept indirect_binary_predicate =
    dereferenceable<I1> && dereferenceable<I2> && std::copy_constructible<F> &&
    std::predicate<F&, iter_value_t<I1>&, iter_value_t<I2>&> && std::predicate<F&, iter_value_t<I1>&, iter_reference_t<I2>> &&
    std::predicate<F&, iter_reference_t<I1>, iter_value_t<I2>&> && std::predicate<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

// [alg.req.ind.swap] indirectly_swappable, relaxed: swappable_iters.
template <class I1, class I2 = I1>
concept indirectly_swappable = swappable_iters<I1, I2>;

} // namespace detail

ZR_END_NAMESPACE_RANGES
