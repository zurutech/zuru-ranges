// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/non_propagating_cache.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/subrange.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file chunk_by_view.h
 *
 * @brief [range.chunk.by] chunk_by_view, views::chunk_by.
 *
 * find-next and find-prev are the loops the draft's adjacent_find / reverse_view formulations
 * specify, advancing iterators in place. The draft's defaulted operator++(int) and
 * operator--(int) ([over.inc.default], C++26) are written out.
 *
 * DEVIATION: the iterator holds current_ and next_ in movable-boxes, so it stays assignable over
 * UE's debug array iterator, which is not; its default constructor requires
 * default_initializable<iterator_t<V>> (Unreal iterators are not), which the draft's
 * value-initialized members imply.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <forward_range V, detail::indirect_binary_predicate<iterator_t<V>, iterator_t<V>> Pred>
    requires view<V> && std::is_object_v<Pred>
class chunk_by_view : public view_interface<chunk_by_view<V, Pred>> {
private:
    V base_ = V();
    ZR_NO_UNIQUE_ADDRESS movable_box<Pred> pred_;

    // [range.chunk.by.view] begin(): "caches the result"; the cached value is find-next(begin).
    ZR_NO_UNIQUE_ADDRESS detail::non_propagating_cache<iterator_t<V>> begin_next_;

    class iterator;

    // ranges::next(ranges::adjacent_find(current, end, not_fn(ref(pred))), 1, end).
    constexpr iterator_t<V> find_next_(iterator_t<V> current) {
        const auto last = zr::ranges::end(base_);
        if (detail::iter_equal(current, last)) {
            return current;
        }
        iterator_t<V> next(current);
        ++next;
        while (!detail::iter_equal(next, last) && std::invoke(*pred_, detail::deref(current), detail::deref(next))) {
            ++current;
            ++next;
        }
        return next;
    }

    // The first i in [begin, current) from which [i, current) has no adjacent pair failing
    // pred, and !pred(*prev(i), *i) unless i is begin.
    // DEVIATION: also requires base_decrementable<V> (adaptor_helpers.h).
    constexpr iterator_t<V> find_prev_(iterator_t<V> current)
        requires detail::base_decrementable<V>
    {
        const auto first = zr::ranges::begin(base_);
        --current;
        while (!detail::iter_equal(current, first)) {
            iterator_t<V> prev(current);
            --prev;
            if (!std::invoke(*pred_, detail::deref(prev), detail::deref(current))) {
                break;
            }
            --current;
        }
        return current;
    }

public:
    chunk_by_view()
        requires std::default_initializable<V> && std::default_initializable<Pred>
    = default;

    // Parameters named apart from base() and pred(): GCC < 14 -Wshadow.
    constexpr explicit chunk_by_view(V base_view, Pred predicate)
        : base_(std::move(base_view))
        , pred_(std::move(predicate)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr const Pred& pred() const { return *pred_; }

    constexpr iterator begin() {
        if (!begin_next_) {
            begin_next_.emplace(find_next_(zr::ranges::begin(base_)));
        }
        return iterator(*this, zr::ranges::begin(base_), *begin_next_);
    }

    constexpr auto end() {
        if constexpr (common_range<V>) {
            return iterator(*this, zr::ranges::end(base_), zr::ranges::end(base_));
        } else {
            return std::default_sentinel;
        }
    }
};

template <class R, class Pred> chunk_by_view(R&&, Pred) -> chunk_by_view<views::all_t<R>, Pred>;

template <forward_range V, detail::indirect_binary_predicate<iterator_t<V>, iterator_t<V>> Pred>
    requires view<V> && std::is_object_v<Pred>
class chunk_by_view<V, Pred>::iterator {
private:
    friend chunk_by_view;

    chunk_by_view* parent_ = nullptr;
    movable_box<iterator_t<V>> current_;
    movable_box<iterator_t<V>> next_;

    constexpr iterator(chunk_by_view& parent, iterator_t<V> current, iterator_t<V> next)
        : parent_(std::addressof(parent))
        , current_(std::in_place, std::move(current))
        , next_(std::in_place, std::move(next)) {}

    static constexpr auto concept_tag() {
        if constexpr (detail::base_decrementable<V>) {
            return std::bidirectional_iterator_tag{};
        } else {
            return std::forward_iterator_tag{};
        }
    }

public:
    using value_type = subrange<iterator_t<V>>;
    using difference_type = range_difference_t<V>;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = decltype(concept_tag());

    iterator()
        requires std::default_initializable<iterator_t<V>>
    = default;

    constexpr value_type operator*() const { return value_type(*current_, *next_); }

    constexpr iterator& operator++() {
        current_ = next_;
        next_ = movable_box<iterator_t<V>>(std::in_place, parent_->find_next_(*current_));
        return *this;
    }
    constexpr iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--()
        requires detail::base_decrementable<V>
    {
        next_ = current_;
        current_ = movable_box<iterator_t<V>>(std::in_place, parent_->find_prev_(*next_));
        return *this;
    }
    constexpr iterator operator--(int)
        requires detail::base_decrementable<V>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) { return detail::iter_equal(*x.current_, *y.current_); }

    friend constexpr bool operator==(const iterator& x, std::default_sentinel_t) { return detail::iter_equal(*x.current_, *x.next_); }
};

template <class V, class Pred> struct range_traits_impl<chunk_by_view<V, Pred>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::bidirectional);
};

// const chunk_by_view has no begin(): not a range.
template <class V, class Pred> struct range_traits_impl<const chunk_by_view<V, Pred>> {};

namespace detail {

// [range.chunk.by.overview]
struct chunk_by_fn {
    template <class E, class F>
        requires requires { chunk_by_view(std::declval<E>(), std::declval<F>()); }
    constexpr auto operator()(E&& e, F&& f) const noexcept(noexcept(chunk_by_view(std::forward<E>(e), std::forward<F>(f)))) {
        return chunk_by_view(std::forward<E>(e), std::forward<F>(f));
    }

    template <class F>
        requires std::constructible_from<std::decay_t<F>, F>
    constexpr auto operator()(F&& f) const noexcept(std::is_nothrow_constructible_v<std::decay_t<F>, F>) {
        return make_bound_closure(*this, std::forward<F>(f));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::chunk_by_fn chunk_by{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
