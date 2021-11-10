/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_FLUSH_MULTI_PASS_HPP
#define BOOST_SPIRIT_FLUSH_MULTI_PASS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core.hpp>
#include <boost/spirit/home/classic/iterator/multi_pass.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace impl {

        template <typename T>
        void flush_iterator(T &) {}

        template <typename T1, typename T2, typename T3, typename T4>
        void flush_iterator(BOOST_SPIRIT_CLASSIC_NS::multi_pass<
            T1, T2, T3, T4, BOOST_SPIRIT_CLASSIC_NS::multi_pass_policies::std_deque> &i)
        {
            i.clear_queue();
        }

    }   // namespace impl

    ///////////////////////////////////////////////////////////////////////////
    //
    //  flush_multi_pass_parser
    //
    //      The flush_multi_pass_parser flushes an underlying
    //      multi_pass_iterator during the normal parsing process. This may
    //      be used at certain points during the parsing process, when it is
    //      clear, that no backtracking is needed anymore and the input
    //      gathered so far may be discarded.
    //
    ///////////////////////////////////////////////////////////////////////////
    class flush_multi_pass_parser
    :   public parser<flush_multi_pass_parser>
    {
    public:
        typedef flush_multi_pass_parser this_t;

        template <typename ScannerT>
        typename parser_result<this_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            impl::flush_iterator(scan.first);
            return scan.empty_match();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  predefined flush_multi_pass_p object
    //
    //      This object should may used to flush a multi_pass_iterator along
    //      the way during the normal parsing process.
    //
    ///////////////////////////////////////////////////////////////////////////

    flush_multi_pass_parser const
        flush_multi_pass_p = flush_multi_pass_parser();

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_FLUSH_MULTI_PASS_HPP
