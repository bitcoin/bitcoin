//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_EXCEPTION_898984B4076411DD973EDFA055D89593
#define BOOST_EXCEPTION_898984B4076411DD973EDFA055D89593

#include <ostream>

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
    to_string_detail
        {
        struct
        partial_ordering_helper1
            {
            template <class CharT,class Traits>
            partial_ordering_helper1( std::basic_ostream<CharT,Traits> & );
            };

        struct
        partial_ordering_helper2
            {
            template <class T>
            partial_ordering_helper2( T const & );
            };

        char operator<<( partial_ordering_helper1, partial_ordering_helper2 );

        template <class T,class CharT,class Traits>
        struct
        is_output_streamable_impl
            {
            static std::basic_ostream<CharT,Traits> & f();
            static T const & g();
            enum e { value=1!=(sizeof(f()<<g())) };
            };
        }

    template <class T, class CharT=char, class Traits=std::char_traits<CharT> >
    struct
    is_output_streamable
        {
        enum e { value=to_string_detail::is_output_streamable_impl<T,CharT,Traits>::value };
        };
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif
