// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <initializer_list>
#include <list>
#include <utility>

// Intrusive singly linked lists: a node is its own list head, iterated from that node. begin/end
// are HIDDEN FRIENDS of the CRTP base (found by ADL only, through the base). Not sized.
template <class NodeType> class TLinkedListIteratorBase {
public:
    explicit TLinkedListIteratorBase(NodeType* node)
        : node_(node) {}

    TLinkedListIteratorBase& operator++() {
        node_ = node_->Next();
        return *this;
    }

    explicit operator bool() const { return node_ != nullptr; }

    bool operator!=(const TLinkedListIteratorBase& rhs) const { return node_ != rhs.node_; }

protected:
    NodeType* node_;
};

// Yields the node's element (**node).
template <class ContainerType, class ElementType> class TLinkedListIterator : public TLinkedListIteratorBase<ContainerType> {
public:
    using TLinkedListIteratorBase<ContainerType>::TLinkedListIteratorBase;

    ElementType& operator*() const { return **this->node_; }
};

// Yields the node itself (*node).
template <class ContainerType, class ElementType> class TIntrusiveLinkedListIterator : public TLinkedListIteratorBase<ElementType> {
public:
    using TLinkedListIteratorBase<ElementType>::TLinkedListIteratorBase;

    ElementType& operator*() const { return *this->node_; }
};

template <class ContainerType, class ElementType, template <class, class> class IteratorType> class TLinkedListBase {
public:
    using TIterator = IteratorType<ContainerType, ElementType>;
    using TConstIterator = IteratorType<ContainerType, const ElementType>;

    void LinkHead(ContainerType*& head) {
        next_ = head;
        head = static_cast<ContainerType*>(this);
    }

    ContainerType* Next() { return next_; }
    const ContainerType* Next() const { return next_; }

    friend TIterator begin(ContainerType& list) { return TIterator(&list); }
    friend TIterator end(ContainerType&) { return TIterator(nullptr); }
    friend TConstIterator begin(const ContainerType& list) { return TConstIterator(const_cast<ContainerType*>(&list)); }
    friend TConstIterator end(const ContainerType&) { return TConstIterator(nullptr); }

private:
    ContainerType* next_ = nullptr;
};

template <class ElementType> class TLinkedList : public TLinkedListBase<TLinkedList<ElementType>, ElementType, TLinkedListIterator> {
public:
    TLinkedList() = default;
    explicit TLinkedList(const ElementType& element)
        : element_(element) {}

    ElementType& operator*() { return element_; }
    const ElementType& operator*() const { return element_; }

private:
    ElementType element_{};
};

// Derived as `struct FNode : TIntrusiveLinkedList<FNode>`; the list type is FNode.
template <class ElementType> class TIntrusiveLinkedList : public TLinkedListBase<ElementType, ElementType, TIntrusiveLinkedListIterator> {};

// begin/end are HIDDEN FRIENDS (found by ADL only), not members. Bidirectional iterator.
// No ElementType member typedef, as UE.
template <class ElementType> class TDoubleLinkedList {
public:
    template <class UnderlyingIt, class ItElementType> class TDoubleLinkedListIterator {
    public:
        explicit TDoubleLinkedListIterator(UnderlyingIt it)
            : it_(std::move(it)) {}

        ItElementType& operator*() const { return *it_; }

        TDoubleLinkedListIterator& operator++() {
            ++it_;
            return *this;
        }

        TDoubleLinkedListIterator& operator--() {
            --it_;
            return *this;
        }

        bool operator!=(const TDoubleLinkedListIterator& rhs) const { return it_ != rhs.it_; }

    private:
        UnderlyingIt it_;
    };

    using TIterator = TDoubleLinkedListIterator<typename std::list<ElementType>::iterator, ElementType>;
    using TConstIterator = TDoubleLinkedListIterator<typename std::list<ElementType>::const_iterator, const ElementType>;

    TDoubleLinkedList() = default;
    TDoubleLinkedList(std::initializer_list<ElementType> init)
        : nodes_(init) {}

    int32 Num() const { return static_cast<int32>(nodes_.size()); }

    bool AddTail(const ElementType& element) {
        nodes_.push_back(element);
        return true;
    }

    friend TIterator begin(TDoubleLinkedList& list) { return TIterator(list.nodes_.begin()); }
    friend TIterator end(TDoubleLinkedList& list) { return TIterator(list.nodes_.end()); }
    friend TConstIterator begin(const TDoubleLinkedList& list) { return TConstIterator(list.nodes_.begin()); }
    friend TConstIterator end(const TDoubleLinkedList& list) { return TConstIterator(list.nodes_.end()); }

private:
    std::list<ElementType> nodes_;
};
