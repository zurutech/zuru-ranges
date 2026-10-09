// views::maybe: compact-and-unwrap a range of nullable objects (pointers, std::optional,
// TOptional, TSharedPtr), over std and Unreal containers, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/maybe_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

TEST(MaybeView, PointersSkipNulls) {
    int x = 10, y = 20, z = 30;
    TArray<int*> ptrs{&x, nullptr, &y, nullptr, &z};
    int sum = 0;
    for (int& v : ptrs | views::maybe) {
        sum += v;
    }
    EXPECT_EQ(sum, 60);
}

TEST(MaybeView, StdOptionalSkipsNullopt) {
    std::vector<std::optional<int>> opts{std::optional<int>{1}, std::nullopt, std::optional<int>{2}};
    int sum = 0;
    int count = 0;
    for (int v : opts | views::maybe) {
        sum += v;
        ++count;
    }
    EXPECT_EQ(sum, 3);
    EXPECT_EQ(count, 2);
}

TEST(MaybeView, RvalueOptionalsMaterialize) {
    // A view yielding PRVALUE optionals: get_value must materialize by value (a
    // decltype(auto) return would dangle).
    std::vector<int> nums{-1, 2, -3, 4};
    auto positive = [](int n) { return n > 0 ? std::optional<int>(n) : std::optional<int>(); };
    int sum = 0;
    for (int v : nums | views::transform(positive) | views::maybe) {
        sum += v;
    }
    EXPECT_EQ(sum, 6); // 2 + 4
}

TEST(MaybeView, ForwardRangeComposesWithTo) {
    int x = 10, y = 20, z = 30;
    TArray<int*> ptrs{&x, nullptr, &y, nullptr, &z};
    static_assert(forward_range<decltype(ptrs | views::maybe)>);
    TArray<int> collected = ptrs | views::maybe | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(collected.Num(), 3);
    EXPECT_EQ(collected[0], 10);
}

TEST(MaybeView, TOptionalSkipsUnset) {
    TArray<TOptional<int>> opts{TOptional<int>(1), TOptional<int>(), TOptional<int>(3)};
    int sum = 0;
    for (int& v : opts | views::maybe) {
        sum += v;
        v = 0;
    }
    EXPECT_EQ(sum, 4);
    EXPECT_EQ(*opts[0], 0); // lvalue access writes through
}

TEST(MaybeView, TSharedPtrSkipsNull) {
    TArray<TSharedPtr<int>> ptrs{TSharedPtr<int>(new int(5)), TSharedPtr<int>(), TSharedPtr<int>(new int(7))}; // NOLINT(cppcoreguidelines-owning-memory)
    int sum = 0;
    for (int v : ptrs | views::maybe) {
        sum += v;
    }
    EXPECT_EQ(sum, 12);
}

TEST(MaybeView, RvalueSharedPtrCopiesNotMoves) {
    // A prvalue TSharedPtr yields an lvalue into the shared object: decay-copy, never move.
    TSharedPtr<std::string> shared(new std::string("payload")); // NOLINT(cppcoreguidelines-owning-memory)
    std::vector<int> keys{0, 1};
    auto lookup = [&shared](int key) { return key == 1 ? shared : TSharedPtr<std::string>(); };
    std::string got;
    for (std::string v : keys | views::transform(lookup) | views::maybe) {
        got = v;
    }
    EXPECT_EQ(got, "payload");
    EXPECT_EQ(*shared, "payload");
}

TEST(MaybeView, RvalueTOptionalMaterializes) {
    std::vector<int> nums{-1, 2, -3, 4};
    auto positive = [](int n) { return n > 0 ? TOptional<int>(n) : TOptional<int>(); };
    int sum = 0;
    for (int v : nums | views::transform(positive) | views::maybe) {
        sum += v;
    }
    EXPECT_EQ(sum, 6);
}
