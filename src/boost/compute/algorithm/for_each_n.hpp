//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_FOR_EACH_N_HPP
#define BOOST_COMPUTE_ALGORITHM_FOR_EACH_N_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/algorithm/for_each.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Calls \p function on each element in the range [\p first, \p first
/// \c + \p count).
///
/// Space complexity: \Omega(1)
///
/// \see for_each()
template<class InputIterator, class Size, class UnaryFunction>
inline UnaryFunction for_each_n(InputIterator first,
                                Size count,
                                UnaryFunction function,
                                command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator>::value);
    return ::boost::compute::for_each(first, first + count, function, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_FOR_EACH_N_HPP
