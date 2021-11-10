//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_GENERATE_N_HPP
#define BOOST_COMPUTE_ALGORITHM_GENERATE_N_HPP

#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/generate.hpp>

namespace boost {
namespace compute {

/// Stores the result of \p generator for each element in the range
/// [\p first, \p first + \p count).
///
/// Space complexity: \Omega(1)
template<class OutputIterator, class Size, class Generator>
inline void generate_n(OutputIterator first,
                       Size count,
                       Generator generator,
                       command_queue &queue = system::default_queue())
{
    ::boost::compute::generate(first, first + count, generator, queue);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_GENERATE_N_HPP
