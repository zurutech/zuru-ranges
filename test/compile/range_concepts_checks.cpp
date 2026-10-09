// Compile-only checks: the range concepts classify std and Unreal containers identically in
// both iterator configurations; one constraint accepts both. Built as an OBJECT library, never
// run.
#include "CoreMinimal.h"
#include "Containers/SortedMap.h"

#include "zr/ranges/range_concepts.h"
#include "zr/unreal/ue_range_traits.h"

#include <array>
#include <type_traits>
#include <vector>

namespace {
using namespace zr::ranges;

// Everything is a range, via the relaxed begin/end + sentinel_for.
static_assert(range<TArray<int>>);
static_assert(range<TSet<int>>);
static_assert(range<TSortedMap<int, float>>);
static_assert(range<std::vector<int>>);

// Category from the trait, identical in both configurations.
static_assert(contiguous_range<TArray<int>>);
static_assert(contiguous_range<FString>);
static_assert(contiguous_range<std::vector<int>>);
static_assert(contiguous_range<std::array<int, 3>>);
static_assert(forward_range<TSet<int>> && !contiguous_range<TSet<int>> && !random_access_range<TSet<int>>);
static_assert(forward_range<TSortedMap<int, float>> && !contiguous_range<TSortedMap<int, float>>);

// Sized + value-type plumbing.
static_assert(sized_range<TArray<int>>);
static_assert(sized_range<TSortedMap<int, float>>);
static_assert(std::is_same_v<range_value_t<TArray<int>>, int>);

// range_difference_t: the standard's answer for std ranges, and ptrdiff_t for the UE
// containers whose iterators supply none: signed, and the same type in both iterator
// configurations although TArray's iterator is a T* in shipping and a class in debug.
static_assert(std::is_same_v<range_difference_t<std::vector<int>>, std::ptrdiff_t>);
static_assert(std::is_same_v<range_difference_t<std::vector<int>>, std::ranges::range_difference_t<std::vector<int>>>);
static_assert(std::is_same_v<range_difference_t<TArray<int>>, std::ptrdiff_t>);
static_assert(std::is_same_v<range_difference_t<TSet<int>>, std::ptrdiff_t>);
static_assert(std::is_same_v<range_difference_t<TSortedMap<int, float>>, std::ptrdiff_t>);
static_assert(std::is_signed_v<range_difference_t<TArray<int>>>);

// The mixing guarantee: one constraint accepts std and UE alike.
template <range R> constexpr bool accepts() { return true; }
static_assert(accepts<TArray<int>>() && accepts<std::vector<int>>() && accepts<TSet<int>>());
} // namespace
