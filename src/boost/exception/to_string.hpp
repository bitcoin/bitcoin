//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_7E48761AD92811DC9011477D56D89593
#define BOOST_EXCEPTION_7E48761AD92811DC9011477D56D89593

#include <boost/utility/enable_if.hpp>
#include <boost/exception/detail/is_output_streamable.hpp>
#include <sstream>

#ifndef BOOST_EXCEPTION_ENABLE_WARNINGS
#if __GNUC__*100+__GNUC_MINOR__>301
#pragma GCC system_header
#endif
#ifdef __clang__
#pragma clang system_header
#endif
#ifdef _MSC_VER
#pragma warning(push,1)
#endif
#endif

namespace
boost
    {
    template <class T,class U>
    std::string to_string( std::pair<T,U> const & );
    std::string to_string( std::exception const & );

    namespace
    to_string_detail
        {
        template <class T>
        typename disable_if<is_output_streamable<T>,char>::type to_string( T const & );
        using boost::to_string;

        template <class,bool IsOutputStreamable>
        struct has_to_string_impl;

        template <class T>
        struct
        has_to_string_impl<T,true>
            {
            enum e { value=1 };
            };

        template <class T>
        struct
        has_to_string_impl<T,false>
            {
            static T const & f();
            enum e { value=1!=sizeof(to_string(f())) };
            };
        }

    template <class T>
    inline
    typename enable_if<is_output_streamable<T>,std::string>::type
    to_string( T const & x )
        {
        std::ostringstream out;
        out << x;
        return out.str();
        }

    template <class T>
    struct
    has_to_string
        {
        enum e { value=to_string_detail::has_to_string_impl<T,is_output_streamable<T>::value>::value };
        };

    template <class T,class U>
    inline
    std::string
    to_string( std::pair<T,U> const & x )
        {
        return std::string("(") + to_string(x.first) + ',' + to_string(x.second) + ')';
        }

    inline
    std::string
    to_string( std::exception const & x )
        {
        return x.what();
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
