#pragma once

template <typename Key, typename Value>
struct KV {
	Key key;
	Value value;
};

template <typename Key, typename Value>
bool operator==(const KV<Key, Value>& lhs, const KV<Key, Value>& rhs) {
    return lhs.key == rhs.key && lhs.value == rhs.value;
}