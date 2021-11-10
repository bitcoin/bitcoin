/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 1999-2003 Jeremiah Willcock
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_OUT_05052005_0121)
#define FUSION_OUT_05052005_0121

#include <boost/fusion/support/config.hpp>
#include <ostream>
#include <boost/fusion/sequence/io/detail/manip.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/equal_to.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename Tag>
    struct delimiter_out
    {
        // print a delimiter
        template <typename OS>
        static void
        print(OS& os, char const* delim, mpl::false_ = mpl::false_())
        {
            detail::string_ios_manip<Tag, OS> manip(os);
            manip.print(delim);
        }

        template <typename OS>
        static void
        print(OS&, char const*, mpl::true_)
        {
        }
    };

    struct print_sequence_loop
    {
        template <typename OS, typename First, typename Last>
        static void
        call(OS&, First const&, Last const&, mpl::true_)
        {
        }

        template <typename OS, typename First, typename Last>
        static void
        call(OS& os, First const& first, Last const& last, mpl::false_)
        {
            result_of::equal_to<
                typename result_of::next<First>::type
              , Last
            >
            is_last;

            os << *first;
            delimiter_out<tuple_delimiter_tag>::print(os, " ", is_last);
            call(os, fusion::next(first), last, is_last);
        }

        template <typename OS, typename First, typename Last>
        static void
        call(OS& os, First const& first, Last const& last)
        {
            result_of::equal_to<First, Last> eq;
            call(os, first, last, eq);
        }
    };

    template <typename OS, typename Sequence>
    inline void
    print_sequence(OS& os, Sequence const& seq)
    {
        delimiter_out<tuple_open_tag>::print(os, "(");
        print_sequence_loop::call(os, fusion::begin(seq), fusion::end(seq));
        delimiter_out<tuple_close_tag>::print(os, ")");
    }
}}}

#endif
