/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_DETAIL_BOOST_CONFIG_HPP_JOFA_101031
#define BOOST_ICL_DETAIL_BOOST_CONFIG_HPP_JOFA_101031

// Since boost_1_44_0 boost/config.hpp can produce warnings too.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996) // Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif

#include <boost/config.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif


#endif


