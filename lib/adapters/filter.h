#pragma once
#include "processing.h"
#include <optional>

template<typename Flow, typename Pred>
class FilterFlow {
public:

    using value_type = typename Flow::value_type;

    FilterFlow(Pred pred) : pred_(std::move(pred)) {}

    auto operator()(Flow flow) {
        flow_ = std::move(flow);
    }

    std::optional<value_type Next() {

        while(true) {
            std::optional<value_type> v = flow_.Next();
            if (!v) return std::nullopt;
            if (pred_(v)) return v;
        }
    }

private:
    Pred pred_;
    Flow flow_;
};

template<typename Pred>
auto Filter(Pred pred) {
    return FilterAdapter(Pred);
}
