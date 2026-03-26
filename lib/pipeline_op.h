#pragma once
#include <utility>

template<typename Flow, typename Adapter>
auto operator|(Flow flow, Adapter adapter) {
    return adapter(std::move(flow));
}