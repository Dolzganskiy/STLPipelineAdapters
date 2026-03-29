#pragma once
#include <fstream>

#include "../unwrap.h"

class OutAdapter {
public:
    explicit OutAdapter(std::ostream& os) : os_(os) {}

    template<typename Flow>
    void operator()(Flow flow) const {
        while (true) {
            auto v = flow.Next();
            if (!v) {
                return;
            }
            os_ << Unwrap(*v);
        }
    }

private:
    std::ostream& os_;
};

inline auto Out(std::ostream& os) {
    return OutAdapter(os);
}