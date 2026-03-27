#pragma once
#include <fstream>

class OutAdapter {
public:
    OutAdapter(std::ostream& os) : os_(os) {}

    template<typename Flow>
    void operator()(Flow flow) {
        while (true) {
            auto v = flow.Next();
            if (!v) return;
            os_ << *v;
        }
    }
private:
    std::ostream& os_;
};

inline auto Out(std::ostream& os) {
    return OutAdapter(os);
}