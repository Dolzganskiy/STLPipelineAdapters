#pragma once
#include <utility>

template <class Flow, class Adapter>
requires requires(Adapter a, Flow f) {
    a(std::move(f));
}
auto operator|(Flow flow, Adapter adapter) {
    return adapter(std::move(flow));
}