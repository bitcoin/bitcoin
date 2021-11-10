#ifndef BOOST_QVM_QUAT_TRAITS_ARRAY_HPP_INCLUDED
#define BOOST_QVM_QUAT_TRAITS_ARRAY_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/inline.hpp>
#include <boost/qvm/deduce_quat.hpp>
#include <boost/qvm/detail/remove_const.hpp>
#include <boost/qvm/assert.hpp>

namespace boost { namespace qvm {

template <class T,int D>
struct
quat_traits<T[D]>
    {
    typedef void scalar_type;
    };
template <class T,int D>
struct
quat_traits<T[D][4]>
    {
    typedef void scalar_type;
    };
template <class T,int D>
struct
quat_traits<T[4][D]>
    {
    typedef void scalar_type;
    };
template <class T>
struct
quat_traits<T[4][4]>
    {
    typedef void scalar_type;
    };
template <class T,int M,int N>
struct
quat_traits<T[M][N]>
    {
    typedef void scalar_type;
    };

template <class T>
struct
quat_traits<T[4]>
    {
    typedef T this_quaternion[4];
    typedef typename qvm_detail::remove_const<T>::type scalar_type;

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element( this_quaternion const & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return x[I];
        }

    template <int I>
    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element( this_quaternion & x )
        {
        BOOST_QVM_STATIC_ASSERT(I>=0);
        BOOST_QVM_STATIC_ASSERT(I<4);
        return x[I];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type
    read_element_idx( int i, this_quaternion const & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<4);
        return x[i];
        }

    static
    BOOST_QVM_INLINE_CRITICAL
    scalar_type &
    write_element_idx( int i, this_quaternion & x )
        {
        BOOST_QVM_ASSERT(i>=0);
        BOOST_QVM_ASSERT(i<4);
        return x[i];
        }
    };

template <class T>
struct
deduce_quat<T[4]>
    {
    typedef quat<T> type;
    };

template <class T>
struct
deduce_quat<T const[4]>
    {
    typedef quat<T> type;
    };

template <class T1,class T2>
struct
deduce_quat2<T1[4],T2[4]>
    {
    typedef quat<typename deduce_scalar<T1,T2>::type> type;
    };

template <class T>
T (&ptr_qref( T * ptr ))[4]
    {
    return *reinterpret_cast<T (*)[4]>(ptr);
    }

} }

#endif
