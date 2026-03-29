#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "../flowiterator.h"

class DirFlowIterator : public FlowRangeMixin<DirFlowIterator> {
public:
    using value_type = std::filesystem::path;

    DirFlowIterator(std::string path, bool recursive) 
        : recursive_(recursive) {
        if (recursive_) {
            it_ = std::filesystem::recursive_directory_iterator(path);
        } else {
            it_ = std::filesystem::directory_iterator(path);
        }
    }

    std::optional<value_type> Next() {
        if (recursive_) {
            auto& it = std::get<std::filesystem::recursive_directory_iterator>(it_);
            if (it == std::filesystem::recursive_directory_iterator{}) return std::nullopt;
            auto result = it->path();
            ++it;
            return result;
        }
        auto& it = std::get<std::filesystem::directory_iterator>(it_);
        if (it == std::filesystem::directory_iterator{}) return std::nullopt;
        auto result = it->path();
        ++it;
        return result;
    }

private:
    bool recursive_;
    std::variant<std::filesystem::directory_iterator, 
        std::filesystem::recursive_directory_iterator> it_;
};

inline auto Dir(std::string path, bool recursive) {
    return DirFlowIterator(std::move(path), recursive);
}