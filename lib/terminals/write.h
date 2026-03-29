#pragma once
#include <iostream>
#include <utility>

#include "../unwrap.h"

template<typename Delimiter>
class WriteAdapter {
public:
    WriteAdapter(std::ostream& os, Delimiter del)
        : os_(os), del_(std::move(del)) {}

    template<typename Flow>
    void operator()(Flow flow) const {
        while (true) {
            auto v = flow.Next();
            if (!v) return;
            os_ << Unwrap(*v) << del_;
        }
    }

private:
    std::ostream& os_;
    Delimiter del_;
};

template<typename Delimiter>
inline auto Write(std::ostream& os, Delimiter del) {
    return WriteAdapter<Delimiter>(os, std::move(del));
}