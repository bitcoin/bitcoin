//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_E788439ED9F011DCB181F25B55D89593
#define BOOST_EXCEPTION_E788439ED9F011DCB181F25B55D89593

#include <boost/exception/to_string.hpp>
#include <boost/exception/detail/object_hex_dump.hpp>
#include <boost/assert.hpp>

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
    namespace
    exception_detail
        {
        template <bool ToStringAvailable>
        struct
        to_string_dispatcher
            {
            template <class T,class Stub>
            static
            std::string
            convert( T const & x, Stub )
                {
                return to_string(x);
                }
            };

        template <>
        struct
        to_string_dispatcher<false>
            {
            template <class T,class Stub>
            static
            std::string
            convert( T const & x, Stub s )
                {
                return s(x);
                }

            template <class T>
            static
            std::string
            convert( T const & x, std::string s )
                {
                return s;
                }

            template <class T>
            static
            std::string
            convert( T const & x, char const * s )
                {
                BOOST_ASSERT(s!=0);
                return s;
                }
            };

        namespace
        to_string_dispatch
            {
            template <class T,class Stub>
            inline
            std::string
            dispatch( T const & x, Stub s )
                {
                return to_string_dispatcher<has_to_string<T>::value>::convert(x,s);
                }
            }

        template <class T>
        inline
        std::string
        string_stub_dump( T const & x )
            {
            return "[ " + exception_detail::object_hex_dump(x) + " ]";
            }
        }

    template <class T>
    inline
    std::string
    to_string_stub( T const & x )
        {
        return exception_detail::to_string_dispatch::dispatch(x,&exception_detail::string_stub_dump<T>);
        }

    template <class T,class Stub>
    inline
    std::string
    to_string_stub( T const & x, Stub s )
        {
        return exception_detail::to_string_dispatch::dispatch(x,s);
        }

    template <class T,class U,class Stub>
    inline
    std::string
    to_string_stub( std::pair<T,U> const & x, Stub s )
        {
        return std::string("(") + to_string_stub(x.first,s) + ',' + to_string_stub(x.second,s) + ')';
        }
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
