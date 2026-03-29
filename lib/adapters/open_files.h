#pragma once

#include <optional>
#include <fstream>
#include <memory>
#include <utility>

#include "../flowiterator.h"

template<typename Flow>
class OpenFilesFlow : public FlowRangeMixin<OpenFilesFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using value_type = std::string;

    explicit OpenFilesFlow(Flow flow) : flow_(std::move(flow)) {}

    std::optional<value_type> Next() {
        while(true) {
            auto v = flow_.Next();
            if (!v) return std::nullopt;

            std::ifstream file(*v);
            if (!file.is_open()) continue;
            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }
    
private:
    Flow flow_;
};

class OpenFilesAdapter {
public:
    template<typename Flow>
    auto operator()(Flow flow) {
        return OpenFilesFlow<Flow>(std::move(flow));
    }
};

inline auto OpenFiles() {
    return OpenFilesAdapter();
}