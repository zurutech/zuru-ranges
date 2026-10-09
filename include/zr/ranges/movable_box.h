// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"

#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @file movable_box.h
 *
 * @brief [range.move.wrap] movable-box (P2494R2; not C++20's copyable-box).
 *
 * "behaves exactly like optional<T>": the primary template derives from std::optional<T>.
 * The specialization stores a bare T per the recommended practice.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [range.move.wrap] "move_constructible<T> && is_object_v<T>".
template <class T>
concept boxable = std::move_constructible<T> && std::is_object_v<T>;

// [range.move.wrap] recommended practice: store only a T.
template <class T>
concept box_stores_value_only =
    (std::copy_constructible<T>
         ? std::copyable<T> || (std::is_nothrow_move_constructible_v<T> && std::is_nothrow_copy_constructible_v<T>)
         : std::movable<T> || std::is_nothrow_move_constructible_v<T>);

// [[no_unique_address]] only when assignment is T's own: destroy + construct_at of a
// potentially-overlapping subobject is not transparently replaceable ([basic.life]).
template <class T>
concept box_value_is_nested = (std::copy_constructible<T> ? std::copyable<T> : std::movable<T>);

// The value of the value-only box; overlapping iff box_value_is_nested.
template <class T> struct box_storage {
    T value;

    template <class... Args>
    constexpr explicit box_storage(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value(std::forward<Args>(args)...) {}
};

template <class T>
    requires box_value_is_nested<T>
struct box_storage<T> {
    ZR_NO_UNIQUE_ADDRESS T value;

    template <class... Args>
    constexpr explicit box_storage(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value(std::forward<Args>(args)...) {}
};

} // namespace detail

// optional<T> plus the differences [range.move.wrap] lists.
template <detail::boxable T>
class movable_box : public std::optional<T> {
public:
    using std::optional<T>::optional;

    // "equivalent to: movable-box{in_place}".
    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        : std::optional<T>{std::in_place} {}

    ~movable_box() = default;
    movable_box(const movable_box&) = default;
    movable_box(movable_box&&) = default;
    using std::optional<T>::operator=;

    // optional's assignments; user-declared ones below would otherwise suppress or outrank them.
    movable_box& operator=(const movable_box&)
        requires std::copyable<T>
    = default;
    movable_box& operator=(movable_box&&)
        requires std::movable<T>
    = default;

    // "If copyable<T> is not modeled, the copy assignment operator is equivalent to".
    constexpr movable_box& operator=(const movable_box& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires(!std::copyable<T> && std::copy_constructible<T>)
    {
        if (this != std::addressof(other)) {
            if (other) {
                this->emplace(*other);
            } else {
                this->reset();
            }
        }
        return *this;
    }

    // "If movable<T> is not modeled, the move assignment operator is equivalent to".
    constexpr movable_box& operator=(movable_box&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires(!std::movable<T>)
    {
        if (this != std::addressof(other)) {
            if (other) {
                this->emplace(std::move(*other));
            } else {
                this->reset();
            }
        }
        return *this;
    }
};

// Recommended practice: a bare T; reconstructing assignment is nothrow here.
template <detail::boxable T>
    requires detail::box_stores_value_only<T>
class movable_box<T> {

    ZR_NO_UNIQUE_ADDRESS detail::box_storage<T> storage_;

public:
    constexpr movable_box() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
        : storage_(std::in_place) {}

    constexpr explicit movable_box(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::copy_constructible<T>
        : storage_(std::in_place, value) {}

    constexpr explicit movable_box(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : storage_(std::in_place, std::move(value)) {}

    template <class... Args>
        requires std::constructible_from<T, Args...>
    constexpr explicit movable_box(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : storage_(std::in_place, std::forward<Args>(args)...) {}

    movable_box(const movable_box&) = default;
    movable_box(movable_box&&) = default;

    movable_box& operator=(const movable_box&)
        requires std::copyable<T>
    = default;
    movable_box& operator=(movable_box&&)
        requires std::movable<T>
    = default;

    constexpr movable_box& operator=(const movable_box& other) noexcept
        requires(!std::copyable<T> && std::copy_constructible<T>)
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        if (this != std::addressof(other)) {
            std::destroy_at(std::addressof(storage_.value));
            std::construct_at(std::addressof(storage_.value), *other);
        }
        return *this;
    }

    constexpr movable_box& operator=(movable_box&& other) noexcept
        requires(!std::movable<T>)
    {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        if (this != std::addressof(other)) {
            std::destroy_at(std::addressof(storage_.value));
            std::construct_at(std::addressof(storage_.value), std::move(*other));
        }
        return *this;
    }

    constexpr T& operator*() & noexcept { return storage_.value; }
    constexpr const T& operator*() const& noexcept { return storage_.value; }
    constexpr T&& operator*() && noexcept { return std::move(storage_.value); }
    constexpr const T&& operator*() const&& noexcept { return std::move(storage_.value); }

    constexpr T* operator->() noexcept { return std::addressof(storage_.value); }
    constexpr const T* operator->() const noexcept { return std::addressof(storage_.value); }

    constexpr bool has_value() const noexcept { return true; }
};

ZR_END_NAMESPACE_RANGES
