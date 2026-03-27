#pragma once
#include <iostream>

#include <fstream>

template<typename Delimiter>
class WriteAdapter {
public:
    WriteAdapter(std::ostream& os, Delimiter del) : os_(os), del_(std::move(del)) {}

    template<typename Flow>
    void operator()(Flow flow) {
        while(true) {
            auto v = flow.Next();
            if (!v) return;
            os_ << *v << del_;
        }
    }
private:
    std::ostream& os_;
    Delimiter del_;
};

template<typename Delimiter>
inline auto Write(std::ostream& os, Delimiter del) {
    return WriteAdapter(os, del);
}