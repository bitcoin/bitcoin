//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALLOCATOR_PINNED_ALLOCATOR_HPP
#define BOOST_COMPUTE_ALLOCATOR_PINNED_ALLOCATOR_HPP

#include <boost/compute/allocator/buffer_allocator.hpp>

namespace boost {
namespace compute {

template<class T>
class pinned_allocator : public buffer_allocator<T>
{
public:
    explicit pinned_allocator(const context &context)
        : buffer_allocator<T>(context)
    {
        buffer_allocator<T>::set_mem_flags(
            buffer::read_write | buffer::alloc_host_ptr
        );
    }

    pinned_allocator(const pinned_allocator<T> &other)
        : buffer_allocator<T>(other)
    {
    }

    pinned_allocator<T>& operator=(const pinned_allocator<T> &other)
    {
        if(this != &other){
            buffer_allocator<T>::operator=(other);
        }

        return *this;
    }

    ~pinned_allocator()
    {
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALLOCATOR_PINNED_ALLOCATOR_HPP
