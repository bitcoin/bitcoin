#ifndef BOOST_QVM_VEC_TRAITS_DEFAULTS_HPP_INCLUDED
#define BOOST_QVM_VEC_TRAITS_DEFAULTS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

template <class>
struct vec_traits;

namespace
qvm_detail
    {
    template <int I,int N>
    struct
    vector_w
        {
        template <class A>
        static
        BOOST_QVM_INLINE_CRITICAL
        typename vec_traits<A>::scalar_type &
        write_element_idx( int i, A & a )
            {
            return I==i?
                vec_traits<A>::template write_element<I>(a) :
                vector_w<I+1,N>::write_element_idx(i,a);
            }
        };

    template <int N>
    struct
    vector_w<N,N>
        {
        template <class A>
        static
        BOOST_QVM_INLINE_TRIVIAL
        typename vec_traits<A>::scalar_type &
        write_element_idx( int, A & a )
            {
            BOOST_QVM_ASSERT(0);
            return vec_traits<A>::template write_element<0>(a);
            }
        };
    }

template <class VecType,class ScalarType,int Dim>
struct
vec_traits_defaults
    {
    typedef VecType vec_type;
    typedef ScalarType scalar_type;
    static int const dim=Dim;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( vec_type const & x )
        {
        return vec_traits<vec_type>::template write_element<I>(const_cast<vec_type &>(x));
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, vec_type const & x )
        {
        return vec_traits<vec_type>::write_element_idx(i,const_cast<vec_type &>(x));
        }

    protected:

    static
    BOOST_QVM_INLINE_TRIVIAL
    scalar_type &
    write_element_idx( int i, vec_type & m )
        {
        return qvm_detail::vector_w<0,vec_traits<vec_type>::dim>::write_element_idx(i,m);
        }
    };

} }

#endif
