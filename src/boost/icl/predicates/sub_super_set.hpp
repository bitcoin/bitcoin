/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_PREDICATES_SUB_SUPER_SET_HPP_JOFA_101102
#define BOOST_ICL_PREDICATES_SUB_SUPER_SET_HPP_JOFA_101102

#include <boost/icl/type_traits/predicate.hpp>
#include <boost/icl/type_traits/type_to_string.hpp>

namespace boost{namespace icl
{

    /// Functor class template contained_in implements the subset relation.
    template<class Type> 
    struct sub_super_set : public relation<Type,Type>
    {
        /// Apply the subset relation.
        /** <tt>contained_in(sub, super)</tt> is true if <tt>sub</tt> 
            is contained in <tt>super</tt> */
        bool operator()(const Type& sub, const Type& super)const
        {
            return contains(super, sub);
        }
    };

    template<>
    inline std::string unary_template_to_string<icl::sub_super_set>::apply()  
    { return "C="; }

    /// Functor class template <b>contains</b> implements the superset relation. 
    template<class Type> 
    struct super_sub_set : public relation<Type,Type>
    {
        /// Apply the superset relation.
        /** <tt>contains(super, sub)</tt> is true if <tt>super</tt> containes 
            <tt>sub</tt> */
        bool operator()(const Type& super, const Type& sub)const
        {
            return contains(super, sub);
        }
    };

    template<>
    inline std::string unary_template_to_string<icl::super_sub_set>::apply()  
    { return "D="; }

}} // namespace icl boost

#endif

