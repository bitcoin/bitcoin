/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2003 Vaclav Vesely
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_DISTINCT_HPP)
#define BOOST_SPIRIT_DISTINCT_HPP

#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>
#include <boost/spirit/home/classic/core/composite/operators.hpp>
#include <boost/spirit/home/classic/core/composite/directives.hpp>
#include <boost/spirit/home/classic/core/composite/epsilon.hpp>
#include <boost/spirit/home/classic/core/non_terminal/rule.hpp>
#include <boost/spirit/home/classic/utility/chset.hpp>

#include <boost/spirit/home/classic/utility/distinct_fwd.hpp>

namespace boost {
    namespace spirit {
    BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------
// distinct_parser class

template <typename CharT, typename TailT>
class distinct_parser
{
public:
    typedef
        contiguous<
            sequence<
                chseq<CharT const*>,
                negated_empty_match_parser<
                    TailT
                >
            >
        >
            result_t;

    distinct_parser()
    :   tail(chset<CharT>())
    {
    }

    explicit distinct_parser(parser<TailT> const & tail_)
    :   tail(tail_.derived())
    {
    }

    explicit distinct_parser(CharT const* letters)
    :   tail(chset_p(letters))
    {
    }

    result_t operator()(CharT const* str) const
    {
        return lexeme_d[chseq_p(str) >> ~epsilon_p(tail)];
    }

    TailT tail;
};

//-----------------------------------------------------------------------------
// distinct_directive class

template <typename CharT, typename TailT>
class distinct_directive
{
public:
    template<typename ParserT>
    struct result {
        typedef
            contiguous<
                sequence<
                    ParserT,
                    negated_empty_match_parser<
                        TailT
                    >
                >
            >
                type;
    };

    distinct_directive()
    :   tail(chset<CharT>())
    {
    }

    explicit distinct_directive(CharT const* letters)
    :   tail(chset_p(letters))
    {
    }

    explicit distinct_directive(parser<TailT> const & tail_)
    :   tail(tail_.derived())
    {
    }

    template<typename ParserT>
    typename result<typename as_parser<ParserT>::type>::type
        operator[](ParserT const &subject) const
    {
        return
            lexeme_d[as_parser<ParserT>::convert(subject) >> ~epsilon_p(tail)];
    }

    TailT tail;
};

//-----------------------------------------------------------------------------
// dynamic_distinct_parser class

template <typename ScannerT>
class dynamic_distinct_parser
{
public:
    typedef typename ScannerT::value_t char_t;

    typedef
        rule<
            typename no_actions_scanner<
                typename lexeme_scanner<ScannerT>::type
            >::type
        >
            tail_t;

    typedef
        contiguous<
            sequence<
                chseq<char_t const*>,
                negated_empty_match_parser<
                    tail_t
                >
            >
        >
            result_t;

    dynamic_distinct_parser()
    :   tail(nothing_p)
    {
    }

    template<typename ParserT>
    explicit dynamic_distinct_parser(parser<ParserT> const & tail_)
    :   tail(tail_.derived())
    {
    }

    explicit dynamic_distinct_parser(char_t const* letters)
    :   tail(chset_p(letters))
    {
    }

    result_t operator()(char_t const* str) const
    {
        return lexeme_d[chseq_p(str) >> ~epsilon_p(tail)];
    }

    tail_t tail;
};

//-----------------------------------------------------------------------------
// dynamic_distinct_directive class

template <typename ScannerT>
class dynamic_distinct_directive
{
public:
    typedef typename ScannerT::value_t char_t;

    typedef
        rule<
            typename no_actions_scanner<
                typename lexeme_scanner<ScannerT>::type
            >::type
        >
            tail_t;

    template<typename ParserT>
    struct result {
        typedef
            contiguous<
                sequence<
                    ParserT,
                    negated_empty_match_parser<
                        tail_t
                    >
                >
            >
                type;
    };

    dynamic_distinct_directive()
    :   tail(nothing_p)
    {
    }

    template<typename ParserT>
    explicit dynamic_distinct_directive(parser<ParserT> const & tail_)
    :   tail(tail_.derived())
    {
    }

    explicit dynamic_distinct_directive(char_t const* letters)
    :   tail(chset_p(letters))
    {
    }

    template<typename ParserT>
    typename result<typename as_parser<ParserT>::type>::type
        operator[](ParserT const &subject) const
    {
        return
            lexeme_d[as_parser<ParserT>::convert(subject) >> ~epsilon_p(tail)];
    }

    tail_t tail;
};

//-----------------------------------------------------------------------------
    BOOST_SPIRIT_CLASSIC_NAMESPACE_END
    } // namespace spirit
} // namespace boost

#endif // !defined(BOOST_SPIRIT_DISTINCT_HPP)
