//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_ITERATOR_TRAITS_HPP
#define BOOST_COMPUTE_DETAIL_ITERATOR_TRAITS_HPP

#include <iterator>

#include <boost/compute/detail/is_contiguous_iterator.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator>
struct iterator_traits : public std::iterator_traits<Iterator>
{
    static const bool is_contiguous = is_contiguous_iterator<Iterator>::value;
    static const bool is_on_device = is_device_iterator<Iterator>::value;
    static const bool is_on_host = !is_on_device;
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_TRAITS_HPP
