#pragma once

#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Init, typename Aggregator, typename KeySelector, typename Flow>
class AggregateByKeyFlow : public FlowRangeMixin<AggregateByKeyFlow<Init, Aggregator, KeySelector, Flow>> {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = decltype(Unwrap(std::declval<input_type&>()));
    using key_type = std::remove_cvref_t<std::invoke_result_t<KeySelector, unwrapped_input_type>>;
    using value_type = std::pair<key_type, Init>;
    using Iterator = typename std::vector<value_type>::iterator;

    AggregateByKeyFlow(Init init, Aggregator aggregator, KeySelector key_selector, Flow flow)
        : init_(std::move(init)),
          aggregator_(std::move(aggregator)),
          key_selector_(std::move(key_selector)),
          flow_(std::move(flow)) {
        std::map<key_type, Init> temp;
        std::vector<key_type> order;

        while (true) {
            auto v = flow_.Next();
            if (!v) break;

            auto& item = Unwrap(*v);
            auto key = key_selector_(item);

            if (temp.find(key) == temp.end()) {
                temp[key] = init_;
                order.push_back(key);
            }

            aggregator_(item, temp[key]);
        }

        for (auto& key : order) {
            buffer_.push_back({key, temp[key]});
        }

        it_ = buffer_.begin();
    }

    std::optional<value_type> Next() {
        if (it_ == buffer_.end()) {
            return std::nullopt;
        }

        auto result = *it_;
        ++it_;
        return result;
    }

private:
    Init init_;
    Aggregator aggregator_;
    KeySelector key_selector_;
    Flow flow_;
    Iterator it_;
    std::vector<value_type> buffer_;
};

template<typename Init, typename Aggregator, typename KeySelector>
class AggregateByKeyAdapter {
public:
    AggregateByKeyAdapter(Init init, Aggregator aggregator, KeySelector key_selector)
        : init_(std::move(init)),
          aggregator_(std::move(aggregator)),
          key_selector_(std::move(key_selector)) {}

    template<typename Flow>
    auto operator()(Flow flow) const {
        return AggregateByKeyFlow<Init, Aggregator, KeySelector, Flow>(
            init_, aggregator_, key_selector_, std::move(flow)
        );
    }

private:
    Init init_;
    Aggregator aggregator_;
    KeySelector key_selector_;
};

template<typename Init, typename Aggregator, typename KeySelector>
inline auto AggregateByKey(Init init, Aggregator aggregator, KeySelector key_selector) {
    return AggregateByKeyAdapter<Init, Aggregator, KeySelector>(
        std::move(init),
        std::move(aggregator),
        std::move(key_selector)
    );
}