// Compile-only checks: range_traits gives a configuration-independent category where the
// iterator type depends on the configuration (TSortedMap, TArray). Built as an OBJECT library,
// never run.
#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "Containers/SortedMap.h"

#include "zr/ranges/range_traits.h"
#include "zr/unreal/ue_range_traits.h"

#include <array>
#include <deque>
#include <vector>

namespace {
using namespace zr::ranges;

// Unreal containers: category comes from the trait, identical in both configurations.
static_assert(contiguous_via_traits<TArray<int>>);
static_assert(contiguous_via_traits<FString>);
static_assert(forward_via_traits<TSet<int>>);
static_assert(!random_access_via_traits<TSet<int>>);
static_assert(!contiguous_via_traits<TSet<int>>);
static_assert(forward_via_traits<TMap<int, float>>);
static_assert(random_access_via_traits<TDeque<int>>);
static_assert(!contiguous_via_traits<TDeque<int>>);

// TSortedMap::begin() is a T* in shipping and a checked class in debug; the category is
// forward in both.
static_assert(forward_via_traits<TSortedMap<int, float>>);
static_assert(!random_access_via_traits<TSortedMap<int, float>>);
static_assert(!contiguous_via_traits<TSortedMap<int, float>>);

// Standard containers flow through the std adapter, no per-type bridging.
static_assert(contiguous_via_traits<std::vector<int>>);
static_assert(contiguous_via_traits<std::array<int, 4>>);
static_assert(random_access_via_traits<std::deque<int>> && !contiguous_via_traits<std::deque<int>>);
} // namespace
