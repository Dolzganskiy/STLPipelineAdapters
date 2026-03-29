#pragma once

#include <functional>

template<typename T>
T& Unwrap(T& value) {
    return value;
}

template<typename T>
T& Unwrap(std::reference_wrapper<T> ref) {
    return ref.get();
}