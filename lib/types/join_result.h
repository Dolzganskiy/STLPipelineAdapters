#pragma once

template <typename Base, typename Joined>
struct JoinResult {
	Base base;
	std::optional<Joined> joined;
};