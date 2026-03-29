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
            if (!current_file_ || !(*current_file_)) {
                auto next_file = flow_.Next();
                if (!next_file) {
                    return std::nullopt;
                }
                current_file_ = *next_file;
            }

            std::string token;
            char ch;

            while (current_file_ && current_file_->get(ch)) {
                if (is_delim(ch)) {
                    if (!token.empty()) {
                        return token;
                    }
                } else {
                    token.push_back(ch);
                }
            }

            if (!token.empty()) {
                return token;
            }

            current_file_.reset();
        }
    }

private:
    Flow flow_;
    std::string delims_;
    std::shared_ptr<std::ifstream> current_file_;

    bool is_delim(char c) const {
        return delims_.find(c) != std::string::npos;
    }
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
