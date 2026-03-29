#pragma once

#include <optional>
#include <unordered_map>
#include <utility>
#include <type_traits>

#include "../types/kv.h"
#include "../types/join_result.h"
#include "../flowiterator.h"

template<typename Left, typename Right, typename LKeySel, typename RKeySel>
class JoinFlow : public FlowRangeMixin<JoinFlow<Left, Right, LKeySel, RKeySel>> {
public:
    using left_value_type = typename Left::value_type;
    using right_value_type = typename Right::value_type;

    using left_key_type = std::invoke_result_t<LKeySel, left_value_type>;
    using right_key_type = std::invoke_result_t<RKeySel, right_value_type>;

    using value_type = JoinResult<left_value_type, right_value_type>;

    JoinFlow(Left left, Right right, LKeySel lkey_sel, RKeySel rkey_sel)
        : left_(std::move(left)), lkey_sel_(std::move(lkey_sel)), rkey_sel_(std::move(rkey_sel)) {
        while (auto r = right.Next()) {
            right_map_[rkey_sel_(*r)] = *r;
        }
    }

    std::optional<value_type> Next() {
        auto l = left_.Next();
        if (!l) {
            return std::nullopt;
        }

        auto key = lkey_sel_(*l);
        auto it = right_map_.find(key);

        if (it == right_map_.end()) {
            return value_type{*l, std::nullopt};
        }

        return value_type{*l, it->second};
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
    using left_kv_type = typename Left::value_type;
    using right_kv_type = typename Right::value_type;

    using key_type = decltype(std::declval<left_kv_type>().key);
    using left_value_type = decltype(std::declval<left_kv_type>().value);
    using right_value_type = decltype(std::declval<right_kv_type>().value);

    using value_type = JoinResult<left_value_type, right_value_type>;

    JoinKVFlow(Left left, Right right) : left_(std::move(left)) {
        while (auto r = right.Next()) {
            right_map_[r->key] = r->value;
        }
    }

    std::optional<value_type> Next() {
        auto l = left_.Next();
        if (!l) {
            return std::nullopt;
        }

        auto it = right_map_.find(l->key);
        if (it == right_map_.end()) {
            return value_type{l->value, std::nullopt};
        }

        return value_type{l->value, it->second};
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
        : right_(std::move(flow)), lkey_sel_(std::move(lkey_sel)), rkey_sel_(std::move(rkey_sel)) {}

    template<typename Left>
    auto operator()(Left flow) {
        return JoinFlow<Left, Right, LKeySel, RKeySel>(std::move(flow), right_, lkey_sel_, rkey_sel_);
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
    return JoinComporatorsAdapter<Flow, LKeySel, RKeySel>(std::move(flow), std::move(left), std::move(right));
}