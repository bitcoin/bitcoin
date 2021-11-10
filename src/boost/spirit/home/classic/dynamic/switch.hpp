/*=============================================================================
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_SWITCH_HPP
#define BOOST_SPIRIT_SWITCH_HPP

///////////////////////////////////////////////////////////////////////////////
//
//  The default_p parser generator template uses the following magic number
//  as the corresponding case label value inside the generated switch()
//  statements. If this number conflicts with your code, please pick a
//  different one.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_DEFAULTCASE_MAGIC)
#define BOOST_SPIRIT_DEFAULTCASE_MAGIC 0x15F97A7
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit predefined maximum number of possible case_p/default_p case branch
//  parsers.
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_SWITCH_CASE_LIMIT)
#define BOOST_SPIRIT_SWITCH_CASE_LIMIT 3
#endif // !defined(BOOST_SPIRIT_SWITCH_CASE_LIMIT)

///////////////////////////////////////////////////////////////////////////////
#include <boost/static_assert.hpp>

///////////////////////////////////////////////////////////////////////////////
//
// Ensure   BOOST_SPIRIT_SELECT_LIMIT > 0
//
///////////////////////////////////////////////////////////////////////////////
BOOST_STATIC_ASSERT(BOOST_SPIRIT_SWITCH_CASE_LIMIT > 0);

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/config.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/epsilon.hpp>
#include <boost/spirit/home/classic/dynamic/impl/switch.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  The switch_parser allows to build switch like parsing constructs, which
//  will have much better performance as comparable straight solutions.
//
//  Input stream driven syntax:
//
//      switch_p
//      [
//          case_p<'a'>
//              (...parser to use, if the next character is 'a'...),
//          case_p<'b'>
//              (...parser to use, if the next character is 'b'...),
//          default_p
//              (...parser to use, if nothing was matched before...)
//      ]
//
//  General syntax:
//
//      switch_p(...lazy expression returning the switch condition value...)
//      [
//          case_p<1>
//              (...parser to use, if the switch condition value is 1...),
//          case_p<2>
//              (...parser to use, if the switch condition value is 2...),
//          default_p
//              (...parser to use, if nothing was matched before...)
//      ]
//
//  The maximum number of possible case_p branches is defined by the p constant
//  BOOST_SPIRIT_SWITCH_CASE_LIMIT (this value defaults to 3 if not otherwise
//  defined).
//
///////////////////////////////////////////////////////////////////////////////
template <typename CaseT, typename CondT = impl::get_next_token_cond>
struct switch_parser
:   public unary<CaseT, parser<switch_parser<CaseT, CondT> > >
{
    typedef switch_parser<CaseT, CondT>     self_t;
    typedef unary_parser_category           parser_category_t;
    typedef unary<CaseT, parser<self_t> >   base_t;

    switch_parser(CaseT const &case_)
    :   base_t(case_), cond(CondT())
    {}

    switch_parser(CaseT const &case_, CondT const &cond_)
    :   base_t(case_), cond(cond_)
    {}

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, nil_t>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return this->subject().parse(scan,
            impl::make_cond_functor<CondT>::do_(cond));
    }

    CondT cond;
};

///////////////////////////////////////////////////////////////////////////////
template <typename CondT>
struct switch_cond_parser
{
    switch_cond_parser(CondT const &cond_)
    :   cond(cond_)
    {}

    template <typename ParserT>
    switch_parser<ParserT, CondT>
    operator[](parser<ParserT> const &p) const
    {
        return switch_parser<ParserT, CondT>(p.derived(), cond);
    }

    CondT const &cond;
};

///////////////////////////////////////////////////////////////////////////////
template <int N, typename ParserT, bool IsDefault>
struct case_parser
:   public unary<ParserT, parser<case_parser<N, ParserT, IsDefault> > >
{
    typedef case_parser<N, ParserT, IsDefault> self_t;
    typedef unary_parser_category               parser_category_t;
    typedef unary<ParserT, parser<self_t> >     base_t;

    typedef typename base_t::subject_t          self_subject_t;

    BOOST_STATIC_CONSTANT(int, value = N);
    BOOST_STATIC_CONSTANT(bool, is_default = IsDefault);
    BOOST_STATIC_CONSTANT(bool, is_simple = true);
    BOOST_STATIC_CONSTANT(bool, is_epsilon = (
        is_default && boost::is_same<self_subject_t, epsilon_parser>::value
    ));

    case_parser(parser<ParserT> const &p)
    :   base_t(p.derived())
    {}

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, nil_t>::type type;
    };

    template <typename ScannerT, typename CondT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan, CondT const &cond) const
    {
        typedef impl::default_case<self_t> default_t;

        if (!scan.at_end()) {
            typedef impl::default_delegate_parse<
                value, is_default, default_t::value> default_parse_t;

            typename ScannerT::iterator_t const save(scan.first);
            return default_parse_t::parse(cond(scan), *this,
                *this, scan, save);
        }

        return default_t::is_epsilon ? scan.empty_match() : scan.no_match();
    }

    template <int N1, typename ParserT1, bool IsDefault1>
    impl::compound_case_parser<
        self_t, case_parser<N1, ParserT1, IsDefault1>, IsDefault1
    >
    operator, (case_parser<N1, ParserT1, IsDefault1> const &p) const
    {
        //  If the following compile time assertion fires, you've probably used
        //  more than one default_p case inside the switch_p parser construct.
        BOOST_STATIC_ASSERT(!is_default || !IsDefault1);

        typedef case_parser<N1, ParserT1, IsDefault1> right_t;
        return impl::compound_case_parser<self_t, right_t, IsDefault1>(*this, p);
    }
};

///////////////////////////////////////////////////////////////////////////////
struct switch_parser_gen {

//  This generates a switch parser, which is driven by the condition value
//  returned by the lazy parameter expression 'cond'. This may be a parser,
//  which result is used or a phoenix actor, which will be dereferenced to
//  obtain the switch condition value.
    template <typename CondT>
    switch_cond_parser<CondT>
    operator()(CondT const &cond) const
    {
        return switch_cond_parser<CondT>(cond);
    }

//  This generates a switch parser, which is driven by the next character/token
//  found in the input stream.
    template <typename CaseT>
    switch_parser<CaseT>
    operator[](parser<CaseT> const &p) const
    {
        return switch_parser<CaseT>(p.derived());
    }
};

switch_parser_gen const switch_p = switch_parser_gen();

///////////////////////////////////////////////////////////////////////////////
template <int N, typename ParserT>
inline case_parser<N, ParserT, false>
case_p(parser<ParserT> const &p)
{
    return case_parser<N, ParserT, false>(p);
}

///////////////////////////////////////////////////////////////////////////////
struct default_parser_gen
:   public case_parser<BOOST_SPIRIT_DEFAULTCASE_MAGIC, epsilon_parser, true>
{
    default_parser_gen()
    :   case_parser<BOOST_SPIRIT_DEFAULTCASE_MAGIC, epsilon_parser, true>
            (epsilon_p)
    {}

    template <typename ParserT>
    case_parser<BOOST_SPIRIT_DEFAULTCASE_MAGIC, ParserT, true>
    operator()(parser<ParserT> const &p) const
    {
        return case_parser<BOOST_SPIRIT_DEFAULTCASE_MAGIC, ParserT, true>(p);
    }
};

default_parser_gen const default_p = default_parser_gen();

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}  // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_SWITCH_HPP
