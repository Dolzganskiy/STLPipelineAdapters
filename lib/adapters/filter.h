#pragma once
#include "processing.h"
#include <optional>

template<typename Flow, typename Pred>
class FilterFlow {
public:
    using value_type = typename Flow::value_type;

    FilterFlow(Flow flow, Pred pred) : flow_(std::move(flow)) pred_(std::move(pred)) {}

    std::optional<value_type Next() {

        while(true) {
            std::optional<value_type> v = flow_.Next();
            if (!v) return std::nullopt;
            if (pred_(*v)) return v;
        }
    }
private:
    Flow flow_;
    Pred pred_;
};

template<typename Pred>
class FilterAdapter {
public:
    FilterAdapter(Pred pred) : pred_(pred) {}

    template<typename Flow>
    auto operator()(Flow flow) {
        return FilterFlow<Flow, Pred>(std::move(flow), pred_);
    }
private:
    Pred pred_;
};

template<typename Pred>
auto Filter(Pred pred) {
    return FilterAdapter<Pred>(pred);
}