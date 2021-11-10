/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_GREGORIAN_DATE_HPP_JOFA_080416
#define BOOST_ICL_GREGORIAN_DATE_HPP_JOFA_080416

#include <boost/icl/detail/boost_config.hpp>
#include <boost/detail/workaround.hpp>

#ifdef BOOST_MSVC 
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4244) // 'argument' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning(disable:4702) // boost\lexical_cast.hpp(1159) : warning C4702: unreachable code
#pragma warning(disable:4996) // Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/date_time/gregorian/gregorian.hpp>

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/type_traits/is_discrete.hpp>
#include <boost/icl/type_traits/difference_type_of.hpp>
#include <boost/icl/type_traits/size_type_of.hpp>

namespace boost{namespace icl
{
    template<> struct is_discrete<boost::gregorian::date>
    {
        typedef is_discrete type;
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template<> 
    inline boost::gregorian::date identity_element<boost::gregorian::date>::value()
    { 
        return boost::gregorian::date(boost::gregorian::min_date_time); 
    }

    template<> 
    struct identity_element<boost::gregorian::date_duration>
    {
        static boost::gregorian::date_duration value()
        { 
            return boost::gregorian::date(boost::gregorian::min_date_time) 
                 - boost::gregorian::date(boost::gregorian::min_date_time); 
        }
    };

    template<> 
    struct has_difference<boost::gregorian::date> 
    { 
        typedef has_difference type;
        BOOST_STATIC_CONSTANT(bool, value = true);
    };  

    template<> 
    struct difference_type_of<boost::gregorian::date> 
    { typedef boost::gregorian::date_duration type; };  

    template<> 
    struct size_type_of<boost::gregorian::date> 
    { typedef boost::gregorian::date_duration type; };  



    // ------------------------------------------------------------------------
    inline boost::gregorian::date operator ++(boost::gregorian::date& x)
    {
        return x += boost::gregorian::date::duration_type::unit();
    }

    inline boost::gregorian::date operator --(boost::gregorian::date& x)
    {
        return x -= boost::gregorian::date::duration_type::unit();
    }

    // ------------------------------------------------------------------------
    template<> struct is_discrete<boost::gregorian::date_duration>
    {
        typedef is_discrete type;
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template<> 
    struct has_difference<boost::gregorian::date_duration> 
    { 
        typedef has_difference type;
        BOOST_STATIC_CONSTANT(bool, value = true);
    };  

    template<> 
    struct size_type_of<boost::gregorian::date_duration> 
    { 
        typedef boost::gregorian::date_duration type; 
    };  

    inline boost::gregorian::date_duration operator ++(boost::gregorian::date_duration& x)
    {
        return x += boost::gregorian::date::duration_type::unit();
    }

    inline boost::gregorian::date_duration operator --(boost::gregorian::date_duration& x)
    {
        return x -= boost::gregorian::date::duration_type::unit();
    }

    // ------------------------------------------------------------------------


}} // namespace icl boost

#endif


