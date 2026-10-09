// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See README.md.
#pragma once

// The engine version mirrored, as 100 * major + minor: 503 to 507. Gates only what differs between
// versions in a way neither shape covers (README.md); never read by zr.
#ifndef ZR_FAKE_UE_VERSION
#define ZR_FAKE_UE_VERSION 507
#endif
#if ZR_FAKE_UE_VERSION < 503 || ZR_FAKE_UE_VERSION > 507
#error "ZR_FAKE_UE_VERSION: 503 to 507"
#endif

// The fundamental types of UE's GenericPlatform.h (int32 is signed int, not via <cstdint>), so
// type-identity checks agree with real UE.
typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef signed char int8;
typedef signed short int int16;
typedef signed int int32;
typedef signed long long int64;

typedef char ANSICHAR;
typedef wchar_t WIDECHAR;
typedef WIDECHAR TCHAR;
typedef char8_t UTF8CHAR;

#define TEXT(x) L##x

// Misc/CoreMiscDefines.h (included by CoreTypes.h in UE).
enum EInPlace { InPlace };

// Placeholder allocators so the container templates carry UE's arity
// (TArray<T, Allocator>, TSortedMap<K, V, ArrayAllocator, SortPredicate>, ...).
struct FDefaultAllocator {};
struct FDefaultAllocator64 {};
struct FDefaultSetAllocator {};
template <class KeyType> struct TLess {};
