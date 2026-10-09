// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"

#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file all.h
 *
 * @brief ref_view, owning_view ([range.ref.view], [range.owning.view]) and the
 *        views::all adaptor ([range.all]).
 *
 * Every zr view specializes range_traits_impl: its category cannot be read from Unreal
 * iterators.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// range_traits of a view whose elements are those of `v.base()`.
template <class View, class Base> struct forwarding_range_traits {
    static constexpr range_category category = category_of<Base>();

    template <is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr decltype(auto) element_at(View& v, Index n) {
        return zr::ranges::element_at(v.base(), n);
    }
    template <is_integer_like Index>
        requires(category >= range_category::random_access) && requires(const View& v) { zr::ranges::element_at(v.base(), 0); }
    static constexpr decltype(auto) element_at(const View& v, Index n) {
        return zr::ranges::element_at(v.base(), n);
    }
};

} // namespace detail

// [range.ref.view]
template <range R>
    requires std::is_object_v<R>
class ref_view : public view_interface<ref_view<R>> {
private:
    R* r_;

    static void fun(R&);
    static void fun(R&&) = delete;

public:
    template <detail::different_from<ref_view> T>
        requires std::convertible_to<T, R&> && requires { fun(std::declval<T>()); }
    constexpr ref_view(T&& t) noexcept(noexcept(static_cast<R&>(std::forward<T>(t))))
        : r_(std::addressof(static_cast<R&>(std::forward<T>(t)))) {}

    constexpr R& base() const { return *r_; }

    constexpr iterator_t<R> begin() const { return zr::ranges::begin(*r_); }
    constexpr sentinel_t<R> end() const { return zr::ranges::end(*r_); }

    constexpr bool empty() const
        requires requires { zr::ranges::empty(*r_); }
    {
        return zr::ranges::empty(*r_);
    }

    constexpr auto size() const
        requires sized_range<R>
    {
        return zr::ranges::size(*r_);
    }

    constexpr auto reserve_hint() const
        requires approximately_sized_range<R>
    {
        return zr::ranges::reserve_hint(*r_);
    }

    constexpr auto data() const
        requires contiguous_range<R>
    {
        return zr::ranges::data(*r_);
    }
};

template <class R> ref_view(R&) -> ref_view<R>;

// [range.owning.view]
template <range R>
    requires std::movable<R> && (!detail::is_initializer_list<R>)
class owning_view : public view_interface<owning_view<R>> {
private:
    R r_ = R();

public:
    owning_view()
        requires std::default_initializable<R>
    = default;
    constexpr owning_view(R&& t)
        : r_(std::move(t)) {}

    owning_view(owning_view&&) = default;
    owning_view& operator=(owning_view&&) = default;

    constexpr R& base() & noexcept { return r_; }
    constexpr const R& base() const& noexcept { return r_; }
    constexpr R&& base() && noexcept { return std::move(r_); }
    constexpr const R&& base() const&& noexcept { return std::move(r_); }

    constexpr iterator_t<R> begin() { return zr::ranges::begin(r_); }
    constexpr sentinel_t<R> end() { return zr::ranges::end(r_); }

    constexpr auto begin() const
        requires range<const R>
    {
        return zr::ranges::begin(r_);
    }
    constexpr auto end() const
        requires range<const R>
    {
        return zr::ranges::end(r_);
    }

    constexpr bool empty()
        requires requires { zr::ranges::empty(r_); }
    {
        return zr::ranges::empty(r_);
    }
    constexpr bool empty() const
        requires requires { zr::ranges::empty(r_); }
    {
        return zr::ranges::empty(r_);
    }

    constexpr auto size()
        requires sized_range<R>
    {
        return zr::ranges::size(r_);
    }
    constexpr auto size() const
        requires sized_range<const R>
    {
        return zr::ranges::size(r_);
    }

    constexpr auto reserve_hint()
        requires approximately_sized_range<R>
    {
        return zr::ranges::reserve_hint(r_);
    }
    constexpr auto reserve_hint() const
        requires approximately_sized_range<const R>
    {
        return zr::ranges::reserve_hint(r_);
    }

    constexpr auto data()
        requires contiguous_range<R>
    {
        return zr::ranges::data(r_);
    }
    constexpr auto data() const
        requires contiguous_range<const R>
    {
        return zr::ranges::data(r_);
    }
};

template <class R> struct range_traits_impl<ref_view<R>> : detail::forwarding_range_traits<ref_view<R>, R> {};
template <class R> struct range_traits_impl<owning_view<R>> : detail::forwarding_range_traits<owning_view<R>, R> {};
template <class R> struct range_traits_impl<const owning_view<R>> : detail::forwarding_range_traits<const owning_view<R>, const R> {};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

template <class R> inline constexpr bool enable_borrowed_range<zr::ranges::ref_view<R>> = true;
template <class R> inline constexpr bool enable_borrowed_range<zr::ranges::owning_view<R>> = enable_borrowed_range<R>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class E>
concept ref_view_from = requires { ref_view{std::declval<E>()}; };
template <class E>
concept owning_view_from = requires { owning_view{std::declval<E>()}; };

// [range.all.general]
struct all_fn : range_adaptor_closure<all_fn> {
private:
    template <class E> static constexpr bool nothrow() {
        if constexpr (view<std::decay_t<E>>) {
            return std::is_nothrow_constructible_v<std::decay_t<E>, E>;
        } else if constexpr (ref_view_from<E>) {
            return noexcept(ref_view{std::declval<E>()});
        } else {
            return noexcept(owning_view{std::declval<E>()});
        }
    }

public:
    template <viewable_range E>
        requires view<std::decay_t<E>> || ref_view_from<E> || owning_view_from<E>
    constexpr auto operator()(E&& e) const noexcept(nothrow<E>()) {
        if constexpr (view<std::decay_t<E>>) {
            return decay_copy(std::forward<E>(e));
        } else if constexpr (ref_view_from<E>) {
            return ref_view{std::forward<E>(e)};
        } else {
            return owning_view{std::forward<E>(e)};
        }
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::all_fn all{};

template <viewable_range R> using all_t = decltype(all(std::declval<R>()));

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
