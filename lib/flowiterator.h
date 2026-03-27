#pragma once
#include <optional>
#include <iterator>
#include <utility>

template<class Derived>
class FlowRangeMixin {
public:
    class iterator {
    public:
        using value_type = typename Derived::value_type;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using reference = const value_type&;
        using pointer = const value_type*;

        iterator() = default;

        explicit iterator(Derived* flow) : flow_(flow) {
            ++(*this);
        }

        reference operator*() const {
            return *current_;
        }

        pointer operator->() const {
            return &(*current_);
        }

        iterator& operator++() {
            if (flow_) {
                current_ = flow_->Next();
                if (!current_) {
                    flow_ = nullptr;
                }
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return flow_ == other.flow_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        Derived* flow_ = nullptr;
        std::optional<value_type> current_;
    };

    iterator begin() {
        return iterator(static_cast<Derived*>(this));
    }

    iterator end() {
        return iterator();
    }

    iterator begin() const {
        return iterator(const_cast<Derived*>(static_cast<const Derived*>(this)));
    }

    iterator end() const {
        return iterator();
    }
};