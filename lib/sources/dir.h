#pragma once
#include <filesystem>
#include <optional>
#include <utility>
#include <variant>
#include <system_error>

#include "../flowiterator.h"

class DirFlowIterator : public FlowRangeMixin<DirFlowIterator> {
public:
    using value_type = std::filesystem::path;

    DirFlowIterator(std::filesystem::path path, bool recursive)
        : recursive_(recursive) {
        std::error_code ec;
        if (recursive_) {
            it_ = std::filesystem::recursive_directory_iterator(path, ec);
            if (ec) {
                it_ = std::filesystem::recursive_directory_iterator{};
            }
        } else {
            it_ = std::filesystem::directory_iterator(path, ec);
            if (ec) {
                it_ = std::filesystem::directory_iterator{};
            }
        }
    }

    std::optional<value_type> Next() {
        if (recursive_) {
            auto& it = std::get<std::filesystem::recursive_directory_iterator>(it_);
            if (it == std::filesystem::recursive_directory_iterator{}) {
                return std::nullopt;
            }
            auto result = it->path();
            ++it;
            return result;
        } else {
            auto& it = std::get<std::filesystem::directory_iterator>(it_);
            if (it == std::filesystem::directory_iterator{}) {
                return std::nullopt;
            }
            auto result = it->path();
            ++it;
            return result;
        }
    }

private:
    bool recursive_;
    std::variant<
        std::filesystem::directory_iterator,
        std::filesystem::recursive_directory_iterator
    > it_;
};

inline auto Dir(std::filesystem::path path, bool recursive) {
    return DirFlowIterator(std::move(path), recursive);
}