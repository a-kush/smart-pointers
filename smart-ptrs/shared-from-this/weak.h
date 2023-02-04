#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
    template <typename Y>
    friend class WeakPtr;

    template <typename Y>
    friend class SharedPtr;

public:
    // All template shit

    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncWeakRefCnt();
        }
    }
    template <typename Y>
    WeakPtr(WeakPtr<Y>&& other) {
        if (ptr_ != other.ptr_) {
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
    }
    template <typename Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) {
        if (block_) {
            block_->DecWeakRefCnt();
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncWeakRefCnt();
        }
        return *this;
    }
    template <typename Y>
    WeakPtr& operator=(WeakPtr<Y>&& other) {
        if (ptr_ != other.ptr_) {
            if (block_) {
                block_->DecWeakRefCnt();
            }
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Constructors

    WeakPtr() {
    }

    WeakPtr(const WeakPtr& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncWeakRefCnt();
        }
    }
    WeakPtr(WeakPtr&& other) {
        if (ptr_ != other.ptr_) {
            block_ = other.block_;
            ptr_ = other.ptr_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <typename Y>
    WeakPtr(const SharedPtr<Y>& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncWeakRefCnt();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (block_) {
            block_->DecWeakRefCnt();
        }
        block_ = other.block_;
        ptr_ = other.ptr_;
        if (block_) {
            block_->IncWeakRefCnt();
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (ptr_ != other.ptr_) {
            if (block_) {
                block_->DecWeakRefCnt();
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

    ~WeakPtr() {
        if (block_) {
            block_->DecWeakRefCnt();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            block_->DecWeakRefCnt();
            block_ = nullptr;
        }
        ptr_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(*this, other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->GetStrongRefCnt();
        }
        return 0;
    }
    bool Expired() const {
        if (!block_) {
            return 1;
        }
        return block_->GetStrongRefCnt() == 0;
    }
    SharedPtr<T> Lock() const {
        if (this->Expired()) {
            return SharedPtr<T>(nullptr);
        } else {
            return SharedPtr<T>(block_, ptr_);
        }
    }

private:
    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;
};
