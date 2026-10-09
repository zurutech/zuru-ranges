// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"

#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file non_propagating_cache.h
 *
 * @brief [range.nonprop.cache] non-propagating-cache.
 *
 * Own storage rather than std::optional: emplace-deref direct-non-list-initializes from `*i`,
 * which optional::emplace cannot do for a non-movable prvalue. Only emplace-deref reconstructs
 * the union object (the mem-initializer is where the prvalue is elided into the member); emplace
 * constructs the member, as optional does. MSVC 14.29's constant evaluator loses the active
 * member in both forms: the cached views (filter, drop, reverse, join, chunk_by) are not usable
 * in constant expressions there (test/compile/constexpr_checks.cpp skips them).
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class T>
    requires std::is_object_v<T>
class non_propagating_cache {
    struct deref_tag {};

    // The value is a variant member initialized in a mem-initializer, so a prvalue `*i`
    // initializes it directly ([dcl.init.general]/16.6.1).
    union storage_t {
        char empty_;
        std::remove_cv_t<T> value_;

        constexpr storage_t() noexcept
            : empty_() {}
        template <class I>
        constexpr storage_t(deref_tag, const I& i)
            : value_(*i) {}
        constexpr ~storage_t() {}
    };

    storage_t storage_;
    bool engaged_ = false;

public:
    constexpr non_propagating_cache() noexcept = default;

    constexpr non_propagating_cache(const non_propagating_cache&) noexcept {}

    constexpr non_propagating_cache(non_propagating_cache&& other) noexcept { other.reset(); }

    constexpr non_propagating_cache& operator=(const non_propagating_cache& other) noexcept {
        if (std::addressof(other) != this) {
            reset();
        }
        return *this;
    }

    constexpr non_propagating_cache& operator=(non_propagating_cache&& other) noexcept {
        reset();
        other.reset();
        return *this;
    }

    constexpr ~non_propagating_cache() { reset(); }

    constexpr bool has_value() const noexcept { return engaged_; }
    constexpr explicit operator bool() const noexcept { return engaged_; }

    constexpr T& operator*() noexcept { return storage_.value_; }
    constexpr const T& operator*() const noexcept { return storage_.value_; }
    constexpr T* operator->() noexcept { return std::addressof(storage_.value_); }
    constexpr const T* operator->() const noexcept { return std::addressof(storage_.value_); }

    constexpr void reset() noexcept {
        if (engaged_) {
            engaged_ = false;
            std::destroy_at(std::addressof(storage_.value_));
        }
    }

    template <class... Args> constexpr T& emplace(Args&&... args) {
        reset();
        std::construct_at(std::addressof(storage_.value_), std::forward<Args>(args)...);
        engaged_ = true;
        return storage_.value_;
    }

    // emplace-deref
    template <class I> constexpr T& emplace_deref(const I& i) {
        reset();
        std::construct_at(std::addressof(storage_), deref_tag{}, i);
        engaged_ = true;
        return storage_.value_;
    }
};

// non_propagating_cache<T> if Present, else an empty type (T is not named otherwise).
struct no_cache {};

template <bool Present, class T> struct cache_if_impl {
    using type = no_cache;
};
template <class T> struct cache_if_impl<true, T> {
    using type = non_propagating_cache<T>;
};
template <bool Present, class T> using cache_if = typename cache_if_impl<Present, T>::type;

} // namespace detail

ZR_END_NAMESPACE_RANGES
