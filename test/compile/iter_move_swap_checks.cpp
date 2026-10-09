// Compile-only checks: iter_move yields tuple<T&&...> (not a decayed tuple) and iter_swap is
// available for zip, enumerate, join and cartesian_product over std and Unreal containers, in
// both iterator configurations. Unreal iterators are not std::indirectly_readable, so
// std::ranges::iter_swap alone rejects them. Built as an OBJECT library, never run.
#include "CoreMinimal.h"

#include "zr/ranges/views/cartesian_product_view.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace {
using namespace zr::ranges;

template <class It> concept has_iter_move = requires(It it) { std::ranges::iter_move(it); };
template <class It> concept has_iter_swap = requires(It a, It b) { std::ranges::iter_swap(a, b); };

template <class... Rs> using zip_iter = decltype(views::zip(std::declval<Rs&>()...).begin());
template <class R> using enum_iter = decltype(views::enumerate(std::declval<R&>()).begin());
template <class R> using join_iter = decltype(views::join(std::declval<R&>()).begin());
template <class... Rs> using cart_iter = decltype(views::cartesian_product(std::declval<Rs&>()...).begin());

template <class It> using rvref = decltype(std::ranges::iter_move(std::declval<const It&>()));

// --- zip: iter_move is tuple<T&&...> (rvalue references, not values); iter_swap
//     available in both worlds ---------------------------------------------------------
using ZStd = zip_iter<std::vector<int>, std::vector<std::string>>;
static_assert(std::is_same_v<rvref<ZStd>, std::tuple<int&&, std::string&&>>);
static_assert(has_iter_move<ZStd> && has_iter_swap<ZStd>);

using ZMixed = zip_iter<TArray<int>, std::vector<char>>;
static_assert(std::is_same_v<rvref<ZMixed>, std::tuple<int&&, char&&>>);
static_assert(has_iter_move<ZMixed> && has_iter_swap<ZMixed>); // Unreal leg swaps too

using ZUnreal = zip_iter<TArray<int>, TArray<int>>;
static_assert(std::is_same_v<rvref<ZUnreal>, std::tuple<int&&, int&&>>);
static_assert(has_iter_move<ZUnreal> && has_iter_swap<ZUnreal>);

// --- enumerate: tuple<difference_type, T&&>; iter_move only (the standard defines no
// iter_swap). The index half is the base's signed difference type per
// [range.enumerate.iterator]: ptrdiff_t for std and Unreal bases, in both iterator
// configurations. ---
using EStd = enum_iter<std::vector<std::string>>;
static_assert(std::is_same_v<rvref<EStd>, std::tuple<std::ptrdiff_t, std::string&&>>);
static_assert(has_iter_move<EStd> && !has_iter_swap<EStd>);

using EUnreal = enum_iter<TArray<int>>;
static_assert(std::is_same_v<rvref<EUnreal>, std::tuple<std::ptrdiff_t, int&&>>);
static_assert(has_iter_move<EUnreal>);

// --- join: forwards the inner iterator's rvalue reference (T&&); iter_swap in both worlds ---
using JStd = join_iter<std::vector<std::vector<int>>>;
static_assert(std::is_same_v<rvref<JStd>, int&&>);
static_assert(has_iter_move<JStd> && has_iter_swap<JStd>);

using JUnreal = join_iter<TArray<TArray<int>>>;
static_assert(std::is_same_v<rvref<JUnreal>, int&&>);
static_assert(has_iter_move<JUnreal> && has_iter_swap<JUnreal>);

// --- cartesian_product: like zip, operator* builds a prvalue tuple of references, so
//     iter_move must yield tuple<T&&...> element-wise (not a decayed tuple) and iter_swap
//     must swap every component. The last base is restarted at every carry, so the Unreal
//     cases also check that its movable_box'd position stays swappable. --------
using CStd = cart_iter<std::vector<int>, std::vector<std::string>>;
static_assert(std::is_same_v<rvref<CStd>, std::tuple<int&&, std::string&&>>);
static_assert(has_iter_move<CStd> && has_iter_swap<CStd>);

using CMixed = cart_iter<TArray<int>, std::vector<char>>;
static_assert(std::is_same_v<rvref<CMixed>, std::tuple<int&&, char&&>>);
static_assert(has_iter_move<CMixed> && has_iter_swap<CMixed>); // Unreal leg swaps too

using CUnreal = cart_iter<TArray<int>, TArray<int>>;
static_assert(std::is_same_v<rvref<CUnreal>, std::tuple<int&&, int&&>>);
static_assert(has_iter_move<CUnreal> && has_iter_swap<CUnreal>);

// --- the noexcept clauses are non-vacuous and propagate from the element ops.
//     std iterators dereference noexcept, so iter_move (which only rebinds references, even
//     for throwing-move types like std::string) and iter_swap of nothrow-swappable elements
//     are noexcept ---------------------------------------------------------------------------
// join reads through optional<InnerIter>::operator*, noexcept only from LWG 2762 (absent in
// the MSVC 14.29 STL).
constexpr bool optional_deref_noexcept = noexcept(*std::declval<const std::optional<int>&>());
static_assert(noexcept(std::ranges::iter_move(std::declval<const ZStd&>())));
static_assert(noexcept(std::ranges::iter_move(std::declval<const EStd&>())));
static_assert(noexcept(std::ranges::iter_move(std::declval<const JStd&>())) == optional_deref_noexcept);
static_assert(noexcept(std::ranges::iter_move(std::declval<const CStd&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<ZStd&>(), std::declval<ZStd&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<JStd&>(), std::declval<JStd&>())) == optional_deref_noexcept);
static_assert(noexcept(std::ranges::iter_swap(std::declval<CStd&>(), std::declval<CStd&>())));

// The Unreal checked iterator's operator* is not noexcept (as UE's); the raw pointer's is. The
// clause follows the element operation in both configurations.
#if ZR_FAKE_UE_RANGED_FOR_CHECKS
static_assert(!noexcept(std::ranges::iter_move(std::declval<const ZUnreal&>())));
static_assert(!noexcept(std::ranges::iter_swap(std::declval<ZUnreal&>(), std::declval<ZUnreal&>())));
static_assert(!noexcept(std::ranges::iter_swap(std::declval<JUnreal&>(), std::declval<JUnreal&>())));
static_assert(!noexcept(std::ranges::iter_move(std::declval<const CUnreal&>())));
#else
static_assert(noexcept(std::ranges::iter_move(std::declval<const ZUnreal&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<ZUnreal&>(), std::declval<ZUnreal&>())));
static_assert(noexcept(std::ranges::iter_swap(std::declval<JUnreal&>(), std::declval<JUnreal&>())) == optional_deref_noexcept);
static_assert(noexcept(std::ranges::iter_move(std::declval<const CUnreal&>())));
#endif

} // namespace
