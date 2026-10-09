// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/const_access.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"

#include <concepts>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#include <stdexcept>
#else
#include <cstdlib>
#endif

/**
 * @file view_interface.h
 *
 * @brief [range.view] view_base, enable_view, view; [range.refinements] viewable_range;
 *        [range.utility.helpers]; [view.interface] view_interface.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [range.view]
struct view_base {};

template <class D>
    requires std::is_class_v<D> && std::same_as<D, std::remove_cv_t<D>>
class view_interface;

namespace detail {

// [range.view] is-derived-from-view-interface
template <class T, class U>
    requires(!std::same_as<T, view_interface<U>>)
void is_derived_from_view_interface(const T&, const view_interface<U>&); // not defined

template <class T>
concept derived_from_view_interface = requires(T t) { detail::is_derived_from_view_interface(t, t); };

// [range.view] enable_view's formula over zr's view_base / view_interface; see the
// std::ranges::enable_view specialization at the end of this file.
template <class T>
concept zr_view = std::derived_from<T, view_base> || derived_from_view_interface<T>;

} // namespace detail

// [range.view] enable_view: the std trait itself.
using std::ranges::enable_view;

template <class T>
concept view = range<T> && std::movable<T> && enable_view<T>;

// [range.refinements]
template <class T>
concept viewable_range =
    range<T> && ((view<std::remove_cvref_t<T>> && std::constructible_from<std::remove_cvref_t<T>, T>) ||
                 (!view<std::remove_cvref_t<T>> &&
                  (std::is_lvalue_reference_v<T> || (std::movable<std::remove_reference_t<T>> && !detail::is_initializer_list<T>))));

namespace detail {

// [range.utility.helpers]
template <class R>
concept simple_view = view<R> && range<const R> && std::same_as<iterator_t<R>, iterator_t<const R>> &&
                      std::same_as<sentinel_t<R>, sentinel_t<const R>>;

template <class I>
concept has_arrow = std::input_iterator<I> && (std::is_pointer_v<I> || requires(I i) { i.operator->(); });

template <class R>
concept range_with_movable_references =
    input_range<R> && std::move_constructible<range_reference_t<R>> && std::move_constructible<range_rvalue_reference_t<R>>;

// [ranges.syn] maybe-const
template <bool Const, class T> using maybe_const = std::conditional_t<Const, const T, T>;

// [view.interface.members] at: "Throws: out_of_range".
// DEVIATION: aborts when exceptions are disabled (UE builds).
[[noreturn]] inline void throw_out_of_range(const char* what) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    throw std::out_of_range(what);
#else
    (void)what;
    std::abort();
#endif
}

} // namespace detail

// [view.interface]
// Constrained members are templates on D: Clang < 16 checks a non-template member's
// constraints when view_interface<D> is instantiated, with D incomplete.
template <class D>
    requires std::is_class_v<D> && std::same_as<D, std::remove_cv_t<D>>
class view_interface {
private:
    constexpr D& derived() noexcept { return static_cast<D&>(*this); }
    constexpr const D& derived() const noexcept { return static_cast<const D&>(*this); }

public:
    template <class Self = D>
        requires sized_range<Self> || forward_range<Self>
    constexpr bool empty() {
        if constexpr (sized_range<Self>) {
            return zr::ranges::size(derived()) == 0;
        } else {
            return detail::iter_equal(zr::ranges::begin(derived()), zr::ranges::end(derived()));
        }
    }

    template <class Self = D>
        requires sized_range<const Self> || forward_range<const Self>
    constexpr bool empty() const {
        if constexpr (sized_range<const Self>) {
            return zr::ranges::size(derived()) == 0;
        } else {
            return detail::iter_equal(zr::ranges::begin(derived()), zr::ranges::end(derived()));
        }
    }

    template <class Self = D>
        requires input_range<Self>
    constexpr auto cbegin() {
        return zr::ranges::cbegin(derived());
    }
    template <class Self = D>
        requires input_range<const Self>
    constexpr auto cbegin() const {
        return zr::ranges::cbegin(derived());
    }
    template <class Self = D>
        requires input_range<Self>
    constexpr auto cend() {
        return zr::ranges::cend(derived());
    }
    template <class Self = D>
        requires input_range<const Self>
    constexpr auto cend() const {
        return zr::ranges::cend(derived());
    }

    template <class Self = D>
        requires requires(Self& d) { zr::ranges::empty(d); }
    constexpr explicit operator bool() {
        return !zr::ranges::empty(derived());
    }

    template <class Self = D>
        requires requires(const Self& d) { zr::ranges::empty(d); }
    constexpr explicit operator bool() const {
        return !zr::ranges::empty(derived());
    }

    // DEVIATION: also requires contiguous range_traits (see detail::begin_data).
    template <class Self = D>
        requires std::contiguous_iterator<iterator_t<Self>> && contiguous_via_traits<Self>
    constexpr auto data() {
        return std::to_address(zr::ranges::begin(derived()));
    }

    template <class Self = D>
        requires range<const Self> && std::contiguous_iterator<iterator_t<const Self>> && contiguous_via_traits<const Self>
    constexpr auto data() const {
        return std::to_address(zr::ranges::begin(derived()));
    }

    template <class Self = D>
        requires forward_range<Self> && std::sized_sentinel_for<sentinel_t<Self>, iterator_t<Self>>
    constexpr auto size() {
        return detail::to_unsigned_like(zr::ranges::end(derived()) - zr::ranges::begin(derived()));
    }

    template <class Self = D>
        requires forward_range<const Self> && std::sized_sentinel_for<sentinel_t<const Self>, iterator_t<const Self>>
    constexpr auto size() const {
        return detail::to_unsigned_like(zr::ranges::end(derived()) - zr::ranges::begin(derived()));
    }

    template <class Self = D>
        requires forward_range<Self>
    constexpr decltype(auto) front() {
        return *zr::ranges::begin(derived());
    }

    template <class Self = D>
        requires forward_range<const Self>
    constexpr decltype(auto) front() const {
        return *zr::ranges::begin(derived());
    }

private:
    // `*ranges::prev(ranges::end(derived()))`.
    // DEVIATION: `--last` without ranges::prev for a decrementable non-std iterator, else
    // element_at(derived(), size - 1) (Unreal).
    template <class Self> static constexpr decltype(auto) back_(Self& self) {
        if constexpr (std::bidirectional_iterator<iterator_t<Self>>) {
            auto last = zr::ranges::end(self);
            --last;
            return *last;
        } else if constexpr (prefix_decrementable<iterator_t<Self>>) {
            auto last = zr::ranges::end(self);
            --last;
            return detail::deref(last);
        } else {
            return zr::ranges::element_at(self, zr::ranges::size(self) - 1);
        }
    }

    template <class Self>
    static constexpr bool backable = std::bidirectional_iterator<iterator_t<Self>> || prefix_decrementable<iterator_t<Self>> ||
                                     (sized_range<Self> && requires(Self& s) { zr::ranges::element_at(s, 0); });

    template <class R> static constexpr decltype(auto) subscript_(R& r, range_difference_t<R> n) {
        if constexpr (std::random_access_iterator<iterator_t<R>>) {
            return zr::ranges::begin(r)[n];
        } else {
            return zr::ranges::element_at(r, n);
        }
    }

public:
    template <class Self = D>
        requires bidirectional_range<Self> && common_range<Self> && backable<Self>
    constexpr decltype(auto) back() {
        return back_(derived());
    }

    template <class Self = D>
        requires bidirectional_range<const Self> && common_range<const Self> && backable<const Self>
    constexpr decltype(auto) back() const {
        return back_(derived());
    }

    // `ranges::begin(derived())[n]`.
    // DEVIATION: element_at(derived(), n) when the iterator is not random access (Unreal).
    template <random_access_range R = D>
        requires std::random_access_iterator<iterator_t<R>> || requires(R& r) { zr::ranges::element_at(r, 0); }
    constexpr decltype(auto) operator[](range_difference_t<R> n) {
        return subscript_<R>(derived(), n);
    }

    template <random_access_range R = const D>
        requires std::random_access_iterator<iterator_t<R>> || requires(R& r) { zr::ranges::element_at(r, 0); }
    constexpr decltype(auto) operator[](range_difference_t<R> n) const {
        return subscript_<R>(derived(), n);
    }

    template <random_access_range R = D>
        requires sized_range<R> && (std::random_access_iterator<iterator_t<R>> || requires(R& r) { zr::ranges::element_at(r, 0); })
    constexpr decltype(auto) at(range_difference_t<R> n) {
        if (n < 0 || n >= static_cast<range_difference_t<R>>(zr::ranges::size(derived()))) {
            detail::throw_out_of_range("zr::ranges::view_interface::at");
        }
        return subscript_<R>(derived(), n);
    }

    template <random_access_range R = const D>
        requires sized_range<R> && (std::random_access_iterator<iterator_t<R>> || requires(R& r) { zr::ranges::element_at(r, 0); })
    constexpr decltype(auto) at(range_difference_t<R> n) const {
        if (n < 0 || n >= static_cast<range_difference_t<R>>(zr::ranges::size(derived()))) {
            detail::throw_out_of_range("zr::ranges::view_interface::at");
        }
        return subscript_<R>(derived(), n);
    }
};

ZR_END_NAMESPACE_RANGES

// [range.view]/7: zr's views opt in to std::ranges::enable_view, for cv-unqualified types only.
namespace std::ranges {
template <class T>
    requires zr::ranges::detail::zr_view<T> && same_as<T, remove_cv_t<T>>
inline constexpr bool enable_view<T> = true;
} // namespace std::ranges
