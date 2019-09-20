//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/experimental/detail/dvektor_impl.hpp>
#include <immer/memory_policy.hpp>

namespace immer {

template <typename T,
          int B = 5,
          typename MemoryPolicy = default_memory_policy>
class dvektor
{
    using impl_t = detail::dvektor::impl<T, B, MemoryPolicy>;

public:
    using value_type = T;
    using reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using const_reference = const T&;

    using iterator         = detail::dvektor::iterator<T, B, MemoryPolicy>;
    using const_iterator   = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    dvektor() = default;

    iterator begin() const { return {impl_}; }
    iterator end()   const { return {impl_, typename iterator::end_t{}}; }

    reverse_iterator rbegin() const { return reverse_iterator{end()}; }
    reverse_iterator rend()   const { return reverse_iterator{begin()}; }

    std::size_t size() const { return impl_.size; }
    bool empty() const { return impl_.size == 0; }

    reference operator[] (size_type index) const
    { return impl_.get(index); }

    dvektor push_back(value_type value) const
    { return { impl_.push_back(std::move(value)) }; }

    dvektor assoc(std::size_t idx, value_type value) const
    { return { impl_.assoc(idx, std::move(value)) }; }

    template <typename FnT>
    dvektor update(std::size_t idx, FnT&& fn) const
    { return { impl_.update(idx, std::forward<FnT>(fn)) }; }

private:
    dvektor(impl_t impl) : impl_(std::move(impl)) {}
    impl_t impl_ = detail::dvektor::empty<T, B, MemoryPolicy>;
};

} // namespace immer
