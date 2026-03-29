#pragma once
#include <utility>
#include <optional>
#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Flow, typename Pred>
class FilterFlow : public FlowRangeMixin<FilterFlow<Flow, Pred>>{
public:
    using value_type = typename Flow::value_type;

    FilterFlow(Flow flow, Pred pred) 
        : flow_(std::move(flow)), pred_(std::move(pred)) {}

    std::optional<value_type> Next() {
        while(true) {
            auto v = flow_.Next();
            if (!v) return std::nullopt;
            if (pred_(Unwrap(*v))) return v;
        }
    }
private:
    Flow flow_;
    Pred pred_;
};

template<typename Pred>
class FilterAdapter {
public:
    explicit FilterAdapter(Pred pred) : pred_(std::move(pred)) {}

    template<typename Flow>
    auto operator()(Flow flow) const {
        return FilterFlow<Flow, Pred>(std::move(flow), pred_);
    }

private:
    Pred pred_;
};

template<typename Pred>
inline auto Filter(Pred pred) {
    return FilterAdapter<Pred>(std::move(pred));
}