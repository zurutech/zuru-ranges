// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/range_traits.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

/**
 * @file range_access.h
 *
 * @brief [range.access] begin, end, rbegin, rend; [range.prim.size] size, [range.prim.ssize] ssize,
 *        [range.prim.size.hint] reserve_hint, [range.prim.empty] empty, [range.prim.data] data;
 *        the non-standard element_at.
 *
 * Types with explicit range_traits (ue_traits_type: Unreal containers, zr views) take size and
 * data from range_traits when it declares them, never from their iterator: TArray's is a T* in
 * Shipping only.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [expos.only.entity] decay-copy.
struct decay_copy_impl final {
    template <class T>
    constexpr std::decay_t<T> operator()(T&& v) const noexcept(std::is_nothrow_convertible_v<T, std::decay_t<T>>) {
        return std::forward<T>(v);
    }
};

inline constexpr decay_copy_impl decay_copy{};

// "If E is an rvalue and enable_borrowed_range<remove_cv_t<T>> is false, ... ill-formed."
template <class T>
concept maybe_borrowed_range = std::is_lvalue_reference_v<T> || enable_borrowed_range<std::remove_cvref_t<T>>;

template <class T>
concept class_or_enum = std::is_class_v<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>;

// A type with explicit (non-std-adapter) range_traits: its size and data come from the trait
// when it declares them. Unreal containers, and the zr views whose trait forwards data (take,
// drop) even where the view is a std::ranges::range (over detail::index_iterator).
template <class T>
concept ue_traits_type = has_range_traits<std::remove_cv_t<T>> && !std_adapted<std::remove_cv_t<T>>;

template <class T>
concept member_begin = requires(T& t) {
    { decay_copy(t.begin()) } -> input_or_output_iterator;
};

// ADL-only lookup ([basic.lookup.argdep]).
namespace adl {

#if (defined(_MSC_VER) && !defined(__clang__)) || (defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 12)
// DEVIATION (toolchain): pre-P2602R2 poison pills: cl (all versions) and GCC 11 select the deleted
// `void begin()` over ADL candidates. A type whose ADL begin/end take only const T& is not a
// range here.
template <class T> void begin(T&) = delete;
template <class T> void begin(const T&) = delete;
template <class T> void end(T&) = delete;
template <class T> void end(const T&) = delete;
template <class T> void rbegin(T&) = delete;
template <class T> void rbegin(const T&) = delete;
template <class T> void rend(T&) = delete;
template <class T> void rend(const T&) = delete;
template <class T> void size(T&) = delete;
template <class T> void size(const T&) = delete;
template <class T> void reserve_hint(T&) = delete;
template <class T> void reserve_hint(const T&) = delete;
#else
void begin() = delete;
void end() = delete;
void rbegin() = delete;
void rend() = delete;
void size() = delete;
void reserve_hint() = delete;
#endif

template <class T>
concept has_begin = class_or_enum<T> && requires(T& t) {
    { decay_copy(begin(t)) } -> input_or_output_iterator;
};

template <class T> constexpr auto begin_(T& t) noexcept(noexcept(decay_copy(begin(t)))) { return begin(t); }
template <class T> constexpr auto end_(T& t) noexcept(noexcept(decay_copy(end(t)))) { return end(t); }
template <class T> constexpr auto rbegin_(T& t) noexcept(noexcept(decay_copy(rbegin(t)))) { return rbegin(t); }
template <class T> constexpr auto rend_(T& t) noexcept(noexcept(decay_copy(rend(t)))) { return rend(t); }
template <class T> constexpr auto size_(T& t) noexcept(noexcept(decay_copy(size(t)))) { return size(t); }
template <class T> constexpr auto reserve_hint_(T& t) noexcept(noexcept(decay_copy(reserve_hint(t)))) { return reserve_hint(t); }

} // namespace adl

// [range.access.begin]
struct begin_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (std::is_array_v<std::remove_reference_t<T>>) {
            return true;
        } else if constexpr (member_begin<T>) {
            return noexcept(decay_copy(std::declval<T&>().begin()));
        } else {
            return noexcept(adl::begin_(std::declval<T&>()));
        }
    }

    template <class T>
        requires maybe_borrowed_range<T> &&
                 (std::is_array_v<std::remove_reference_t<T>> || member_begin<T> || adl::has_begin<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (std::is_array_v<std::remove_reference_t<T>>) {
            return t + 0;
        } else if constexpr (member_begin<T>) {
            return t.begin();
        } else {
            return adl::begin_(t);
        }
    }
};

template <class T>
concept member_end = requires(T& t) {
    { decay_copy(t.end()) } -> sentinel_for<decltype(begin_impl{}(t))>;
};

namespace adl {

template <class T>
concept has_end = class_or_enum<T> && requires(T& t) {
    { decay_copy(end(t)) } -> sentinel_for<decltype(begin_impl{}(t))>;
};

} // namespace adl

// [range.access.end]
struct end_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>) {
            return true;
        } else if constexpr (member_end<T>) {
            return noexcept(decay_copy(std::declval<T&>().end()));
        } else {
            return noexcept(adl::end_(std::declval<T&>()));
        }
    }

    template <class T>
        requires maybe_borrowed_range<T> &&
                 (std::is_bounded_array_v<std::remove_reference_t<T>> || member_end<T> || adl::has_end<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>) {
            return t + std::extent_v<std::remove_reference_t<T>>;
        } else if constexpr (member_end<T>) {
            return t.end();
        } else {
            return adl::end_(t);
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::begin_impl begin{};
inline constexpr detail::end_impl end{};
} // namespace cpo

namespace detail {

template <class T>
concept member_rbegin = requires(T& t) {
    { decay_copy(t.rbegin()) } -> input_or_output_iterator;
};

namespace adl {

template <class T>
concept has_rbegin = class_or_enum<T> && requires(T& t) {
    { decay_copy(rbegin(t)) } -> input_or_output_iterator;
};

} // namespace adl

template <class T>
concept reversible = requires(T& t) {
    { zr::ranges::begin(t) } -> std::bidirectional_iterator;
    { zr::ranges::end(t) } -> std::same_as<decltype(zr::ranges::begin(t))>;
};

// [range.access.rbegin]
struct rbegin_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (member_rbegin<T>) {
            return noexcept(decay_copy(std::declval<T&>().rbegin()));
        } else if constexpr (adl::has_rbegin<T>) {
            return noexcept(adl::rbegin_(std::declval<T&>()));
        } else {
            return noexcept(zr::ranges::end(std::declval<T&>())) &&
                   std::is_nothrow_copy_constructible_v<decltype(zr::ranges::end(std::declval<T&>()))>;
        }
    }

    template <class T>
        requires maybe_borrowed_range<T> && (member_rbegin<T> || adl::has_rbegin<T> || reversible<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (member_rbegin<T>) {
            return t.rbegin();
        } else if constexpr (adl::has_rbegin<T>) {
            return adl::rbegin_(t);
        } else {
            return std::make_reverse_iterator(zr::ranges::end(t));
        }
    }
};

template <class T>
concept member_rend = requires(T& t) {
    { decay_copy(t.rend()) } -> sentinel_for<decltype(rbegin_impl{}(t))>;
};

namespace adl {

template <class T>
concept has_rend = class_or_enum<T> && requires(T& t) {
    { decay_copy(rend(t)) } -> sentinel_for<decltype(rbegin_impl{}(t))>;
};

} // namespace adl

// [range.access.rend]
struct rend_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (member_rend<T>) {
            return noexcept(decay_copy(std::declval<T&>().rend()));
        } else if constexpr (adl::has_rend<T>) {
            return noexcept(adl::rend_(std::declval<T&>()));
        } else {
            return noexcept(zr::ranges::begin(std::declval<T&>())) &&
                   std::is_nothrow_copy_constructible_v<decltype(zr::ranges::begin(std::declval<T&>()))>;
        }
    }

    template <class T>
        requires maybe_borrowed_range<T> && (member_rend<T> || adl::has_rend<T> || reversible<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (member_rend<T>) {
            return t.rend();
        } else if constexpr (adl::has_rend<T>) {
            return adl::rend_(t);
        } else {
            return std::make_reverse_iterator(zr::ranges::begin(t));
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::rbegin_impl rbegin{};
inline constexpr detail::rend_impl rend{};
} // namespace cpo

// [range.sized] disable_sized_range: the std trait itself.
using std::ranges::disable_sized_range;

namespace detail {

template <class T>
concept size_enabled = !disable_sized_range<std::remove_cvref_t<T>>;

template <class T>
concept member_size = size_enabled<T> && requires(T& t) {
    { decay_copy(t.size()) } -> is_integer_like;
};

namespace adl {

template <class T>
concept has_size = class_or_enum<T> && size_enabled<T> && requires(T& t) {
    { decay_copy(size(t)) } -> is_integer_like;
};

} // namespace adl

// DEVIATION: an Unreal container's size is range_traits<T>::size (Num(), Len()).
template <class T>
concept trait_size = ue_traits_type<std::remove_reference_t<T>> && requires(T& t) {
    { decay_copy(range_traits<T>::size(t)) } -> is_integer_like;
};

template <class T>
concept subtractable_size = !ue_traits_type<std::remove_reference_t<T>> && requires(T& t) {
    { zr::ranges::begin(t) } -> std::forward_iterator;
    requires std::sized_sentinel_for<decltype(zr::ranges::end(t)), decltype(zr::ranges::begin(t))>;
};

// [range.prim.size]
struct size_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>) {
            return true;
        } else if constexpr (member_size<T>) {
            return noexcept(decay_copy(std::declval<T&>().size()));
        } else if constexpr (adl::has_size<T>) {
            return noexcept(adl::size_(std::declval<T&>()));
        } else if constexpr (trait_size<T>) {
            return noexcept(decay_copy(range_traits<T>::size(std::declval<T&>())));
        } else {
            return noexcept(zr::ranges::end(std::declval<T&>()) - zr::ranges::begin(std::declval<T&>()));
        }
    }

    template <class T>
        requires std::is_bounded_array_v<std::remove_reference_t<T>> || member_size<T> || adl::has_size<T> || trait_size<T> ||
                 subtractable_size<T>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>) {
            return decay_copy(std::extent_v<std::remove_reference_t<T>>);
        } else if constexpr (member_size<T>) {
            return t.size();
        } else if constexpr (adl::has_size<T>) {
            return adl::size_(t);
        } else if constexpr (trait_size<T>) {
            return range_traits<T>::size(t);
        } else {
            return to_unsigned_like(zr::ranges::end(t) - zr::ranges::begin(t));
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::size_impl size{};
} // namespace cpo

namespace detail {

// [range.prim.ssize]
struct ssize_impl final {
    template <class T>
        requires requires(T& t) { zr::ranges::size(t); }
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(noexcept(zr::ranges::size(t))) {
        using size_type = make_signed_like_t<decltype(zr::ranges::size(t))>;
        using D = std::conditional_t<(sizeof(std::ptrdiff_t) > sizeof(size_type)), std::ptrdiff_t, size_type>;
        return static_cast<D>(zr::ranges::size(t));
    }
};

template <class T>
concept member_reserve_hint = requires(T& t) {
    { decay_copy(t.reserve_hint()) } -> is_integer_like;
};

namespace adl {

template <class T>
concept has_reserve_hint = class_or_enum<T> && requires(T& t) {
    { decay_copy(reserve_hint(t)) } -> is_integer_like;
};

} // namespace adl

template <class T>
concept hint_by_size = requires { zr::ranges::size(std::declval<T>()); };

// [range.prim.size.hint]
struct reserve_hint_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (hint_by_size<T>) {
            return noexcept(zr::ranges::size(std::declval<T>()));
        } else if constexpr (member_reserve_hint<T>) {
            return noexcept(decay_copy(std::declval<T&>().reserve_hint()));
        } else {
            return noexcept(adl::reserve_hint_(std::declval<T&>()));
        }
    }

    template <class T>
        requires hint_by_size<T> || member_reserve_hint<T> || adl::has_reserve_hint<T>
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (hint_by_size<T>) {
            return zr::ranges::size(std::forward<T>(t));
        } else if constexpr (member_reserve_hint<T>) {
            return t.reserve_hint();
        } else {
            return adl::reserve_hint_(t);
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::ssize_impl ssize{};
inline constexpr detail::reserve_hint_impl reserve_hint{};
} // namespace cpo

namespace detail {

template <class T>
concept member_empty = requires(T& t) { bool(t.empty()); };

template <class T>
concept size_empty = requires(T& t) { zr::ranges::size(t) == 0; };

// DEVIATION: forward per range_traits, `!(begin != end)` through iter_equal: Unreal iterators
// are not std::forward_iterator and may lack ==.
template <class T>
concept begin_end_empty = requires(T& t) {
    requires forward_via_traits<std::remove_reference_t<T>> || std::forward_iterator<decltype(zr::ranges::begin(t))>;
    detail::iter_equal(zr::ranges::begin(t), zr::ranges::end(t));
};

// [range.prim.empty]
struct empty_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (member_empty<T>) {
            return noexcept(bool(std::declval<T&>().empty()));
        } else if constexpr (size_empty<T>) {
            return noexcept(zr::ranges::size(std::declval<T&>()) == 0);
        } else {
            return noexcept(detail::iter_equal(zr::ranges::begin(std::declval<T&>()), zr::ranges::end(std::declval<T&>())));
        }
    }

    template <class T>
        requires(!std::is_unbounded_array_v<std::remove_reference_t<T>>) && (member_empty<T> || size_empty<T> || begin_end_empty<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr bool operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (member_empty<T>) {
            return bool(t.empty());
        } else if constexpr (size_empty<T>) {
            return zr::ranges::size(t) == 0;
        } else {
            return detail::iter_equal(zr::ranges::begin(t), zr::ranges::end(t));
        }
    }
};

template <class T>
concept pointer_to_object = std::is_pointer_v<T> && std::is_object_v<std::remove_pointer_t<T>>;

template <class T>
concept member_data = requires(T& t) {
    { decay_copy(t.data()) } -> pointer_to_object;
};

// DEVIATION: an Unreal container's data is range_traits<T>::data (GetData(), GetCharArray()).
template <class T>
concept trait_data = ue_traits_type<std::remove_reference_t<T>> && requires(T& t) {
    { decay_copy(range_traits<T>::data(t)) } -> pointer_to_object;
};

// DEVIATION: also requires contiguous range_traits, so a view whose iterator is a TSortedMap's
// (a T* in Shipping only) has no data().
template <class T>
concept begin_data = !ue_traits_type<std::remove_reference_t<T>> && contiguous_via_traits<std::remove_reference_t<T>> &&
                     requires(T& t) {
                         { zr::ranges::begin(t) } -> std::contiguous_iterator;
                     };

// [range.prim.data]
struct data_impl final {
    template <class T> static constexpr bool nothrow() {
        if constexpr (member_data<T>) {
            return noexcept(decay_copy(std::declval<T&>().data()));
        } else if constexpr (trait_data<T>) {
            return noexcept(decay_copy(range_traits<T>::data(std::declval<T&>())));
        } else {
            return noexcept(std::to_address(zr::ranges::begin(std::declval<T&>())));
        }
    }

    template <class T>
        requires maybe_borrowed_range<T> && (member_data<T> || trait_data<T> || begin_data<T>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue t.
    constexpr auto operator()(T&& t) const noexcept(nothrow<T>()) {
        if constexpr (member_data<T>) {
            return t.data();
        } else if constexpr (trait_data<T>) {
            return range_traits<T>::data(t);
        } else {
            return std::to_address(zr::ranges::begin(t));
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::empty_impl empty{};
inline constexpr detail::data_impl data{};
} // namespace cpo

namespace detail {

template <class R>
concept trait_element_at = requires(R& r, std::ptrdiff_t i) { range_traits<R>::element_at(r, i); };

template <class R>
concept data_element_at = contiguous_via_traits<std::remove_reference_t<R>> && requires(R& r) { zr::ranges::data(r); };

template <class R>
concept begin_element_at = requires(R& r) {
    { zr::ranges::begin(r) } -> std::random_access_iterator;
};

// Not in the standard. Unchecked element access of a random-access range, never through
// iterator arithmetic of an Unreal iterator: data()[n], range_traits<R>::element_at(r, n),
// else begin(r)[n].
struct element_at_impl final {
    template <class R, is_integer_like Index>
        requires random_access_via_traits<std::remove_reference_t<R>> &&
                 (data_element_at<R> || trait_element_at<std::remove_reference_t<R>> || begin_element_at<R>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): acts on the lvalue r.
    constexpr decltype(auto) operator()(R&& r, Index n) const {
        if constexpr (data_element_at<R>) {
            return zr::ranges::data(r)[n];
        } else if constexpr (trait_element_at<std::remove_reference_t<R>>) {
            return range_traits<R>::element_at(r, n);
        } else {
            using D = std::iter_difference_t<decltype(zr::ranges::begin(r))>;
            return zr::ranges::begin(r)[static_cast<D>(n)];
        }
    }
};

} // namespace detail

inline namespace cpo {
inline constexpr detail::element_at_impl element_at{};
} // namespace cpo

namespace detail {

// A named concept: GCC 11 cannot expand a pack in a requires-expression's parameter list.
template <class R>
concept const_element_at = requires(const R& r) { zr::ranges::element_at(r, 0); };

} // namespace detail

ZR_END_NAMESPACE_RANGES
