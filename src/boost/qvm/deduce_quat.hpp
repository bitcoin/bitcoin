#ifndef BOOST_QVM_DEDUCE_QUAT_HPP_INCLUDED
#define BOOST_QVM_DEDUCE_QUAT_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/deduce_scalar.hpp>
#include <boost/qvm/quat_traits.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

template <class T>
struct quat;

namespace
qvm_detail
    {
    template <class Q,class S,
        class QS=typename quat_traits<Q>::scalar_type>
    struct
    deduce_q_default
        {
        BOOST_QVM_STATIC_ASSERT(is_quat<Q>::value);
        typedef quat<typename quat_traits<Q>::scalar_type> type;
        };

    template <class Q,class S>
    struct
    deduce_q_default<Q,S,S>
        {
        BOOST_QVM_STATIC_ASSERT(is_quat<Q>::value);
        typedef Q type;
        };
    }

template <class Q,class S=typename quat_traits<Q>::scalar_type>
struct
deduce_quat
    {
    BOOST_QVM_STATIC_ASSERT(is_quat<Q>::value);
    typedef typename qvm_detail::deduce_q_default<Q,S>::type type;
    };

namespace
qvm_detail
    {
    template <class A,class B,class S,
        bool IsScalarA=is_scalar<A>::value,
        bool IsScalarB=is_scalar<B>::value>
    struct
    deduce_q2_default
        {
        typedef quat<S> type;
        };

    template <class Q,class S>
    struct
    deduce_q2_default<Q,Q,S,false,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_quat<Q>::value);
        typedef Q type;
        };

    template <class A,class B,class S>
    struct
    deduce_q2_default<A,B,S,false,true>
        {
        BOOST_QVM_STATIC_ASSERT(is_quat<A>::value);
        typedef typename deduce_quat<A,S>::type type;
        };

    template <class A,class B,class S>
    struct
    deduce_q2_default<A,B,S,true,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_quat<B>::value);
        typedef typename deduce_quat<B,S>::type type;
        };
    }

template <class A,class B,class S=typename deduce_scalar_detail::deduce_scalar_impl<typename scalar<A>::type,typename scalar<B>::type>::type>
struct
deduce_quat2
    {
    BOOST_QVM_STATIC_ASSERT(is_quat<A>::value || is_quat<B>::value);
    typedef typename qvm_detail::deduce_q2_default<A,B,S>::type type;
    };

} }

#endif
