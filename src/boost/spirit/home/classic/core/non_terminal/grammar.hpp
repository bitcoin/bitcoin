/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    Copyright (c) 2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_GRAMMAR_HPP)
#define BOOST_SPIRIT_GRAMMAR_HPP

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_SPIRIT_THREADSAFE) && defined(BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#undef BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE
#endif

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/non_terminal/parser_context.hpp>
#include <boost/spirit/home/classic/core/non_terminal/impl/grammar.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  grammar class
//
///////////////////////////////////////////////////////////////////////////////
template <typename DerivedT, typename ContextT = parser_context<> >
struct grammar
    : public parser<DerivedT>
    , public ContextT::base_t
    , public context_aux<ContextT, DerivedT>
    BOOST_SPIRIT_GRAMMAR_ID
{
    typedef grammar<DerivedT, ContextT>         self_t;
    typedef DerivedT const&                     embed_t;
    typedef typename ContextT::context_linker_t context_t;
    typedef typename context_t::attr_t          attr_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, attr_t>::type type;
    };

    grammar() {}
    ~grammar() { impl::grammar_destruct(this); }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse_main(ScannerT const& scan) const
    { return impl::grammar_parser_parse<0>(this, scan); }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef parser_scanner_linker<ScannerT> scanner_t;
        BOOST_SPIRIT_CONTEXT_PARSE(scan, *this, scanner_t, context_t, result_t)
    }

    template <int N>
    impl::entry_grammar<DerivedT, N, ContextT>
    use_parser() const
    { return impl::entry_grammar<DerivedT, N, ContextT>( this->derived()); }

    BOOST_SPIRIT_GRAMMAR_STATE
};

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#undef BOOST_SPIRIT_GRAMMAR_ID
#undef BOOST_SPIRIT_GRAMMAR_STATE
#endif

