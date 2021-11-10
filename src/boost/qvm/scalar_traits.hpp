#ifndef BOOST_QVM_SCALAR_TRAITS_HPP_INCLUDED
#define BOOST_QVM_SCALAR_TRAITS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/quat_traits.hpp>
#include <boost/qvm/vec_traits.hpp>
#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/inline.hpp>

namespace boost { namespace qvm {

template <class Scalar>
struct
scalar_traits
    {
    static
    BOOST_QVM_INLINE_CRITICAL
    Scalar
    value( int v )
        {
        return Scalar(v);
        }
    };

template <class T>
struct
is_scalar
    {
    static bool const value=false;
    };
template <> struct is_scalar<char> { static bool const value=true; };
template <> struct is_scalar<signed char> { static bool const value=true; };
template <> struct is_scalar<unsigned char> { static bool const value=true; };
template <> struct is_scalar<signed short> { static bool const value=true; };
template <> struct is_scalar<unsigned short> { static bool const value=true; };
template <> struct is_scalar<signed int> { static bool const value=true; };
template <> struct is_scalar<unsigned int> { static bool const value=true; };
template <> struct is_scalar<signed long> { static bool const value=true; };
template <> struct is_scalar<unsigned long> { static bool const value=true; };
template <> struct is_scalar<float> { static bool const value=true; };
template <> struct is_scalar<double> { static bool const value=true; };
template <> struct is_scalar<long double> { static bool const value=true; };

namespace
qvm_detail
    {
    template <class A,
        bool IsQ=is_quat<A>::value,
        bool IsV=is_vec<A>::value,
        bool IsM=is_mat<A>::value,
        bool IsS=is_scalar<A>::value>
    struct
    scalar_impl
        {
        typedef void type;
        };

    template <class A>
    struct
    scalar_impl<A,false,false,false,true>
        {
        typedef A type;
        };

    template <class A>
    struct
    scalar_impl<A,false,false,true,false>
        {
        typedef typename mat_traits<A>::scalar_type type;
        };

    template <class A>
    struct
    scalar_impl<A,false,true,false,false>
        {
        typedef typename vec_traits<A>::scalar_type type;
        };

    template <class A>
    struct
    scalar_impl<A,true,false,false,false>
        {
        typedef typename quat_traits<A>::scalar_type type;
        };
    }

template <class A>
struct
scalar
    {
    typedef typename qvm_detail::scalar_impl<A>::type type;
    };

} }

#endif
