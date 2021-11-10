//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_EXPERIMENTAL_CLAMP_RANGE_HPP
#define BOOST_COMPUTE_EXPERIMENTAL_CLAMP_RANGE_HPP

#include <iterator>

#include <boost/compute/lambda.hpp>
#include <boost/compute/algorithm/transform.hpp>

namespace boost {
namespace compute {
namespace experimental {

template<class InputIterator, class OutputIterator>
inline OutputIterator
clamp_range(InputIterator first,
            InputIterator last,
            OutputIterator result,
            typename std::iterator_traits<InputIterator>::value_type lo,
            typename std::iterator_traits<InputIterator>::value_type hi,
            command_queue &queue)
{
    using ::boost::compute::lambda::_1;
    using ::boost::compute::lambda::_2;
    using ::boost::compute::lambda::clamp;

    return ::boost::compute::transform(
        first,
        last,
        result,
        clamp(_1, lo, hi),
        queue
    );
}

} // end experimental namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_EXPERIMENTAL_CLAMP_RANGE_HPP
