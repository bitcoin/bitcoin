#ifndef BOOST_QVM_QUAT_VEC_OPERATIONS_HPP_INCLUDED
#define BOOST_QVM_QUAT_VEC_OPERATIONS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/quat_traits.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/inline.hpp>
#include <boost/qvm/enable_if.hpp>

namespace boost { namespace qvm {

template <class A,class B>
BOOST_QVM_INLINE_OPERATIONS
typename lazy_enable_if_c<
    is_quat<A>::value &&
    is_vec<B>::value && vec_traits<B>::dim==3,
    deduce_vec2<A,B,3> >::type
operator*( A const & a, B const & b )
    {
    typedef typename deduce_vec2<A,B,3>::type R;
    typedef typename quat_traits<A>::scalar_type TA;
    typedef typename vec_traits<B>::scalar_type TB;
    TA const aa = quat_traits<A>::template read_element<0>(a);
    TA const ab = quat_traits<A>::template read_element<1>(a);
    TA const ac = quat_traits<A>::template read_element<2>(a);
    TA const ad = quat_traits<A>::template read_element<3>(a);
    TA const t2 = aa*ab;
    TA const t3 = aa*ac;
    TA const t4 = aa*ad;
    TA const t5 = -ab*ab;
    TA const t6 = ab*ac;
    TA const t7 = ab*ad;
    TA const t8 = -ac*ac;
    TA const t9 = ac*ad;
    TA const t10     = -ad*ad;
    TB const bx = vec_traits<B>::template read_element<0>(b);
    TB const by = vec_traits<B>::template read_element<1>(b);
    TB const bz = vec_traits<B>::template read_element<2>(b);
    R r;
    vec_traits<R>::template write_element<0>(r) = 2*((t8+t10)*bx + (t6-t4)*by + (t3+t7)*bz) + bx;
    vec_traits<R>::template write_element<1>(r) = 2*((t4+t6)*bx + (t5+t10)*by + (t9-t2)*bz) + by;
    vec_traits<R>::template write_element<2>(r) = 2*((t7-t3)*bx + (t2+t9)*by + (t5+t8)*bz) + bz;
    return r;
    }

namespace
sfinae
    {
    using ::boost::qvm::operator*;
    }

} }

#endif
