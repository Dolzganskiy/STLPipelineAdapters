#pragma once
#include "processing.h"

#include <vector>

class AsVectorAdapter {

    template<typename Flow>
    std::vector<typename Flow::value_type> operator()(Flow flow) {
        std::vector<typename Flow::value_type> result;
        while (true) {
            if (!flow.Next()) break;
            result.push_back(flow.Next());
        }
        return result;
    }
};


template<typename T>
auto AsVector() {
    return AsVectorAdapter()
}