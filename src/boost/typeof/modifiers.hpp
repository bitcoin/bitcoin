// Copyright (C) 2004 Arkadiy Vertleyb
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_MODIFIERS_HPP_INCLUDED
#define BOOST_TYPEOF_MODIFIERS_HPP_INCLUDED

#include <boost/typeof/encode_decode.hpp>
#include <boost/preprocessor/facilities/identity.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

// modifiers

#define BOOST_TYPEOF_modifier_support(ID, Fun)\
    template<class V, class T> struct encode_type_impl<V, Fun(T)>\
    {\
        typedef\
            typename boost::type_of::encode_type<\
            typename boost::type_of::push_back<\
            V\
            , boost::type_of::constant<std::size_t,ID> >::type\
            , T>::type\
            type;\
    };\
    template<class Iter> struct decode_type_impl<boost::type_of::constant<std::size_t,ID>, Iter>\
    {\
        typedef boost::type_of::decode_type<Iter> d1;\
        typedef Fun(typename d1::type) type;\
        typedef typename d1::iter iter;\
    }


#define BOOST_TYPEOF_const_fun(T) const T
#define BOOST_TYPEOF_volatile_fun(T) volatile T
#define BOOST_TYPEOF_volatile_const_fun(T) volatile const T
#define BOOST_TYPEOF_pointer_fun(T) T*
#define BOOST_TYPEOF_reference_fun(T) T&

#if defined(__BORLANDC__) && !defined(__clang__) && (__BORLANDC__ < 0x600)
//Borland incorrectly handles T const, T const volatile and T volatile.
//It drops the decoration no matter what, so we need to try to handle T* const etc. without loosing the top modifier.
#define BOOST_TYPEOF_const_pointer_fun(T) T const *
#define BOOST_TYPEOF_const_reference_fun(T) T const &
#define BOOST_TYPEOF_volatile_pointer_fun(T) T volatile*
#define BOOST_TYPEOF_volatile_reference_fun(T) T volatile&
#define BOOST_TYPEOF_volatile_const_pointer_fun(T) T volatile const *
#define BOOST_TYPEOF_volatile_const_reference_fun(T) T volatile const &
#endif

BOOST_TYPEOF_BEGIN_ENCODE_NS

BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_const_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_const_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_pointer_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_reference_fun);

#if defined(__BORLANDC__) && !defined(__clang__) && (__BORLANDC__ < 0x600)
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_const_pointer_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_const_reference_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_pointer_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_reference_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_const_pointer_fun);
BOOST_TYPEOF_modifier_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_TYPEOF_volatile_const_reference_fun);
#endif

BOOST_TYPEOF_END_ENCODE_NS

#undef BOOST_TYPEOF_modifier_support
#undef BOOST_TYPEOF_const_fun
#undef BOOST_TYPEOF_volatile_fun
#undef BOOST_TYPEOF_volatile_const_fun
#undef BOOST_TYPEOF_pointer_fun
#undef BOOST_TYPEOF_reference_fun

#if defined(__BORLANDC__) && !defined(__clang__) && (__BORLANDC__ < 0x600)
#undef BOOST_TYPEOF_const_pointer_fun
#undef BOOST_TYPEOF_const_reference_fun
#undef BOOST_TYPEOF_volatile_pointer_fun
#undef BOOST_TYPEOF_volatile_reference_fun
#undef BOOST_TYPEOF_volatile_const_pointer_fun
#undef BOOST_TYPEOF_volatile_const_reference_fun
#endif

// arrays

#define BOOST_TYPEOF_array_support(ID, Qualifier)\
    template<class V, class T, int N>\
    struct encode_type_impl<V, Qualifier() T[N]>\
    {\
        typedef\
            typename boost::type_of::encode_type<\
            typename boost::type_of::push_back<\
            typename boost::type_of::push_back<\
            V\
            , boost::type_of::constant<std::size_t,ID> >::type\
            , boost::type_of::constant<std::size_t,N> >::type\
            , T>::type\
        type;\
    };\
    template<class Iter>\
    struct decode_type_impl<boost::type_of::constant<std::size_t,ID>, Iter>\
    {\
        enum{n = Iter::type::value};\
        typedef boost::type_of::decode_type<typename Iter::next> d;\
        typedef typename d::type Qualifier() type[n];\
        typedef typename d::iter iter;\
    }

BOOST_TYPEOF_BEGIN_ENCODE_NS

BOOST_TYPEOF_array_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_PP_EMPTY);
BOOST_TYPEOF_array_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_PP_IDENTITY(const));
BOOST_TYPEOF_array_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_PP_IDENTITY(volatile));
BOOST_TYPEOF_array_support(BOOST_TYPEOF_UNIQUE_ID(), BOOST_PP_IDENTITY(volatile const));
BOOST_TYPEOF_END_ENCODE_NS

#undef BOOST_TYPEOF_array_support

#endif//BOOST_TYPEOF_MODIFIERS_HPP_INCLUDED
