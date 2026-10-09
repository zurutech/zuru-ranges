// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"

#include <cstddef>
#include <type_traits>

/**
 * @file empty_view.h
 *
 * @brief [range.empty] empty_view, views::empty.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <class T>
    requires std::is_object_v<T>
class empty_view : public view_interface<empty_view<T>> {
public:
    static constexpr T* begin() noexcept { return nullptr; }
    static constexpr T* end() noexcept { return nullptr; }
    static constexpr T* data() noexcept { return nullptr; }
    static constexpr std::size_t size() noexcept { return 0; }
    static constexpr bool empty() noexcept { return true; }
};

template <class T> struct range_traits_impl<empty_view<T>> {
    static constexpr range_category category = range_category::contiguous;
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

// [ranges.syn] enable_borrowed_range<empty_view<T>> = true.
template <class T> inline constexpr bool enable_borrowed_range<zr::ranges::empty_view<T>> = true;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_VIEWS

template <class T> inline constexpr empty_view<T> empty{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
