#pragma once

#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Flow>
class SplitFlow : public FlowRangeMixin<SplitFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using source_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type&>()))>;
    using value_type = std::string;

    SplitFlow(Flow flow, std::string delims)
        : flow_(std::move(flow)), delims_(std::move(delims)) {}

    std::optional<value_type> Next() {
        if constexpr (std::is_same_v<source_type, std::string>) {
            return NextFromString();
        } else {
            return NextFromStream();
        }
    }

private:
    bool is_delim(char c) const {
        return delims_.find(c) != std::string::npos;
    }

    std::optional<value_type> NextFromString() {
        while (true) {
            if (!has_current_string_) {
                auto next_chunk = flow_.Next();
                if (!next_chunk) {
                    return std::nullopt;
                }
                current_string_ = Unwrap(*next_chunk);
                pos_ = 0;
                has_current_string_ = true;
            }

            if (pos_ > current_string_.size()) {
                has_current_string_ = false;
                current_string_.clear();
                pos_ = 0;
                continue;
            }

            std::size_t start = pos_;

            while (pos_ < current_string_.size() && !is_delim(current_string_[pos_])) {
                ++pos_;
            }

            std::string token = current_string_.substr(start, pos_ - start);

            if (pos_ < current_string_.size() && is_delim(current_string_[pos_])) {
                ++pos_;
            } else if (pos_ == current_string_.size()) {
                has_current_string_ = false;
                current_string_.clear();
                pos_ = 0;
            }

            return token;
        }
    }

    std::optional<value_type> NextFromStream() {
        while (true) {
            if (!current_stream_) {
                auto next_chunk = flow_.Next();
                if (!next_chunk) {
                    return std::nullopt;
                }
                current_stream_ = &Unwrap(*next_chunk);
            }

            std::string token;
            char ch;

            while (current_stream_->get(ch)) {
                if (is_delim(ch)) {
                    return token;   // даже если token пустой
                }
                token.push_back(ch);
            }

            if (current_stream_->eof()) {
                current_stream_ = nullptr;
                return token;       // вернуть последний токен, даже пустой
            }

            current_stream_ = nullptr;
        }
    }

private:
    Flow flow_;
    std::string delims_;

    std::string current_string_;
    std::size_t pos_ = 0;
    bool has_current_string_ = false;

    std::istream* current_stream_ = nullptr;
};

class SplitAdapter {
public:
    explicit SplitAdapter(std::string delims) : delims_(std::move(delims)) {}

    template<typename Flow>
    auto operator()(Flow flow) const {
        return SplitFlow<Flow>(std::move(flow), delims_);
    }

private:
    std::string delims_;
};

inline auto Split(std::string delims) {
    return SplitAdapter(std::move(delims));
}