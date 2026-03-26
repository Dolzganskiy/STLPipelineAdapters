#pragma once
#include <optional>
#include <utility>

template<typename T>
struct optional_inner;

template<typename T> 
struct optional_inner<std::optional<T>> {
    using type = T;
};

template<typename T>
using optional_inner_t = typename optional_inner<T>::type;


template<typename Flow>
class DropNulloptFlow {
public:
    using input_type = typename Flow::value_type;
    using value_type = optional_inner_t<input_type>;

    DropNulloptFlow(Flow flow) : flow_(std::move(flow)) {}

    std::optional<value_type> Next() {
        while (true) {
            auto v = flow_.Next();

            if (!v) return std::nullopt;

            if (*v) return **v;
        }
    }

private:
    Flow flow_;
};

class DropNulloptAdapter {
public:
    template<typename Flow>
    auto operator()(Flow flow) {
        return DropNulloptFlow<Flow>(std::move(flow));
    }
};

auto DropNullopt() {
    return DropNulloptAdapter();
}