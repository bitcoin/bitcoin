// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Original implementation here: https://github.com/whoshuu/cpp_range/blob/cd6897e40176c3031ad1e4c8069d672b2e544996/include/range.h under MIT software license

#ifndef BITCOIN_UTIL_IRANGE_H
#define BITCOIN_UTIL_IRANGE_H

#include <iterator>
#include <stdexcept>

namespace irange {
namespace detail {

template <typename T>
class Range {
    // T must be an integral type
    static_assert(std::is_integral<T>());

public:
    Range(const T start, const T stop, const T step) : start_{start}, stop_{stop}, step_{step} {
        if (step_ == 0) {
            throw std::invalid_argument("Range step argument must not be zero");
        } else {
            if ((start_ > stop_ && step_ > 0) || (start_ < stop_ && step_ < 0)) {
                throw std::invalid_argument("Range arguments must result in termination");
            }
        }
    }

    class iterator {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;

        iterator(value_type value, value_type step, value_type boundary) : value_{value}, step_{step},
                                                                           boundary_{boundary},
                                                                           positive_step_(step_ > 0) {}
        iterator operator++() { value_ += step_; return *this; }
        reference operator*() { return value_; }
        const pointer operator->() { return &value_; }
        bool operator==(const iterator& rhs) const { return positive_step_ ? (value_ >= rhs.value_ && value_ > boundary_)
                                                                           : (value_ <= rhs.value_ && value_ < boundary_); }
        bool operator!=(const iterator& rhs) const { return positive_step_ ? (value_ < rhs.value_ && value_ >= boundary_)
                                                                           : (value_ > rhs.value_ && value_ <= boundary_); }

    private:
        value_type value_;
        const T step_;
        const T boundary_;
        const bool positive_step_;
    };

    iterator begin() const {
        return iterator(start_, step_, start_);
    }

    iterator end() const {
        return iterator(stop_, step_, start_);
    }

private:
    const T start_;
    const T stop_;
    const T step_;
};

} // namespace detail

template <typename T>
detail::Range<T> range(const T stop) {
    return detail::Range<T>(T{0}, stop, T{1});
}

template <typename T>
detail::Range<T> range(const T start, const T stop) {
    return detail::Range<T>(start, stop, T{1});
}

template <typename T>
detail::Range<T> range(const T start, const T stop, const T step) {
    return detail::Range<T>(start, stop, step);
}

} // namespace irange

#endif // BITCOIN_UTIL_IRANGE_H
