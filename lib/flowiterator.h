#pragma once
#include <optional>

template<typename Flow>
class FlowInputIterator {
public:
    using value_type = typename Flow::value_type;

    FlowInputIterator() : flow_(nullptr) {}

    explicit FlowInputIterator(Flow* flow) : flow_(flow) {
        ++(*this);
    }

    const value_type& operator*() const {
        return *current_;
    }

    const value_type* operator->() const {
        return &(*current_);
    }

    bool operator!=(const FlowInputIterator& other) const {
        return flow_ != other.flow_;
    }

    FlowInputIterator& operator++() {
        if (flow_) {
            current_ = flow_->Next();
            if (!current_) {
                flow_ = nullptr;
            }
        }
        return *this;
    }

private:
    Flow* flow_;
    std::optional<value_type> current_;
};

template<typename Flow>
auto begin(Flow& flow) {
    return FlowInputIterator<Flow>(&flow);
}

template<typename Flow>
auto end(Flow& flow) {
    return FlowInputIterator<Flow>();
}