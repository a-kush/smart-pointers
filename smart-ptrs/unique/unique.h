#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

struct Slug {
    template <typename T>
    void operator()(T ptr) {
        delete ptr;
    }
};

struct SlugArray {
    template <typename T>
    void operator()(T ptr) {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = Slug>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    UniquePtr() {
        ptr_.GetFirst() = nullptr;
    }

    template <typename Ptr>
    UniquePtr(Ptr* ptr = nullptr) noexcept {
        ptr_.GetFirst() = ptr;
    }

    UniquePtr(T* ptr, Deleter deleter) noexcept {
        ptr_.GetFirst() = ptr;
        this->GetDeleter() = std::move(deleter);
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename Ptr>
    UniquePtr(Ptr&& other) noexcept {
        ptr_.GetFirst() = other.Get();
        this->GetDeleter() = std::move(other.GetDeleter());
        other.Release();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr operator=(const UniquePtr& other) = delete;

    template <typename Ptr>
    UniquePtr& operator=(Ptr&& other) noexcept {
        if constexpr (std::is_same_v<Ptr, decltype(NULL)>) {
            operator=(nullptr);
        } else if (this->Get() != other.Get()) {
            this->Reset();
            ptr_.GetFirst() = other.Get();
            ptr_.GetSecond() = std::move(other.GetDeleter());
            other.Release();
        }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        auto temp = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        if (temp) {
            ptr_.GetSecond()(temp);
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (ptr_.GetFirst()) {
            ptr_.GetSecond()(ptr_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto to_return = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        return to_return;
    }
    void Reset(T* ptr = nullptr) {
        auto temp = ptr_.GetFirst();
        ptr_.GetFirst() = ptr;
        if (temp) {
            ptr_.GetSecond()(temp);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(*this, other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    }
    explicit operator bool() const {
        return ptr_.GetFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    typename std::add_lvalue_reference<T>::type operator*() const {
        return *(ptr_.GetFirst());
    }
    T* operator->() const {
        return ptr_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};

// Specialization for arrays

template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    UniquePtr() {
        ptr_.GetFirst() = nullptr;
    }

    template <typename Ptr>
    UniquePtr(Ptr* ptr = nullptr) noexcept {
        ptr_.GetFirst() = ptr;
    }

    UniquePtr(T* ptr, Deleter deleter) noexcept {
        ptr_.GetFirst() = ptr;
        this->GetDeleter() = std::move(deleter);
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename Ptr>
    UniquePtr(Ptr&& other) noexcept {
        ptr_.GetFirst() = other.Get();
        this->GetDeleter() = std::move(other.GetDeleter());
        other.Release();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr operator=(const UniquePtr& other) = delete;

    template <typename Ptr>
    UniquePtr& operator=(Ptr&& other) noexcept {
        if constexpr (std::is_same_v<Ptr, decltype(NULL)>) {
            *this->operator=(nullptr);
        } else if (this->Get() != other.Get()) {
            this->Reset();
            ptr_.GetFirst() = other.Get();
            ptr_.GetSecond() = std::move(other.GetDeleter());
            other.Release();
        }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        auto temp = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        if (temp) {
            if constexpr (std::is_same_v<decltype(ptr_.GetSecond()), Slug&>) {
                SlugArray temp_deleter;
                temp_deleter(temp);
            } else {
                ptr_.GetSecond()(temp);
            }
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (ptr_.GetFirst()) {
            if constexpr (std::is_same_v<decltype(ptr_.GetSecond()), Slug&>) {
                SlugArray temp_deleter;
                temp_deleter(ptr_.GetFirst());
            } else {
                ptr_.GetSecond()(ptr_.GetFirst());
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto to_return = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        return to_return;
    }
    void Reset(T* ptr = nullptr) {
        auto temp = ptr_.GetFirst();
        ptr_.GetFirst() = ptr;
        if (temp) {
            if constexpr (std::is_same_v<decltype(ptr_.GetSecond()), Slug&>) {
                SlugArray temp_deleter;
                temp_deleter(temp);
            } else {
                ptr_.GetSecond()(temp);
            }
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(*this, other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Setter
    T& operator[](size_t i) {
        return (this->Get())[i];
    }
    // Observers

    T operator[](size_t i) const {
        return *(this->Get())[i];
    }

    T* Get() const {
        return ptr_.GetFirst();
    }
    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    }
    explicit operator bool() const {
        return ptr_.GetFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    typename std::add_lvalue_reference<T>::type operator*() const {
        return *(ptr_.GetFirst());
    }
    T* operator->() const {
        return ptr_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};
