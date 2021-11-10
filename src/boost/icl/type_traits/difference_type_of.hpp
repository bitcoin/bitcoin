/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_DIFFERENCE_TYPE_OF_HPP_JOFA_080911
#define BOOST_ICL_TYPE_TRAITS_DIFFERENCE_TYPE_OF_HPP_JOFA_080911

#include <boost/config.hpp> // For macro BOOST_STATIC_CONSTANT
#include <boost/type_traits/is_pointer.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/icl/type_traits/no_type.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/icl/type_traits/rep_type_of.hpp>

namespace boost{ namespace icl
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(difference_type)
    }

    //--------------------------------------------------------------------------
    template <class Type>
    struct has_difference_type 
      : mpl::bool_<detail::has_difference_type<Type>::value>
    {};

    //--------------------------------------------------------------------------
    template<class Type> // type_of(T-T)==T
    struct is_subtraction_closed
    {
        typedef is_subtraction_closed type;
        BOOST_STATIC_CONSTANT(bool, 
            value = (mpl::or_< is_numeric<Type>
                             , mpl::and_< has_rep_type<Type>
                                        , mpl::not_<has_difference_type<Type> > 
                                        >
                             >::value)
            );
    };

    //--------------------------------------------------------------------------
    template<class Type>
    struct has_difference
    {
        typedef has_difference type;
        BOOST_STATIC_CONSTANT(bool, 
            value = (mpl::or_< is_subtraction_closed<Type>
                             , is_pointer<Type> 
                             , has_difference_type<Type> >::value) 
            );
    };

    //--------------------------------------------------------------------------
    template <class Type, bool has_difference, bool has_diff_type> 
    struct get_difference_type;

    template <class Type>
    struct get_difference_type<Type, false, false>
    {
        typedef no_type type;
    };

    template <class Type>
    struct get_difference_type<Type*, true, false>
    {
        typedef std::ptrdiff_t type;
    };

    template <class Type>
    struct get_difference_type<Type, true, false>
    {
        typedef Type type;
    };

    template <class Type>
    struct get_difference_type<Type, true, true>
    {
        typedef typename Type::difference_type type;
    };

    //--------------------------------------------------------------------------
    template<class Type> 
    struct difference_type_of
    { 
        typedef typename 
            get_difference_type< Type 
                               , has_difference<Type>::value
                               , has_difference_type<Type>::value
                               >::type type;
    };

}} // namespace boost icl

#endif


