// views::all (ref_view / owning_view) and the closure/pipe machinery, both configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/all.h"
#include "zr/unreal/ue_range_traits.h"

#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

static_assert(view<ref_view<TArray<int>>>);
static_assert(contiguous_range<ref_view<TArray<int>>>);
static_assert(forward_range<ref_view<TSet<int>>>);
static_assert(!contiguous_range<ref_view<TSet<int>>>);
static_assert(sized_range<ref_view<TArray<int>>>);

static_assert(std::is_same_v<views::all_t<TArray<int>&>, ref_view<TArray<int>>>);
static_assert(std::is_same_v<views::all_t<std::vector<int>&>, ref_view<std::vector<int>>>);
static_assert(std::is_same_v<views::all_t<std::vector<int>>, owning_view<std::vector<int>>>);

namespace {
struct count_fn : range_adaptor_closure<count_fn> {
    // r is only measured (size), used as an lvalue; nothing to forward.
    template <sized_range R> constexpr auto operator()(R&& r) const { return static_cast<int>(zr::ranges::size(r)); } // NOLINT(cppcoreguidelines-missing-std-forward)
};
inline constexpr count_fn count{};
} // namespace

TEST(ViewsAll, RefViewOverUnrealContainer) {
    TArray<int> a{1, 2, 3, 4};
    auto v = views::all(a);

    int sum = 0;
    for (auto it = zr::ranges::begin(v); it != zr::ranges::end(v); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 10);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v.front(), 1);
}

TEST(ViewsAll, PipeAndCompose) {
    TArray<int> a{1, 2, 3, 4};
    EXPECT_EQ(a | count, 4);
    EXPECT_EQ(a | views::all | count, 4);

    std::vector<int> sv{5, 6};
    EXPECT_EQ(sv | views::all | count, 2);
}
