// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>

// Mirrors UE's TARRAY_RANGED_FOR_CHECKS. When enabled, TArray's ranged-for
// iterator is a checked class; when disabled it is a raw pointer. The type changes between
// configurations, so the category must come from range_traits.
#ifndef ZR_FAKE_UE_RANGED_FOR_CHECKS
#define ZR_FAKE_UE_RANGED_FOR_CHECKS 0
#endif

// Mirror of UE's TCheckedPointerIterator: dereferenceable, pre-incrementable, != only before UE 5.7
// (== and -> from 5.7), not default-constructible, and not copy/move-assignable (reference member
// CurrentNum, as UE); copy/move-constructible. Code that assigns an iterator does not compile
// against it.
template <class ElementType> class TCheckedPointerIterator {
public:
    TCheckedPointerIterator(ElementType* ptr, const int32& currentNum)
        : ptr_(ptr)
        , currentNum_(currentNum)
        , initialNum_(currentNum) {}

    TCheckedPointerIterator() = delete;

    ElementType& operator*() const { return *ptr_; }

#if ZR_FAKE_UE_VERSION >= 507
    ElementType* operator->() const { return ptr_; }
#endif

    TCheckedPointerIterator& operator++() {
        ++ptr_;
        return *this;
    }

    // UE provides it for FString's end().
    TCheckedPointerIterator& operator--() {
        --ptr_;
        return *this;
    }

    // UE asserts here that the container was not resized mid-iteration (initialNum_ == currentNum_).
    bool operator!=(const TCheckedPointerIterator& rhs) const { return ptr_ != rhs.ptr_; }
#if ZR_FAKE_UE_VERSION >= 507
    bool operator==(const TCheckedPointerIterator& rhs) const { return ptr_ == rhs.ptr_; }
#endif

private:
    ElementType* ptr_;
    const int32& currentNum_; // reference member -> non-assignable, exactly like UE
    int32 initialNum_;
};

template <class InElementType, class AllocatorType> class TArray {
public:
    using ElementType = InElementType;
    using SizeType = int32;

#if ZR_FAKE_UE_RANGED_FOR_CHECKS
    using RangedForIteratorType = TCheckedPointerIterator<ElementType>;
    using RangedForConstIteratorType = TCheckedPointerIterator<const ElementType>;
#else
    using RangedForIteratorType = ElementType*;
    using RangedForConstIteratorType = const ElementType*;
#endif

    TArray() = default;

    TArray(std::initializer_list<ElementType> init) {
        Reserve(static_cast<int32>(init.size()));
        for (const ElementType& element : init) {
            Add(element);
        }
    }

    TArray(const TArray& other) {
        Reserve(other.num_);
        for (int32 i = 0; i < other.num_; ++i) {
            Add(other[i]);
        }
    }

    TArray(TArray&& other) noexcept
        : data_(std::move(other.data_))
        , num_(std::exchange(other.num_, 0))
        , max_(std::exchange(other.max_, 0)) {}

    TArray& operator=(TArray other) noexcept {
        std::swap(data_, other.data_);
        std::swap(num_, other.num_);
        std::swap(max_, other.max_);
        return *this;
    }

    ~TArray() = default;

    int32 Num() const { return num_; }

    ElementType* GetData() { return data_.get(); }
    const ElementType* GetData() const { return data_.get(); }

    ElementType& operator[](int32 index) { return data_[static_cast<std::size_t>(index)]; }
    const ElementType& operator[](int32 index) const { return data_[static_cast<std::size_t>(index)]; }

    // Element loops and default-initialized storage (slots past num_ are written before read):
    // GCC 12-14 -O3 cannot bound num_ by max_ in the memmove/memset that std::move,
    // move_backward and new T[n]() lower to, and report them out of bounds (-Warray-bounds).
    void Reserve(int32 newMax) {
        if (newMax <= max_) {
            return;
        }
        std::unique_ptr<ElementType[]> fresh(new ElementType[static_cast<std::size_t>(newMax)]);
        for (int32 i = 0; i < num_; ++i) {
            fresh[static_cast<std::size_t>(i)] = std::move(data_[static_cast<std::size_t>(i)]);
        }
        data_ = std::move(fresh);
        max_ = newMax;
    }

    void Add(const ElementType& element) {
        Insert(element, num_);
    }

    void Insert(const ElementType& element, int32 index) {
        ElementType value(element); // may alias an element moved by Reserve
        if (num_ == max_) {
            Reserve(max_ ? 2 * max_ : 4);
        }
        for (int32 i = num_; i > index; --i) {
            data_[static_cast<std::size_t>(i)] = std::move(data_[static_cast<std::size_t>(i - 1)]);
        }
        data_[static_cast<std::size_t>(index)] = std::move(value);
        ++num_;
    }

#if ZR_FAKE_UE_RANGED_FOR_CHECKS
    RangedForIteratorType begin() { return {GetData(), num_}; }
    RangedForIteratorType end() { return {GetData() + num_, num_}; }
    RangedForConstIteratorType begin() const { return {GetData(), num_}; }
    RangedForConstIteratorType end() const { return {GetData() + num_, num_}; }
#else
    RangedForIteratorType begin() { return GetData(); }
    RangedForIteratorType end() { return GetData() + num_; }
    RangedForConstIteratorType begin() const { return GetData(); }
    RangedForConstIteratorType end() const { return GetData() + num_; }
#endif

private:
    // Not std::vector: vector<bool> would make TArray<bool> a bit-proxy container.
    std::unique_ptr<ElementType[]> data_;
    int32 num_{};
    int32 max_{};
};

// TArrayView / TConstArrayView use raw-pointer iterators in every configuration.
template <class InElementType, class SizeType> class TArrayView {
public:
    using ElementType = InElementType;

    TArrayView() = default;
    TArrayView(ElementType* data, SizeType count)
        : data_(data)
        , num_(count) {}

    SizeType Num() const { return num_; }
    ElementType* GetData() const { return data_; }

    ElementType& operator[](SizeType index) const { return data_[index]; }

    ElementType* begin() const { return data_; }
    ElementType* end() const { return data_ + num_; }

private:
    ElementType* data_{};
    SizeType num_{};
};

template <class InElementType, class SizeType = int32> using TConstArrayView = TArrayView<const InElementType, SizeType>;
