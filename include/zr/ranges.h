// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @file ranges.h
 *
 * @brief Umbrella header of zuru-ranges (single include).
 *
 * Pure, std-only: no Unreal dependency. To use with Unreal containers, include this
 * and "zr/unreal/ue_range_traits.h" (after "Core.h"/"CoreMinimal.h"); the latter is
 * kept separate so this header stays free of Epic source.
 *
 * @par Synopsis (mirrors [ranges.syn])
 * Foundation:
 *   - range_traits.h     semantic range_category trait (the customization point)
 *   - range_access.h, range_concepts.h   CPOs and concepts
 *   - const_access.h     cbegin family, constant_range
 *   - dangling.h         dangling, borrowed_iterator_t, borrowed_subrange_t
 *   - view_interface.h, subrange.h, range_adaptor.h           view utilities + pipe
 *   - movable_box.h, non_propagating_cache.h, adaptor_helpers.h   exposition-only helpers
 *   - to.h               ranges::to
 * Views (zr::ranges + adaptor objects in zr::views):
 *   - all (ref_view, owning_view), transform, filter, take, drop, join, elements (keys,
 *     values), reverse, maybe (non-standard)
 *   - zip, enumerate, chunk_by, cartesian_product (C++23), concat (C++26)
 * Factories (zr::views):
 *   - empty, single, iota, indices (C++26)
 * Iterators: zr/iterator.h ([iterator.synopsis]). Algorithms: zr/algorithm.h ([algorithm.syn],
 * ranges:: overloads).
 */

#include "zr/algorithm.h"
#include "zr/iterator.h"
#include "zr/ranges/const_access.h"
#include "zr/ranges/dangling.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/to.h"
#include "zr/ranges/view_interface.h"

#include "zr/ranges/views/all.h"
#include "zr/ranges/views/cartesian_product_view.h"
#include "zr/ranges/views/chunk_by_view.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/drop_view.h"
#include "zr/ranges/views/elements_view.h"
#include "zr/ranges/views/empty_view.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/maybe_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/single_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/take_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/ranges/views/zip_view.h"
