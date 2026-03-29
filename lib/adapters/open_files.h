#pragma once

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "../flowiterator.h"
#include "../unwrap.h"

template<typename Flow>
class OpenFilesFlow : public FlowRangeMixin<OpenFilesFlow<Flow>> {
public:
    using input_type = typename Flow::value_type;
    using value_type = std::string;

    explicit OpenFilesFlow(Flow flow) : flow_(std::move(flow)) {}

    std::optional<value_type> Next() {
        while (true) {
            auto path = flow_.Next();
            if (!path) {
                return std::nullopt;
            }

            std::ifstream file(Unwrap(*path));
            if (!file.is_open()) {
                continue;
            }

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
    auto operator()(Flow flow) const {
        return OpenFilesFlow<Flow>(std::move(flow));
    }
};

inline auto OpenFiles() {
    return OpenFilesAdapter();
}