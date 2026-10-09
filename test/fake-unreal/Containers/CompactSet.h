// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/ContainersFwd.h"

#include <initializer_list>

// Densely packed set: ranged-for yields TCheckedPointerIterator in debug, a raw pointer in
// Shipping (keyed on the same switch as TArray), although semantically a forward set.
template <class InElementType, class KeyFuncs, class Allocator> class TCompactSet {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    TCompactSet() = default;
    TCompactSet(std::initializer_list<ElementType> init) {
        for (const ElementType& element : init) {
            Add(element);
        }
    }

    int32 Num() const { return elements_.Num(); }

    void Reserve(int32 n) { elements_.Reserve(n); }

    void Add(const ElementType& element) {
        for (const ElementType& existing : elements_) {
            if (existing == element) {
                return;
            }
        }
        elements_.Add(element);
    }

    auto begin() { return elements_.begin(); }
    auto end() { return elements_.end(); }
    auto begin() const { return elements_.begin(); }
    auto end() const { return elements_.end(); }

private:
    TArray<ElementType> elements_;
};
