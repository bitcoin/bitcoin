/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_PARAMETRIC_HPP
#define BOOST_SPIRIT_PARAMETRIC_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  f_chlit class [ functional version of chlit ]
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ChGenT>
    struct f_chlit : public char_parser<f_chlit<ChGenT> >
    {
        f_chlit(ChGenT chgen_)
        : chgen(chgen_) {}

        template <typename T>
        bool test(T ch) const
        { return ch == chgen(); }

        ChGenT   chgen;
    };

    template <typename ChGenT>
    inline f_chlit<ChGenT>
    f_ch_p(ChGenT chgen)
    { return f_chlit<ChGenT>(chgen); }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  f_range class [ functional version of range ]
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ChGenAT, typename ChGenBT>
    struct f_range : public char_parser<f_range<ChGenAT, ChGenBT> >
    {
        f_range(ChGenAT first_, ChGenBT last_)
        : first(first_), last(last_)
        {}

        template <typename T>
        bool test(T ch) const
        {
            BOOST_SPIRIT_ASSERT(first() <= last());
            return (ch >= first()) && (ch <= last());
        }

        ChGenAT first;
        ChGenBT last;
    };

    template <typename ChGenAT, typename ChGenBT>
    inline f_range<ChGenAT, ChGenBT>
    f_range_p(ChGenAT first, ChGenBT last)
    { return f_range<ChGenAT, ChGenBT>(first, last); }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  f_chseq class [ functional version of chseq ]
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IterGenAT, typename IterGenBT>
    class f_chseq : public parser<f_chseq<IterGenAT, IterGenBT> >
    {
    public:

        typedef f_chseq<IterGenAT, IterGenBT> self_t;

        f_chseq(IterGenAT first_, IterGenBT last_)
        : first(first_), last(last_) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            return impl::string_parser_parse<result_t>(first(), last(), scan);
        }

    private:

        IterGenAT first;
        IterGenBT last;
    };

    template <typename IterGenAT, typename IterGenBT>
    inline f_chseq<IterGenAT, IterGenBT>
    f_chseq_p(IterGenAT first, IterGenBT last)
    { return f_chseq<IterGenAT, IterGenBT>(first, last); }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  f_strlit class [ functional version of strlit ]
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IterGenAT, typename IterGenBT>
    class f_strlit : public parser<f_strlit<IterGenAT, IterGenBT> >
    {
    public:

        typedef f_strlit<IterGenAT, IterGenBT> self_t;

        f_strlit(IterGenAT first, IterGenBT last)
        : seq(first, last) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename parser_result<self_t, ScannerT>::type result_t;
            return impl::contiguous_parser_parse<result_t>
                (seq, scan, scan);
        }

    private:

        f_chseq<IterGenAT, IterGenBT> seq;
    };

    template <typename IterGenAT, typename IterGenBT>
    inline f_strlit<IterGenAT, IterGenBT>
    f_str_p(IterGenAT first, IterGenBT last)
    { return f_strlit<IterGenAT, IterGenBT>(first, last); }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
