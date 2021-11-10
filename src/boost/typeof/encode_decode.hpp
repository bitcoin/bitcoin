// Copyright (C) 2004 Arkadiy Vertleyb

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// boostinspect:nounnamed

#ifndef BOOST_TYPEOF_ENCODE_DECODE_HPP_INCLUDED
#define BOOST_TYPEOF_ENCODE_DECODE_HPP_INCLUDED

#ifndef BOOST_TYPEOF_SUPPRESS_UNNAMED_NAMESPACE

#   define BOOST_TYPEOF_BEGIN_ENCODE_NS namespace { namespace boost_typeof {
#   define BOOST_TYPEOF_END_ENCODE_NS }}
#   define BOOST_TYPEOF_ENCODE_NS_QUALIFIER boost_typeof

#else

#   define BOOST_TYPEOF_BEGIN_ENCODE_NS namespace boost { namespace type_of {
#   define BOOST_TYPEOF_END_ENCODE_NS }}
#   define BOOST_TYPEOF_ENCODE_NS_QUALIFIER boost::type_of

#   define BOOST_TYPEOF_TEXT "unnamed namespace is off"
#   include <boost/typeof/message.hpp>

#endif

BOOST_TYPEOF_BEGIN_ENCODE_NS

template<class V, class Type_Not_Registered_With_Typeof_System>
struct encode_type_impl;

template<class T, class Iter>
struct decode_type_impl
{
    typedef int type;  // MSVC ETI workaround
};

template<class T>
struct decode_nested_template_helper_impl;

BOOST_TYPEOF_END_ENCODE_NS

namespace boost { namespace type_of {

    template<class V, class T>
    struct encode_type : BOOST_TYPEOF_ENCODE_NS_QUALIFIER::encode_type_impl<V, T>
    {};

    template<class Iter>
    struct decode_type : BOOST_TYPEOF_ENCODE_NS_QUALIFIER::decode_type_impl<
        typename Iter::type,
        typename Iter::next
    >
    {};
}}

#endif//BOOST_TYPEOF_ENCODE_DECODE_HPP_INCLUDED
