#ifndef BOOST_QVM_DEDUCE_VEC_HPP_INCLUDED
#define BOOST_QVM_DEDUCE_VEC_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/deduce_scalar.hpp>
#include <boost/qvm/vec_traits.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

template <class T,int D>
struct vec;

namespace
qvm_detail
    {
    template <class V,int D,class S,
        int VD=vec_traits<V>::dim,
        class VS=typename vec_traits<V>::scalar_type>
    struct
    deduce_v_default
        {
        BOOST_QVM_STATIC_ASSERT(is_vec<V>::value);
        typedef vec<typename vec_traits<V>::scalar_type,D> type;
        };

    template <class V,int D,class S>
    struct
    deduce_v_default<V,D,S,D,S>
        {
        BOOST_QVM_STATIC_ASSERT(is_vec<V>::value);
        typedef V type;
        };
    }

template <class V,int D=vec_traits<V>::dim,class S=typename vec_traits<V>::scalar_type>
struct
deduce_vec
    {
    BOOST_QVM_STATIC_ASSERT(is_vec<V>::value);
    typedef typename qvm_detail::deduce_v_default<V,D,S>::type type;
    };

namespace
qvm_detail
    {
    template <class A,class B,int D,class S,
        bool IsScalarA=is_scalar<A>::value,
        bool IsScalarB=is_scalar<B>::value>
    struct
    deduce_v2_default
        {
        typedef vec<S,D> type;
        };

    template <class V,int D,class S>
    struct
    deduce_v2_default<V,V,D,S,false,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_vec<V>::value);
        typedef V type;
        };

    template <class A,class B,int D,class S>
    struct
    deduce_v2_default<A,B,D,S,false,true>
        {
        BOOST_QVM_STATIC_ASSERT(is_vec<A>::value);
        typedef typename deduce_vec<A,D,S>::type type;
        };

    template <class A,class B,int D,class S>
    struct
    deduce_v2_default<A,B,D,S,true,false>
        {
        BOOST_QVM_STATIC_ASSERT(is_vec<B>::value);
        typedef typename deduce_vec<B,D,S>::type type;
        };
    }

template <class A,class B,int D,class S=typename deduce_scalar_detail::deduce_scalar_impl<typename scalar<A>::type,typename scalar<B>::type>::type>
struct
deduce_vec2
    {
    BOOST_QVM_STATIC_ASSERT(is_vec<A>::value || is_vec<B>::value);
    typedef typename qvm_detail::deduce_v2_default<A,B,D,S>::type type;
    };

} }

#endif
