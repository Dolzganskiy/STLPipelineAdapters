#pragma once

#include <optional>
#include <unordered_map>
#include <utility>
#include <type_traits>

#include "../types/kv.h"
#include "../types/join_result.h"
#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Left, typename Right, typename LKeySel, typename RKeySel>
class JoinFlow : public FlowRangeMixin<JoinFlow<Left, Right, LKeySel, RKeySel>> {
public:
    using left_input_type = typename Left::value_type;
    using right_input_type = typename Right::value_type;

    using left_value_type = std::remove_cvref_t<decltype(Unwrap(std::declval<left_input_type&>()))>;
    using right_value_type = std::remove_cvref_t<decltype(Unwrap(std::declval<right_input_type&>()))>;

    using left_key_type = std::remove_cvref_t<std::invoke_result_t<LKeySel, left_value_type>>;
    using right_key_type = std::remove_cvref_t<std::invoke_result_t<RKeySel, right_value_type>>;

    using value_type = JoinResult<left_value_type, right_value_type>;

    JoinFlow(Left left, Right right, LKeySel lkey_sel, RKeySel rkey_sel)
        : left_(std::move(left)),
          lkey_sel_(std::move(lkey_sel)),
          rkey_sel_(std::move(rkey_sel)) {
        while (auto r = right.Next()) {
            auto& right_item = Unwrap(*r);
            right_map_[rkey_sel_(right_item)] = right_item;
        }
    }

    std::optional<value_type> Next() {
        auto l = left_.Next();
        if (!l) {
            return std::nullopt;
        }

        auto& left_item = Unwrap(*l);
        auto key = lkey_sel_(left_item);
        auto it = right_map_.find(key);

        if (it == right_map_.end()) {
            return value_type{left_item, std::nullopt};
        }

        return value_type{left_item, it->second};
    }

private:
    Left left_;
    LKeySel lkey_sel_;
    RKeySel rkey_sel_;
    std::unordered_map<left_key_type, right_value_type> right_map_;
};

template<typename Left, typename Right>
class JoinKVFlow : public FlowRangeMixin<JoinKVFlow<Left, Right>> {
public:
    using left_input_type = typename Left::value_type;
    using right_input_type = typename Right::value_type;

    using left_kv_type = std::remove_cvref_t<decltype(Unwrap(std::declval<left_input_type>()))>;
    using right_kv_type = std::remove_cvref_t<decltype(Unwrap(std::declval<right_input_type>()))>;

    using key_type = std::remove_cvref_t<decltype(std::declval<left_kv_type>().key)>;
    using left_value_type = std::remove_cvref_t<decltype(std::declval<left_kv_type>().value)>;
    using right_value_type = std::remove_cvref_t<decltype(std::declval<right_kv_type>().value)>;

    using value_type = JoinResult<left_value_type, right_value_type>;

    JoinKVFlow(Left left, Right right) : left_(std::move(left)) {
        while (auto r = right.Next()) {
            auto& right_item = Unwrap(*r);
            right_map_[right_item.key] = right_item.value;
        }
    }

    std::optional<value_type> Next() {
        auto l = left_.Next();
        if (!l) {
            return std::nullopt;
        }

        auto& left_item = Unwrap(*l);
        auto it = right_map_.find(left_item.key);

        if (it == right_map_.end()) {
            return value_type{left_item.value, std::nullopt};
        }

        return value_type{left_item.value, it->second};
    }

private:
    Left left_;
    std::unordered_map<key_type, right_value_type> right_map_;
};

template<typename Right>
class JoinKVAdapter {
public:
    explicit JoinKVAdapter(Right flow) : right_(std::move(flow)) {}

    template<typename Left>
    auto operator()(Left left) {
        return JoinKVFlow<Left, Right>(std::move(left), std::move(right_));
    }

private:
    Right right_;
};

template<typename Right, typename LKeySel, typename RKeySel>
class JoinComporatorsAdapter {
public:
    JoinComporatorsAdapter(Right flow, LKeySel lkey_sel, RKeySel rkey_sel)
        : right_(std::move(flow)),
          lkey_sel_(std::move(lkey_sel)),
          rkey_sel_(std::move(rkey_sel)) {}

    template<typename Left>
    auto operator()(Left flow) {
        return JoinFlow<Left, Right, LKeySel, RKeySel>(
            std::move(flow),
            std::move(right_),
            lkey_sel_,
            rkey_sel_
        );
    }

private:
    Right right_;
    LKeySel lkey_sel_;
    RKeySel rkey_sel_;
};

template<typename Flow>
inline auto Join(Flow flow) {
    return JoinKVAdapter<Flow>(std::move(flow));
}

template<typename Flow, typename LKeySel, typename RKeySel>
inline auto Join(Flow flow, LKeySel left, RKeySel right) {
    return JoinComporatorsAdapter<Flow, LKeySel, RKeySel>(
        std::move(flow),
        std::move(left),
        std::move(right)
    );
}