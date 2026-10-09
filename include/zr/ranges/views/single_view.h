// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file single_view.h
 *
 * @brief [range.single] single_view, views::single.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <std::move_constructible T>
    requires std::is_object_v<T>
class single_view : public view_interface<single_view<T>> {
private:
    ZR_NO_UNIQUE_ADDRESS movable_box<T> value_;

public:
    single_view()
        requires std::default_initializable<T>
    = default;
    constexpr explicit single_view(const T& t)
        requires std::copy_constructible<T>
        : value_(t) {}
    constexpr explicit single_view(T&& t)
        : value_(std::move(t)) {}
    template <class... Args>
        requires std::constructible_from<T, Args...>
    constexpr explicit single_view(std::in_place_t, Args&&... args)
        : value_{std::in_place, std::forward<Args>(args)...} {}

    constexpr T* begin() noexcept { return data(); }
    constexpr const T* begin() const noexcept { return data(); }
    constexpr T* end() noexcept { return data() + 1; }
    constexpr const T* end() const noexcept { return data() + 1; }
    static constexpr bool empty() noexcept { return false; }
    static constexpr std::size_t size() noexcept { return 1; }
    constexpr T* data() noexcept { return value_.operator->(); }
    constexpr const T* data() const noexcept { return value_.operator->(); }
};

template <class T> single_view(T) -> single_view<T>;

template <class T> struct range_traits_impl<single_view<T>> {
    static constexpr range_category category = range_category::contiguous;
};

namespace detail {

// [range.single.overview] views::single(E): single_view<decay_t<decltype((E))>>(E).
struct single_fn {
    template <class E>
        requires requires { single_view<std::decay_t<E>>(std::declval<E>()); }
    constexpr auto operator()(E&& e) const noexcept(noexcept(single_view<std::decay_t<E>>(std::forward<E>(e)))) {
        return single_view<std::decay_t<E>>(std::forward<E>(e));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::single_fn single{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
