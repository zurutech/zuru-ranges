// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include <type_traits>

// The nested ElementType of a container type, cv- and reference-qualified or not.
template <class T> using TElementType_T = typename std::remove_cvref_t<T>::ElementType;
