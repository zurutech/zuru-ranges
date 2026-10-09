// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/subrange.h"

#include <ranges>
#include <type_traits>

/**
 * @file dangling.h
 *
 * @brief [range.dangling] dangling, borrowed_iterator_t, borrowed_subrange_t.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [range.dangling]
using std::ranges::dangling;

template <range R> using borrowed_iterator_t = std::conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;
template <range R> using borrowed_subrange_t = std::conditional_t<borrowed_range<R>, subrange<iterator_t<R>>, dangling>;

ZR_END_NAMESPACE_RANGES
