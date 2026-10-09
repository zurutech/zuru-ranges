// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include <optional>
#include <utility>

// Engagement is IsSet() or an EXPLICIT operator bool; no has_value()/HasValue(). operator*
// has no rvalue overload: on an rvalue it yields an lvalue into the temporary.
template <class OptionalType> class TOptional {
public:
    using ElementType = OptionalType;

    TOptional() = default;
    TOptional(const OptionalType& value)
        : value_(value) {}
    TOptional(OptionalType&& value)
        : value_(std::move(value)) {}

    bool IsSet() const { return value_.has_value(); }
    explicit operator bool() const { return IsSet(); }

    OptionalType& GetValue() { return *value_; }
    const OptionalType& GetValue() const { return *value_; }

    OptionalType& operator*() { return *value_; }
    const OptionalType& operator*() const { return *value_; }
    OptionalType* operator->() { return &*value_; }
    const OptionalType* operator->() const { return &*value_; }

private:
    std::optional<OptionalType> value_;
};
