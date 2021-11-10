/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_DOMAIN_TYPE_OF_HPP_JOFA_100902
#define BOOST_ICL_TYPE_TRAITS_DOMAIN_TYPE_OF_HPP_JOFA_100902

#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/icl/type_traits/no_type.hpp>

namespace boost{ namespace icl
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(domain_type)
    }

    template <class Type>
    struct has_domain_type 
      : mpl::bool_<detail::has_domain_type<Type>::value>
    {};


    template <class Type, bool has_domain_type> 
    struct get_domain_type;

    template <class Type>
    struct get_domain_type<Type, false>
    {
        typedef no_type type;
    };

    template <class Type>
    struct get_domain_type<Type, true>
    {
        typedef typename Type::domain_type type;
    };

    template <class Type>
    struct domain_type_of
    {
        typedef typename 
            get_domain_type<Type, has_domain_type<Type>::value>::type type;
    };

}} // namespace boost icl

#endif


