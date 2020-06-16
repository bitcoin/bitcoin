//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/rbts/rrbtree.hpp>
#include <immer/detail/iterator_facade.hpp>

namespace immer {
namespace detail {
namespace rbts {

template <typename T, typename MP, bits_t B, bits_t BL>
struct rrbtree_iterator
    : iterator_facade<rrbtree_iterator<T, MP, B, BL>,
                      std::random_access_iterator_tag,
                      T,
                      const T&,
                      std::ptrdiff_t,
                      const T*>
{
    using tree_t   = rrbtree<T, MP, B, BL>;
    using region_t = std::tuple<const T*, size_t, size_t>;

    struct end_t {};

    const tree_t& impl() const { return *v_; }
    size_t index() const { return i_; }

    rrbtree_iterator() = default;

    rrbtree_iterator(const tree_t& v)
        : v_    { &v }
        , i_    { 0 }
        , curr_ { nullptr, ~size_t{}, ~size_t{} }
    {
    }

    rrbtree_iterator(const tree_t& v, end_t)
        : v_    { &v }
        , i_    { v.size }
        , curr_ { nullptr, ~size_t{}, ~size_t{} }
    {}

private:
    friend iterator_core_access;

    const tree_t* v_;
    size_t   i_;
    mutable region_t curr_;

    void increment()
    {
        using std::get;
        assert(i_ < v_->size);
        ++i_;
    }

    void decrement()
    {
        using std::get;
        assert(i_ > 0);
        --i_;
    }

    void advance(std::ptrdiff_t n)
    {
        using std::get;
        assert(n <= 0 || i_ + static_cast<size_t>(n) <= v_->size);
        assert(n >= 0 || static_cast<size_t>(-n) <= i_);
        i_ += n;
    }

    bool equal(const rrbtree_iterator& other) const
    {
        return i_ == other.i_;
    }

    std::ptrdiff_t distance_to(const rrbtree_iterator& other) const
    {
        return other.i_ > i_
            ?   static_cast<std::ptrdiff_t>(other.i_ - i_)
            : - static_cast<std::ptrdiff_t>(i_ - other.i_);
    }

    const T& dereference() const
    {
        using std::get;
        if (i_ < get<1>(curr_) || i_ >= get<2>(curr_))
            curr_ = v_->region_for(i_);
        return get<0>(curr_)[i_ - get<1>(curr_)];
    }
};

} // namespace rbts
} // namespace detail
} // namespace immer
