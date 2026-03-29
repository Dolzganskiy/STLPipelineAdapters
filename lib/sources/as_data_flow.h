#pragma once

#include <functional>
#include <iterator>
#include <optional>

#include "../flowiterator.h"

template<typename It>
class Flowiterator : public FlowRangeMixin<Flowiterator<It>> {
public:
    using raw_value_type = typename std::iterator_traits<It>::value_type;
    using value_type = std::reference_wrapper<raw_value_type>;

    Flowiterator(It cur, It end) : current_(cur), end_(end) {}

    std::optional<value_type> Next() {
        if (current_ == end_) {
            return std::nullopt;
        }
        return std::ref(*current_++);
    }

private:
    It current_;
    It end_;
};

template<typename Container>
inline auto AsDataFlow(Container& container) {
    return Flowiterator(container.begin(), container.end());
}

template<typename Container>
inline auto AsDataFlow(const Container& container) {
    return Flowiterator(container.begin(), container.end());
}