/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_ABSORBS_IDENTITIES_HPP_JOFA_081004
#define BOOST_ICL_TYPE_TRAITS_ABSORBS_IDENTITIES_HPP_JOFA_081004

namespace boost{ namespace icl
{
    template <class Type> struct absorbs_identities
    {
        typedef absorbs_identities<Type> type;
        BOOST_STATIC_CONSTANT(bool, value = false); 
    };

}} // namespace boost icl

#endif


