// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @file config.h
 *
 * @brief Version, ABI namespace and portability macros.
 */

// Release version; equals project(VERSION) in CMakeLists.txt (checked by the test build).
#define ZR_VERSION_MAJOR 0
#define ZR_VERSION_MINOR 1
#define ZR_VERSION_PATCH 1

// ABI namespace: every entity is declared in `inline namespace ZR_ABI_NAMESPACE` nested in
// zr::ranges and zr::ranges::views, so binaries built against releases with different ABIs
// link without ODR violations. One per compatibility unit of the CMake package
// (SameMinorVersion): v<major>_<minor> before 1.0, v<major> from 1.0.
//
// Innermost, not zr::inline v0_1::ranges: MSVC does not implement CWG2061, so a user's
// `namespace zr::ranges { template <> struct range_traits_impl<X> ...; }` would declare a
// second zr::ranges instead of reopening the inline one. The views namespace is likewise a
// member of zr::ranges, not of the inline namespace.
#define ZR_ABI_NAMESPACE v0_1

#define ZR_BEGIN_NAMESPACE_RANGES \
    namespace zr::ranges { \
    inline namespace ZR_ABI_NAMESPACE {
#define ZR_END_NAMESPACE_RANGES \
    } \
    }
#define ZR_BEGIN_NAMESPACE_VIEWS \
    namespace zr::ranges::views { \
    inline namespace ZR_ABI_NAMESPACE {
#define ZR_END_NAMESPACE_VIEWS \
    } \
    }

// [[no_unique_address]]: the MSVC ABI (_MSC_VER: cl, clang-cl) ignores it and needs
// [[msvc::no_unique_address]]; keyed on the ABI, not _WIN32 (MinGW is Itanium ABI).
// clang-cl < 18 has neither form: layouts differ from cl's, do not mix the two across an ABI
// boundary.
#if defined(_MSC_VER) && defined(__clang__) && !__has_cpp_attribute(msvc::no_unique_address)
#define ZR_NO_UNIQUE_ADDRESS
#elif defined(_MSC_VER)
#define ZR_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define ZR_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
