/*-----------------------------------------------------------------------------+
Copyright (c) 2007-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TO_STRING_HPP_JOFA_080416
#define BOOST_ICL_TYPE_TO_STRING_HPP_JOFA_080416

#include <stdio.h>
#include <string>
#include <sstream>

#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_float.hpp>
#include <boost/mpl/if.hpp>

namespace boost{ namespace icl
{
    //--------------------------------------------------------------------------
    template<class Type>
    struct type_to_string
    {
        /** Convert the type to it's typestring */
        static std::string apply();
    };


    //--------------------------------------------------------------------------
    template<>inline std::string type_to_string<bool>::apply() { return "bool"; }
    template<>inline std::string type_to_string<char>::apply() { return "char"; }
    template<>inline std::string type_to_string<short>::apply(){ return "short"; }
    template<>inline std::string type_to_string<int>::apply()  { return "int"; }
    template<>inline std::string type_to_string<long>::apply() { return "long"; }
    template<>inline std::string type_to_string<long long>::apply(){ return "Long"; }

    template<>inline std::string type_to_string<unsigned char>::apply(){ return "char+"; }
    template<>inline std::string type_to_string<unsigned short>::apply(){ return "short+"; }
    template<>inline std::string type_to_string<unsigned int>::apply()  { return "int+"; }
    template<>inline std::string type_to_string<unsigned long>::apply() { return "long+"; }
    template<>inline std::string type_to_string<unsigned long long>::apply(){ return "Long+"; }

    template<>inline std::string type_to_string<float>::apply() { return "flt"; }
    template<>inline std::string type_to_string<double>::apply() { return "dbl"; }

    //-------------------------------------------------------------------------
    template<template<class> class Templ>
    struct unary_template_to_string
    {
        static std::string apply();
    };

    template <template<class>class Unary, class Type>
    struct type_to_string<Unary<Type> >
    {
        static std::string to_string()
        { 
            return unary_template_to_string<Unary>::apply()+"<"+type_to_string<Type>::apply()+">"; 
        }
    };

    // ---------------------------------------------------------------------------
    template<template<class,class>class Templ>
    struct binary_template_to_string
    {
        static std::string apply();
    };

    template <template<class Type1, class Type2>class Binary, class Type1, class Type2>
    struct type_to_string<Binary<Type1, Type2> >
    {
        static std::string apply()
        { 
            return binary_template_to_string<Binary>::apply()+
                "<"+type_to_string<Type1>::apply()+","+type_to_string<Type2>::apply()+">"; 
        }
    };

    // ---------------------------------------------------------------------------
    template<>
    struct type_to_string<std::string>
    {
        static std::string apply() { return "string"; }
    };

}} // namespace boost icl

#endif


