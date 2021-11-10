#ifndef BOOST_QVM_DEDUCE_SCALAR_HPP_INCLUDED
#define BOOST_QVM_DEDUCE_SCALAR_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/qvm/scalar_traits.hpp>
#include <boost/qvm/static_assert.hpp>

namespace boost { namespace qvm {

namespace
deduce_scalar_detail
    {
    template <class A,class B>
    struct
    deduce_scalar_impl
        {
        typedef void type;
        };

    template <class T>
    struct
    deduce_scalar_impl<T,T>
        {
        typedef T type;
        };

    template <> struct deduce_scalar_impl<signed char,unsigned char> { typedef unsigned char type; };
    template <> struct deduce_scalar_impl<signed char,unsigned short> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<signed char,unsigned int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<signed char,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed char,signed short> { typedef signed short type; };
    template <> struct deduce_scalar_impl<signed char,signed int> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed char,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<signed char,float> { typedef float type; };
    template <> struct deduce_scalar_impl<signed char,double> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned char,unsigned short> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<unsigned char,unsigned int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned char,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<unsigned char,signed short> { typedef signed short type; };
    template <> struct deduce_scalar_impl<unsigned char,signed int> { typedef signed int type; };
    template <> struct deduce_scalar_impl<unsigned char,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<unsigned char,float> { typedef float type; };
    template <> struct deduce_scalar_impl<unsigned char,double> { typedef double type; };
    template <> struct deduce_scalar_impl<signed short,unsigned short> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<signed short,unsigned int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<signed short,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed short,signed int> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed short,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<signed short,float> { typedef float type; };
    template <> struct deduce_scalar_impl<signed short,double> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned short,unsigned int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned short,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<unsigned short,signed int> { typedef signed int type; };
    template <> struct deduce_scalar_impl<unsigned short,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<unsigned short,float> { typedef float type; };
    template <> struct deduce_scalar_impl<unsigned short,double> { typedef double type; };
    template <> struct deduce_scalar_impl<signed int,unsigned int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<signed int,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed int,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<signed int,float> { typedef float type; };
    template <> struct deduce_scalar_impl<signed int,double> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned int,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<unsigned int,signed long> { typedef signed long type; };
    template <> struct deduce_scalar_impl<unsigned int,float> { typedef float type; };
    template <> struct deduce_scalar_impl<unsigned int,double> { typedef double type; };
    template <> struct deduce_scalar_impl<signed long,unsigned long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed long,float> { typedef float type; };
    template <> struct deduce_scalar_impl<signed long,double> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned long,float> { typedef float type; };
    template <> struct deduce_scalar_impl<unsigned long,double> { typedef double type; };
    template <> struct deduce_scalar_impl<float,double> { typedef double type; };

    template <> struct deduce_scalar_impl<unsigned char,signed char> { typedef unsigned char type; };
    template <> struct deduce_scalar_impl<unsigned short,signed char> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<unsigned int,signed char> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned long,signed char> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed short,signed char> { typedef signed short type; };
    template <> struct deduce_scalar_impl<signed int,signed char> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed long,signed char> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,signed char> { typedef float type; };
    template <> struct deduce_scalar_impl<double,signed char> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned short,unsigned char> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<unsigned int,unsigned char> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned long,unsigned char> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed short,unsigned char> { typedef signed short type; };
    template <> struct deduce_scalar_impl<signed int,unsigned char> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed long,unsigned char> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,unsigned char> { typedef float type; };
    template <> struct deduce_scalar_impl<double,unsigned char> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned short,signed short> { typedef unsigned short type; };
    template <> struct deduce_scalar_impl<unsigned int,signed short> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned long,signed short> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed int,signed short> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed long,signed short> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,signed short> { typedef float type; };
    template <> struct deduce_scalar_impl<double,signed short> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned int,unsigned short> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned long,unsigned short> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed int,unsigned short> { typedef signed int type; };
    template <> struct deduce_scalar_impl<signed long,unsigned short> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,unsigned short> { typedef float type; };
    template <> struct deduce_scalar_impl<double,unsigned short> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned int,signed int> { typedef unsigned int type; };
    template <> struct deduce_scalar_impl<unsigned long,signed int> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed long,signed int> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,signed int> { typedef float type; };
    template <> struct deduce_scalar_impl<double,signed int> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned long,unsigned int> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<signed long,unsigned int> { typedef signed long type; };
    template <> struct deduce_scalar_impl<float,unsigned int> { typedef float type; };
    template <> struct deduce_scalar_impl<double,unsigned int> { typedef double type; };
    template <> struct deduce_scalar_impl<unsigned long,signed long> { typedef unsigned long type; };
    template <> struct deduce_scalar_impl<float,signed long> { typedef float type; };
    template <> struct deduce_scalar_impl<double,signed long> { typedef double type; };
    template <> struct deduce_scalar_impl<float,unsigned long> { typedef float type; };
    template <> struct deduce_scalar_impl<double,unsigned long> { typedef double type; };
    template <> struct deduce_scalar_impl<double,float> { typedef double type; };
    }

template <class A,class B>
struct
deduce_scalar
    {
    BOOST_QVM_STATIC_ASSERT(is_scalar<A>::value);
    BOOST_QVM_STATIC_ASSERT(is_scalar<B>::value);
    typedef typename deduce_scalar_detail::deduce_scalar_impl<A,B>::type type;
    };

} }

#endif
