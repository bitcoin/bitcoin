/*-----------------------------------------------------------------------------+
Author: Joachim Faulhaber
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------+
States of comparison and inclusion relations as static constants
+-----------------------------------------------------------------------------*/
#ifndef  BOOST_ICL_RELATION_STATE_HPP_JOFA_090214
#define  BOOST_ICL_RELATION_STATE_HPP_JOFA_090214

namespace boost{namespace icl
{
    namespace comparison
    {
        static const int less    = -1;
        static const int equal   =  0;
        static const int greater =  1;
    }

    namespace inclusion
    {
        static const int unrelated =  0;
        static const int subset    =  1;
        static const int superset  =  2;
        static const int equal     =  3;
    }


}} // namespace icl boost

#endif // BOOST_ICL_RELATION_STATE_HPP_JOFA_090214


