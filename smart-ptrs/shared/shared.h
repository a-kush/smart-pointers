#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <utility>

// https://en.cppreference.com/w/cpp/memory/shared_ptr

struct ControlBlockBase {
    void IncRefCnt() {
        ++ref_cnt_;
    }

    size_t GetRefCnt() {
        return ref_cnt_;
    }

    virtual void DecRefCnt() = 0;

    size_t ref_cnt_ = 0;
};

template <typename T>
struct ControlBlockNew : ControlBlockBase {
    ControlBlockNew(T* ptr) : ptr_(ptr) {
    }

    void DecRefCnt() override {
        --(this->ref_cnt_);
        if (this->ref_cnt_ == 0) {
            delete ptr_;
            delete this;
        }
    }

    T* ptr_;
};

template <typename T>
struct ControlBlockMakeShared : ControlBlockBase {
    template <typename... Args>
    ControlBlockMakeShared(T*& ptr, Args&&... args) : object(std::forward<Args>(args)...) {
        ptr = &object;
        this->IncRefCnt();
    }

    void DecRefCnt() override {
        --(this->ref_cnt_);
        if (this->ref_cnt_ == 0) {
            delete this;
        }
    }

    T object;
};

template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // All template shit:

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncRefCnt();
        }
    }
    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        if (block_) {
            block_->DecRefCnt();
        }
        if (ptr_ != other.ptr_) {
            block_ = other.block_;
            ptr_ = other.ptr_;
            if (block_) {
                block_->IncRefCnt();
            }
        }
        return *this;
    }
    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        if (ptr_ != other.ptr_) {
            if (block_) {
                block_->DecRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // MakeSharedConstructor

    SharedPtr(ControlBlockBase* block, T* ptr) : block_(block), ptr_(ptr) {
    }

    // Constructors

    SharedPtr() {
    }
    SharedPtr(std::nullptr_t) {
    }
    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        block_ = new ControlBlockNew(ptr);
        block_->IncRefCnt();
        ptr_ = ptr;
    }

    SharedPtr(const SharedPtr& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncRefCnt();
        }
    }
    SharedPtr(SharedPtr&& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        block_ = other.block_;
        ptr_ = ptr;
        if (block_) {
            block_->IncRefCnt();
        }
    }

    /*// Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);*/

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            if (block_) {
                block_->DecRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            if (block_) {
                block_->IncRefCnt();
            }
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            if (block_) {
                block_->DecRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_) {
            block_->DecRefCnt();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->DecRefCnt();
            block_ = nullptr;
            ptr_ = nullptr;
        }
    }
    template <typename Y>
    void Reset(Y* ptr) {
        if (block_) {
            block_->DecRefCnt();
        }
        block_ = new ControlBlockNew(ptr);
        ptr_ = ptr;
        block_->IncRefCnt();
    }
    void Swap(SharedPtr& other) {
        std::swap(*this, other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
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
        if (block_) {
            return block_->GetRefCnt();
        }
        return 0;
    }
    explicit operator bool() const {
        return block_ != nullptr;
    }

private:
    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    T* ptr = nullptr;
    return SharedPtr<T>(new ControlBlockMakeShared<T>(ptr, std::forward<Args>(args)...), ptr);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
