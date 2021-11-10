//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_ROTATE_HPP
#define BOOST_COMPUTE_ALGORITHM_ROTATE_HPP

#include <boost/compute/system.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/container/vector.hpp>

namespace boost {
namespace compute {

/// Performs left rotation such that element at \p n_first comes to the
/// beginning.
///
/// Space complexity: \Omega(distance(\p first, \p last))
///
/// \see rotate_copy()
template<class InputIterator>
inline void rotate(InputIterator first,
                   InputIterator n_first,
                   InputIterator last,
                   command_queue &queue = system::default_queue())
{
    //Handle trivial cases
    if (n_first==first || n_first==last)
    {
        return;
    }

    //Handle others
    typedef typename std::iterator_traits<InputIterator>::value_type T;

    size_t count = detail::iterator_range_size(first, n_first);
    size_t count2 = detail::iterator_range_size(first, last);

    const context &context = queue.get_context();
    vector<T> temp(count2, context);
    ::boost::compute::copy(first, last, temp.begin(), queue);

    ::boost::compute::copy(temp.begin()+count, temp.end(), first, queue);
    ::boost::compute::copy(temp.begin(), temp.begin()+count, last-count, queue);
}

} //end compute namespace
} //end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_ROTATE_HPP
