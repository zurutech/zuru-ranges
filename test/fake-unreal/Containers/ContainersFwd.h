// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
// Mirrors UE's forward declarations: default template arguments live here, and TDeque is
// deliberately absent (UE does not forward-declare it).
#pragma once

#include "CoreTypes.h"

#include <type_traits>

template <class InElementType, class AllocatorType = FDefaultAllocator> class TArray;
template <class InElementType, class SizeType = int32> class TArrayView;
template <class InElementType, class KeyFuncs = void, class Allocator = FDefaultSetAllocator> class TSet;
template <class InElementType, class KeyFuncs = void, class Allocator = FDefaultSetAllocator> class TSparseSet;
template <class InElementType, class KeyFuncs = void, class Allocator = FDefaultSetAllocator> class TCompactSet;
template <class InKeyType, class InValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TMap;
template <class InKeyType, class InValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TSparseMap;
template <class InKeyType, class InValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TCompactMap;
template <class InKeyType, class InValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TMultiMap;
template <class KeyType, class ValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TSparseMultiMap;
template <class KeyType, class ValueType, class SetAllocator = FDefaultSetAllocator, class KeyFuncs = void> class TCompactMultiMap;
template <class InKeyType, class InValueType, class ArrayAllocator = FDefaultAllocator, class SortPredicate = TLess<InKeyType>> class TSortedMap;
template <class InElementType, class ArrayAllocator = FDefaultAllocator, class SortPredicate = TLess<InElementType>> class TSortedSet;
template <class InElementType, class InSizeType = int32> class TStridedView;

// UE's StringFwd.h: a constrained template head, declared (not defined) by CoreMinimal.h.
namespace UE {
template <class T>
concept CCharType = std::is_same_v<T, ANSICHAR> || std::is_same_v<T, WIDECHAR> || std::is_same_v<T, UTF8CHAR>;
}
template <UE::CCharType CharType> class TStringView;

class FString;
class FAnsiString;
class FUtf8String;
