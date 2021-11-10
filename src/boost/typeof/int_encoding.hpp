// Copyright (C) 2004 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_INT_ENCODING_HPP_INCLUDED
#define BOOST_TYPEOF_INT_ENCODING_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/typeof/constant.hpp>
#include <cstddef> // for std::size_t

namespace boost { namespace type_of {

    template<class T> struct get_unsigned
    {
        typedef T type;
    };
    template<> struct get_unsigned<signed char>
    {
        typedef unsigned char type;
    };
    template<> struct get_unsigned<char>
    {
        typedef unsigned char type;
    };
    template<> struct get_unsigned<short>
    {
        typedef unsigned short type;
    };
    template<> struct get_unsigned<int>
    {
        typedef unsigned int type;
    };
    template<> struct get_unsigned<long>
    {
        typedef unsigned long type;
    };

    //////////////////////////

    template<std::size_t n, bool Overflow>
    struct pack
    {
        BOOST_STATIC_CONSTANT(std::size_t , value=((n + 1) * 2 + (Overflow ? 1 : 0)));
    };

    template<std::size_t m>
    struct unpack
    {
        BOOST_STATIC_CONSTANT(std::size_t, value = (m / 2) - 1);
        BOOST_STATIC_CONSTANT(std::size_t, overflow = (m % 2 == 1));
    };

    ////////////////////////////////

    template<class V, std::size_t n, bool overflow = (n >= 0x3fffffff)>
    struct encode_size_t : push_back<
        V,
        boost::type_of::constant<std::size_t,pack<n, false>::value>
    >
    {};

    template<class V, std::size_t n>
    struct encode_size_t<V, n, true> : push_back<typename push_back<
        V,
        boost::type_of::constant<std::size_t,pack<n % 0x3ffffffe, true>::value> >::type,
        boost::type_of::constant<std::size_t,n / 0x3ffffffe>
    >
    {};

    template<class V, class T, T n>
    struct encode_integral : encode_size_t< V, (typename get_unsigned<T>::type)n,(((typename get_unsigned<T>::type)n)>=0x3fffffff) >
    {};

    template<class V, bool b>
    struct encode_integral<V, bool, b> : encode_size_t< V, b?1:0, false>
    {};
    ///////////////////////////

    template<std::size_t n, class Iter, bool overflow>
    struct decode_size_t;

    template<std::size_t n, class Iter>
    struct decode_size_t<n, Iter, false>
    {
        BOOST_STATIC_CONSTANT(std::size_t,value = n);
        typedef Iter iter;
    };

    template<std::size_t n, class Iter>
    struct decode_size_t<n, Iter, true>
    {
        BOOST_STATIC_CONSTANT(std::size_t,m = Iter::type::value);

        BOOST_STATIC_CONSTANT(std::size_t,value = (std::size_t)m * 0x3ffffffe + n);
        typedef typename Iter::next iter;
    };

    template<class T, class Iter>
    struct decode_integral
    {
        typedef decode_integral<T,Iter> self_t;
        BOOST_STATIC_CONSTANT(std::size_t,m = Iter::type::value);

        BOOST_STATIC_CONSTANT(std::size_t,n = unpack<m>::value);

        BOOST_STATIC_CONSTANT(std::size_t,overflow = unpack<m>::overflow);

        typedef typename Iter::next nextpos;

        static const T value = (T)(std::size_t)decode_size_t<n, nextpos, overflow>::value;

        typedef typename decode_size_t<self_t::n, nextpos, self_t::overflow>::iter iter;
    };

}}//namespace

#endif//BOOST_TYPEOF_INT_ENCODING_HPP_INCLUDED
