//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_F0EE17BE6C1211DE87FF459155D89593
#define BOOST_EXCEPTION_F0EE17BE6C1211DE87FF459155D89593

#include <boost/exception/info.hpp>
#include <errno.h>
#include <string.h>

#ifndef BOOST_EXCEPTION_ENABLE_WARNINGS
#if __GNUC__*100+__GNUC_MINOR__>301
#pragma GCC system_header
#endif
#ifdef __clang__
#pragma clang system_header
#endif
#ifdef _MSC_VER
#pragma warning(push,1)
#pragma warning(disable:4996)
#endif
#endif

namespace
boost
    {
    typedef error_info<struct errinfo_errno_,int> errinfo_errno;

    //Usage hint:
    //if( c_function(....)!=0 )
    //    BOOST_THROW_EXCEPTION(
    //        failure() <<
    //        errinfo_errno(errno) <<
    //        errinfo_api_function("c_function") );
    inline
    std::string
    to_string( errinfo_errno const & e )
        {
        std::ostringstream tmp;
        int v=e.value();
        tmp  << '[' << error_info_name(e) << "] = " << v << ", \"" << strerror(v) << "\"\n";
        return tmp.str();
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
