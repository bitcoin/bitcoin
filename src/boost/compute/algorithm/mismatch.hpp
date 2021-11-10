//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_MISMATCH_HPP
#define BOOST_COMPUTE_ALGORITHM_MISMATCH_HPP

#include <iterator>
#include <utility>

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/find.hpp>
#include <boost/compute/iterator/transform_iterator.hpp>
#include <boost/compute/iterator/zip_iterator.hpp>
#include <boost/compute/functional/detail/unpack.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

/// Returns a pair of iterators pointing to the first position where the
/// range [\p first1, \p last1) and the range starting at \p first2
/// differ.
///
/// Space complexity: \Omega(1)
template<class InputIterator1, class InputIterator2>
inline std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1,
         InputIterator1 last1,
         InputIterator2 first2,
         command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);
    typedef typename std::iterator_traits<InputIterator1>::value_type value_type;

    ::boost::compute::equal_to<value_type> op;

    InputIterator2 last2 = first2 + std::distance(first1, last1);

    InputIterator1 iter =
        boost::get<0>(
            ::boost::compute::find(
                ::boost::compute::make_transform_iterator(
                    ::boost::compute::make_zip_iterator(
                        boost::make_tuple(first1, first2)
                    ),
                    detail::unpack(op)
                ),
                ::boost::compute::make_transform_iterator(
                    ::boost::compute::make_zip_iterator(
                        boost::make_tuple(last1, last2)
                    ),
                    detail::unpack(op)
                ),
                false,
                queue
            ).base().get_iterator_tuple()
        );

    return std::make_pair(iter, first2 + std::distance(first1, iter));
}

/// \overload
template<class InputIterator1, class InputIterator2>
inline std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1,
         InputIterator1 last1,
         InputIterator2 first2,
         InputIterator2 last2,
         command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);
    if(std::distance(first1, last1) < std::distance(first2, last2)){
        return ::boost::compute::mismatch(first1, last1, first2, queue);
    }
    else {
        return ::boost::compute::mismatch(
            first1, first1 + std::distance(first2, last2), first2, queue
        );
    }
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_MISMATCH_HPP
