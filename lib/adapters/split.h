#pragma once
#pragma once

#include <optional>
#include <string>
#include <utility>

#include "../flowiterator.h"


template<typename Flow>
class SplitFlow : public FlowRangeMixin<SplitFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using value_type = std::string;

    SplitFlow(Flow flow, std::string delims)
        : flow_(std::move(flow)), delims_(std::move(delims)) {}

    std::optional<value_type> Next() {
        while (true) {
            if (has_current_) {
                skip_delims();
                if (pos_ < current_.size()) {
                    std::size_t start = pos_;
                    while (pos_ < current_.size() && !is_delim(current_[pos_])) {
                        ++pos_;
                    }
                    return current_.substr(start, pos_ - start);
                }
                has_current_ = false;
            }

            auto next_chunk = flow_.Next();
            if (!next_chunk) {
                return std::nullopt;
            }

            current_ = *next_chunk;
            pos_ = 0;
            has_current_ = true;
        }
    }

private:
    bool is_delim(char c) const {
        return delims_.find(c) != std::string::npos;
    }

    void skip_delims() {
        while (pos_ < current_.size() && is_delim(current_[pos_])) {
            ++pos_;
        }
    }

private:
    Flow flow_;
    std::string delims_;

    std::string current_;
    std::size_t pos_ = 0;
    bool has_current_ = false;
};

class SplitAdapter {
public:
    explicit SplitAdapter(std::string delims) : delims_(std::move(delims)) {}

    template<typename Flow>
    auto operator()(Flow flow) {
        return SplitFlow<Flow>(std::move(flow), delims_);
    }

private:
    std::string delims_;
};

inline auto Split(std::string delims) {
    return SplitAdapter(std::move(delims));
}
