#ifndef BOOST_QVM_DETAIL_QUAT_ASSIGN_HPP_INCLUDED
#define BOOST_QVM_DETAIL_QUAT_ASSIGN_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/enable_if.hpp>
#include <boost/qvm/quat_traits.hpp>

namespace boost { namespace qvm {

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename enable_if_c<
    is_quat<A>::value && is_quat<B>::value,
    A &>::type
assign( A & a, B const & b )
    {
    quat_traits<A>::template write_element<0>(a) = quat_traits<B>::template read_element<0>(b);
    quat_traits<A>::template write_element<1>(a) = quat_traits<B>::template read_element<1>(b);
    quat_traits<A>::template write_element<2>(a) = quat_traits<B>::template read_element<2>(b);
    quat_traits<A>::template write_element<3>(a) = quat_traits<B>::template read_element<3>(b);
    return a;
    }

} }

#endif
