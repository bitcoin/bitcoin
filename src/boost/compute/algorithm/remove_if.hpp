//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_REMOVE_IF_HPP
#define BOOST_COMPUTE_ALGORITHM_REMOVE_IF_HPP

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/algorithm/copy_if.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/logical.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Removes each element for which \p predicate returns \c true in the
/// range [\p first, \p last).
///
/// Space complexity: \Omega(3n)
///
/// \see remove()
template<class Iterator, class Predicate>
inline Iterator remove_if(Iterator first,
                          Iterator last,
                          Predicate predicate,
                          command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<Iterator>::value);
    typedef typename std::iterator_traits<Iterator>::value_type value_type;

    // temporary storage for the input data
    ::boost::compute::vector<value_type> tmp(first, last, queue);

    return ::boost::compute::copy_if(tmp.begin(),
                                     tmp.end(),
                                     first,
                                     not1(predicate),
                                     queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_REMOVE_IF_HPP
