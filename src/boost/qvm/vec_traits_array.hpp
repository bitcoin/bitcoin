#ifndef BOOST_QVM_VEC_TRAITS_ARRAY_HPP_INCLUDED
#define BOOST_QVM_VEC_TRAITS_ARRAY_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/detail/remove_const.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

template <class T,int M,int N>
struct
vec_traits<T[M][N]>
    {
    static int const dim=0;
    typedef void scalar_type;
    };

template <class T,int Dim>
struct
vec_traits<T[Dim]>
    {
    typedef T this_vector[Dim];
    typedef typename qvm_detail::remove_const<T>::type scalar_type;
    static int const dim=Dim;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_vector const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<Dim);
        return x[I];
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_vector & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<Dim);
        return x[I];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_vector const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<Dim);
        return x[i];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_vector & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<Dim);
        return x[i];
        }
    };

template <class T,int Dim,int D>
struct
deduce_vec<T[Dim],D>
    {
    typedef vec<T,D> type;
    };

template <class T,int Dim,int D>
struct
deduce_vec<T const[Dim],D>
    {
    typedef vec<T,D> type;
    };

template <class T1,class T2,int Dim,int D>
struct
deduce_vec2<T1[Dim],T2[Dim],D>
    {
    typedef vec<typename deduce_scalar<T1,T2>::type,D> type;
    };

template <int Dim,class T>
T (&ptr_vref( T * ptr ))[Dim]
    {
    return *reinterpret_cast<T (*)[Dim]>(ptr);
    }

} }

#endif
