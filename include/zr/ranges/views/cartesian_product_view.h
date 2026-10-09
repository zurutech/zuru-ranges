// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/single_view.h"

#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file cartesian_product_view.h
 *
 * @brief [range.cartesian] cartesian_product_view, views::cartesian_product.
 *
 * DEVIATION: current_ holds each base iterator in a movable_box: next()/prev() restart a base
 * with `it = ranges::begin(rng)`, and UE's debug array iterator is not assignable; the box's
 * assignment reconstructs ([range.move.wrap]).
 *
 * DEVIATION: cartesian-product-is-random-access / -is-bidirectional, and the
 * begin() + distance branch of cartesian-product-common-arg, read random access and
 * decrement through the base-iterator gates of adaptor_helpers.h. operator== requires every
 * base iterator, not only First's, to be (relaxed) equality-comparable: Unreal forward
 * iterators need not be.
 *
 * range_traits: the draft's iterator_concept formula over range_traits categories;
 * element_at(v, n) decomposes n mixed-radix over the bases' sizes.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <bool Const, class First, class... Vs>
concept cartesian_product_is_random_access =
    (base_random_access<maybe_const<Const, First>> && ... && (base_random_access<maybe_const<Const, Vs>> && sized_range<maybe_const<Const, Vs>>));

template <class R>
concept cartesian_product_common_arg = common_range<R> || (sized_range<R> && base_random_access<R>);

template <bool Const, class First, class... Vs>
concept cartesian_product_is_bidirectional =
    (base_decrementable<maybe_const<Const, First>> && ... &&
     (base_decrementable<maybe_const<Const, Vs>> && cartesian_product_common_arg<maybe_const<Const, Vs>>));

template <class First, class...>
concept cartesian_product_is_common = cartesian_product_common_arg<First>;

template <class... Vs>
concept cartesian_product_is_sized = (sized_range<Vs> && ...);

// cartesian-is-sized-sentinel<Const, FirstSent, First, Vs...>; FirstSent is sentinel_t if
// FirstIsSentinel, else iterator_t.
template <bool Const, bool FirstIsSentinel, class First, class... Vs>
concept cartesian_is_sized_sentinel =
    (std::sized_sentinel_for<std::conditional_t<FirstIsSentinel, sentinel_t<maybe_const<Const, First>>, iterator_t<maybe_const<Const, First>>>,
                             iterator_t<maybe_const<Const, First>>> &&
     ... && (sized_range<maybe_const<Const, Vs>> && std::sized_sentinel_for<iterator_t<maybe_const<Const, Vs>>, iterator_t<maybe_const<Const, Vs>>>));

template <cartesian_product_common_arg R> constexpr auto cartesian_common_arg_end(R& r) {
    if constexpr (common_range<R>) {
        return zr::ranges::end(r);
    } else {
        return zr::ranges::begin(r) + static_cast<range_difference_t<R>>(zr::ranges::size(r));
    }
}

// Lexicographic synth-three-way ([expos.only.entity]) of two tuples of boxed iterators.
template <class T, class U> constexpr auto synth_three_way(const T& t, const U& u) {
    if constexpr (std::three_way_comparable_with<T, U>) {
        return t <=> u;
    } else {
        if (t < u) {
            return std::weak_ordering::less;
        }
        if (u < t) {
            return std::weak_ordering::greater;
        }
        return std::weak_ordering::equivalent;
    }
}

} // namespace detail

template <input_range First, forward_range... Vs>
    requires(view<First> && ... && view<Vs>)
class cartesian_product_view : public view_interface<cartesian_product_view<First, Vs...>> {
private:
    std::tuple<First, Vs...> bases_;

    template <class T> friend struct range_traits_impl;

    template <bool Const> class iterator {
    public:
        using iterator_category = std::input_iterator_tag;

    private:
        static constexpr auto concept_tag() {
            if constexpr (detail::cartesian_product_is_random_access<Const, First, Vs...>) {
                return std::random_access_iterator_tag{};
            } else if constexpr (detail::cartesian_product_is_bidirectional<Const, First, Vs...>) {
                return std::bidirectional_iterator_tag{};
            } else if constexpr (forward_range<detail::maybe_const<Const, First>>) {
                return std::forward_iterator_tag{};
            } else {
                return std::input_iterator_tag{};
            }
        }

    public:
        using iterator_concept = decltype(concept_tag());
        using value_type = std::tuple<range_value_t<detail::maybe_const<Const, First>>, range_value_t<detail::maybe_const<Const, Vs>>...>;
        using reference = std::tuple<range_reference_t<detail::maybe_const<Const, First>>, range_reference_t<detail::maybe_const<Const, Vs>>...>;
        using difference_type =
            std::common_type_t<std::ptrdiff_t, range_difference_t<detail::maybe_const<Const, First>>, range_difference_t<detail::maybe_const<Const, Vs>>...>;

    private:
        friend cartesian_product_view;
        friend iterator<!Const>;

        using Parent = detail::maybe_const<Const, cartesian_product_view>;
        using current_t = std::tuple<movable_box<iterator_t<detail::maybe_const<Const, First>>>, movable_box<iterator_t<detail::maybe_const<Const, Vs>>>...>;

        static constexpr std::size_t last = sizeof...(Vs);

        Parent* parent_ = nullptr;
        current_t current_;

        template <std::size_t N> constexpr auto& base_() const { return std::get<N>(parent_->bases_); }

        template <std::size_t N = last> constexpr void next() {
            auto& it = std::get<N>(current_);
            ++*it;
            if constexpr (N > 0) {
                if (detail::iter_equal(*it, zr::ranges::end(base_<N>()))) {
                    it = std::tuple_element_t<N, current_t>(zr::ranges::begin(base_<N>()));
                    next<N - 1>();
                }
            }
        }

        template <std::size_t N = last> constexpr void prev() {
            auto& it = std::get<N>(current_);
            if constexpr (N > 0) {
                if (detail::iter_equal(*it, zr::ranges::begin(base_<N>()))) {
                    it = std::tuple_element_t<N, current_t>(detail::cartesian_common_arg_end(base_<N>()));
                    prev<N - 1>();
                }
            }
            --*it;
        }

        // scaled-size(N)
        template <std::size_t N> constexpr difference_type scaled_size() const {
            if constexpr (N <= last) {
                return static_cast<difference_type>(zr::ranges::size(base_<N>())) * scaled_size<N + 1>();
            } else {
                return static_cast<difference_type>(1);
            }
        }

        template <class Tuple> constexpr difference_type distance_from(const Tuple& t) const {
            return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                return (difference_type(0) + ... +
                        static_cast<difference_type>(static_cast<difference_type>(*std::get<Ns>(current_) - std::get<Ns>(t)) * scaled_size<Ns + 1>()));
            }(std::make_index_sequence<last + 1>{});
        }

        constexpr iterator(Parent& parent, current_t current)
            : parent_(std::addressof(parent))
            , current_(std::move(current)) {}

        // x > 0 (< 0): next() (prev()) |x| times, in O(1).
        template <std::size_t N = last> constexpr void advance_(difference_type x) {
            auto& it = std::get<N>(current_);
            if constexpr (N == 0) {
                *it += static_cast<range_difference_t<detail::maybe_const<Const, First>>>(x);
            } else {
                auto& rng = base_<N>();
                const auto size = static_cast<difference_type>(zr::ranges::size(rng));
                const auto pos = static_cast<difference_type>(*it - zr::ranges::begin(rng)) + x;
                auto carry = pos / size;
                auto rem = pos % size;
                if (rem < 0) {
                    rem += size;
                    --carry;
                }
                it = std::tuple_element_t<N, current_t>(zr::ranges::begin(rng) + static_cast<range_difference_t<std::remove_reference_t<decltype(rng)>>>(rem));
                if (carry != 0) {
                    advance_<N - 1>(carry);
                }
            }
        }

        template <class Self> static constexpr auto end_tuple(Self& i) {
            return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                return std::tuple(zr::ranges::end(i.template base_<0>()), zr::ranges::begin(i.template base_<Ns + 1>())...);
            }(std::make_index_sequence<last>{});
        }

    public:
        iterator() = default;

        constexpr iterator(iterator<!Const> i)
            requires Const && detail::const_convertible_iterators<First, Vs...>
            : parent_(i.parent_)
            , current_(std::apply([](auto&... boxes) { return current_t(std::move(*boxes)...); }, i.current_)) {}

        constexpr auto operator*() const {
            return detail::tuple_transform([](auto& i) -> decltype(auto) { return detail::deref(*i); }, current_);
        }

        constexpr iterator& operator++() {
            next();
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        constexpr iterator operator++(int)
            requires forward_range<detail::maybe_const<Const, First>>
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--()
            requires detail::cartesian_product_is_bidirectional<Const, First, Vs...>
        {
            prev();
            return *this;
        }
        constexpr iterator operator--(int)
            requires detail::cartesian_product_is_bidirectional<Const, First, Vs...>
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x)
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            if (x != 0) {
                advance_(x);
            }
            return *this;
        }
        constexpr iterator& operator-=(difference_type x)
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            *this += -x;
            return *this;
        }

        constexpr reference operator[](difference_type n) const
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            return *((*this) + n);
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
            requires(detail::relaxed_equality_comparable<iterator_t<detail::maybe_const<Const, First>>> && ... &&
                     detail::relaxed_equality_comparable<iterator_t<detail::maybe_const<Const, Vs>>>)
        {
            return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                return (detail::iter_equal(*std::get<Ns>(x.current_), *std::get<Ns>(y.current_)) && ...);
            }(std::make_index_sequence<last + 1>{});
        }

        friend constexpr bool operator==(const iterator& x, std::default_sentinel_t) {
            return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                return (detail::iter_equal(*std::get<Ns>(x.current_), zr::ranges::end(x.template base_<Ns>())) || ...);
            }(std::make_index_sequence<last + 1>{});
        }

        friend constexpr auto operator<=>(const iterator& x, const iterator& y)
            requires detail::all_random_access<Const, First, Vs...>
        {
            return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                using R = std::common_comparison_category_t<decltype(detail::synth_three_way(*std::get<Ns>(x.current_), *std::get<Ns>(y.current_)))...>;
                R result = R::equivalent;
                (void)((result = detail::synth_three_way(*std::get<Ns>(x.current_), *std::get<Ns>(y.current_)), result != 0) || ...);
                return result;
            }(std::make_index_sequence<last + 1>{});
        }

        friend constexpr iterator operator+(const iterator& x, difference_type y)
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            return iterator(x) += y;
        }
        friend constexpr iterator operator+(difference_type x, const iterator& y)
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            return y + x;
        }
        friend constexpr iterator operator-(const iterator& x, difference_type y)
            requires detail::cartesian_product_is_random_access<Const, First, Vs...>
        {
            return iterator(x) -= y;
        }
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
            requires detail::cartesian_is_sized_sentinel<Const, false, First, Vs...>
        {
            return x.distance_from(std::apply([](const auto&... boxes) { return std::tuple(*boxes...); }, y.current_));
        }

        friend constexpr difference_type operator-(const iterator& i, std::default_sentinel_t)
            requires detail::cartesian_is_sized_sentinel<Const, true, First, Vs...>
        {
            return i.distance_from(end_tuple(i));
        }
        friend constexpr difference_type operator-(std::default_sentinel_t s, const iterator& i)
            requires detail::cartesian_is_sized_sentinel<Const, true, First, Vs...>
        {
            return -(i - s);
        }

        friend constexpr auto iter_move(const iterator& i) noexcept(
            (noexcept(detail::relaxed_iter_move(std::declval<const iterator_t<detail::maybe_const<Const, First>>&>())) && ... &&
             noexcept(detail::relaxed_iter_move(std::declval<const iterator_t<detail::maybe_const<Const, Vs>>&>()))) &&
            (std::is_nothrow_move_constructible_v<range_rvalue_reference_t<detail::maybe_const<Const, First>>> && ... &&
             std::is_nothrow_move_constructible_v<range_rvalue_reference_t<detail::maybe_const<Const, Vs>>>)) {
            return detail::tuple_transform([](const auto& box) -> decltype(auto) { return detail::relaxed_iter_move(*box); }, i.current_);
        }

        friend constexpr void iter_swap(const iterator& l, const iterator& r) noexcept(
            (noexcept(detail::relaxed_iter_swap(std::declval<const iterator_t<detail::maybe_const<Const, First>>&>(),
                                                std::declval<const iterator_t<detail::maybe_const<Const, First>>&>())) &&
             ... &&
             noexcept(detail::relaxed_iter_swap(std::declval<const iterator_t<detail::maybe_const<Const, Vs>>&>(),
                                                std::declval<const iterator_t<detail::maybe_const<Const, Vs>>&>()))))
            requires(detail::indirectly_swappable<iterator_t<detail::maybe_const<Const, First>>> && ... &&
                     detail::indirectly_swappable<iterator_t<detail::maybe_const<Const, Vs>>>)
        {
            [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
                (detail::relaxed_iter_swap(*std::get<Ns>(l.current_), *std::get<Ns>(r.current_)), ...);
            }(std::make_index_sequence<last + 1>{});
        }
    };

    template <bool Const, class Self> static constexpr auto begin_(Self& self) {
        using current_t = typename iterator<Const>::current_t;
        return iterator<Const>(self, std::apply([](auto&... bases) { return current_t(zr::ranges::begin(bases)...); }, self.bases_));
    }

    template <bool Const, class Self> static constexpr auto end_(Self& self) {
        using current_t = typename iterator<Const>::current_t;
        return std::apply(
            [&](auto& first, auto&... rest) {
                const bool is_empty = (zr::ranges::empty(rest) || ...);
                auto first_it = is_empty ? zr::ranges::begin(first) : detail::cartesian_common_arg_end(first);
                return iterator<Const>(self, current_t(std::move(first_it), zr::ranges::begin(rest)...));
            },
            self.bases_);
    }

    template <class Self> static constexpr auto size_(Self& self) {
        return std::apply(
            [](auto&... bases) {
                using CT = detail::make_unsigned_like_t<std::common_type_t<std::size_t, decltype(zr::ranges::size(bases))...>>;
                return (CT(1) * ... * static_cast<CT>(zr::ranges::size(bases)));
            },
            self.bases_);
    }

public:
    constexpr cartesian_product_view() = default;
    constexpr explicit cartesian_product_view(First first_base, Vs... bases)
        : bases_(std::move(first_base), std::move(bases)...) {}

    constexpr iterator<false> begin()
        requires(!detail::simple_view<First> || ... || !detail::simple_view<Vs>)
    {
        return begin_<false>(*this);
    }
    constexpr iterator<true> begin() const
        requires(range<const First> && ... && range<const Vs>)
    {
        return begin_<true>(*this);
    }

    constexpr iterator<false> end()
        requires((!detail::simple_view<First> || ... || !detail::simple_view<Vs>) && detail::cartesian_product_is_common<First, Vs...>)
    {
        return end_<false>(*this);
    }
    constexpr iterator<true> end() const
        requires detail::cartesian_product_is_common<const First, const Vs...>
    {
        return end_<true>(*this);
    }
    constexpr std::default_sentinel_t end() const noexcept { return std::default_sentinel; }

    constexpr auto size()
        requires detail::cartesian_product_is_sized<First, Vs...>
    {
        return size_(*this);
    }
    constexpr auto size() const
        requires detail::cartesian_product_is_sized<const First, const Vs...>
    {
        return size_(*this);
    }
};

template <class... Vs> cartesian_product_view(Vs&&...) -> cartesian_product_view<views::all_t<Vs>...>;

namespace detail {

template <class R>
concept cartesian_traits_common_arg = common_range<R> || (sized_range<R> && random_access_range<R>);

template <class First, class... Vs> constexpr range_category cartesian_traits_category() {
    if constexpr (random_access_range<First> && ((random_access_range<Vs> && sized_range<Vs>) && ...)) {
        return range_category::random_access;
    } else if constexpr (bidirectional_range<First> && ((bidirectional_range<Vs> && cartesian_traits_common_arg<Vs>) && ...)) {
        return range_category::bidirectional;
    } else if constexpr (forward_range<First>) {
        return range_category::forward;
    } else {
        return range_category::input;
    }
}

} // namespace detail

template <class First, class... Vs> struct range_traits_impl<cartesian_product_view<First, Vs...>> {
    static constexpr range_category category = detail::cartesian_traits_category<First, Vs...>();

private:
    // Mixed radix, last base fastest.
    template <class Self, class Index> static constexpr auto element_at_(Self& v, Index n) {
        constexpr bool c = std::is_const_v<Self>;
        using reference = std::tuple<range_reference_t<detail::maybe_const<c, First>>, range_reference_t<detail::maybe_const<c, Vs>>...>;
        return [&]<std::size_t... Ns>(std::index_sequence<Ns...>) {
            constexpr std::size_t bases = sizeof...(Ns);
            std::size_t sizes[bases] = {static_cast<std::size_t>(zr::ranges::size(std::get<Ns>(v.bases_)))...};
            std::size_t digits[bases] = {};
            auto rest = static_cast<std::size_t>(n);
            for (std::size_t k = bases; k-- > 0;) {
                digits[k] = (k == 0) ? rest : rest % sizes[k];
                rest = (k == 0) ? 0 : rest / sizes[k];
            }
            return reference(zr::ranges::element_at(std::get<Ns>(v.bases_), digits[Ns])...);
        }(std::index_sequence_for<First, Vs...>{});
    }

public:
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && sized_range<First>
    static constexpr auto element_at(cartesian_product_view<First, Vs...>& v, Index n) {
        return element_at_(v, n);
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && sized_range<const First> && (sized_range<const Vs> && ...) &&
                detail::const_element_at<First> && (detail::const_element_at<Vs> && ...)
    static constexpr auto element_at(const cartesian_product_view<First, Vs...>& v, Index n) {
        return element_at_(v, n);
    }
};

template <class First, class... Vs>
struct range_traits_impl<const cartesian_product_view<First, Vs...>> : range_traits_impl<cartesian_product_view<First, Vs...>> {
    static constexpr range_category category = detail::cartesian_traits_category<const First, const Vs...>();
};


namespace detail {

// [range.cartesian.overview]
struct cartesian_product_fn {
    constexpr auto operator()() const noexcept { return views::single(std::tuple()); }

    template <class... Es>
        requires(sizeof...(Es) > 0) && requires { cartesian_product_view<views::all_t<Es>...>(std::declval<Es>()...); }
    constexpr auto operator()(Es&&... es) const noexcept(noexcept(cartesian_product_view<views::all_t<Es>...>(std::forward<Es>(es)...))) {
        return cartesian_product_view<views::all_t<Es>...>(std::forward<Es>(es)...);
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::cartesian_product_fn cartesian_product{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
