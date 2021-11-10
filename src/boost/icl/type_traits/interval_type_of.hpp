/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_INTERVAL_TYPE_OF_HPP_JOFA_100910
#define BOOST_ICL_TYPE_TRAITS_INTERVAL_TYPE_OF_HPP_JOFA_100910

#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/icl/type_traits/no_type.hpp>

namespace boost{ namespace icl
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(interval_type)
    }

    template <class Type>
    struct has_interval_type 
      : mpl::bool_<detail::has_interval_type<Type>::value>
    {};

    template <class Type, bool has_interval_type> 
    struct get_interval_type;

    template <class Type>
    struct get_interval_type<Type, false>
    {
        typedef no_type type;
    };

    template <class Type>
    struct get_interval_type<Type, true>
    {
        typedef typename Type::interval_type type;
    };

    template <class Type>
    struct interval_type_of
    {
        typedef typename 
            get_interval_type<Type, has_interval_type<Type>::value>::type type;
    };

}} // namespace boost icl

#endif


