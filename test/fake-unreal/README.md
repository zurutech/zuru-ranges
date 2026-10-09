# fake-unreal: behavioural mirror of Unreal Engine's public API

These headers contain **no Epic Games source code**. They reproduce only the
*names*, *signatures*, and *observable quirks* of UE's public container API,
implemented on top of the C++ standard library. They:

1. **Provide the names** (`int32`, `TArray`, `TSet`, `TMap`, `FString`, `TPair`, ...
   in the global namespace, under UE's include paths like `Containers/Array.h`) so
   that `zr/unreal/ue_range_traits.h` compiles and is exercised in open CI
   without Epic's headers.
2. **Reproduce the quirks** that force every deviation in `zr::ranges`, so the
   deviations are tested.

## Quirks reproduced

| Quirk                                                   | Where                         | Forces                          |
|---------------------------------------------------------|-------------------------------|---------------------------------|
| `Num()` returns signed `int32`, not `size_t`            | all containers                | `range_traits<>::size` typing   |
| Container iterators are **not default-constructible**   | `TCheckedPointerIterator`, `TFakeContainerIterator` | dropping `semiregular` from `sentinel_for` |
| Iterators support **only `!=`** (no `==`)               | same (`TCheckedPointerIterator` before 5.7) | `weakly_inequality_testable_with` relaxation |
| `TArray` ranged-for iterator **type changes between debug and shipping** | `ZR_FAKE_UE_RANGED_FOR_CHECKS` | semantic `range_traits<>` instead of iterator-tag deduction |
| `TArray` debug iterator is copy-constructible but **not copy/move-assignable** (reference member `CurrentNum`, as UE) | `TCheckedPointerIterator` | views emplace iterators and never assign them |
| `TSortedMap` **inherits the backing array's iterator**: raw pointer in Shipping (looks contiguous), checked class in debug, but semantically forward | `Containers/SortedMap.h` | trait must override the iterator; tests must pass identically in both configurations |
| Contiguous access via `GetData()`, not `data()`         | `TArray`, `FString`           | `range_traits<>::data` customization |
| String is contiguous via `GetCharArray().GetData()`     | `FString`                     | `range_traits<FString>`         |
| Set/map iterators are **not copy/move-assignable in any configuration** | `TFakeContainerIterator` | same as above, for `TSet`/`TMap` |
| `TSortedSet`, `TCompactSet` ranged-for iterator switches like `TArray`'s; `TIndirectArray`'s is assignable only in Shipping | `Containers/SortedSet.h`, `CompactSet.h`, `IndirectArray.h` | classification must not track the iterator |
| `TSparseArray`: `++it` yields a **base-class** reference; ranged-for type switches with the configuration | `Containers/SparseArray.h` | `prefix_incrementable` relaxation |
| `TChunkedArray`: `void operator++()`, **asymmetric** `!=` (`end != it` is always false); appends via `AddElement` | `Containers/ChunkedArray.h` | same; never compare sentinel-first |
| Linked lists: `begin`/`end` are **hidden friends** (of a CRTP base for `TLinkedList`/`TIntrusiveLinkedList`); an intrusive list's type is the derived node | `Containers/List.h` | ADL branch of the `begin`/`end` CPOs; trait keyed on derivation |
| `TStridedView` iterator has a **non-const** `operator*` | `Containers/StridedView.h` | views dereference const iterators through `detail::deref` |
| `TBitArray` dereferences to **proxy** prvalues: `FBitReference` (copy-assignment writes the bit, `operator=(bool)` void), `FConstBitReference` for const access (a class, not `bool`) | `Containers/BitArray.h` | proxy references through views and `element_at`; value type `bool` (`detail::proxy_value`) |
| `TRingBuffer`'s iterator **decrements** (`--`), without arithmetic; `TRingBuffer::Reserve` takes the unsigned `SizeType` while `Num()` is the signed `IndexType` | `Containers/RingBuffer.h` | bidirectional view paths over it; `ranges::to`'s saturating reserve |
| `TDeque`'s iterator is a `TIteratorAdapter` over a base without `Decrement`: `--`, `+=`, `-=`, `+`, `-` are **declared but ill-formed** | `Containers/IteratorAdapter.h`, `Deque.h` | `detail::decrement_disabled`; `TDeque` reversed by position |
| Over-aligned `TStaticArray<T, N, Alignment>` stores padded elements | `Containers/StaticArray.h` | random access, not contiguous, for that specialization |
| `TRingBuffer`, `TIndirectArray`, `TChunkedArray`, `TStridedView`, `TBitArray`: random access only through `operator[]` | respective headers | `range_traits::element_at` |
| `TSortedMap::Emplace` is an **unconstrained** template; no `Add(TPair)` | `Containers/SortedMap.h` | `requires`-based detection gives false positives; `ranges::to` tries `Add(Key, Value)` first |
| `TDeque` appends only through `PushLast`/`EmplaceLast`; strings through `AppendChar`, reserved by `Len` | `Containers/Deque.h`, `UnrealString.h` | `ranges::to` append/reserve branches |
| `FString` `operator==` is **case-insensitive** | `Containers/UnrealString.h` | `ranges::find` on strings matches regardless of case |
| `TOptional`: `IsSet()` and an **explicit** `operator bool`; `TSharedPtr<T, ESPMode>`: explicit `operator bool`, `operator*` aliases shared state; `TSharedRef` has **no** `operator bool` | `Misc/Optional.h`, `Templates/SharedPointer.h` | P1255R14 nullable-object detection by contextual conversion; rvalue unwrap by decay-copy; `TSharedRef` is not nullable |
| `TPair` is an **alias** of `TTuple<K, V>`; the tuple protocol is a member `Get<N>()` and a global `get<int N>()` found by ADL | `Templates/Tuple.h` | `elements_view`'s `tuple_get` (std::get, member `get`, then ADL `get`) |
| `TChunkedArray::Add(int32 Count)` appends `Count` default elements; elements are appended with `AddElement` | `Containers/ChunkedArray.h` | `ranges::to` tries `AddElement` before `Add` |
| `TSet`/`TMap` have named storages: `TSparseSet`/`TSparseMap`/`TSparseMultiMap` and `TCompactSet`/`TCompactMap`/`TCompactMultiMap` (UE 5.7) | `Containers/Set.h`, `Map.h`, `CompactSet.h` | one trait per type |
| Every container has `Reserve` (as UE), so `ranges::to` reserves through it | all containers | `ranges::to`'s reserve branch |
| `TStringView`'s template head is **constrained** (`UE::CCharType`) and only declared by `CoreMinimal.h` | `Containers/ContainersFwd.h`, `StringView.h` | the unconstrained `range_traits_impl<TStringView<C>>` partial specialization |
| `CoreMinimal.h` only forward-declares `TSortedMap`/`TSortedSet`/`TCompactSet`/`TStridedView`/`FAnsiString`/`FUtf8String` and does **not** declare `TDeque`, `TStaticArray`, `TIndirectArray`, `TChunkedArray`, `TRingBuffer` or the linked lists | `CoreMinimal.h`, `Containers/ContainersFwd.h` | `ue_range_traits.h` forward-declares them |

Deliberately stricter than UE: the `TDeque` iterator (UE's is default-constructible and
assignable), and the `!=`-only iterators of `TSet`/`TMap`, `TSparseArray`, `TBitArray` and the
linked lists, which have `==` in UE (some from before 5.3), so the `!=` path stays tested in
every version.

Not faked: `TList` and `TCircularBuffer`. Neither has `begin`/`end`, so neither is a range
([range.range]); `ue_range_traits.h` gives them no trait.

## Engine version

`ZR_FAKE_UE_VERSION` (100 * major + minor, 503 to 507, default 507) selects the engine shape
where versions differ and neither shape is a subset of the other; a shape that is stricter in
one version and works unchanged in the other needs no gate.

| From | Change | Where |
|---|---|---|
| 5.4 | `TBitArray` has `begin`/`end` (a range) | `Containers/BitArray.h` |
| 5.7 | `TCheckedPointerIterator` has `==` and `operator->` | `Containers/Array.h` |

CI builds the debug configuration again with `ZR_FAKE_UE_VERSION=503`. zr never reads the
version: it detects each shape (concepts, `ue_range_traits.h`).

## The debug/shipping switch

`ZR_FAKE_UE_RANGED_FOR_CHECKS` mirrors UE's `TARRAY_RANGED_FOR_CHECKS`:

- **`0` (default, shipping-like)**: `TArray::begin()` returns a raw `T*`, a fully
  conforming, semiregular, equality-comparable iterator.
- **`1` (debug-like)**: it returns `TCheckedPointerIterator<T>`, a non-semiregular class,
  `!=`-only before 5.7.

CI builds the test suite both ways, so every test runs against both iterator types.
