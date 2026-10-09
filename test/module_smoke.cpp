// Consumer of the zr named module. Exercises a symbol from (nearly) every ranges
// header so a name missing from modules/zr.cppm surfaces as an unresolved name.
//
// No standard header is included textually: `import zr;` plus a textual <ranges> in one TU
// trips a clang/libc++ bug (__promote_t redefinition). Inputs are built-in arrays.
import zr;

// Defined in module_mix_header.cpp, which includes zr instead of importing it.
int zr_mix_sum(zr::ranges::iota_view<int, int> v);

int main() {
    int arr[5]{1, 2, 3, 4, 5};

    // range_concepts.h / range_traits.h / view_interface.h
    static_assert(zr::ranges::range<int (&)[5]>);
    static_assert(zr::ranges::forward_range<int (&)[5]>);
    static_assert(zr::ranges::range_category::contiguous >= zr::ranges::range_category::input);
    static_assert(zr::ranges::viewable_range<int (&)[5]>);
    // iterator/concepts.h / range_concepts.h associated types
    static_assert(zr::ranges::range_difference_t<int (&)[5]>(-1) < 0);
    static_assert(zr::ranges::iter_difference_t<int*>(-1) < 0);

    // algorithm/nonmodifying.h
    int values[3]{3, 1, 2};
    const bool any = zr::ranges::any_of(values, [](int x) { return x == 2; });
    const int* found = zr::ranges::find_if(values, [](int x) { return x < 3; });
    const bool all = zr::ranges::all_of(values, [](int x) { return x > 0; });
    const bool none = zr::ranges::none_of(values, [](int x) { return x > 3; });
    const int* three = zr::ranges::find(values, 3);
    const auto evens = zr::ranges::count_if(values, [](int x) { return x % 2 == 0; });
    const auto ones = zr::ranges::count(values, 1);
    const int* not_three = zr::ranges::find_if_not(values, [](int x) { return x == 3; });
    if (!any || !all || !none || three != values || evens != 1 || *found != 1 || ones != 1 || *not_three != 1) {
        return 1;
    }

    // algorithm/fold.h, algorithm/results.h
    const int folded = zr::ranges::fold_left(values, 0, [](int a, int b) { return a + b; });
    const auto last = zr::ranges::fold_right_last(values, [](int a, int b) { return a - b; });
    const zr::ranges::fold_left_with_iter_result<const int*, int> with_iter = zr::ranges::fold_left_with_iter(values, 0, [](int a, int b) { return a + b; });
    const auto first = zr::ranges::fold_left_first_with_iter(values, [](int a, int b) { return a * b; });
    const zr::ranges::in_value_result<const int*, int> in_value = with_iter;
    if (folded != 6 || *last != 3 - (1 - 2) || with_iter.in != values + 3 || in_value.value != 6 || *first.value != 6 ||
        zr::ranges::fold_right(values, 0, [](int a, int b) { return a + b; }) != 6 || *zr::ranges::fold_left_first(values, [](int a, int b) { return a + b; }) != 6) {
        return 1;
    }

    // range_access.h CPOs + subrange.h
    zr::ranges::subrange sub{zr::ranges::begin(arr), zr::ranges::end(arr)};

    // views: all / reverse / filter / transform / enumerate / single / empty / iota / zip / cartesian_product
    auto a = zr::views::all(arr);
    auto rev = arr | zr::views::reverse;
    auto flt = arr | zr::views::filter([](int x) { return x % 2 == 0; });
    auto trn = arr | zr::views::transform([](int x) { return x + 1; });
    auto enm = arr | zr::views::enumerate;
    auto sgl = zr::views::single(7);
    auto emp = zr::views::empty<int>;
    auto iot = zr::views::iota(0, 3);
    auto zip = zr::views::zip(arr, arr);
    auto cart = zr::views::cartesian_product(arr, arr);
    auto cat = zr::views::concat(arr, arr);
    int* ptrs[2]{&arr[0], nullptr};
    auto may = ptrs | zr::views::maybe;
    int nested[2][2]{{1, 2}, {3, 4}};
    auto jn = nested | zr::views::join;
    auto tk = arr | zr::views::take(2);
    auto dr = arr | zr::views::drop(3);
    auto chk = arr | zr::views::chunk_by([](int x, int y) { return x < y; });
    auto ks = zr::views::zip(arr, arr) | zr::views::keys;
    auto vs = zr::views::zip(arr, arr) | zr::views::values;
    auto el = zr::views::zip(arr, arr) | zr::views::elements<1>;
    zr::ranges::take_view<zr::ranges::ref_view<int[5]>>* tkp = &tk;
    zr::ranges::drop_view<zr::ranges::ref_view<int[5]>>* drp = &dr;
    (void)tkp; (void)drp;
    static_assert(sizeof(zr::ranges::chunk_by_view<zr::ranges::ref_view<int[5]>, bool (*)(int, int)>) > 0);
    static_assert(sizeof(zr::ranges::keys_view<decltype(zr::views::zip(arr, arr))>) > 0);
    static_assert(sizeof(zr::ranges::values_view<decltype(zr::views::zip(arr, arr))>) > 0);
    static_assert(sizeof(zr::ranges::elements_view<decltype(zr::views::zip(arr, arr)), 0>) > 0);

    int sum = 0;
    // rev's iterator is std::reverse_iterator<int*>: its non-member comparisons belong to
    // <iterator>, not reachable here, so only members are used.
    for (long i = 0; i < 5; ++i) sum += rev[i];
    for (int x : flt) sum += x;
    for (int x : trn) sum += x;
    for (int x : may) sum += x;
    for (int x : jn) sum += x;
    for (int x : tk) sum += x;
    for (int x : dr) sum += x;
    for (int x : ks) sum += x;
    for (int x : vs) sum += x;
    for (int x : el) sum += x;
    if (zr::ranges::distance(chk) != 1 || zr::ranges::distance(arr) != 5) return 1;
    (void)a; (void)enm; (void)sgl; (void)emp; (void)iot; (void)zip; (void)cart; (void)sub; (void)cat;
    if (zr::ranges::size(arr) != 5 || zr::ranges::ssize(arr) != 5 || zr::ranges::element_at(arr, 1) != 2) return 1;
    // iterator/const_iterator.h, ranges/const_access.h
    static_assert(zr::ranges::constant_range<const int (&)[5]>);
    if (*zr::ranges::cbegin(arr) != 1 || zr::ranges::cdata(arr) != arr || trn.cbegin()[0] != 2) return 1;
    if (zr_mix_sum(zr::views::iota(0, 4)) != 6) return 1;
    return sum == 0 ? 1 : 0;
}
