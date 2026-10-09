// Member pointers as transform/filter functions (projection via std::invoke), and the
// non-assignable debug iterator quirk. Both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/to.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <type_traits>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
struct Person {
    int age{};
    bool active{};
    constexpr int Age() const { return age; }
    constexpr bool IsActive() const { return active; }
};

// The TArray debug iterator is copy-constructible but not copy-assignable (reference
// member), exactly like UE. In shipping it is a raw pointer (assignable).
using ArrIt = TArray<int>::RangedForIteratorType;
#if ZR_FAKE_UE_RANGED_FOR_CHECKS
static_assert(std::is_copy_constructible_v<ArrIt> && !std::is_copy_assignable_v<ArrIt>);
#else
static_assert(std::is_copy_assignable_v<ArrIt>); // raw pointer
#endif
} // namespace

TEST(ProjectionInvoke, TransformDataMemberPointer) {
    TArray<Person> people{Person{30, true}, Person{40, false}, Person{50, true}};
    int sum = 0;
    for (int age : people | views::transform(&Person::age)) {
        sum += age;
    }
    EXPECT_EQ(sum, 120);
}

TEST(ProjectionInvoke, TransformMemberFunctionPointer) {
    TArray<Person> people{Person{30, true}, Person{40, false}};
    EXPECT_EQ((people | views::transform(&Person::Age)).front(), 30);
}

TEST(ProjectionInvoke, FilterMemberFunctionPredicateThenProject) {
    TArray<Person> people{Person{30, true}, Person{40, false}, Person{50, true}};
    TArray<int> active_ages = people | views::filter(&Person::IsActive) | views::transform(&Person::Age) | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(active_ages.Num(), 2);
    EXPECT_EQ(active_ages[0] + active_ages[1], 80); // 30 + 50
}

TEST(ProjectionInvoke, FilterDataMemberPredicate) {
    TArray<Person> people{Person{30, true}, Person{40, false}, Person{50, true}};
    int count = 0;
    int age_sum = 0;
    for (const Person& p : people | views::filter(&Person::active)) {
        ++count;
        age_sum += p.age;
    }
    EXPECT_EQ(count, 2);
    EXPECT_EQ(age_sum, 80);
}
