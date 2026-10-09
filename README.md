# zuru-ranges

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/header--only-yes-blue.svg)](#using-it)

| | |
|---|---|
| **Compilers** | [![gcc](https://github.com/zurutech/zuru-ranges/actions/workflows/gcc.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/gcc.yml) [![clang](https://github.com/zurutech/zuru-ranges/actions/workflows/clang.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/clang.yml) [![msvc](https://github.com/zurutech/zuru-ranges/actions/workflows/msvc.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/msvc.yml) [![clang-cl](https://github.com/zurutech/zuru-ranges/actions/workflows/clang-cl.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/clang-cl.yml) [![apple-clang](https://github.com/zurutech/zuru-ranges/actions/workflows/apple-clang.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/apple-clang.yml) |
| **Modules** | [![module-gcc](https://github.com/zurutech/zuru-ranges/actions/workflows/module-gcc.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/module-gcc.yml) [![module-clang](https://github.com/zurutech/zuru-ranges/actions/workflows/module-clang.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/module-clang.yml) |
| **Analysis** | [![sanitizers](https://github.com/zurutech/zuru-ranges/actions/workflows/sanitizers.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/sanitizers.yml) [![fuzz](https://github.com/zurutech/zuru-ranges/actions/workflows/fuzz.yml/badge.svg?branch=main)](https://github.com/zurutech/zuru-ranges/actions/workflows/fuzz.yml) |

Header-only C++20 implementation of the standard `<ranges>` library, and of the `ranges::`
algorithms, that works with Unreal Engine containers as well as standard ones. The library
itself does not depend on Unreal.

### Stability and ABI

The API is that of the standard, within the documented deviations (see
[Conformance to the standard](#conformance-to-the-standard)). Layouts, internal types and
`detail` are not stable.

Every entity is declared in an inline ABI namespace, `zr::ranges::v0_1` and
`zr::ranges::views::v0_1` (`ZR_ABI_NAMESPACE` in `zr/config.h`), one per compatibility unit
of the CMake package (`SameMinorVersion`: `v<major>_<minor>` before 1.0, `v<major>` after).
Binaries built against incompatible releases therefore mangle differently and link into one
program without ODR violations. Customizations name the public namespace only:
`namespace zr::ranges { template <> struct range_traits_impl<X> { ... }; }`. The inline
namespace is the innermost one because MSVC does not implement CWG2061: it would not reopen
an outer `zr::inline v0_1::ranges` through `namespace zr::ranges`.

## Conformance to the standard

Every component that exists in the standard follows the **current working draft**, not only
C++20: names, template heads, constraints, members, `noexcept` and semantics, including the
exposition-only helpers. Each header names its clause (`[range.filter]`, `[range.move.wrap]`,
...). Hence `movable-box` (P2494R2), views that need not be default-constructible (P2325R3),
`reserve_hint` (P2846R6), `zip`, `enumerate`, `chunk_by`, `cartesian_product`, `concat`,
`basic_const_iterator` and the `cbegin` family.

Deviations are those Unreal containers require (iterators that are not semiregular, not
assignable, or comparable with `!=` only) and are marked `// DEVIATION:` with the reason. The
recurring ones:

- Range categories come from `range_traits`, not from `iterator_t<R>` ([range.refinements]).
- View iterators provide the draft's decrement, arithmetic and ordering only when the base
  iterator does; random access to an Unreal range goes through `ranges::element_at`
  (`adaptor_helpers.h`). `take_view`, `zip_view` and `reverse_view` over a random-access sized
  Unreal range iterate it by position through `element_at` (`detail::index_iterator`), so they
  are random access and common, with O(1) `end()`, in every build configuration.
- `==` against an Unreal iterator is `!(a != b) && !(b != a)` (`detail::iter_equal`): both
  orders, because `TChunkedArray`'s `!=` is `Count < Rhs.Count` and `end != it` is always false;
  base iterators are emplaced, never assigned.
- `elements_view` accepts, besides *tuple-like* types ([tuple.like], a closed set), any type
  with the structured-binding tuple protocol ([dcl.struct.bind]): Unreal's `TPair`/`TTuple`.
- `take_view` uses `std::counted_iterator` for std iterators and an equivalent without
  iterator arithmetic for forward Unreal ones (`TSet`, `TMap`).
- MSVC (cl.exe) and GCC 11 keep the pre-P2602 poison pills in `ranges::begin`/`end`: a type
  whose ADL `begin`/`end` take only `const T&` is not a range there (a toolchain bug).

Conformance is tested: `test/ranges/conformance.cpp` has a test per individually checked
requirement, `test/ranges/std_interop.cpp` checks the std concepts and
std adaptors and algorithms over zr views, and `test/compile/` holds the static classification
checks. All run in both Unreal build configurations on GCC, Clang (libstdc++, libc++), Apple Clang, MSVC and clang-cl.

A few components are not part of the standard:

- `views::maybe`, which yields the engaged values of a range of nullable objects (pointers,
  `std::optional`, `TOptional`, smart pointers): `all(r) | filter(has-value) |
  transform(value)`. Inspired by P1255R14, whose `views::nullable` instead adapts a single
  nullable object; *nullable-object* is the paper's.
- `ranges::element_at(r, n)`, unchecked element access for random-access ranges that indexes
  through `data()`, `range_traits::element_at` or a random-access iterator and never through
  an Unreal iterator, so it behaves identically in every Unreal build configuration.
  `view_interface::operator[]` and `at` use it when the iterator is not random access.
- `range_traits`, the per-container category trait described below, and its Unreal
  specializations in `zr/unreal/ue_range_traits.h`.

## Features

Everything lives in `zr::ranges`, with the view objects in `zr::views` (an alias of
`zr::ranges::views`, as in the standard); iterator and algorithm components are in
`zr::ranges` too. The layout follows the standard's clauses: `zr/iterator/` ([iterators]),
`zr/ranges/` ([ranges]) and `zr/algorithm/` ([algorithms]), with the umbrellas
`zr/iterator.h`, `zr/algorithm.h` and `zr/ranges.h` (everything). Header paths are relative to
`zr/ranges/`; `iterator/...` and `algorithm/...` are relative to `zr/`.

### Views

The standard specifies view objects in two kinds:

- **Range adaptor objects** ([range.adaptor.object]) take one range as their first argument
  and compose with `|`: `r | views::filter(pred) | views::transform(f)`.
- **Customization point objects** ([customization.point.object]) take any number of ranges,
  including none, and so are not range adaptors and do not support `|`:
  `views::zip(a, b, c)`.

| Kind | Object | View | Clause | Header |
|---|---|---|---|---|
| Range adaptor closure | `views::all` | `ref_view`, `owning_view` | [range.all] | `views/all.h` |
| Range adaptor | `views::filter` | `filter_view` | [range.filter] | `views/filter_view.h` |
| Range adaptor | `views::transform` | `transform_view` | [range.transform] | `views/transform_view.h` |
| Range adaptor | `views::take` | `take_view` | [range.take] | `views/take_view.h` |
| Range adaptor | `views::drop` | `drop_view` | [range.drop] | `views/drop_view.h` |
| Range adaptor closure | `views::reverse` | `reverse_view` | [range.reverse] | `views/reverse_view.h` |
| Range adaptor closure | `views::join` | `join_view` | [range.join] | `views/join_view.h` |
| Range adaptor closure | `views::elements<N>`, `views::keys`, `views::values` | `elements_view`, `keys_view`, `values_view` | [range.elements] | `views/elements_view.h` |
| Range adaptor closure | `views::enumerate` | `enumerate_view` | [range.enumerate] (C++23) | `views/enumerate_view.h` |
| Range adaptor | `views::chunk_by` | `chunk_by_view` | [range.chunk.by] (C++23) | `views/chunk_by_view.h` |
| Range adaptor closure | `views::maybe` | `filter` + `transform` | non-standard, after P1255R14 | `views/maybe_view.h` |
| Customization point object | `views::zip` | `zip_view` | [range.zip] (C++23) | `views/zip_view.h` |
| Customization point object | `views::cartesian_product` | `cartesian_product_view` | [range.cartesian] (C++23) | `views/cartesian_product_view.h` |
| Customization point object | `views::concat` | `concat_view` | [range.concat] (C++26) | `views/concat_view.h` |

### Range factories

| Object | View | Clause | Header |
|---|---|---|---|
| `views::empty<T>` | `empty_view` | [range.empty] | `views/empty_view.h` |
| `views::single` | `single_view` | [range.single] | `views/single_view.h` |
| `views::iota` | `iota_view` | [range.iota] | `views/iota_view.h` |
| `views::indices` | `iota_view` | [range.iota.overview] (C++26) | `views/iota_view.h` |

### Range primitives and utilities

| Component | Clause | Header |
|---|---|---|
| `ranges::begin`, `end`, `rbegin`, `rend`, `size`, `ssize`, `reserve_hint`, `data`, `empty` | [range.access] | `range_access.h` |
| `ranges::cbegin`, `cend`, `crbegin`, `crend`, `cdata`, `constant_range`, `const_iterator_t` | [range.access], [range.refinements] | `const_access.h` |
| `ranges::element_at` (non-standard) | | `range_access.h` |
| `dangling`, `borrowed_iterator_t`, `borrowed_subrange_t` | [range.dangling] | `dangling.h` |
| `range`, `sized_range`, `approximately_sized_range`, `input_range`, `output_range`, `forward_range` ... `contiguous_range`, `common_range`, `borrowed_range`, and the `iterator_t`/`range_value_t`/... aliases | [range.req], [range.range] | `range_concepts.h` |
| `view`, `enable_view`, `viewable_range`, `view_interface` | [range.view], [view.interface] | `view_interface.h` |
| `subrange`, `subrange_kind` | [range.subrange] | `views/subrange.h` |
| `range_adaptor_closure` and pipe composition | [range.adaptor.object] | `range_adaptor.h` |
| `ranges::to` | [range.utility.conv] (C++23) | `to.h` |
| `range_traits` (non-standard) | | `range_traits.h` |

### Iterators

| Component | Clause | Header |
|---|---|---|
| Iterator concepts and associated types, relaxed for Unreal iterators | [iterator.concepts], [iterator.traits] | `iterator/concepts.h` |
| Indirect callable requirements (relaxed) | [projected], [indirectcallable], [alg.req] | `iterator/indirect_requirements.h` |
| `ranges::distance` | [range.iter.op.distance] | `iterator/operations.h` |
| `basic_const_iterator`, `const_iterator`, `const_sentinel`, `iter_const_reference_t` | [const.iterators] | `iterator/const_iterator.h` |
| `counted_iterator` (`std::counted_iterator` or the Unreal equivalent; exposition-only) | [counted.iterator] | `iterator/counted_iterator.h` |

### Algorithms

Algorithm function objects ([alg.func.obj]) in `zr::ranges`, under `zr/algorithm/`, each with
the standard's iterator/sentinel and range overloads (and, in [alg.nonmodifying], an optional
projection). Only the `ranges::` forms.

| Algorithms | Clause | Header |
|---|---|---|
| `all_of`, `any_of`, `none_of`, `find`, `find_if`, `find_if_not`, `count`, `count_if` | [alg.nonmodifying] | `algorithm/nonmodifying.h` |
| `fold_left`, `fold_left_first`, `fold_right`, `fold_right_last`, `fold_left_with_iter`, `fold_left_first_with_iter` (P2322R6) | [alg.fold] | `algorithm/fold.h` |
| `in_value_result` | [algorithms.results] | `algorithm/results.h` |

`fold_right` and `fold_right_last` fold an Unreal random-access container by position
(`ranges::element_at`) and `TDoubleLinkedList` through its iterator.

## Why this exists

The standard `<ranges>` does not work with Unreal containers. Unreal iterators carry no
`iterator_category` tags, are not semiregular, are not equality comparable (only `!=`),
and `TArray`'s ranged-for iterator even changes type between debug and shipping builds.
The standard range concepts reject such iterators outright, and any classification keyed
on the iterator type is unstable across build configurations.

zr::ranges solves this by deriving a range's category from an explicit, per-container
trait (`range_traits`) rather than from its iterator. The trait is the single source of
truth; standard containers are fed into it automatically, Unreal containers via a small
set of declarations. Standard and Unreal containers therefore mix in one pipeline, e.g.
`concat(TArray, std::vector, TSet, std::array)`, and stays correct whether `TArray`'s
iterator is a raw pointer (shipping) or a checked class (debug).

## Licensing boundary

The library is free of Epic Games source. It is tested against the standard library and
against a behavioural mirror of Unreal's public container API (`test/fake-unreal/`,
original code, no Epic source) that reproduces the relevant quirks. Real-Unreal
integration is exercised in the consuming project, never here. This keeps the open
repository and its CI clear of Unreal's licence.

## Using it

Header-only, C++20. Add `include/` to the include path, or with CMake
`add_subdirectory` (or `FetchContent`) or `find_package(zuru-ranges)` after `cmake --install`,
and link `zuru-ranges::zuru-ranges`. Then include the headers:

```cpp
#include "zr/ranges.h"                 // the whole library

// With Unreal containers, after Core.h / CoreMinimal.h:
#include "zr/unreal/ue_range_traits.h" // range_traits for TArray, TMap, ...

using namespace zr;

TArray<int> nums = views::iota(1, 100)
                 | views::filter([](int n) { return n % 2 == 0; })
                 | views::transform([](int n) { return n * n; })
                 | ranges::to<TArray<int>>();
```

### As a C++20 module

The same library is available as the named module `zr`.
A module's compiled interface is tied to the exact compiler and flags that build it, so
none is shipped: `zr_add_modules` adds the interface units to your target and
compiles them with your settings. This needs CMake 3.28+ (`ZR_MODULE` only enables
the library's own module test; consumers do not need it):

```cmake
add_subdirectory(zuru-ranges)  # or FetchContent, or find_package(zuru-ranges)
zr_add_modules(app)            # `import zr;` is now usable in app's sources
```

```cpp
import zr;

using namespace zr;

auto squares = views::iota(1, 10) | views::transform([](int n) { return n * n; });
```

The module units include the plain headers in the global module fragment and only
re-export names, so `import zr;` and `#include "zr/ranges.h"` can be mixed in one
program. Compiler support: [Requirements](#requirements).

### Notes

See [Features](#features) for the full list. Functions and predicates are invoked through
`std::invoke`, so a member pointer acts as a projection (`views::transform(&Person::name)`).

Algorithms take the standard's overloads (an iterator/sentinel pair, or one range) with an
optional projection. Several ranges are one `views::concat`:

```cpp
bool found = ranges::any_of(views::concat(walls, slabs), &FEntity::IsSelected);
auto it = ranges::find_if(devices, &FDevice::bActive);   // iterator, or ranges::dangling
```

`find` and `find_if` return an iterator; on a temporary owning range
(`find_if(GetChildren(), pred)`) they return `ranges::dangling` (`std::ranges::dangling`).

There is no `sort`: it would forward to `std::ranges::sort`, which the libc++ 15 that UE 5.3
to 5.5 build against on Linux ships only as experimental. Sort contiguous storage as a span
(`std::ranges::sort(std::span(Rooms.GetData(), Rooms.Num()), {}, &FRoom::Area)`) and other
containers with `Algo::Sort`.

## Unreal containers

`ue_range_traits.h` gives every Unreal container that has `begin`/`end` a semantic
category, identical in every build configuration. The traits cover UE 5.3 (the first engine
built as C++20) through 5.7, with the platform limits in [Unreal Engine](#unreal-engine);
`TBitArray` has `begin`/`end`, and so is a range, from 5.4.

| Category | Containers |
|---|---|
| contiguous | `TArray`, `TArrayView`, `TStaticArray`, `FString`, `FAnsiString`, `FUtf8String`, `TStringView` |
| random access (through `operator[]`) | over-aligned `TStaticArray`, `TDeque`, `TChunkedArray`, `TIndirectArray`, `TRingBuffer`, `TStridedView`, `TBitArray` |
| bidirectional | `TDoubleLinkedList` |
| forward | `TSet`, `TSparseSet`, `TCompactSet`, `TSortedSet`, `TMap`, `TSparseMap`, `TCompactMap`, `TMultiMap`, `TSparseMultiMap`, `TCompactMultiMap`, `TSortedMap`, `TSparseArray`, `TLinkedList`, `TIntrusiveLinkedList` nodes |

A type without `ElementType` that forwards `TArray`'s ranged-for iterator (a game-side
wrapper) is contiguous and sized in every configuration from UE 5.7; before, debug builds make
it bidirectional and unsized (the checked iterator has no `operator->`), as a `subrange` of
those iterators.

`TBitArray`'s references are proxies (`FBitReference`, `FConstBitReference`); its value type is
`bool`, as `vector<bool>`'s. `TDeque`'s iterator declares `--` but cannot decrement (UE's
`TIteratorAdapter` over a base without `Decrement`): views reverse a `TDeque` by position, and
the iterator of a view that steps the base iterator (`filter`, `chunk_by`) has no `--` over it.

A nested `ElementType` is what tells an Unreal container apart from a standard range: a class
that declares one is never classified from its iterator (`standard_range`), so a non-Unreal
container with that typedef needs its own `range_traits_impl` to be more than an input range.

UE 5.7 deprecates the alignment parameter of `TStaticArray`: naming `TStaticArray<T, N, A>`
with `A != uint32(-1)` warns on every instantiation, in user code and the trait alike.
`TSharedRef` has no `operator bool`, so it is not a nullable object for `views::maybe`.

`ranges::to` fills Unreal containers through their own API: `Add(Key, Value)` for pair-like
elements (`TPair`, `std::pair`, a `zip` element) into `TMap`/`TMultiMap`/`TSortedMap`, then
`Add`, `Emplace`, `PushLast` (`TDeque`), `AppendChar` (strings), reserving with `Reserve`.

**Not supported: `TList` and `TCircularBuffer`.** A range is a type on which `ranges::begin`
and `ranges::end` are valid ([range.range]); neither has `begin`/`end`. `TList` is a bare
singly linked node (`Element`, `Next`); `TCircularBuffer`'s `operator[]` wraps any index, so
it has no first or last element to delimit. Use `TLinkedList`/`TDoubleLinkedList` and
`TRingBuffer`/`TDeque`, or iterate them by hand.

## Building the tests

The library needs no build. Tests use CMake and GoogleTest (found if installed, else
fetched):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or `scripts/make.sh`, which the GCC, Clang, Apple Clang and module-gcc workflows run (`-h` lists the options):

```sh
scripts/make.sh -c clang++ -W        # warnings as errors
scripts/make.sh -s                   # ASan + UBSan
scripts/make.sh -f                   # libFuzzer smoke run
scripts/make.sh -c clang++ -m        # `import zr;` module smoke test
scripts/make.sh -c msvc --clean      # MSVC cl.exe via msvc-wine, tests under wine
scripts/make.sh -c clang-cl --clean  # clang-cl + lld-link against the msvc-wine STL/SDK
```

CMake options: `-DZR_WARNINGS_AS_ERRORS=ON`, `-DZR_ENABLE_SANITIZERS=ON` (ASan + UBSan),
`-DZR_BUILD_FUZZERS=ON` (libFuzzer, Clang only), `-DZR_MODULE=ON` (`import zr;`).

Compile-time checks (`static_assert`) are in `test/compile/`, built but not run; runtime tests
are in `test/ranges/`, `test/iterator/` and `test/algorithm/` (GoogleTest); every header is
also compiled on its own. Each test is built three times: against the raw-pointer (shipping)
and the checked-class (debug, `ZR_FAKE_UE_RANGED_FOR_CHECKS`) Unreal iterator, and debug
against the UE 5.3 mocks (`ZR_FAKE_UE_VERSION=503`, `test/fake-unreal/README.md`). CI covers the matrix in [Requirements](#requirements), plus C++23,
ASan/UBSan, libFuzzer and module mode (`.github/workflows`).

## Requirements

C++20, and a standard library with `<ranges>`.

### Compilers and standard libraries

The floor of each family is the oldest version that needs no workaround of substance. CI
builds every major between the floor and the newest, warnings as errors, each test both with
UE's shipping (raw pointer) and debug (checked) iterators.

| Compiler | Standard library | Versions | Configurations in CI |
|---|---|---|---|
| GCC | libstdc++ | 11 to 16 | Debug: all; Release: 11, 16; `_GLIBCXX_ASSERTIONS`: 16 |
| Clang | libstdc++ | 13 to 23 | Debug: all; Release: 13, 23 |
| Clang | libc++ | 15 to 23 | Debug: all; Release: 15, 16, 18, 20, 23; hardened (debug mode): 23 |
| MSVC | MSVC STL | 14.29 (VS 2019 16.11), 14.44, newest | Debug and Release |
| clang-cl | MSVC STL | 13 (STL 14.29), newest | Debug and Release |
| Apple Clang | Apple libc++ | 15 (Xcode 15.4), newest | Debug and Release |

C++23 is built with GCC 16 and Clang 23 (libc++); `import zr;` with Clang 20 and GCC 16
(GCC 14 and 15 reject it; CMake 3.28+). libc++ 14 and Xcode 14 have no `std::ranges` at all.

Every view, `subrange` and `ranges::to` are usable in constant expressions
(`test/compile/constexpr_checks.cpp`). `concat` needs a constexpr `std::variant` (P2231R1:
`__cpp_lib_variant >= 202106L`, libstdc++ 12) and `join` a constexpr `std::optional`; MSVC 14.29
cannot evaluate the views that cache `begin()` (filter, drop, reverse, join, chunk_by) at compile
time.

Three standard library limits affect only the std interoperability of zr views (zr views
feeding std adaptors); everything else works there:

- Clang before 16 cannot instantiate libstdc++'s range adaptors, and libc++ 15 ships its
  own as experimental.
- The MSVC STL from 14.30 to 14.36 specializes `basic_common_reference` for `std::tuple`
  without the matching constructor, so `zip`, `enumerate` and `cartesian_product` iterators
  are not `std::indirectly_readable` there.
- In C++20 `std::tuple` has no `basic_common_reference` (P2321R2, C++23): `zip`, `enumerate`
  and `cartesian_product` iterators whose tuple holds a `const T&` are not
  `std::indirectly_readable`. zr's own concepts accept them (`detail::relaxed_input_iterator`).

### Unreal Engine

An engine version is supported only if the library builds with the toolchain UE uses on every
platform. On Linux that is UE's bundled clang **and the libc++ UE compiles against**, which
until 5.5 is a libc++ 15 shipped with the engine, not the toolchain's. On macOS UE uses the
installed Xcode, so the library's minimum can be above UE's.

| UE | Windows (UBT minimum) | Linux (UBT toolchain) | macOS (UBT range) |
|---|---|---|---|
| 5.7 | MSVC 14.38, clang-cl 18.1.8 | clang 20.1.8, its libc++ 20 | Xcode 15.2 to 26 |
| 5.6 | MSVC 14.38, clang-cl 18.1.3 | clang 18.1.0, bundled libc++ 19 | Xcode 15.2 to 26 |
| 5.5 | MSVC 14.38, clang-cl 16 | clang 18.1.0, bundled libc++ 15 | Xcode 15.2 to 16 |
| 5.4 | MSVC 14.34, clang-cl 15 | clang 16.0.6, bundled libc++ 15 | Xcode 15 to 15.9 (UE: from 14.1) |
| 5.3 | MSVC 14.29, clang-cl 13 | clang 16, bundled libc++ 15 | Xcode 15 (UE: from 14.1) |

Every cell is supported, with the library's own Xcode 15 minimum on 5.3 and 5.4. It uses nothing
from the standard library that libc++ 15 leaves experimental. Validated beyond the fakes
against the real engine sources of 5.3 to 5.7, and on Linux against each version's bundled
libc++ (local, never published).
