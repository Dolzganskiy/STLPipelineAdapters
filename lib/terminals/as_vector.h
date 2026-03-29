#pragma once

#include <vector>
#include <type_traits>

#include "../unwrap.h"

class AsVectorAdapter {
public:
    template<typename Flow>
    auto operator()(Flow flow) {
        using input_type = typename Flow::value_type;
        using value_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type&>()))>;
        std::vector<value_type> result;

        while (true) {
            auto v = flow.Next();
            if (!v) {
                break;
            }
            result.push_back(Unwrap(*v));
        }

        return result;
    }
};

inline auto AsVector() {
    return AsVectorAdapter();
}