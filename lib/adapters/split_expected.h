#pragma once

#include <deque>
#include <expected>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename T>
struct expected_traits;

template<typename T, typename E>
struct expected_traits<std::expected<T, E>> {
    using value_type = T;
    using error_type = E;
};

template<typename Flow>
class SplitExpectedState {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type&>()))>;

    using expected_type = unwrapped_input_type;
    using ok_type = typename expected_traits<expected_type>::value_type;
    using err_type = typename expected_traits<expected_type>::error_type;

    explicit SplitExpectedState(Flow flow) : flow_(std::move(flow)) {}

    std::optional<ok_type> NextOk() {
        while (ok_buffer_.empty()) {
            if (!Pump()) {
                return std::nullopt;
            }
        }

        auto value = std::move(ok_buffer_.front());
        ok_buffer_.pop_front();
        return value;
    }

    std::optional<err_type> NextErr() {
        while (err_buffer_.empty()) {
            if (!Pump()) {
                return std::nullopt;
            }
        }

        auto value = std::move(err_buffer_.front());
        err_buffer_.pop_front();
        return value;
    }

private:
    bool Pump() {
        if (finished_) {
            return false;
        }

        auto item = flow_.Next();
        if (!item) {
            finished_ = true;
            return false;
        }

        auto& exp = Unwrap(*item);
        if (exp.has_value()) {
            ok_buffer_.push_back(exp.value());
        } else {
            err_buffer_.push_back(exp.error());
        }

        return true;
    }

private:
    Flow flow_;
    std::deque<ok_type> ok_buffer_;
    std::deque<err_type> err_buffer_;
    bool finished_ = false;
};

template<typename Flow>
class SplitExpectedUnexpectedFlow : public FlowRangeMixin<SplitExpectedUnexpectedFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type&>()))>;
    using value_type = typename expected_traits<unwrapped_input_type>::error_type;

    explicit SplitExpectedUnexpectedFlow(std::shared_ptr<SplitExpectedState<Flow>> state)
        : state_(std::move(state)) {}

    std::optional<value_type> Next() {
        return state_->NextErr();
    }

private:
    std::shared_ptr<SplitExpectedState<Flow>> state_;
};

template<typename Flow>
class SplitExpectedGoodFlow : public FlowRangeMixin<SplitExpectedGoodFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using unwrapped_input_type = std::remove_cvref_t<decltype(Unwrap(std::declval<input_type&>()))>;
    using value_type = typename expected_traits<unwrapped_input_type>::value_type;

    explicit SplitExpectedGoodFlow(std::shared_ptr<SplitExpectedState<Flow>> state)
        : state_(std::move(state)) {}

    std::optional<value_type> Next() {
        return state_->NextOk();
    }

private:
    std::shared_ptr<SplitExpectedState<Flow>> state_;
};

class SplitExpectedAdapter {
public:
    template<typename Flow>
    auto operator()(Flow flow) const {
        auto state = std::make_shared<SplitExpectedState<Flow>>(std::move(flow));
        return std::make_pair(
            SplitExpectedUnexpectedFlow<Flow>(state),
            SplitExpectedGoodFlow<Flow>(state)
        );
    }
};

inline auto SplitExpected() {
    return SplitExpectedAdapter();
}