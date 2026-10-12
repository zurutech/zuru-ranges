// Conformance regression tests, one per fixed requirement, both configurations. std adaptors and
// algorithms over zr views: std_interop.cpp.
#include "CoreMinimal.h"
#include "Containers/List.h"
#include "Containers/StridedView.h"

#include "zr/ranges.h"
#include "zr/unreal/ue_range_traits.h"

#include <array>
#include <cstddef>
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

// Clang < 16 cannot instantiate libstdc++'s std views; libc++ < 16 has them experimental only
// (see test/CMakeLists.txt).
#if (defined(__clang__) && __clang_major__ < 16 && !defined(_LIBCPP_VERSION)) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 160000)
#define ZR_TEST_STD_VIEWS 0
#else
#define ZR_TEST_STD_VIEWS 1
#endif

// The MSVC STL after 14.29 and before 14.37 specializes basic_common_reference for tuple
// (P2321R2) without the tuple(tuple<U...>&) constructor, so no tuple-of-references iterator is
// std::indirectly_readable there (see std_interop.cpp).
#if defined(_MSVC_STL_UPDATE) && _MSVC_STL_UPDATE > 202105L && _MSVC_STL_UPDATE < 202305L
#define ZR_TEST_STD_TUPLE_PROXIES 0
#else
#define ZR_TEST_STD_TUPLE_PROXIES 1
#endif

namespace {

using vec = std::vector<int>;
using istream_it = std::istream_iterator<int>;

constexpr bool is_even(int x) { return x % 2 == 0; }
constexpr int twice(int x) { return x * 2; }

template <class F> struct counting_pred {
    int* calls;
    F f;
    bool operator()(int x) const {
        ++*calls;
        return f(x);
    }
};

template <class R> std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

// a user view whose iterators are pointers.
struct pointer_view : view_interface<pointer_view> {
    int* first = nullptr;
    int* last = nullptr;
    int* begin() const { return first; }
    int* end() const { return last; }
};

// size() only on the non-const object; iterators not sized sentinels.
struct nonconst_size {
    std::list<int> list;
    std::list<int>::const_iterator begin() const { return list.begin(); }
    std::list<int>::const_iterator end() const { return list.end(); }
    int size() { return 0; }
};

struct move_only_pred {
    move_only_pred() = default;
    move_only_pred(move_only_pred&&) = default;
    move_only_pred& operator=(move_only_pred&&) = default;
    bool operator()(int) const { return true; }
};

struct not_callable {};

// Detection idioms: dependent requires-expressions.
template <class R, class P> concept can_filter = requires { views::filter(std::declval<R>(), std::declval<P>()); };
template <class R, class F> concept can_transform = requires { views::transform(std::declval<R>(), std::declval<F>()); };
template <class T> concept can_empty = requires { zr::ranges::empty(std::declval<T>()); };
template <class T> concept can_all = requires { views::all(std::declval<T>()); };
template <class T> concept can_single = requires { views::single(std::declval<T>()); };
template <class C, class R> concept can_to = requires { zr::ranges::to<C>(std::declval<R>()); };
template <class I> concept has_iterator_category = requires { typename I::iterator_category; };
template <class R> concept can_enumerate = requires { views::enumerate(std::declval<R>()); };
template <class T, class... A> concept implicitly_constructible = requires(void (*fn)(T)) { fn({std::declval<A>()...}); };

// ADL begin/end taking const& only ([range.access.begin], P2602R2).
namespace adl {
struct only_const {};
[[maybe_unused]] const int* begin(const only_const&) { return nullptr; }
[[maybe_unused]] const int* end(const only_const&) { return nullptr; }
} // namespace adl

// a closure with a non-const call operator.
struct nonconst_call : range_adaptor_closure<nonconst_call> {
    template <class R> int operator()(R&& r) { return static_cast<int>(zr::ranges::size(r)); } // NOLINT(cppcoreguidelines-missing-std-forward)
};

// a range whose reference is not move-constructible.
struct pinned {
    explicit pinned(int) {}
    pinned(pinned&&) = delete;
};
struct pinned_range {
    struct iterator {
        using value_type = int;
        using difference_type = std::ptrdiff_t;
        pinned operator*() const { return pinned(0); }
        iterator& operator++() { return *this; }
        void operator++(int) {}
        bool operator==(std::default_sentinel_t) const { return true; }
    };
    iterator begin() const { return {}; }
    std::default_sentinel_t end() const { return {}; }
};

} // namespace

TEST(Conformance, ReverseOfBorrowedIsBorrowedAndSafe) {
    vec v{1, 2, 3};
    static_assert(borrowed_range<decltype(views::reverse(v))>);
    EXPECT_EQ(*zr::ranges::find(views::reverse(v), 3), 3);
    TArray<int> a{1, 2, 3};
    static_assert(borrowed_range<decltype(views::reverse(a))>);
    EXPECT_EQ(*zr::ranges::find(views::reverse(a), 2), 2);
}

TEST(Conformance, AdaptorsNameTheirViewType) {
    std::vector<std::vector<vec>> vvv{{{1, 2}}, {{3}}};
    static_assert(std::is_same_v<range_reference_t<decltype(vvv | views::join | views::join)>, int&>);
    EXPECT_EQ(collect(vvv | views::join | views::join), (vec{1, 2, 3}));

    vec v{1, 2, 3};
    using e = decltype(views::enumerate(v));
    static_assert(!std::is_same_v<decltype(views::enumerate(std::declval<e>())), e>);
    static_assert(std::is_same_v<decltype(v | views::reverse | views::reverse), ref_view<vec>>);
    EXPECT_EQ(collect(v | views::reverse | views::reverse), v);

    using z = decltype(views::zip(v));
    static_assert(std::is_same_v<range_reference_t<decltype(views::zip(std::declval<z>()))>, std::tuple<std::tuple<int&>>>);
    using c = decltype(views::cartesian_product(v));
    static_assert(std::is_same_v<range_reference_t<decltype(views::cartesian_product(std::declval<c>()))>, std::tuple<std::tuple<int&>>>);

    // [range.reverse.overview]: reverse of subrange<reverse_iterator<I>> is subrange<I>.
    auto rs = subrange(std::make_reverse_iterator(v.end()), std::make_reverse_iterator(v.begin()));
    static_assert(std::is_same_v<decltype(views::reverse(rs)), subrange<vec::iterator, vec::iterator, subrange_kind::sized>>);
}

TEST(Conformance, EnumerateEndIndex) {
    vec v{1, 2, 3};
    EXPECT_EQ(views::enumerate(v).end().index(), 3);
    TArray<int> a{1, 2, 3};
    EXPECT_EQ(views::enumerate(a).end().index(), 3);
}

TEST(Conformance, JoinOverInputIsInput) {
    std::istringstream in("ab cd");
    using s = subrange<std::istream_iterator<std::string>, std::default_sentinel_t>;
    using j = join_view<s>;
    static_assert(input_range<j> && !forward_range<j>);
    std::string out;
    for (char ch : s(std::istream_iterator<std::string>(in), std::default_sentinel) | views::join) {
        out += ch;
    }
    EXPECT_EQ(out, "abcd");
}

TEST(Conformance, SubrangeCategory) {
    static_assert(input_range<subrange<istream_it, std::default_sentinel_t>> && !forward_range<subrange<istream_it, std::default_sentinel_t>>);
    static_assert(contiguous_range<subrange<int*>> && sized_range<subrange<int*>>);
    static_assert(bidirectional_range<subrange<std::list<int>::iterator>> && !sized_range<subrange<std::list<int>::iterator>>);
}

TEST(Conformance, EmptyRequiresForwardAndPrefersMember) {
#if ZR_TEST_STD_VIEWS
    using istream_view = std::ranges::basic_istream_view<int, char, std::char_traits<char>>;
    static_assert(!can_empty<istream_view&>);
    static_assert(!std::is_constructible_v<bool, ref_view<istream_view>>);
#endif
    struct member_empty {
        int* begin() const { return nullptr; }
        int* end() const { return nullptr; }
        bool empty() const { return false; } // deliberately disagrees with begin == end
    };
    EXPECT_FALSE(zr::ranges::empty(member_empty{}));
}

TEST(Conformance, ValueType) {
    static_assert(std::is_same_v<range_value_t<const std::array<int, 3>>, int>);
    static_assert(std::is_same_v<range_value_t<std::vector<bool>>, bool>);
    static_assert(std::is_same_v<range_value_t<decltype(views::zip(std::declval<vec&>()))>, std::tuple<int>>);
}

TEST(Conformance, FilterOverUnboundedRange) {
    vec out;
    for (int x : views::filter(views::iota(0), is_even)) {
        if (x > 6) {
            break;
        }
        out.push_back(x);
    }
    EXPECT_EQ(out, (vec{0, 2, 4, 6}));
}

TEST(Conformance, FilterCachesBegin) {
    vec v{1, 3, 4, 5};
    int calls = 0;
    auto f = views::filter(v, counting_pred<decltype(&is_even)>{&calls, &is_even});
    (void)f.begin();
    const int first = calls;
    (void)f.begin();
    EXPECT_EQ(calls, first);
    static_assert(!range<const decltype(f)>);
}

TEST(Conformance, TransformSentinelSemiregular) {
    using t = decltype(views::transform(views::iota(0), twice));
    static_assert(!common_range<t>);
    static_assert(std::semiregular<sentinel_t<t>>);
    static_assert(std::ranges::range<t>);
}

TEST(Conformance, ZipWithUnbounded) {
    vec v{1, 2, 3};
    int sum = 0;
    for (auto [x, i] : views::zip(v, views::iota(0))) {
        sum += x * i;
    }
    EXPECT_EQ(sum, 8);
}

TEST(Conformance, ViewInterfaceWithPointers) {
    int arr[3]{1, 2, 3};
    pointer_view p;
    p.first = arr;
    p.last = arr + 3;
    static_assert(contiguous_range<pointer_view> && sized_range<pointer_view>);
    EXPECT_EQ(p.size(), 3u);
    EXPECT_EQ(p.data(), arr);
    EXPECT_EQ(p.back(), 3);
}

TEST(Conformance, ToRecursesFirst) {
    std::vector<std::list<int>> src{{1, 2}, {3}};
    auto out = zr::ranges::to<std::vector<vec>>(src);
    EXPECT_EQ(out, (std::vector<vec>{{1, 2}, {3}}));
}

TEST(Conformance, StoredClosuresCopy) {
    vec v{1, 2, 3, 4};
    auto f = views::filter(is_even);
    auto t = f | views::transform(twice);
    EXPECT_EQ(collect(v | t), (vec{4, 8}));
    auto c = zr::ranges::to<vec>();
    EXPECT_EQ(v | c, v);
}

TEST(Conformance, SimpleViewsShareIterators) {
    using r = ref_view<vec>;
    static_assert(std::is_same_v<iterator_t<zip_view<r>>, iterator_t<const zip_view<r>>>);
    static_assert(std::is_same_v<iterator_t<enumerate_view<r>>, iterator_t<const enumerate_view<r>>>);
    static_assert(std::is_same_v<iterator_t<concat_view<r, r>>, iterator_t<const concat_view<r, r>>>);
    static_assert(std::is_same_v<iterator_t<cartesian_product_view<r, r>>, iterator_t<const cartesian_product_view<r, r>>>);
    static_assert(std::is_same_v<iterator_t<join_view<ref_view<std::vector<vec>>>>, iterator_t<const join_view<ref_view<std::vector<vec>>>>>);
}

TEST(Conformance, PostfixIncrementOnUnrealForward) {
    TSet<int> s{1, 2};
    auto z = views::zip(s);
    auto it = z.begin();
    static_assert(std::is_same_v<decltype(it++), decltype(it)>);
    auto e = views::enumerate(s);
    auto ei = e.begin();
    static_assert(std::is_same_v<decltype(ei++), decltype(ei)>);
    auto f = views::filter(s, is_even);
    auto fi = f.begin();
    static_assert(std::is_same_v<decltype(fi++), decltype(fi)>);
    auto t = views::transform(s, twice);
    auto ti = t.begin();
    static_assert(std::is_same_v<decltype(ti++), decltype(ti)>);
}

TEST(Conformance, FullCategoriesOverStdBases) {
#if ZR_TEST_STD_TUPLE_PROXIES
    static_assert(std::ranges::random_access_range<decltype(views::zip(std::declval<vec&>(), std::declval<vec&>()))>);
    static_assert(std::ranges::random_access_range<decltype(views::enumerate(std::declval<vec&>()))>);
    static_assert(std::ranges::random_access_range<decltype(views::cartesian_product(std::declval<vec&>(), std::declval<vec&>()))>);
#endif
    static_assert(std::ranges::random_access_range<decltype(views::transform(std::declval<vec&>(), twice))>);
    static_assert(std::ranges::bidirectional_range<decltype(views::filter(std::declval<vec&>(), is_even))>);
    static_assert(std::ranges::random_access_range<decltype(views::reverse(std::declval<vec&>()))>);
    static_assert(std::ranges::random_access_range<decltype(views::concat(std::declval<vec&>(), std::declval<vec&>()))>);
    static_assert(std::ranges::bidirectional_range<decltype(views::join(std::declval<std::vector<vec>&>()))>);
    static_assert(std::ranges::bidirectional_range<decltype(views::reverse(std::declval<std::list<int>&>()))>);
    // enumerate: <=> and - need only the index.
    TSet<int> s{1, 2};
    auto e = views::enumerate(s);
    auto second = e.begin();
    ++second;
    EXPECT_EQ(second - e.begin(), 1);
    EXPECT_TRUE(e.begin() < second);
}

TEST(Conformance, ConstConversionAndMixedComparison) {
    vec v{1, 2};
    std::vector<vec> vv{{1}, {2}};
    auto z = views::zip(v, v);
    EXPECT_TRUE(z.end() == std::as_const(z).end());
    auto j = views::join(vv);
    EXPECT_TRUE(j.end() == std::as_const(j).end());
    auto c = views::concat(v, v);
    EXPECT_TRUE(c.end() == std::as_const(c).end());
    auto cp = views::cartesian_product(v, v);
    EXPECT_TRUE(cp.begin() == std::as_const(cp).begin());
}

TEST(Conformance, Sentinels) {
    using e = decltype(views::enumerate(views::iota(0)));
    static_assert(std::default_initializable<sentinel_t<e>>);
    static_assert(std::is_same_v<decltype(std::declval<e&>().end().base()), std::unreachable_sentinel_t>);
    using t = decltype(views::transform(std::declval<subrange<int*, const int*>>(), twice));
    static_assert(!common_range<t> && std::sized_sentinel_for<sentinel_t<t>, iterator_t<t>>);
    static_assert(std::is_convertible_v<sentinel_t<t>, sentinel_t<const t>>);
}

TEST(Conformance, ConstexprSize) {
    static constexpr std::array<int, 3> a{1, 2, 3};
    static constexpr std::array<int, 2> b{4, 5};
    static_assert(views::zip(a, b).size() == 2);
    static_assert(views::enumerate(a).size() == 3);
    static_assert(views::transform(a, twice).size() == 3);
    static_assert(views::reverse(a).size() == 3);
    static_assert(views::concat(a, b).size() == 5);
    static_assert(views::cartesian_product(a, b).size() == 6);
}

TEST(Conformance, ConstAwareTraits) {
    static_assert(sized_range<nonconst_size> && !sized_range<const nonconst_size>);
}

TEST(Conformance, AdaptorsAreSfinaeFriendly) {
    static_assert(!can_transform<vec&, not_callable>);
    static_assert(!can_filter<vec&, not_callable>);
    static_assert(!can_filter<vec&, move_only_pred>);
    static_assert(!can_single<pinned>);
    static_assert(!can_all<int>);
}

TEST(Conformance, CposNoexcept) {
    static_assert(noexcept(zr::ranges::begin(std::declval<vec&>())));
    static_assert(noexcept(zr::ranges::end(std::declval<vec&>())));
    static_assert(noexcept(zr::ranges::size(std::declval<vec&>())));
    static_assert(noexcept(zr::ranges::empty(std::declval<vec&>())));
    static_assert(noexcept(zr::ranges::data(std::declval<vec&>())));
    static_assert(noexcept(views::all(std::declval<vec&>())));
    static_assert(noexcept(views::single(1)) == noexcept(single_view<int>(1)));
    static_assert(noexcept(views::empty<int>.begin()));
}

TEST(Conformance, Zip) {
    static_assert(std::default_initializable<zip_view<views::all_t<std::span<int>>>>);
    static_assert(std::is_same_v<decltype(views::zip()), empty_view<std::tuple<>>>);
    using s = subrange<istream_it, std::default_sentinel_t>;
    using z = zip_view<s>;
    static_assert(!has_iterator_category<iterator_t<z>>);
    static_assert(has_iterator_category<iterator_t<decltype(views::zip(std::declval<vec&>()))>>);
}

TEST(Conformance, Enumerate) {
    static_assert(range<pinned_range> && !can_enumerate<pinned_range>);
    TSet<int> s{1};
    using e = decltype(views::enumerate(s));
    static_assert(std::is_same_v<std::iter_value_t<iterator_t<e>>, std::tuple<std::ptrdiff_t, int>>);
    static_assert(!std::is_constructible_v<iterator_t<decltype(views::enumerate(std::declval<vec&>()))>, vec::iterator, std::ptrdiff_t>);
    static_assert(noexcept(std::declval<iterator_t<e>>() == std::declval<iterator_t<e>>()));
}

TEST(Conformance, CartesianProduct) {
    using unbounded = decltype(views::cartesian_product(views::iota(0)));
    static_assert(noexcept(std::declval<const unbounded&>().end()));
    static_assert(std::is_same_v<decltype(std::declval<const unbounded&>().end()), std::default_sentinel_t>);
    using io = decltype(views::iota(0, 3L));
    static_assert(common_range<decltype(views::cartesian_product(std::declval<io>(), std::declval<io>()))>);
    vec v{1, 2};
    auto cp = views::cartesian_product(v, v);
    using it = iterator_t<decltype(cp)>;
    static_assert(std::is_same_v<it::reference, std::tuple<int&, int&>>);
    auto i = cp.begin() + 3;
    EXPECT_EQ(std::get<0>(*i), 2);
    EXPECT_EQ(std::get<1>(*--i), 1);
    EXPECT_EQ(std::default_sentinel - cp.begin(), 4);
}

TEST(Conformance, Join) {
    auto prvalue_inners = views::iota(1, 4) | views::transform([](int n) { return vec(static_cast<std::size_t>(n), n); }) | views::join;
    EXPECT_EQ(collect(prvalue_inners), (vec{1, 2, 2, 3, 3, 3}));
    std::vector<vec> vv{{1, 2}, {}, {3}};
    auto j = views::join(vv);
    static_assert(common_range<decltype(j)>);
    static_assert(std::is_same_v<range_difference_t<decltype(j)>, std::ptrdiff_t>);
    auto last = j.end();
    EXPECT_EQ(*--last, 3);
    std::vector<std::vector<std::string>> vs{{"a"}};
    EXPECT_EQ(views::join(vs).begin()->size(), 1u);
}

TEST(Conformance, Reverse) {
    std::list<int> l{1, 2, 3};
    EXPECT_EQ(collect(views::reverse(l)), (vec{3, 2, 1}));
    static_assert(sized_range<decltype(views::reverse(l))>);
    EXPECT_EQ(views::reverse(l).size(), 3u);
    vec v{3, 1, 2};
    auto r = views::reverse(v);
    static_assert(std::ranges::random_access_range<decltype(r)>);
}

TEST(Conformance, Concat) {
    vec v{1};
    vec w{2, 3};
    auto c = concat_view(v, w);
    static_assert(std::is_same_v<decltype(c), concat_view<ref_view<vec>, ref_view<vec>>>);
    static_assert(std::default_initializable<concat_view<views::all_t<std::span<int>>>>);
    EXPECT_EQ(c[2], 3);
    EXPECT_EQ(std::default_sentinel - c.begin(), 3);
    EXPECT_EQ(c.begin() - std::default_sentinel, -3);
    auto i = c.begin();
    i += 2;
    EXPECT_EQ(*i, 3);
    std::list<int> l{4};
    using nc = decltype(views::concat(views::iota(0, 1L), l));
    static_assert(!std::ranges::bidirectional_range<nc>);
}

TEST(Conformance, Filter) {
    vec v{1, 2, 3, 4};
    auto f = views::filter(v, is_even);
    static_assert(!implicitly_constructible<decltype(f), ref_view<vec>, decltype(&is_even)>);
    EXPECT_EQ(f.pred()(2), true);
    EXPECT_EQ(*--f.end(), 4);
#if ZR_TEST_STD_VIEWS
    using iota_filter = decltype(views::filter(std::views::iota(0, 10), is_even));
    static_assert(std::is_same_v<std::iterator_traits<iterator_t<iota_filter>>::iterator_category, std::input_iterator_tag>);
#endif
    std::vector<std::string> s{"ab"};
    auto fs = views::filter(s, [](const std::string&) { return true; });
    EXPECT_EQ(fs.begin()->size(), 2u);
}

TEST(Conformance, Transform) {
    vec v{1, 2, 3};
    auto t = views::transform(v, [](int& x) -> int& { return x; });
    static_assert(std::is_same_v<std::iterator_traits<iterator_t<decltype(t)>>::iterator_category, std::random_access_iterator_tag>);
    auto p = views::transform(v, twice);
    static_assert(std::is_same_v<std::iterator_traits<iterator_t<decltype(p)>>::iterator_category, std::input_iterator_tag>);
    EXPECT_EQ(p.reserve_hint(), 3u);
}

TEST(Conformance, AllOwningRef) {
    static_assert(!can_all<std::initializer_list<int>>);
    static_assert(std::is_same_v<decltype(std::declval<owning_view<vec>>().base()), vec&&>);
    static_assert(std::is_same_v<views::all_t<std::span<int>>, std::span<int>>);
    static_assert(std::is_same_v<views::all_t<std::string_view>, std::string_view>);
}

TEST(Conformance, Subrange) {
    vec v{1, 2, 3, 4};
    subrange s(v);
    auto [b, e] = s;
    EXPECT_EQ(e - b, 4);
    std::pair<vec::iterator, vec::iterator> p = s;
    EXPECT_EQ(p.first, v.begin());
    EXPECT_EQ(s.next().size(), 3u);
    EXPECT_EQ(s.next(2).prev().front(), 2);
    std::list<int> l{1, 2, 3};
    subrange ls(l.begin(), l.end(), 3);
    EXPECT_EQ(ls.size(), 3u);
    EXPECT_EQ(ls.advance(1).size(), 2u);
    static_assert(std::tuple_size_v<decltype(s)> == 2);
}

TEST(Conformance, EmptySingle) {
    static_assert(empty_view<int>::size() == 0);
    static_assert(empty_view<int>::empty());
    static_assert(noexcept(empty_view<int>::begin()));
    single_view<std::pair<int, int>> sp(std::in_place, 1, 2);
    EXPECT_EQ(sp.data()->second, 2);
    static_assert(single_view<int>::size() == 1 && !single_view<int>::empty());
    int x = 1;
    auto ref_lambda = [&x] { return x; };
    static_assert(view<single_view<decltype(ref_lambda)>>);
    struct no_copy {
        no_copy(int) {}
        no_copy(no_copy&&) = default;
    };
    static_assert(!std::is_constructible_v<single_view<no_copy>, const no_copy&>);
}

TEST(Conformance, MaybeViewable) {
    using cvo = const std::vector<std::optional<int>>;
    static_assert(!can_maybe_view<cvo&&>);
    static_assert(can_maybe_view<cvo&>);
}

TEST(Conformance, RangeAccess) {
#if !((defined(_MSC_VER) && !defined(__clang__)) || (defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 12))
    // MSVC and GCC 11 keep the pre-P2602R2 poison pills (range_access.h), which reject this.
    static_assert(range<adl::only_const>);
#endif
    static_assert(requires { zr::ranges::data(std::span<int>{}); });
    vec v{1, 2, 3};
    EXPECT_EQ(*zr::ranges::rbegin(v), 3);
    EXPECT_EQ(zr::ranges::ssize(v), 3);
    static_assert(std::is_signed_v<decltype(zr::ranges::ssize(v))>);
    EXPECT_EQ(zr::ranges::reserve_hint(v), 3u);
    EXPECT_EQ(*zr::ranges::cbegin(v), 1);
    static_assert(std::is_same_v<decltype(zr::ranges::cdata(v)), const int*>);
    static_assert(std::is_same_v<const_iterator_t<vec>, vec::const_iterator>);
    static_assert(constant_range<const vec> && !constant_range<vec>);
}

TEST(Conformance, ViewInterface) {
    vec v{1, 2, 3};
    const auto r = views::all(v);
    EXPECT_EQ(r.back(), 3);
    EXPECT_EQ(views::iota(0)[3], 3);
    EXPECT_EQ(r.at(1), 2);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    EXPECT_THROW((void)r.at(3), std::out_of_range);
#endif
    EXPECT_EQ(*r.cbegin(), 1);
    static_assert(!std::ranges::enable_view<const ref_view<vec>>);
}

TEST(Conformance, To) {
    struct move_only_arg {
        move_only_arg() = default;
        move_only_arg(move_only_arg&&) = default;
    };
    struct takes_arg {
        takes_arg(move_only_arg) {}
        void push_back(int) {}
    };
    vec v{1, 2};
    (void)(v | zr::ranges::to<takes_arg>(move_only_arg{}));
    static_assert(!can_to<std::string_view, std::string&>);
    auto d = zr::ranges::to<std::vector>(v);
    static_assert(std::is_same_v<decltype(d), vec>);
    auto d2 = v | zr::ranges::to<std::vector>();
    EXPECT_EQ(d2, v);
}

TEST(Conformance, RangeAdaptor) {
    vec v{1, 2};
    nonconst_call c;
    EXPECT_EQ(v | c, 2);
    auto composed = views::all | c;
    EXPECT_EQ(v | composed, 2);
    struct range_closure : range_adaptor_closure<range_closure> {
        int* begin() const { return nullptr; }
        int* end() const { return nullptr; }
    };
    static_assert(!detail::closure<range_closure>);
}

TEST(Conformance, MovableBox) {
    struct copy_ctor_movable {
        copy_ctor_movable() = default;
        copy_ctor_movable(const copy_ctor_movable&) {} // not nothrow: primary movable_box
        copy_ctor_movable(copy_ctor_movable&&) = default;
        copy_ctor_movable& operator=(const copy_ctor_movable&) = delete;
        copy_ctor_movable& operator=(copy_ctor_movable&&) = default;
    };
    static_assert(std::is_move_assignable_v<movable_box<copy_ctor_movable>>);
}

TEST(Conformance, Algorithms) {
    vec v{1, 2, 2, 3};
    EXPECT_EQ(zr::ranges::count(v, 2), 2);
    EXPECT_EQ(*zr::ranges::find_if_not(v, [](int x) { return x < 2; }), 2);
    static_assert(std::is_same_v<borrowed_subrange_t<vec&>, subrange<vec::iterator>>);
    static_assert(std::is_same_v<borrowed_subrange_t<vec>, dangling>);
    TArray<int> a{1, 1};
    EXPECT_EQ(zr::ranges::count(a, 1), 2);
}

namespace {

// [indirectcallable.indirectinvocable]: no predicate<F&, iter_common_reference_t<I>>.
struct proxy_val {};
struct proxy_ref {
    operator proxy_val() const { return {}; }
};
struct proxy_iter {
    using value_type = proxy_val;
    using difference_type = std::ptrdiff_t;
    proxy_ref operator*() const { return {}; }
    proxy_iter& operator++() { return *this; }
    void operator++(int) {}
};
struct val_and_ref_only {
    bool operator()(proxy_val&) const { return true; }
    bool operator()(proxy_ref) const { return true; }
    template <class T> bool operator()(T) const = delete;
};

// const begin() of a lower category than begin().
struct split_category_view : view_interface<split_category_view> {
    std::forward_list<int>* list = nullptr;
    int* begin() { return nullptr; }
    int* end() { return nullptr; }
    std::forward_list<int>::const_iterator begin() const { return list->begin(); }
    std::forward_list<int>::const_iterator end() const { return list->end(); }
};

struct non_copyable {
    non_copyable() = default;
    non_copyable(const non_copyable&) = delete;
};

template <class E> concept can_reverse = requires { views::reverse(std::declval<E>()); };

} // namespace

TEST(Conformance, TupleProxyReferencesAreInputInCxx20) {
    const vec cv{1, 2};
    static_assert(input_range<decltype(views::enumerate(cv))> && input_range<decltype(views::zip(cv))>);
    auto t = views::enumerate(cv) | views::transform([](auto e) { return std::get<1>(e); });
    EXPECT_EQ(collect(t), (vec{1, 2}));
}

TEST(Conformance, PredicateNeedsNoCommonReference) {
    static_assert(detail::indirect_unary_predicate<val_and_ref_only, proxy_iter>);
}

TEST(Conformance, ConstViewsTakeConstBaseCategory) {
    using t = transform_view<split_category_view, int (*)(int)>;
    static_assert(random_access_range<t> && forward_range<const t> && !bidirectional_range<const t>);
    static_assert(!bidirectional_range<const enumerate_view<split_category_view>>);
    static_assert(!bidirectional_range<const zip_view<split_category_view>>);
}

TEST(Conformance, ReverseOfReverseIsSfinaeFriendly) {
    static_assert(!can_reverse<const reverse_view<owning_view<vec>>&>);
    static_assert(can_reverse<reverse_view<owning_view<vec>>>);
}

TEST(Conformance, MaybeRejectsNonCopyablePrvalueValues) {
    using r = decltype(views::iota(0, 1) | views::transform([](int) { return std::make_unique<non_copyable>(); }));
    static_assert(!can_maybe_view<r>);
}

TEST(Conformance, JoinOverUnrealIterators) {
    TArray<TArray<int>> nested;
    nested.Add(TArray<int>{1, 2});
    nested.Add(TArray<int>{3});
    auto j = views::join(nested);
    if constexpr (requires(iterator_t<decltype(j)> it) { --it; }) {
        auto it = j.begin();
        ++it;
        --it;
        EXPECT_EQ(*it, 1);
    }
    int sum = 0;
    for (auto [x, i] : views::zip(views::join(nested), views::iota(0))) {
        sum += x * i;
    }
    EXPECT_EQ(sum, 8);
}

TEST(Conformance, ConcatElementAtWithNarrowIndex) {
    vec a(65546);
    for (std::size_t k = 0; k < a.size(); ++k) {
        a[k] = static_cast<int>(k);
    }
    vec b{1, 2, 3, 4, 5};
    EXPECT_EQ(zr::ranges::element_at(views::concat(a, b), short(20)), 20);
}

namespace {

// value int, reference std::string: no common reference.
struct incoherent_iter {
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;
    std::string operator*() const { return {}; }
    incoherent_iter& operator++() { return *this; }
    void operator++(int) {}
};

// A std::input_or_output_iterator without a value type.
struct valueless_iter {
    using difference_type = std::ptrdiff_t;
    int operator*() const { return 0; }
    valueless_iter& operator++() { return *this; }
    void operator++(int) {}
    bool operator==(const valueless_iter&) const { return true; }
};

struct split_projection {
    int operator()(int&) const { return 0; }
    std::string operator()(const int&&) const { return {}; }
};

template <class I, class T> concept can_find_iter = requires(I i, const T& t) { zr::ranges::find(i, i, t); };

} // namespace

TEST(Conformance, ProjectedValueTypeFromValueType) {
    vec a{1, 2};
    vec b{10, 20};
    EXPECT_EQ(zr::ranges::count(views::zip(a, b), {2, 20}), 1);
    static_assert(std::is_same_v<detail::projected_value_t<iterator_t<decltype(views::zip(a, b))>, std::identity>, std::tuple<int, int>>);
}

TEST(Conformance, InputIteratorRelaxationIsNarrow) {
    static_assert(!detail::relaxed_input_iterator<incoherent_iter>);
    static_assert(!can_find_iter<valueless_iter, int>);
}

TEST(Conformance, ProjectionNeedsCommonReference) {
    using it = std::move_iterator<vec::iterator>;
    static_assert(!detail::indirectly_regular_unary_invocable_relaxed<split_projection, it>);
}

TEST(Conformance, SubrangeOfTArraySameInBothConfigs) {
    TArray<int> a{1, 2, 3};
    auto s = subrange(a);
#if ZR_FAKE_UE_VERSION >= 507 || !ZR_FAKE_UE_RANGED_FOR_CHECKS // the checked iterator's operator-> (UE 5.7)
    static_assert(contiguous_range<decltype(s)> && sized_range<decltype(s)>);
    EXPECT_EQ(zr::ranges::data(s)[2], 3);
#else
    static_assert(bidirectional_range<decltype(s)> && sized_range<decltype(s)>);
#endif
    const auto cs = subrange(zr::ranges::begin(a), zr::ranges::end(a));
    const auto& [b, e] = cs;
    EXPECT_EQ(*b, 1);
    (void)e;
}

TEST(Conformance, ConstSentinelOfUnrealCommonRange) {
    static_assert(std::is_same_v<const_iterator_t<TStridedView<int>>, const_sentinel_t<TStridedView<int>>>);
}

TEST(Conformance, BackAndReverseOverDecrementingUnrealIterator) {
    TDoubleLinkedList<int> l;
    l.AddTail(1);
    l.AddTail(2);
    l.AddTail(3);
    EXPECT_EQ(views::all(l).back(), 3);
    EXPECT_EQ(collect(views::reverse(l)), (vec{3, 2, 1}));
    EXPECT_EQ(views::reverse(l).back(), 1);
}
