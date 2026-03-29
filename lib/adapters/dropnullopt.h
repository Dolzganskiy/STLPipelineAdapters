#pragma once
#include <optional>
#include <utility>
#include <functional>
#include <type_traits>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename T>
struct optional_inner;

template<typename T>
struct optional_inner<std::optional<T>> {
    using type = T;
};

template<typename T>
using optional_inner_t = typename optional_inner<T>::type;

template<typename Flow>
class DropNulloptFlow : public FlowRangeMixin<DropNulloptFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type>()))>;
    using value_type = optional_inner_t<unwrapped_input_type>;

    explicit DropNulloptFlow(Flow flow) : flow_(std::move(flow)) {}

    std::optional<value_type> Next() {
        while (true) {
            auto v = flow_.Next();
            if (!v) {
                return std::nullopt;
            }

            auto& opt = Unwrap(*v);
            if (opt) {
                return *opt;
            }
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

inline auto DropNullopt() {
    return DropNulloptAdapter();
}