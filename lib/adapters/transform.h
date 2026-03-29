#pragma once

#include <optional>
#include <type_traits>
#include <utility>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Flow, typename Func>
class TransformFlow : public FlowRangeMixin<TransformFlow<Flow, Func>> {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = decltype(Unwrap(std::declval<input_type>()));
    using value_type = std::invoke_result_t<Func, unwrapped_input_type>;

    TransformFlow(Flow flow, Func func) 
        : flow_(std::move(flow)), func_(std::move(func)) {}

    std::optional<value_type> Next() {
        auto v = flow_.Next();
        if (!v) return std::nullopt;
        return func_(Unwrap(*v));
    }

private:
    Flow flow_;
    Func func_;
};

template<typename Func>
class TransformAdapter {
public:
    explicit TransformAdapter(Func func) : func_(std::move(func)) {}

    template<typename Flow>
    auto operator()(Flow flow) const {
        return TransformFlow<Flow, Func>(std::move(flow), func_);
    }

private:
    Func func_;
};

template<typename Func>
inline auto Transform(Func func) {
    return TransformAdapter<Func>(std::move(func));
}