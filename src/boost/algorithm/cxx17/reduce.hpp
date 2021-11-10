/*
   Copyright (c) Marshall Clow 2017.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

/// \file  reduce.hpp
/// \brief Combine the elements of a sequence into a single value
/// \author Marshall Clow

#ifndef BOOST_ALGORITHM_REDUCE_HPP
#define BOOST_ALGORITHM_REDUCE_HPP

#include <functional>     // for std::plus
#include <iterator>       // for std::iterator_traits

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

namespace boost { namespace algorithm {

template<class InputIterator, class T, class BinaryOperation>
T reduce(InputIterator first, InputIterator last, T init, BinaryOperation bOp)
{
    ;
    for (; first != last; ++first)
        init = bOp(init, *first);
    return init;
}

template<class InputIterator, class T>
T reduce(InputIterator first, InputIterator last, T init)
{
	typedef typename std::iterator_traits<InputIterator>::value_type VT;
    return boost::algorithm::reduce(first, last, init, std::plus<VT>());
}

template<class InputIterator>
typename std::iterator_traits<InputIterator>::value_type
reduce(InputIterator first, InputIterator last)
{
    return boost::algorithm::reduce(first, last,
       typename std::iterator_traits<InputIterator>::value_type());
}

template<class Range>
typename boost::range_value<Range>::type
reduce(const Range &r)
{
    return boost::algorithm::reduce(boost::begin(r), boost::end(r));
}

//	Not sure that this won't be ambiguous (1)
template<class Range, class T>
T reduce(const Range &r, T init)
{
    return boost::algorithm::reduce(boost::begin (r), boost::end (r), init);
}


//	Not sure that this won't be ambiguous (2)
template<class Range, class T, class BinaryOperation>
T reduce(const Range &r, T init, BinaryOperation bOp)
{
    return boost::algorithm::reduce(boost::begin(r), boost::end(r), init, bOp);
}

}} // namespace boost and algorithm

#endif // BOOST_ALGORITHM_REDUCE_HPP
