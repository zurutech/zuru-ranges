// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/transform_view.h"

#include <cassert>
#include <concepts>
#include <type_traits>
#include <utility>

#include "zr/config.h"

/**
 * @file maybe_view.h
 *
 * @brief views::maybe: not in the standard. A zr extension inspired by P1255R14
 *        (`views::nullable`), which adapts one nullable object into a view of 0 or 1 elements;
 *        views::maybe instead adapts a range of nullable objects into the range of their
 *        engaged values: views::all(r) | views::filter(has-value) | views::transform(value).
 *
 * nullable-object is P1255R14's [range.nullable.view]/3.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// P1255R14 [range.nullable.view] readable-references, nullable-object.
template <class Ref, class ConstRef>
concept readable_references = std::is_lvalue_reference_v<Ref> && std::is_object_v<std::remove_reference_t<Ref>> &&
                              std::is_lvalue_reference_v<ConstRef> && std::is_object_v<std::remove_reference_t<ConstRef>> &&
                              std::convertible_to<std::add_pointer_t<ConstRef>, const std::remove_reference_t<Ref>*>;

template <class T>
concept nullable_object = std::is_object_v<T> && requires(T& t, const T& ct) {
    bool(ct);
    *t;
    *ct;
} && readable_references<iter_reference_t<T>, iter_reference_t<const T>>;

struct has_value_fn {
    template <nullable_object T> constexpr bool operator()(const T& opt) const { return static_cast<bool>(opt); }
};

inline constexpr has_value_fn has_value{};

// Precondition: engaged.
struct get_value_fn {
    // Raw pointer: the pointee outlives a prvalue pointer.
    template <class T> constexpr T& operator()(T* ptr) const {
        assert(ptr != nullptr);
        return *ptr;
    }

    template <class T>
        requires(!std::is_pointer_v<T>)
    constexpr decltype(auto) operator()(T& opt) const {
        assert(has_value(opt));
        return *opt;
    }

    // auto(*opt): a reference into an rvalue nullable dangles.
    template <class T, class V = std::remove_cvref_t<decltype(*std::declval<T>())>>
        requires(!std::is_pointer_v<std::remove_reference_t<T>> && !std::is_lvalue_reference_v<T&&> &&
                 std::constructible_from<V, decltype(*std::declval<T>())>)
    constexpr V operator()(T&& opt) const {
        assert(has_value(opt));
        return V(*std::forward<T>(opt));
    }
};

inline constexpr get_value_fn get_value{};

} // namespace detail

template <class Range>
concept can_maybe_view = viewable_range<Range> && detail::nullable_object<range_value_t<Range>> && requires {
    detail::has_value(std::declval<range_reference_t<Range>>());
    detail::get_value(std::declval<range_reference_t<Range>>());
};

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

struct maybe_fn : range_adaptor_closure<maybe_fn> {
    template <class Range>
        requires can_maybe_view<Range>
    constexpr auto operator()(Range&& r) const {
        return views::all(std::forward<Range>(r)) | views::filter(detail::has_value) | views::transform(detail::get_value);
    }
};

inline constexpr maybe_fn maybe{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
