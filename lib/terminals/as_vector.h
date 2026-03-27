#pragma once

#include <vector>

class AsVectorAdapter {
public:
    template<typename Flow>
    std::vector<typename Flow::value_type> operator()(Flow flow) {
        std::vector<typename Flow::value_type> result;
        
        while (true) {
            auto v = flow.Next();
            if (!v) break;
            result.push_back(*v);
        }
        return result;
    }
};

inline auto AsVector() {
    return AsVectorAdapter();
}