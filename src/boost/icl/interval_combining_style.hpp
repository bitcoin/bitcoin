/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_INTERVAL_COMBINING_STYLE_HPP_JOFA_100906
#define BOOST_ICL_INTERVAL_COMBINING_STYLE_HPP_JOFA_100906

namespace boost{ namespace icl
{

namespace interval_combine
{
    BOOST_STATIC_CONSTANT(int, unknown    = 0);
    BOOST_STATIC_CONSTANT(int, joining    = 1);
    BOOST_STATIC_CONSTANT(int, separating = 2);
    BOOST_STATIC_CONSTANT(int, splitting  = 3);
    BOOST_STATIC_CONSTANT(int, elemental  = 4);

} // namespace interval_combine

}} // namespace boost icl

#endif


