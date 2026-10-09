// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0

/**
 * @file zr.cppm
 *
 * @brief C++20 named-module interface for zuru-ranges (`import zr;`).
 *
 * The headers are included in the global module fragment, so every zr entity stays
 * attached to the global module exactly as a plain #include produces it; the purview
 * only re-exports names (the libc++ `import std;` layout). This is what lets a
 * program mix `#include <zr/ranges.h>` and `import zr;`: under strong ownership a
 * purview-attached zr::ranges::iota_view mangles differently from the header one.
 *
 * A name missing from this list is invisible to importers and nothing else notices:
 * add it here and use it in test/module_smoke.cpp.
 */
module;

#include "zr/ranges.h"

export module zr;

export namespace zr::ranges {
// algorithm/nonmodifying.h, algorithm/fold.h, algorithm/results.h, ranges/dangling.h
using zr::ranges::all_of;
using zr::ranges::any_of;
using zr::ranges::borrowed_iterator_t;
using zr::ranges::borrowed_subrange_t;
using zr::ranges::count;
using zr::ranges::count_if;
using zr::ranges::dangling;
using zr::ranges::find;
using zr::ranges::find_if;
using zr::ranges::find_if_not;
using zr::ranges::fold_left;
using zr::ranges::fold_left_first;
using zr::ranges::fold_left_first_with_iter;
using zr::ranges::fold_left_first_with_iter_result;
using zr::ranges::fold_left_with_iter;
using zr::ranges::fold_left_with_iter_result;
using zr::ranges::fold_right;
using zr::ranges::fold_right_last;
using zr::ranges::in_value_result;
using zr::ranges::none_of;

// iterator/const_iterator.h, ranges/const_access.h
using zr::ranges::basic_const_iterator;
using zr::ranges::cbegin;
using zr::ranges::cdata;
using zr::ranges::cend;
using zr::ranges::const_iterator;
using zr::ranges::const_iterator_t;
using zr::ranges::const_sentinel;
using zr::ranges::const_sentinel_t;
using zr::ranges::constant_range;
using zr::ranges::crbegin;
using zr::ranges::crend;
using zr::ranges::iter_const_reference_t;
using zr::ranges::make_const_iterator;
using zr::ranges::make_const_sentinel;
using zr::ranges::range_const_reference_t;

// ranges/range_traits.h
using zr::ranges::bidirectional_via_traits;
using zr::ranges::contiguous_via_traits;
using zr::ranges::enable_borrowed_range;
using zr::ranges::forward_via_traits;
using zr::ranges::has_element_type;
using zr::ranges::has_range_traits;
using zr::ranges::random_access_via_traits;
using zr::ranges::range_category;
using zr::ranges::range_traits;
using zr::ranges::range_traits_impl;
using zr::ranges::standard_range;

// iterator/concepts.h
using zr::ranges::const_ref;
using zr::ranges::dereferenceable;
using zr::ranges::inequality_comparable;
using zr::ranges::iter_common_reference_t;
using zr::ranges::input_or_output_iterator;
using zr::ranges::iter_difference_t;
using zr::ranges::iter_reference_t;
using zr::ranges::iter_rvalue_reference_t;
using zr::ranges::iter_value_t;
using zr::ranges::postfix_decrementable;
using zr::ranges::postfix_incrementable;
using zr::ranges::prefix_decrementable;
using zr::ranges::prefix_incrementable;
using zr::ranges::sentinel_for;
using zr::ranges::strict_sentinel_for;
using zr::ranges::weakly_inequality_testable_with;

// iterator/operations.h
using zr::ranges::distance;

// ranges/range_access.h
using zr::ranges::begin;
using zr::ranges::data;
using zr::ranges::disable_sized_range;
using zr::ranges::element_at;
using zr::ranges::empty;
using zr::ranges::end;
using zr::ranges::rbegin;
using zr::ranges::rend;
using zr::ranges::reserve_hint;
using zr::ranges::size;
using zr::ranges::ssize;

// ranges/range_concepts.h
using zr::ranges::approximately_sized_range;
using zr::ranges::bidirectional_range;
using zr::ranges::borrowed_range;
using zr::ranges::common_range;
using zr::ranges::contiguous_range;
using zr::ranges::forward_range;
using zr::ranges::input_range;
using zr::ranges::iterator_t;
using zr::ranges::output_range;
using zr::ranges::random_access_range;
using zr::ranges::range;
using zr::ranges::range_common_reference_t;
using zr::ranges::range_difference_t;
using zr::ranges::range_reference_t;
using zr::ranges::range_rvalue_reference_t;
using zr::ranges::range_size_t;
using zr::ranges::range_value_t;
using zr::ranges::sentinel_t;
using zr::ranges::sized_range;

// ranges/view_interface.h
using zr::ranges::enable_view;
using zr::ranges::view;
using zr::ranges::view_base;
using zr::ranges::view_interface;
using zr::ranges::viewable_range;

// ranges/range_adaptor.h
using zr::ranges::range_adaptor_closure;
using zr::ranges::operator|;

// ranges/movable_box.h, ranges/to.h
using zr::ranges::movable_box;
using zr::ranges::to;

// ranges/views/
using zr::ranges::can_maybe_view;
using zr::ranges::cartesian_product_view;
using zr::ranges::chunk_by_view;
using zr::ranges::concat_view;
using zr::ranges::drop_view;
using zr::ranges::elements_view;
using zr::ranges::empty_view;
using zr::ranges::enumerate_view;
using zr::ranges::filter_view;
using zr::ranges::iota_view;
using zr::ranges::join_view;
using zr::ranges::keys_view;
using zr::ranges::owning_view;
using zr::ranges::ref_view;
using zr::ranges::reverse_view;
using zr::ranges::single_view;
using zr::ranges::get;
using zr::ranges::subrange;
using zr::ranges::subrange_kind;
using zr::ranges::take_view;
using zr::ranges::transform_view;
using zr::ranges::values_view;
using zr::ranges::zip_view;
} // namespace zr::ranges

export namespace zr::ranges::views {
using zr::ranges::views::all;
using zr::ranges::views::all_t;
using zr::ranges::views::cartesian_product;
using zr::ranges::views::chunk_by;
using zr::ranges::views::concat;
using zr::ranges::views::drop;
using zr::ranges::views::elements;
using zr::ranges::views::empty;
using zr::ranges::views::enumerate;
using zr::ranges::views::filter;
using zr::ranges::views::indices;
using zr::ranges::views::iota;
using zr::ranges::views::join;
using zr::ranges::views::keys;
using zr::ranges::views::maybe;
using zr::ranges::views::reverse;
using zr::ranges::views::single;
using zr::ranges::views::take;
using zr::ranges::views::transform;
using zr::ranges::views::values;
using zr::ranges::views::zip;
} // namespace zr::ranges::views

// A using-declaration cannot name a namespace, so the alias is redeclared.
export namespace zr {
namespace views = ranges::views;
} // namespace zr
