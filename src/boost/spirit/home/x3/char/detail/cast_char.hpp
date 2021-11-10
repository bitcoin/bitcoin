/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_CAST_CHAR_NOVEMBER_10_2006_0907AM)
#define BOOST_SPIRIT_X3_CAST_CHAR_NOVEMBER_10_2006_0907AM

#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/type_traits/make_signed.hpp>

namespace boost { namespace spirit { namespace x3 { namespace detail
{
    // Here's the thing... typical encodings (except ASCII) deal with unsigned
    // integers > 127 (ASCII uses only 127). Yet, most char and wchar_t are signed.
    // Thus, a char with value > 127 is negative (e.g. char 233 is -23). When you
    // cast this to an unsigned int with 32 bits, you get 4294967273!
    //
    // The trick is to cast to an unsigned version of the source char first
    // before casting to the target. {P.S. Don't worry about the code, the
    // optimizer will optimize the if-else branches}

    template <typename TargetChar, typename SourceChar>
    TargetChar cast_char(SourceChar ch)
    {
#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif
        if (is_signed<TargetChar>::value != is_signed<SourceChar>::value)
        {
            if (is_signed<SourceChar>::value)
            {
                 // source is signed, target is unsigned
                typedef typename make_unsigned<SourceChar>::type USourceChar;
                return TargetChar(USourceChar(ch));
            }
            else
            {
                 // source is unsigned, target is signed
                typedef typename make_signed<SourceChar>::type SSourceChar;
                return TargetChar(SSourceChar(ch));
            }
        }
        else
        {
            // source and target has same signedness
            return TargetChar(ch); // just cast
        }
#if defined(_MSC_VER)
# pragma warning(pop)
#endif
    }
}}}}

#endif


