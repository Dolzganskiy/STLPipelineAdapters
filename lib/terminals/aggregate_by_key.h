#pragma once

#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include"../flowiterator.h"

template<typename Init, typename Aggregator, typename KeySelector, typename Flow>
class AggregateByKeyFlow : public FlowRangeMixin<AggregateByKeyFlow<Init, Aggregator, KeySelector, Flow>> {
public:

    using input_type = typename Flow::value_type;
    using value_type = std::pair<std::invoke_result_t<KeySelector, input_type>, Init>;
    using Iterator = typename std::vector<std::pair<std::invoke_result_t<KeySelector, input_type>, Init>>::iterator;

    AggregateByKeyFlow(Init init, Aggregator aggregator, KeySelector key_selector, Flow flow) :
            init_(std::move(init)), aggregator_(std::move(aggregator)), 
            key_selector_(std::move(key_selector)), flow_(std::move(flow)) {
        std::map<std::invoke_result_t<KeySelector, input_type>, Init> temp;
        std::vector<std::invoke_result_t<KeySelector, input_type>> order;
        while(true) {
            auto v = flow_.Next();
            if (!v) break;
            auto key = key_selector_(*v);
            if (temp.find(key) == temp.end()) {
                temp[key] = init_;
                order.push_back(key);
            }
            aggregator_(*v, temp[key]);
        }
        for (auto& c : order) {
            buffer_.push_back({c, temp[c]});
        }
        it_ = buffer_.begin();
    }

    std::optional<value_type> Next() {
        if (it_ == buffer_.end()) return std::nullopt;
        auto [key, value] = *it_;
        ++it_;
        return value_type{key, value};
    }

private:
    Init init_;
    Aggregator aggregator_;
    KeySelector key_selector_;
    Flow flow_;
    Iterator it_;
    std::vector<std::pair<std::invoke_result_t<KeySelector, input_type>, Init>> buffer_;
};


template<typename Init, typename Aggregator, typename KeySelector>
class AggregateByKeyAdapter {
public:

    AggregateByKeyAdapter(Init init, Aggregator aggregator, KeySelector key_selector) :
            init_(std::move(init)), aggregator_(std::move(aggregator)), key_selector_(std::move(key_selector)) {}

    template<typename Flow>
    auto operator()(Flow flow) {
        return AggregateByKeyFlow<Init, Aggregator, KeySelector, Flow>( 
        init_, aggregator_, key_selector_, std::move(flow));
    }

private:
    Init init_;
    Aggregator aggregator_;
    KeySelector key_selector_;
};

template<typename Init, typename Aggregator, typename KeySelector>
inline auto AggregateByKey(Init init, Aggregator aggregator, KeySelector key_selector) {
    return AggregateByKeyAdapter<Init, Aggregator, KeySelector>(init, aggregator, key_selector);
}