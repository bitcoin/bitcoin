/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_PREDICATES_DISTINCT_EQUAL_HPP_JOFA_101102
#define BOOST_ICL_PREDICATES_DISTINCT_EQUAL_HPP_JOFA_101102

#include <boost/icl/type_traits/predicate.hpp>
#include <boost/icl/type_traits/type_to_string.hpp>

namespace boost{namespace icl
{
    template <class Type> 
    struct distinct_equal : public relation<Type,Type>
    {
        bool operator()(const Type& lhs, const Type& rhs)const
        {
            return is_distinct_equal(lhs, rhs);
        }
    };

    template<>
    inline std::string unary_template_to_string<icl::distinct_equal>::apply()  
    { return "==/0"; }

}} // namespace icl boost

#endif

