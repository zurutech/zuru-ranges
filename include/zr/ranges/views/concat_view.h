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

#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

/**
 * @file concat_view.h
 *
 * @brief [range.concat] concat_view, views::concat (C++26).
 *
 * DEVIATION: concat-is-random-access / concat-is-bidirectional read all-random-access /
 * all-bidirectional through the base-iterator gates of adaptor_helpers.h. Comparisons of a
 * base iterator with its range's begin/end use detail::iter_equal.
 *
 * range_traits: input unless every base is forward; random_access (bidirectional) when every
 * base is and every base but the last is common, as concat-is-random-access
 * (concat-is-bidirectional); element_at(v, n) when every base is also sized.
 * Iterator defined in-class: MSVC loses the nested template's parameters in an out-of-line
 * definition inside a variadic class template.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class... Rs> using concat_reference_t = std::common_reference_t<range_reference_t<Rs>...>;
template <class... Rs> using concat_value_t = std::common_type_t<range_value_t<Rs>...>;
template <class... Rs> using concat_rvalue_reference_t = std::common_reference_t<range_rvalue_reference_t<Rs>...>;

// [range.concat.view] concat-indirectly-readable
template <class Ref, class RRef, class It>
concept concat_indirectly_readable_impl = requires(const It it) {
    { detail::deref(it) } -> std::convertible_to<Ref>;
    { detail::relaxed_iter_move(it) } -> std::convertible_to<RRef>;
};

template <class... Rs>
concept concat_indirectly_readable =
    std::common_reference_with<concat_reference_t<Rs...>&&, concat_value_t<Rs...>&> &&
    std::common_reference_with<concat_reference_t<Rs...>&&, concat_rvalue_reference_t<Rs...>&&> &&
    std::common_reference_with<concat_rvalue_reference_t<Rs...>&&, const concat_value_t<Rs...>&> &&
    (concat_indirectly_readable_impl<concat_reference_t<Rs...>, concat_rvalue_reference_t<Rs...>, iterator_t<Rs>> && ...);

// [range.concat.view] concatable
template <class... Rs>
concept concatable = requires {
    typename concat_reference_t<Rs...>;
    typename concat_value_t<Rs...>;
    typename concat_rvalue_reference_t<Rs...>;
} && concat_indirectly_readable<Rs...>;

// (common_range<Fs> && ...), Fs = all of Rs but the last.
template <class... Rs> constexpr bool concat_front_common() {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
        return ((Is + 1 == sizeof...(Rs) || common_range<std::tuple_element_t<Is, std::tuple<Rs...>>>) && ...);
    }(std::index_sequence_for<Rs...>{});
}

// (sized_range<Fs> && ...), Fs = all of Rs but the first.
template <class... Rs> constexpr bool concat_back_sized() {
    return []<std::size_t... Is>(std::index_sequence<Is...>) {
        return ((Is == 0 || sized_range<std::tuple_element_t<Is, std::tuple<Rs...>>>) && ...);
    }(std::index_sequence_for<Rs...>{});
}

// [range.concat.view]
template <bool Const, class... Rs>
concept concat_is_random_access = all_random_access<Const, Rs...> && concat_front_common<maybe_const<Const, Rs>...>();

template <bool Const, class... Rs>
concept concat_is_bidirectional = all_bidirectional<Const, Rs...> && concat_front_common<maybe_const<Const, Rs>...>();

// f.template operator()<I>() for I == index.
template <std::size_t Count, std::size_t I = 0, class F> constexpr decltype(auto) visit_index(std::size_t index, F&& f) {
    if constexpr (I + 1 == Count) {
        return std::forward<F>(f).template operator()<I>();
    } else {
        if (index == I) {
            return std::forward<F>(f).template operator()<I>();
        }
        return visit_index<Count, I + 1>(index, std::forward<F>(f));
    }
}

// ranges::distance(r) of a sized or random-access common range.
template <class R> constexpr auto range_distance(R& r) {
    if constexpr (sized_range<R>) {
        return static_cast<range_difference_t<R>>(zr::ranges::size(r));
    } else {
        return zr::ranges::end(r) - zr::ranges::begin(r);
    }
}

template <bool Const, class... Views>
inline constexpr bool concat_has_category = all_forward<Const, Views...> && (has_cpp17_category<iterator_t<maybe_const<Const, Views>>> && ...);

template <bool Const, class... Views> constexpr auto concat_category() {
    if constexpr (concat_has_category<Const, Views...>) {
        if constexpr (!std::is_reference_v<concat_reference_t<maybe_const<Const, Views>...>>) {
            return std::input_iterator_tag{};
        } else if constexpr ((std::derived_from<cpp17_category_t<iterator_t<maybe_const<Const, Views>>>, std::random_access_iterator_tag> &&
                              ...) &&
                             concat_is_random_access<Const, Views...>) {
            return std::random_access_iterator_tag{};
        } else if constexpr ((std::derived_from<cpp17_category_t<iterator_t<maybe_const<Const, Views>>>, std::bidirectional_iterator_tag> &&
                              ...) &&
                             concat_is_bidirectional<Const, Views...>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr ((std::derived_from<cpp17_category_t<iterator_t<maybe_const<Const, Views>>>, std::forward_iterator_tag> && ...)) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }
}

} // namespace detail

template <input_range... Views>
    requires(view<Views> && ...) && (sizeof...(Views) > 0) && detail::concatable<Views...>
class concat_view : public view_interface<concat_view<Views...>> {
private:
    static constexpr std::size_t N = sizeof...(Views);

    std::tuple<Views...> views_;

    template <class T> friend struct range_traits_impl;

    template <bool Const>
    class iterator : public detail::iterator_category_base<detail::concat_has_category<Const, Views...>,
                                                           decltype(detail::concat_category<Const, Views...>())> {
    private:
        friend concat_view;
        friend iterator<!Const>;

        using Parent = detail::maybe_const<Const, concat_view>;
        using base_iter = std::variant<iterator_t<detail::maybe_const<Const, Views>>...>;
        using reference = detail::concat_reference_t<detail::maybe_const<Const, Views>...>;
        using rvalue_reference = detail::concat_rvalue_reference_t<detail::maybe_const<Const, Views>...>;

        static constexpr auto concept_tag() {
            if constexpr (detail::concat_is_random_access<Const, Views...>) {
                return std::random_access_iterator_tag{};
            } else if constexpr (detail::concat_is_bidirectional<Const, Views...>) {
                return std::bidirectional_iterator_tag{};
            } else if constexpr (detail::all_forward<Const, Views...>) {
                return std::forward_iterator_tag{};
            } else {
                return std::input_iterator_tag{};
            }
        }

    public:
        using iterator_concept = decltype(concept_tag());
        using value_type = detail::concat_value_t<detail::maybe_const<Const, Views>...>;
        using difference_type = std::common_type_t<range_difference_t<detail::maybe_const<Const, Views>>...>;

    private:
        Parent* parent_ = nullptr;
        base_iter it_;

        template <std::size_t I> constexpr auto& view_() const { return std::get<I>(parent_->views_); }

        template <std::size_t I> constexpr void satisfy() {
            if constexpr (I < sizeof...(Views) - 1) {
                if (detail::iter_equal(std::get<I>(it_), zr::ranges::end(view_<I>()))) {
                    it_.template emplace<I + 1>(zr::ranges::begin(view_<I + 1>()));
                    satisfy<I + 1>();
                }
            }
        }

        template <std::size_t I> constexpr void prev() {
            if constexpr (I == 0) {
                --std::get<0>(it_);
            } else {
                if (detail::iter_equal(std::get<I>(it_), zr::ranges::begin(view_<I>()))) {
                    it_.template emplace<I - 1>(zr::ranges::end(view_<I - 1>()));
                    prev<I - 1>();
                } else {
                    --std::get<I>(it_);
                }
            }
        }

        template <std::size_t I> constexpr void advance_fwd([[maybe_unused]] difference_type offset, difference_type steps) {
            using underlying_diff_type = std::iter_difference_t<std::variant_alternative_t<I, base_iter>>;
            if constexpr (I == sizeof...(Views) - 1) {
                std::get<I>(it_) += static_cast<underlying_diff_type>(steps);
            } else {
                auto n_size = detail::range_distance(view_<I>());
                if (offset + steps < n_size) {
                    std::get<I>(it_) += static_cast<underlying_diff_type>(steps);
                } else {
                    it_.template emplace<I + 1>(zr::ranges::begin(view_<I + 1>()));
                    advance_fwd<I + 1>(0, offset + steps - n_size);
                }
            }
        }

        template <std::size_t I> constexpr void advance_bwd([[maybe_unused]] difference_type offset, difference_type steps) {
            using underlying_diff_type = std::iter_difference_t<std::variant_alternative_t<I, base_iter>>;
            if constexpr (I == 0) {
                std::get<I>(it_) -= static_cast<underlying_diff_type>(steps);
            } else {
                if (offset >= steps) {
                    std::get<I>(it_) -= static_cast<underlying_diff_type>(steps);
                } else {
                    auto prev_size = detail::range_distance(view_<I - 1>());
                    it_.template emplace<I - 1>(zr::ranges::end(view_<I - 1>()));
                    advance_bwd<I - 1>(prev_size, steps - offset);
                }
            }
        }

        template <class... Args>
            requires std::constructible_from<base_iter, Args&&...>
        constexpr explicit iterator(Parent* parent, Args&&... args)
            : parent_(parent)
            , it_(std::forward<Args>(args)...) {}

        // begin(): the draft's `it.satisfy<0>()` on a named iterator; a parameter, not a local, because
        // MSVC's constant evaluator (19.44) rejects the friend's access to the private constructor
        // when it initializes a local variable of a static member function template.
        static constexpr iterator satisfied(iterator it) {
            it.template satisfy<0>();
            return it;
        }

        // Sum of the sizes of views [First, Last).
        template <std::size_t First, std::size_t Last> constexpr difference_type sizes_between() const {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return (difference_type(0) + ... + static_cast<difference_type>(zr::ranges::size(view_<First + Is>())));
            }(std::make_index_sequence<Last - First>{});
        }

    public:
        iterator() = default;

        constexpr iterator(iterator<!Const> it)
            requires Const && detail::const_convertible_iterators<Views...>
            : parent_(it.parent_)
            , it_(detail::visit_index<sizeof...(Views)>(it.it_.index(), [&]<std::size_t I>() {
                return base_iter(std::in_place_index<I>, std::get<I>(std::move(it.it_)));
            })) {}

        constexpr decltype(auto) operator*() const {
            assert(!it_.valueless_by_exception());
            return std::visit([](auto&& it) -> reference { return detail::deref(it); }, it_);
        }

        constexpr iterator& operator++() {
            detail::visit_index<sizeof...(Views)>(it_.index(), [&]<std::size_t I>() {
                ++std::get<I>(it_);
                satisfy<I>();
            });
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
            requires detail::concat_is_bidirectional<Const, Views...>
        {
            detail::visit_index<sizeof...(Views)>(it_.index(), [&]<std::size_t I>() { prev<I>(); });
            return *this;
        }
        constexpr iterator operator--(int)
            requires detail::concat_is_bidirectional<Const, Views...>
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
            requires detail::concat_is_random_access<Const, Views...>
        {
            detail::visit_index<sizeof...(Views)>(it_.index(), [&]<std::size_t I>() {
                if (n > 0) {
                    advance_fwd<I>(std::get<I>(it_) - zr::ranges::begin(view_<I>()), n);
                } else if (n < 0) {
                    advance_bwd<I>(std::get<I>(it_) - zr::ranges::begin(view_<I>()), -n);
                }
            });
            return *this;
        }
        constexpr iterator& operator-=(difference_type n)
            requires detail::concat_is_random_access<Const, Views...>
        {
            *this += -n;
            return *this;
        }

        constexpr decltype(auto) operator[](difference_type n) const
            requires detail::concat_is_random_access<Const, Views...>
        {
            return *((*this) + n);
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
            requires(detail::relaxed_equality_comparable<iterator_t<detail::maybe_const<Const, Views>>> && ...)
        {
            return x.it_.index() == y.it_.index() &&
                   detail::visit_index<sizeof...(Views)>(x.it_.index(), [&]<std::size_t I>() { return detail::iter_equal(std::get<I>(x.it_), std::get<I>(y.it_)); });
        }

        friend constexpr bool operator==(const iterator& it, std::default_sentinel_t) {
            constexpr auto last_idx = sizeof...(Views) - 1;
            return it.it_.index() == last_idx && detail::iter_equal(std::get<last_idx>(it.it_), zr::ranges::end(it.template view_<last_idx>()));
        }

        friend constexpr bool operator<(const iterator& x, const iterator& y)
            requires detail::all_random_access<Const, Views...>
        {
            return x.it_ < y.it_;
        }
        friend constexpr bool operator>(const iterator& x, const iterator& y)
            requires detail::all_random_access<Const, Views...>
        {
            return x.it_ > y.it_;
        }
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
            requires detail::all_random_access<Const, Views...>
        {
            return x.it_ <= y.it_;
        }
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
            requires detail::all_random_access<Const, Views...>
        {
            return x.it_ >= y.it_;
        }
        friend constexpr auto operator<=>(const iterator& x, const iterator& y)
            requires(detail::all_random_access<Const, Views...> && (std::three_way_comparable<iterator_t<detail::maybe_const<Const, Views>>> && ...))
        {
            return x.it_ <=> y.it_;
        }

        friend constexpr iterator operator+(const iterator& it, difference_type n)
            requires detail::concat_is_random_access<Const, Views...>
        {
            auto temp = it;
            temp += n;
            return temp;
        }
        friend constexpr iterator operator+(difference_type n, const iterator& it)
            requires detail::concat_is_random_access<Const, Views...>
        {
            return it + n;
        }
        friend constexpr iterator operator-(const iterator& it, difference_type n)
            requires detail::concat_is_random_access<Const, Views...>
        {
            auto temp = it;
            temp -= n;
            return temp;
        }

        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
            requires detail::concat_is_random_access<Const, Views...>
        {
            const std::size_t ix = x.it_.index();
            const std::size_t iy = y.it_.index();
            if (ix > iy) {
                return detail::visit_index<sizeof...(Views)>(iy, [&]<std::size_t Iy>() {
                    return detail::visit_index<sizeof...(Views)>(ix, [&]<std::size_t Ix>() -> difference_type {
                        if constexpr (Ix > Iy) {
                            const auto dy = static_cast<difference_type>(zr::ranges::end(y.template view_<Iy>()) - std::get<Iy>(y.it_));
                            const auto dx = static_cast<difference_type>(std::get<Ix>(x.it_) - zr::ranges::begin(x.template view_<Ix>()));
                            difference_type s = 0;
                            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                                ((s += static_cast<difference_type>(detail::range_distance(x.template view_<Iy + 1 + Is>()))), ...);
                            }(std::make_index_sequence<Ix - Iy - 1>{});
                            return dy + s + dx;
                        } else {
                            return 0;
                        }
                    });
                });
            } else if (ix < iy) {
                return -(y - x);
            } else {
                return detail::visit_index<sizeof...(Views)>(ix, [&]<std::size_t I>() { return static_cast<difference_type>(std::get<I>(x.it_) - std::get<I>(y.it_)); });
            }
        }

        friend constexpr difference_type operator-(const iterator& x, std::default_sentinel_t)
            requires(std::sized_sentinel_for<sentinel_t<detail::maybe_const<Const, Views>>, iterator_t<detail::maybe_const<Const, Views>>> &&
                     ...) &&
                    (detail::concat_back_sized<detail::maybe_const<Const, Views>...>())
        {
            return detail::visit_index<sizeof...(Views)>(x.it_.index(), [&]<std::size_t I>() {
                const auto dx = static_cast<difference_type>(zr::ranges::end(x.template view_<I>()) - std::get<I>(x.it_));
                return -(dx + x.template sizes_between<I + 1, sizeof...(Views)>());
            });
        }

        friend constexpr difference_type operator-(std::default_sentinel_t, const iterator& x)
            requires(std::sized_sentinel_for<sentinel_t<detail::maybe_const<Const, Views>>, iterator_t<detail::maybe_const<Const, Views>>> &&
                     ...) &&
                    (detail::concat_back_sized<detail::maybe_const<Const, Views>...>())
        {
            return -(x - std::default_sentinel);
        }

        friend constexpr decltype(auto) iter_move(const iterator& it) noexcept(
            ((noexcept(detail::relaxed_iter_move(std::declval<const iterator_t<detail::maybe_const<Const, Views>>&>())) &&
              std::is_nothrow_convertible_v<range_rvalue_reference_t<detail::maybe_const<Const, Views>>, rvalue_reference>) &&
             ...)) {
            assert(!it.it_.valueless_by_exception());
            return std::visit([](const auto& i) -> rvalue_reference { return detail::relaxed_iter_move(i); }, it.it_);
        }

        friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(
            noexcept(std::ranges::swap(*x, *y)) &&
            (noexcept(detail::relaxed_iter_swap(std::declval<const iterator_t<detail::maybe_const<Const, Views>>&>(),
                                                std::declval<const iterator_t<detail::maybe_const<Const, Views>>&>())) &&
             ...))
            requires std::swappable_with<reference, reference> &&
                     (... && detail::indirectly_swappable<iterator_t<detail::maybe_const<Const, Views>>>)
        {
            std::visit(
                [&](const auto& it1, const auto& it2) {
                    if constexpr (std::is_same_v<decltype(it1), decltype(it2)>) {
                        detail::relaxed_iter_swap(it1, it2);
                    } else {
                        std::ranges::swap(*x, *y);
                    }
                },
                x.it_, y.it_);
        }
    };

    template <bool Const, class Self> static constexpr auto begin_(Self& self) {
        return iterator<Const>::satisfied(iterator<Const>(std::addressof(self), std::in_place_index<0>, zr::ranges::begin(std::get<0>(self.views_))));
    }

    template <bool Const, class Self> static constexpr auto end_(Self& self) {
        if constexpr (detail::all_forward<Const, Views...> && common_range<detail::maybe_const<Const, std::tuple_element_t<N - 1, std::tuple<Views...>>>>) {
            return iterator<Const>(std::addressof(self), std::in_place_index<N - 1>, zr::ranges::end(std::get<N - 1>(self.views_)));
        } else {
            return std::default_sentinel;
        }
    }

    template <class Self> static constexpr auto size_(Self& self) {
        return std::apply(
            [](auto... sizes) {
                using CT = detail::make_unsigned_like_t<std::common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            detail::tuple_transform(zr::ranges::size, self.views_));
    }

    template <class Self> static constexpr auto reserve_hint_(Self& self) {
        return std::apply(
            [](auto... sizes) {
                using CT = detail::make_unsigned_like_t<std::common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            detail::tuple_transform(zr::ranges::reserve_hint, self.views_));
    }

public:
    constexpr concat_view() = default;
    constexpr explicit concat_view(Views... views)
        : views_(std::move(views)...) {}

    constexpr iterator<false> begin()
        requires(!(detail::simple_view<Views> && ...))
    {
        return begin_<false>(*this);
    }
    constexpr iterator<true> begin() const
        requires(range<const Views> && ...) && detail::concatable<const Views...>
    {
        return begin_<true>(*this);
    }

    constexpr auto end()
        requires(!(detail::simple_view<Views> && ...))
    {
        return end_<false>(*this);
    }
    constexpr auto end() const
        requires(range<const Views> && ...) && detail::concatable<const Views...>
    {
        return end_<true>(*this);
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

    constexpr auto reserve_hint()
        requires(approximately_sized_range<Views> && ...)
    {
        return reserve_hint_(*this);
    }
    constexpr auto reserve_hint() const
        requires(approximately_sized_range<const Views> && ...)
    {
        return reserve_hint_(*this);
    }
};

template <class... R> concat_view(R&&...) -> concat_view<views::all_t<R>...>;

namespace detail {

template <class... Views> constexpr range_category concat_traits_category() {
    if constexpr (!(forward_range<Views> && ...)) {
        return range_category::input;
    } else if constexpr ((random_access_range<Views> && ...) && concat_front_common<Views...>()) {
        return range_category::random_access;
    } else if constexpr ((bidirectional_range<Views> && ...) && concat_front_common<Views...>()) {
        return range_category::bidirectional;
    } else {
        return range_category::forward;
    }
}

} // namespace detail

template <class... Views> struct range_traits_impl<concat_view<Views...>> {
    static constexpr range_category category = detail::concat_traits_category<Views...>();

private:
    template <class Self, class Index> static constexpr decltype(auto) element_at_(Self& v, Index n) {
        using reference = detail::concat_reference_t<detail::maybe_const<std::is_const_v<Self>, Views>...>;
        auto i = static_cast<std::make_unsigned_t<std::common_type_t<Index, std::ptrdiff_t>>>(n);
        std::size_t which = 0;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (void)((i < static_cast<decltype(i)>(zr::ranges::size(std::get<Is>(v.views_))) ? (which = Is, true)
                                                                                              : (i -= static_cast<decltype(i)>(zr::ranges::size(std::get<Is>(v.views_))), false)) ||
                   ...);
        }(std::index_sequence_for<Views...>{});
        return detail::visit_index<sizeof...(Views)>(which, [&]<std::size_t I>() -> reference {
            return zr::ranges::element_at(std::get<I>(v.views_), i);
        });
    }

public:
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && (sized_range<Views> && ...)
    static constexpr decltype(auto) element_at(concat_view<Views...>& v, Index n) {
        return element_at_(v, n);
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && (sized_range<const Views> && ...) &&
                (detail::const_element_at<Views> && ...)
    static constexpr decltype(auto) element_at(const concat_view<Views...>& v, Index n) {
        return element_at_(v, n);
    }
};

template <class... Views> struct range_traits_impl<const concat_view<Views...>> : range_traits_impl<concat_view<Views...>> {
    static constexpr range_category category = detail::concat_traits_category<const Views...>();
};

namespace detail {

// [range.concat.overview]
struct concat_fn {
    template <class E>
        requires input_range<E> && requires { views::all(std::declval<E>()); }
    constexpr auto operator()(E&& e) const noexcept(noexcept(views::all(std::forward<E>(e)))) {
        return views::all(std::forward<E>(e));
    }

    template <class... Es>
        requires(sizeof...(Es) > 1 || !(input_range<Es> && ...)) && requires { concat_view(std::declval<Es>()...); }
    constexpr auto operator()(Es&&... es) const noexcept(noexcept(concat_view(std::forward<Es>(es)...))) {
        return concat_view(std::forward<Es>(es)...);
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::concat_fn concat{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
