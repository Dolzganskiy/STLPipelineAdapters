#pragma once

template<typename It>
class Flowiterator {
public:

    using value_type = typename std::iterator_traits<It>::value_type;

    Flowiterator(It cur, It end) : current_(cur), end_(end) {}

    std::optional<value_type> Next() {
        if (current_ == end_) {
            return std::nullopt;
        }
        return *current_++;
    }
    
private:
    It current_;
    It end_;
};

template<typename Container>
auto AsDataFlow(Container& container) {
    return Flowiterator(container.begin(), container.end());
}

template<typename Container>
auto AsDataFlow(const Container& container) {
    return Flowiterator(container.cbegin(), container.cend());
}