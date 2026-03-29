#pragma once

template <typename Base, typename Joined>
struct JoinResult {
	Base base;
	std::optional<Joined> joined;
};

template <typename Base, typename Joined>
bool operator==(const JoinResult<Base, Joined>& lhs,
                const JoinResult<Base, Joined>& rhs) {
    return lhs.base == rhs.base && lhs.joined == rhs.joined;
}