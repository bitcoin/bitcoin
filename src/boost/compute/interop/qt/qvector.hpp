//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_QT_QVECTOR_HPP
#define BOOST_COMPUTE_INTEROP_QT_QVECTOR_HPP

#include <boost/compute/detail/is_contiguous_iterator.hpp>

#include <QVector>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator>
struct _is_contiguous_iterator<
    Iterator,
    typename boost::enable_if<
        typename boost::is_same<
            Iterator,
            typename QVector<typename Iterator::value_type>::iterator
        >::type
    >::type
> : public boost::true_type {};

template<class Iterator>
struct _is_contiguous_iterator<
    Iterator,
    typename boost::enable_if<
        typename boost::is_same<
            Iterator,
            typename QVector<typename Iterator::value_type>::const_iterator
        >::type
    >::type
> : public boost::true_type {};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_QT_QVECTOR_HPP
