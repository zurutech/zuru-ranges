// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include <memory>

enum class ESPMode : unsigned char { NotThreadSafe, ThreadSafe };

// Engagement is IsValid() or an EXPLICIT operator bool. operator* yields an lvalue into the
// shared object even on an rvalue pointer. Two template parameters, as UE.
template <class ObjectType, ESPMode Mode = ESPMode::ThreadSafe> class TSharedPtr {
public:
    TSharedPtr() = default;
    // Takes ownership.
    explicit TSharedPtr(ObjectType* object)
        : object_(object) {}

    bool IsValid() const { return object_ != nullptr; }
    explicit operator bool() const { return IsValid(); }

    ObjectType* Get() const { return object_.get(); }
    ObjectType& operator*() const { return *object_; }
    ObjectType* operator->() const { return object_.get(); }

private:
    std::shared_ptr<ObjectType> object_;
};

// Never null: no operator bool, so not a nullable object.
template <class ObjectType, ESPMode Mode = ESPMode::ThreadSafe> class TSharedRef {
public:
    // Takes ownership; UE asserts the pointer is not null.
    explicit TSharedRef(ObjectType* object)
        : object_(object) {}

    ObjectType& Get() const { return *object_; }
    ObjectType& operator*() const { return *object_; }
    ObjectType* operator->() const { return object_.get(); }

private:
    std::shared_ptr<ObjectType> object_;
};
