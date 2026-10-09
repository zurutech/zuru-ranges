// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"

#include <concepts>
#include <utility>

/**
 * @file results.h
 *
 * @brief [algorithms.results] in_value_result.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [algorithms.results]
template <class I, class T> struct in_value_result {
    ZR_NO_UNIQUE_ADDRESS I in;
    ZR_NO_UNIQUE_ADDRESS T value;

    template <class I2, class T2>
        requires std::convertible_to<const I&, I2> && std::convertible_to<const T&, T2>
    constexpr operator in_value_result<I2, T2>() const& {
        return {in, value};
    }

    template <class I2, class T2>
        requires std::convertible_to<I, I2> && std::convertible_to<T, T2>
    constexpr operator in_value_result<I2, T2>() && {
        return {std::move(in), std::move(value)};
    }
};

ZR_END_NAMESPACE_RANGES
