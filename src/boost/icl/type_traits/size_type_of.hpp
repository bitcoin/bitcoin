/*-----------------------------------------------------------------------------+    
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_SIZE_TYPE_OF_HPP_JOFA_080911
#define BOOST_ICL_TYPE_TRAITS_SIZE_TYPE_OF_HPP_JOFA_080911

#include <boost/mpl/has_xxx.hpp>
#include <boost/icl/type_traits/difference_type_of.hpp>

namespace boost{ namespace icl
{

    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(size_type)
    }

    //--------------------------------------------------------------------------
    template <class Type>
    struct has_size_type 
      : mpl::bool_<detail::has_size_type<Type>::value>
    {};

    //--------------------------------------------------------------------------
    template <class Type, bool has_size, bool has_diff, bool has_rep> 
    struct get_size_type;

    template <class Type> 
    struct get_size_type<Type, false, false, false>
    { 
        typedef std::size_t type; 
    };

    template <class Type, bool has_diff, bool has_rep> 
    struct get_size_type<Type, true, has_diff, has_rep>
    { 
        typedef typename Type::size_type type; 
    };

    template <class Type, bool has_rep> 
    struct get_size_type<Type, false, true, has_rep>
    { 
        typedef typename Type::difference_type type; 
    };

    template <class Type> 
    struct get_size_type<Type, false, false, true>
    { 
        typedef Type type; 
    };

    //--------------------------------------------------------------------------
    template<class Type> 
    struct size_type_of
    { 
        typedef typename 
            get_size_type< Type
                         , has_size_type<Type>::value
                         , has_difference_type<Type>::value
                         , has_rep_type<Type>::value  
                         >::type type;
    };

}} // namespace boost icl

#endif


