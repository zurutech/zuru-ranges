// Compile-only checks: version macros and the ABI namespace (zr/config.h).
#include "zr/ranges.h"

#include <type_traits>
#include <vector>

static_assert(ZR_VERSION_MAJOR == ZR_TEST_PROJECT_VERSION_MAJOR && ZR_VERSION_MINOR == ZR_TEST_PROJECT_VERSION_MINOR
                  && ZR_VERSION_PATCH == ZR_TEST_PROJECT_VERSION_PATCH,
              "zr/config.h and project(VERSION) disagree");

// Every entity lives in the inline namespace; the public names reach it.
static_assert(std::is_same_v<zr::ranges::ZR_ABI_NAMESPACE::empty_view<int>, zr::ranges::empty_view<int>>);
static_assert(std::is_same_v<decltype(zr::views::ZR_ABI_NAMESPACE::all), decltype(zr::ranges::views::all)>);
static_assert(std::is_same_v<zr::views::ZR_ABI_NAMESPACE::iota_fn, std::remove_cv_t<decltype(zr::views::iota)>>);
static_assert(std::is_same_v<zr::ranges::ZR_ABI_NAMESPACE::range_traits_impl<int>, zr::ranges::range_traits_impl<int>>);

namespace abi_checks {
// ElementType: not a standard_range, so the std adapter does not compete.
struct user_range {
    using ElementType = int;
};
struct user_range_qualified : user_range {};
template <class T> struct user_template : user_range {};
} // namespace abi_checks

// User customizations name the public namespace only: reopening zr::ranges (explicit and
// partial specialization) and the qualified form.
namespace zr::ranges {
template <> struct range_traits_impl<abi_checks::user_range> {
    static constexpr range_category category = range_category::forward;
};
template <class T> struct range_traits_impl<abi_checks::user_template<T>> {
    static constexpr range_category category = range_category::forward;
};
} // namespace zr::ranges
template <> struct zr::ranges::range_traits_impl<abi_checks::user_range_qualified> {
    static constexpr zr::ranges::range_category category = zr::ranges::range_category::forward;
};

static_assert(zr::ranges::range_traits<abi_checks::user_range>::category == zr::ranges::range_category::forward);
static_assert(zr::ranges::range_traits<abi_checks::user_template<int>>::category == zr::ranges::range_category::forward);
static_assert(zr::ranges::range_traits<abi_checks::user_range_qualified>::category
              == zr::ranges::range_category::forward);
