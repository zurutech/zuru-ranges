// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include <utility>

// Mirrors the shape of UE's set/map iterators: forward, dereferenceable, pre-incrementable,
// `!=` only, not default-constructible and not copy/move-assignable in any configuration (UE's hold
// a reference to the container). Copy/move-constructible.
template <class UnderlyingIt>
class TFakeContainerIterator {
public:
    explicit TFakeContainerIterator(UnderlyingIt it)
        : it_(std::move(it)) {}

    TFakeContainerIterator() = delete;
    TFakeContainerIterator(const TFakeContainerIterator&) = default;
    TFakeContainerIterator(TFakeContainerIterator&&) = default;
    TFakeContainerIterator& operator=(const TFakeContainerIterator&) = delete;
    TFakeContainerIterator& operator=(TFakeContainerIterator&&) = delete;
    ~TFakeContainerIterator() = default;

    decltype(auto) operator*() const { return *it_; }

    TFakeContainerIterator& operator++() {
        ++it_;
        return *this;
    }

    bool operator!=(const TFakeContainerIterator& rhs) const { return it_ != rhs.it_; }

private:
    UnderlyingIt it_;
};
