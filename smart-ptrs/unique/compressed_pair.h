#pragma once
#include <utility>

template <typename T, std::size_t I, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct CompressedPairElement {
    CompressedPairElement() {
        value = T();
    }
    CompressedPairElement(T&& element) : value(std::forward<T>(element)) {
    }

    T& GetElement() {
        return value;
    }

    const T& GetElement() const {
        return value;
    }

    T value;
};

template <typename T, std::size_t I>
struct CompressedPairElement<T, I, true> : public T {
    CompressedPairElement() {
    }
    CompressedPairElement(T&& element) {
    }

    T& GetElement() {
        return *this;
    }

    const T& GetElement() const {
        return *this;  // casts because derived from T !!!
    };
};

template <typename F, typename S>
class CompressedPair : public CompressedPairElement<F, 0>, public CompressedPairElement<S, 1> {
    using First = CompressedPairElement<F, 0>;
    using Second = CompressedPairElement<S, 1>;

public:
    CompressedPair() = default;

    CompressedPair(F first, S second)
        : First(std::forward<F>(first)), Second(std::forward<S>(second)) {
    }

    const F& GetFirst() const {
        return First::GetElement();
    }

    const S& GetSecond() const {
        return Second::GetElement();
    }

    F& GetFirst() {
        return First::GetElement();
    }

    S& GetSecond() {
        return Second::GetElement();
    }
};
