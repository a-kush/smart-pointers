#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <utility>

class EnableSharedFromThisBase;
template <typename T>
class EnableSharedFromThis;

// https://en.cppreference.com/w/cpp/memory/shared_ptr

struct ControlBlockBase {
    void IncStrongRefCnt() {
        ++strong_ref_cnt_;
    }

    size_t GetStrongRefCnt() {
        return strong_ref_cnt_;
    }

    void IncWeakRefCnt() {
        ++weak_ref_cnt_;
    }

    size_t GetWeakRefCnt() {
        return weak_ref_cnt_;
    }

    virtual void DecWeakRefCnt() = 0;

    virtual void DecStrongRefCnt() = 0;

    size_t strong_ref_cnt_ = 0;
    size_t weak_ref_cnt_ = 0;
};

template <typename T>
struct ControlBlockNew : ControlBlockBase {
    ControlBlockNew(T* ptr) : ptr_(ptr) {
    }

    void DecStrongRefCnt() override {
        --strong_ref_cnt_;
        if (strong_ref_cnt_ == 0) {
            ++weak_ref_cnt_;
            delete ptr_;
            --weak_ref_cnt_;
            if (weak_ref_cnt_ == 0) {
                delete this;
            }
        }
    }

    void DecWeakRefCnt() {
        --weak_ref_cnt_;
        if (weak_ref_cnt_ == 0 && strong_ref_cnt_ == 0) {
            delete this;
        }
    }

    T* ptr_;
};

template <typename T>
struct ControlBlockMakeShared : ControlBlockBase {
    template <typename... Args>
    ControlBlockMakeShared(T*& ptr, Args&&... args) {
        new (&buffer) T(std::forward<Args>(args)...);
        ptr = reinterpret_cast<T*>(&buffer);
    }

    void DecStrongRefCnt() override {
        --(strong_ref_cnt_);
        if (strong_ref_cnt_ == 0) {
            ++weak_ref_cnt_;
            auto temp_ptr = reinterpret_cast<T*>(&buffer);
            temp_ptr->~T();
            --weak_ref_cnt_;
            if (weak_ref_cnt_ == 0) {
                delete this;
            }
        }
    }

    void DecWeakRefCnt() {
        --weak_ref_cnt_;
        if (weak_ref_cnt_ == 0 && strong_ref_cnt_ == 0) {
            delete this;
        }
    }

    alignas(T) char buffer[sizeof(T)];
};

template <typename T>
class SharedPtr {
    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // All template shit:

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncStrongRefCnt();
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
            block_->DecStrongRefCnt();
        }
        if (ptr_ != other.ptr_) {
            block_ = other.block_;
            ptr_ = other.ptr_;
            if (block_) {
                block_->IncStrongRefCnt();
            }
        }
        return *this;
    }
    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        if (ptr_ != other.ptr_) {
            if (block_) {
                block_->DecStrongRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // MakeSharedConstructor

    SharedPtr(ControlBlockBase* block, T* ptr, bool fl = false) : block_(block), ptr_(ptr) {
        block_->IncStrongRefCnt();
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            if (fl) {
                InitWeakThis(ptr_);
            }
        }
    }

    // Constructors

    SharedPtr() {
    }
    SharedPtr(std::nullptr_t) {
    }
    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        block_ = new ControlBlockNew(ptr);
        block_->IncStrongRefCnt();
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr_);
        }
    }

    SharedPtr(const SharedPtr& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncStrongRefCnt();
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
            block_->IncStrongRefCnt();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        operator=(std::move(other.Lock()));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            if (block_) {
                block_->DecStrongRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            if (block_) {
                block_->IncStrongRefCnt();
            }
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            if (block_) {
                block_->DecStrongRefCnt();
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
            block_->DecStrongRefCnt();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->DecStrongRefCnt();
            block_ = nullptr;
            ptr_ = nullptr;
        }
    }

    template <typename Y>
    void Reset(Y* ptr) {
        if (block_) {
            block_->DecStrongRefCnt();
        }
        block_ = new ControlBlockNew(ptr);
        ptr_ = ptr;
        block_->IncStrongRefCnt();
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
            return block_->GetStrongRefCnt();
        }
        return 0;
    }
    explicit operator bool() const {
        return block_ != nullptr;
    }

private:
    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* esft_ptr) {
        esft_ptr->weak_this_ = *this;
    }

    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    T* ptr = nullptr;
    return SharedPtr<T>(new ControlBlockMakeShared<T>(ptr, std::forward<Args>(args)...), ptr, true);
}

class EnableSharedFromThisBase {};

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
    template <typename Y>
    friend class SharedPtr;

public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr(weak_this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr(weak_this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    }

private:
    WeakPtr<T> weak_this_;
};
