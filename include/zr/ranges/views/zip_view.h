// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/empty_view.h"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file zip_view.h
 *
 * @brief [range.zip] zip_view, views::zip.
 *
 * DEVIATION: each base is iterated with detail::indexed_iterator_t (adaptor_helpers.h): its own
 * iterator, or detail::index_iterator over an index_iterable base, so zip over Unreal arrays is
 * random access and common in every build configuration. zip-is-common, operator== and the
 * iterator operations read all-bidirectional / all-random-access through the gates of that
 * iterator (indexed_decrementable, indexed_random_access).
 *
 * range_traits: the lowest base category capped at random_access; element_at(v, n) is the
 * tuple of element_at(view_i, n). Iterator and sentinel are defined in-class: MSVC loses the
 * nested template's parameters in an out-of-line definition inside a variadic class template.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// all-random-access / all-bidirectional of the iterators zip uses (see file note).
template <bool Const, class... Views>
concept zip_all_random_access = (indexed_random_access<maybe_const<Const, Views>> && ...);
template <bool Const, class... Views>
concept zip_all_bidirectional = (indexed_decrementable<maybe_const<Const, Views>> && ...);

// [range.zip.view] zip-is-common
template <class... Rs>
concept zip_is_common = (sizeof...(Rs) == 1 && (common_range<Rs> && ...)) ||
                        (!(indexed_decrementable<Rs> && ...) && (common_range<Rs> && ...)) ||
                        ((indexed_random_access<Rs> && ...) && (sized_range<Rs> && ...));

// Named concepts: Clang < 16 substitutes a member's requires-clause eagerly.
template <class... Views>
concept zip_const_iterators_convertible = (range<const Views> && ...) && (std::convertible_to<indexed_iterator_t<Views>, indexed_iterator_t<const Views>> && ...);
template <class... Views>
concept zip_const_sentinels_convertible = (range<const Views> && ...) && (std::convertible_to<indexed_sentinel_t<Views>, indexed_sentinel_t<const Views>> && ...);

template <class T> constexpr T abs_min(std::initializer_list<T> values) {
    return std::min(values, [](T a, T b) { return (a < 0 ? -a : a) < (b < 0 ? -b : b); });
}

} // namespace detail

template <input_range... Views>
    requires(view<Views> && ...) && (sizeof...(Views) > 0)
class zip_view : public view_interface<zip_view<Views...>> {
private:
    std::tuple<Views...> views_;

    template <class T> friend struct range_traits_impl;

    template <bool Const> class sentinel;

    template <bool Const>
    class iterator : public detail::iterator_category_base<detail::all_forward<Const, Views...>, std::input_iterator_tag> {
    private:
        friend zip_view;
        friend iterator<!Const>;
        template <bool> friend class sentinel;

        std::tuple<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>...> current_;

        constexpr explicit iterator(std::tuple<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>...> current)
            : current_(std::move(current)) {}

        static constexpr auto concept_tag() {
            if constexpr (detail::zip_all_random_access<Const, Views...>) {
                return std::random_access_iterator_tag{};
            } else if constexpr (detail::zip_all_bidirectional<Const, Views...>) {
                return std::bidirectional_iterator_tag{};
            } else if constexpr (detail::all_forward<Const, Views...>) {
                return std::forward_iterator_tag{};
            } else {
                return std::input_iterator_tag{};
            }
        }

        template <class F> static constexpr bool any_of_(F f) {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return (f(std::integral_constant<std::size_t, Is>{}) || ...);
            }(std::index_sequence_for<Views...>{});
        }

    public:
        using iterator_concept = decltype(concept_tag());
        using value_type = std::tuple<range_value_t<detail::maybe_const<Const, Views>>...>;
        using difference_type = std::common_type_t<range_difference_t<detail::maybe_const<Const, Views>>...>;

        iterator() = default;

        constexpr iterator(iterator<!Const> i)
            requires Const && detail::zip_const_iterators_convertible<Views...>
            : current_(std::move(i.current_)) {}

        constexpr auto operator*() const {
            return detail::tuple_transform([](auto& i) -> decltype(auto) { return detail::deref(i); }, current_);
        }

        constexpr iterator& operator++() {
            detail::tuple_for_each([](auto& i) { ++i; }, current_);
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        constexpr iterator operator++(int)
            requires detail::all_forward<Const, Views...>
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--()
            requires detail::zip_all_bidirectional<Const, Views...>
        {
            detail::tuple_for_each([](auto& i) { --i; }, current_);
            return *this;
        }
        constexpr iterator operator--(int)
            requires detail::zip_all_bidirectional<Const, Views...>
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x)
            requires detail::zip_all_random_access<Const, Views...>
        {
            detail::tuple_for_each([&]<class I>(I& i) { i += std::iter_difference_t<I>(x); }, current_);
            return *this;
        }
        constexpr iterator& operator-=(difference_type x)
            requires detail::zip_all_random_access<Const, Views...>
        {
            detail::tuple_for_each([&]<class I>(I& i) { i -= std::iter_difference_t<I>(x); }, current_);
            return *this;
        }

        constexpr auto operator[](difference_type n) const
            requires detail::zip_all_random_access<Const, Views...>
        {
            return detail::tuple_transform([&]<class I>(I& i) -> decltype(auto) { return i[std::iter_difference_t<I>(n)]; }, current_);
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
            requires(detail::relaxed_equality_comparable<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>> && ...)
        {
            if constexpr (detail::zip_all_bidirectional<Const, Views...>) {
                return !any_of_([&](auto i) { return !detail::iter_equal(std::get<decltype(i)::value>(x.current_), std::get<decltype(i)::value>(y.current_)); });
            } else {
                return any_of_([&](auto i) { return detail::iter_equal(std::get<decltype(i)::value>(x.current_), std::get<decltype(i)::value>(y.current_)); });
            }
        }

        friend constexpr auto operator<=>(const iterator& x, const iterator& y)
            requires detail::zip_all_random_access<Const, Views...>
        {
            return x.current_ <=> y.current_;
        }

        friend constexpr iterator operator+(const iterator& i, difference_type n)
            requires detail::zip_all_random_access<Const, Views...>
        {
            auto r = i;
            r += n;
            return r;
        }
        friend constexpr iterator operator+(difference_type n, const iterator& i)
            requires detail::zip_all_random_access<Const, Views...>
        {
            auto r = i;
            r += n;
            return r;
        }
        friend constexpr iterator operator-(const iterator& i, difference_type n)
            requires detail::zip_all_random_access<Const, Views...>
        {
            auto r = i;
            r -= n;
            return r;
        }
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
            requires(std::sized_sentinel_for<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>,
                                             detail::indexed_iterator_t<detail::maybe_const<Const, Views>>> &&
                     ...)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return detail::abs_min<difference_type>({difference_type(std::get<Is>(x.current_) - std::get<Is>(y.current_))...});
            }(std::index_sequence_for<Views...>{});
        }

        friend constexpr auto iter_move(const iterator& i) noexcept(
            (noexcept(detail::relaxed_iter_move(std::declval<const detail::indexed_iterator_t<detail::maybe_const<Const, Views>>&>())) && ...) &&
            (std::is_nothrow_move_constructible_v<iter_rvalue_reference_t<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>>> && ...)) {
            return detail::tuple_transform([](const auto& it) -> decltype(auto) { return detail::relaxed_iter_move(it); }, i.current_);
        }

        friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
            (noexcept(detail::relaxed_iter_swap(std::declval<const detail::indexed_iterator_t<detail::maybe_const<Const, Views>>&>(),
                                                std::declval<const detail::indexed_iterator_t<detail::maybe_const<Const, Views>>&>())) &&
             ...))
            requires(detail::indirectly_swappable<detail::indexed_iterator_t<detail::maybe_const<Const, Views>>> && ...)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (detail::relaxed_iter_swap(std::get<Is>(l.current_), std::get<Is>(r.current_)), ...);
            }(std::index_sequence_for<Views...>{});
        }
    };

    template <bool Const> class sentinel {
    private:
        friend zip_view;
        friend sentinel<!Const>;

        std::tuple<detail::indexed_sentinel_t<detail::maybe_const<Const, Views>>...> end_;

        constexpr explicit sentinel(std::tuple<detail::indexed_sentinel_t<detail::maybe_const<Const, Views>>...> end)
            : end_(std::move(end)) {}

        // MSVC: a hidden friend does not inherit its class's friendship with iterator (CWG1699), and
        // finds no member name inside a generic lambda; the friends call these.
        template <bool OtherConst, class D> static constexpr D distance_(const iterator<OtherConst>& x, const sentinel& y) {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return detail::abs_min<D>({D(std::get<Is>(x.current_) - std::get<Is>(y.end_))...});
            }(std::index_sequence_for<Views...>{});
        }

        template <bool OtherConst> static constexpr bool equal_(const iterator<OtherConst>& x, const sentinel& y) {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return (detail::iter_equal(std::get<Is>(x.current_), std::get<Is>(y.end_)) || ...);
            }(std::index_sequence_for<Views...>{});
        }

    public:
        sentinel() = default;

        constexpr sentinel(sentinel<!Const> i)
            requires Const && detail::zip_const_sentinels_convertible<Views...>
            : end_(std::move(i.end_)) {}

        template <bool OtherConst>
            requires(sentinel_for<detail::indexed_sentinel_t<detail::maybe_const<Const, Views>>,
                                  detail::indexed_iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                     ...)
        friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
            return sentinel::equal_(x, y);
        }

        template <bool OtherConst>
            requires(std::sized_sentinel_for<detail::indexed_sentinel_t<detail::maybe_const<Const, Views>>,
                                             detail::indexed_iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                     ...)
        friend constexpr std::common_type_t<range_difference_t<detail::maybe_const<OtherConst, Views>>...>
        operator-(const iterator<OtherConst>& x, const sentinel& y) {
            return sentinel::distance_<OtherConst, std::common_type_t<range_difference_t<detail::maybe_const<OtherConst, Views>>...>>(x, y);
        }

        template <bool OtherConst>
            requires(std::sized_sentinel_for<detail::indexed_sentinel_t<detail::maybe_const<Const, Views>>,
                                             detail::indexed_iterator_t<detail::maybe_const<OtherConst, Views>>> &&
                     ...)
        friend constexpr std::common_type_t<range_difference_t<detail::maybe_const<OtherConst, Views>>...>
        operator-(const sentinel& y, const iterator<OtherConst>& x) {
            return -(x - y);
        }
    };

    template <class Self> static constexpr auto size_(Self& self) {
        return std::apply(
            [](auto... sizes) {
                using CT = detail::make_unsigned_like_t<std::common_type_t<decltype(sizes)...>>;
                return std::min({CT(sizes)...});
            },
            detail::tuple_transform(zr::ranges::size, self.views_));
    }

public:
    zip_view() = default;

    constexpr explicit zip_view(Views... views)
        : views_(std::move(views)...) {}

    constexpr auto begin()
        requires(!(detail::simple_view<Views> && ...))
    {
        return iterator<false>(detail::tuple_transform(detail::indexed_begin, views_));
    }
    constexpr auto begin() const
        requires(range<const Views> && ...)
    {
        return iterator<true>(detail::tuple_transform(detail::indexed_begin, views_));
    }

    constexpr auto end()
        requires(!(detail::simple_view<Views> && ...))
    {
        if constexpr (!detail::zip_is_common<Views...>) {
            return sentinel<false>(detail::tuple_transform(detail::indexed_end, views_));
        } else if constexpr ((detail::indexed_random_access<Views> && ...)) {
            return begin() + std::iter_difference_t<iterator<false>>(size());
        } else {
            return iterator<false>(detail::tuple_transform(detail::indexed_end, views_));
        }
    }

    constexpr auto end() const
        requires(range<const Views> && ...)
    {
        if constexpr (!detail::zip_is_common<const Views...>) {
            return sentinel<true>(detail::tuple_transform(detail::indexed_end, views_));
        } else if constexpr ((detail::indexed_random_access<const Views> && ...)) {
            return begin() + std::iter_difference_t<iterator<true>>(size());
        } else {
            return iterator<true>(detail::tuple_transform(detail::indexed_end, views_));
        }
    }

    constexpr auto size()
        requires(sized_range<Views> && ...)
    {
        return size_(*this);
    }
    constexpr auto size() const
        requires(sized_range<const Views> && ...)
    {
        return size_(*this);
    }
};

template <class... Rs> zip_view(Rs&&...) -> zip_view<views::all_t<Rs>...>;

template <class... Views> struct range_traits_impl<zip_view<Views...>> {
    static constexpr range_category category = std::min({detail::category_of<Views>()..., range_category::random_access});

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr auto element_at(zip_view<Views...>& v, Index n) {
        return std::apply([n](Views&... views) { return std::tuple<range_reference_t<Views>...>(zr::ranges::element_at(views, n)...); },
                          v.views_);
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && (detail::const_element_at<Views> && ...)
    static constexpr auto element_at(const zip_view<Views...>& v, Index n) {
        return std::apply(
            [n](const Views&... views) { return std::tuple<range_reference_t<const Views>...>(zr::ranges::element_at(views, n)...); },
            v.views_);
    }
};

template <class... Views> struct range_traits_impl<const zip_view<Views...>> : range_traits_impl<zip_view<Views...>> {
    static constexpr range_category category = std::min({detail::category_of<const Views>()..., range_category::random_access});
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

template <class... Views> inline constexpr bool enable_borrowed_range<zr::ranges::zip_view<Views...>> = (enable_borrowed_range<Views> && ...);

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [range.zip.overview]
struct zip_fn {
    constexpr auto operator()() const noexcept { return views::empty<std::tuple<>>; }

    template <class... Es>
        requires(sizeof...(Es) > 0) && requires { zip_view<views::all_t<Es>...>(std::declval<Es>()...); }
    constexpr auto operator()(Es&&... es) const noexcept(noexcept(zip_view<views::all_t<Es>...>(std::forward<Es>(es)...))) {
        return zip_view<views::all_t<Es>...>(std::forward<Es>(es)...);
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::zip_fn zip{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
