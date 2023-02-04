#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

#include <type_traits>
#include <iostream>

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    }
    size_t DecRef() {
        --count_;
        return count_;
    }
    size_t RefCount() const {
        return count_;
    }

    SimpleCounter& operator=(SimpleCounter&& other) {
        return *this;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    RefCounted() {
    }

    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        auto cur_state = counter_.DecRef();
        if (cur_state == 0) {
            Deleter temp_deleter;
            Derived* temp_ptr = static_cast<Derived*>(this);
            temp_deleter.Destroy(temp_ptr);
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() {
    }
    IntrusivePtr(std::nullptr_t) : ptr_(nullptr) {
    }
    IntrusivePtr(T* ptr) {
        ptr_ = ptr;
        ptr_->IncRef();
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) {
        ptr_ = other.ptr_;
        if (ptr_) {
            other.ptr_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) {
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) {
        ptr_ = other.ptr_;
        if (ptr_) {
            other.ptr_->IncRef();
        }
    }
    IntrusivePtr(IntrusivePtr&& other) {
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (ptr_ != other.ptr_) {
            if (ptr_) {
                ptr_->DecRef();
            }
            ptr_ = other.ptr_;
            if (ptr_) {
                other.ptr_->IncRef();
            }
        }
        return *this;
    }
    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (ptr_ != other.ptr_) {
            if (ptr_) {
                ptr_->DecRef();
            }
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        if (ptr_) {
            ptr_->DecRef();
        }
    }

    // Modifiers
    void Reset() {
        if (ptr_) {
            ptr_->DecRef();
            ptr_ = nullptr;
        }
    }
    void Reset(T* ptr) {
        if (ptr_) {
            ptr_->DecRef();
        }
        ptr_ = ptr;
        ptr_->IncRef();
    }
    void Swap(IntrusivePtr& other) {
        std::swap(*this, other);
    }

    // Observers
    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (ptr_) {
            return ptr_->RefCount();
        }
        return 0;
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }

private:
    T* ptr_ = nullptr;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    T* ptr = new T(args...);
    return IntrusivePtr(ptr);
}
