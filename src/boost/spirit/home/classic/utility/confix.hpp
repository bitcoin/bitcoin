/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CONFIX_HPP
#define BOOST_SPIRIT_CONFIX_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/config.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>
#include <boost/spirit/home/classic/core/composite/operators.hpp>

#include <boost/spirit/home/classic/utility/confix_fwd.hpp>
#include <boost/spirit/home/classic/utility/impl/confix.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  confix_parser class
//
//      Parses a sequence of 3 sub-matches. This class may
//      be used to parse structures, where the opening part is possibly
//      contained in the expression part and the whole sequence is only
//      parsed after seeing the closing part matching the first opening
//      subsequence. Example: C-comments:
//
//      /* This is a C-comment */
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

template<typename NestedT = non_nested, typename LexemeT = non_lexeme>
struct confix_parser_gen;

template <
    typename OpenT, typename ExprT, typename CloseT, typename CategoryT,
    typename NestedT, typename LexemeT
>
struct confix_parser :
    public parser<
        confix_parser<OpenT, ExprT, CloseT, CategoryT, NestedT, LexemeT>
    >
{
    typedef
        confix_parser<OpenT, ExprT, CloseT, CategoryT, NestedT, LexemeT>
        self_t;

    confix_parser(OpenT const &open_, ExprT const &expr_, CloseT const &close_)
    : open(open_), expr(expr_), close(close_)
    {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::confix_parser_type<CategoryT>::
            parse(NestedT(), LexemeT(), *this, scan, open, expr, close);
    }

private:

    typename as_parser<OpenT>::type::embed_t open;
    typename as_parser<ExprT>::type::embed_t expr;
    typename as_parser<CloseT>::type::embed_t close;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Confix parser generator template
//
//      This is a helper for generating a correct confix_parser<> from
//      auxiliary parameters. There are the following types supported as
//      parameters yet: parsers, single characters and strings (see
//      as_parser).
//
//      If the body parser is an action_parser_category type parser (a parser
//      with an attached semantic action) we have to do something special. This
//      happens, if the user wrote something like:
//
//          confix_p(open, body[f], close)
//
//      where 'body' is the parser matching the body of the confix sequence
//      and 'f' is a functor to be called after matching the body. If we would
//      do nothing, the resulting code would parse the sequence as follows:
//
//          start >> (body[f] - close) >> close
//
//      what in most cases is not what the user expects.
//      (If this _is_ what you've expected, then please use the confix_p
//      generator function 'direct()', which will inhibit
//      re-attaching the actor to the body parser).
//
//      To make the confix parser behave as expected:
//
//          start >> (body - close)[f] >> close
//
//      the actor attached to the 'body' parser has to be re-attached to the
//      (body - close) parser construct, which will make the resulting confix
//      parser 'do the right thing'. This refactoring is done by the help of
//      the refactoring parsers (see the files refactoring.[hi]pp).
//
//      Additionally special care must be taken, if the body parser is a
//      unary_parser_category type parser as
//
//          confix_p(open, *anychar_p, close)
//
//      which without any refactoring would result in
//
//          start >> (*anychar_p - close) >> close
//
//      and will not give the expected result (*anychar_p will eat up all the
//      input up to the end of the input stream). So we have to refactor this
//      into:
//
//          start >> *(anychar_p - close) >> close
//
//      what will give the correct result.
//
//      The case, where the body parser is a combination of the two mentioned
//      problems (i.e. the body parser is a unary parser  with an attached
//      action), is handled accordingly too:
//
//          confix_p(start, (*anychar_p)[f], end)
//
//      will be parsed as expected:
//
//          start >> (*(anychar_p - end))[f] >> end.
//
///////////////////////////////////////////////////////////////////////////////

template<typename NestedT, typename LexemeT>
struct confix_parser_gen
{
    // Generic generator function for creation of concrete confix parsers

    template<typename StartT, typename ExprT, typename EndT>
    struct paren_op_result_type
    {
        typedef confix_parser<
            typename as_parser<StartT>::type,
            typename as_parser<ExprT>::type,
            typename as_parser<EndT>::type,
            typename as_parser<ExprT>::type::parser_category_t,
            NestedT,
            LexemeT
        > type;
    };
  
    template<typename StartT, typename ExprT, typename EndT>
    typename paren_op_result_type<StartT, ExprT, EndT>::type 
    operator()(StartT const &start_, ExprT const &expr_, EndT const &end_) const
    {
        typedef typename paren_op_result_type<StartT,ExprT,EndT>::type 
            return_t;

        return return_t(
            as_parser<StartT>::convert(start_),
            as_parser<ExprT>::convert(expr_),
            as_parser<EndT>::convert(end_)
        );
    }

    // Generic generator function for creation of concrete confix parsers
    // which have an action directly attached to the ExprT part of the
    // parser (see comment above, no automatic refactoring)

    template<typename StartT, typename ExprT, typename EndT>
    struct direct_result_type
    {
        typedef confix_parser<
            typename as_parser<StartT>::type,
            typename as_parser<ExprT>::type,
            typename as_parser<EndT>::type,
            plain_parser_category,   // do not re-attach action
            NestedT,
            LexemeT
        > type;
    };

    template<typename StartT, typename ExprT, typename EndT>
    typename direct_result_type<StartT,ExprT,EndT>::type
    direct(StartT const &start_, ExprT const &expr_, EndT const &end_) const
    {
        typedef typename direct_result_type<StartT,ExprT,EndT>::type
            return_t;

        return return_t(
            as_parser<StartT>::convert(start_),
            as_parser<ExprT>::convert(expr_),
            as_parser<EndT>::convert(end_)
        );
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  Predefined non_nested confix parser generators
//
///////////////////////////////////////////////////////////////////////////////

const confix_parser_gen<non_nested, non_lexeme> confix_p =
    confix_parser_gen<non_nested, non_lexeme>();

///////////////////////////////////////////////////////////////////////////////
//
//  Comments are special types of confix parsers
//
//      Comment parser generator template. This is a helper for generating a
//      correct confix_parser<> from auxiliary parameters, which is able to
//      parse comment constructs: (StartToken >> Comment text >> EndToken).
//
//      There are the following types supported as parameters yet: parsers,
//      single characters and strings (see as_parser).
//
//      There are two diffenerent predefined comment parser generators
//      (comment_p and comment_nest_p, see below), which may be used for
//      creating special comment parsers in two different ways.
//
//      If these are used with one parameter, a comment starting with the given
//      first parser parameter up to the end of the line is matched. So for
//      instance the following parser matches C++ style comments:
//
//          comment_p("//").
//
//      If these are used with two parameters, a comment starting with the
//      first parser parameter up to the second parser parameter is matched.
//      For instance a C style comment parser should be constrcuted as:
//
//          comment_p("/*", "*/").
//
//      Please note, that a comment is parsed implicitly as if the whole
//      comment_p(...) statement were embedded into a lexeme_d[] directive.
//
///////////////////////////////////////////////////////////////////////////////

template<typename NestedT>
struct comment_parser_gen
{
    // Generic generator function for creation of concrete comment parsers
    // from an open token. The newline parser eol_p is used as the
    // closing token.

    template<typename StartT>
    struct paren_op1_result_type
    {
        typedef confix_parser<
            typename as_parser<StartT>::type,
            kleene_star<anychar_parser>,
            alternative<eol_parser, end_parser>,
            unary_parser_category,          // there is no action to re-attach
            NestedT,
            is_lexeme                       // insert implicit lexeme_d[]
        >
        type;
    };

    template<typename StartT>
    typename paren_op1_result_type<StartT>::type 
    operator() (StartT const &start_) const
    {
        typedef typename paren_op1_result_type<StartT>::type
            return_t;

        return return_t(
            as_parser<StartT>::convert(start_),
            *anychar_p,
            eol_p | end_p
        );
    }

    // Generic generator function for creation of concrete comment parsers
    // from an open and a close tokens.

    template<typename StartT, typename EndT>
    struct paren_op2_result_type
    {
        typedef confix_parser<
            typename as_parser<StartT>::type,
            kleene_star<anychar_parser>,
            typename as_parser<EndT>::type,
            unary_parser_category,          // there is no action to re-attach
            NestedT,
            is_lexeme                       // insert implicit lexeme_d[]
        > type;
    };

    template<typename StartT, typename EndT>
    typename paren_op2_result_type<StartT,EndT>::type
    operator() (StartT const &start_, EndT const &end_) const
    {
        typedef typename paren_op2_result_type<StartT,EndT>::type
            return_t;

        return return_t(
            as_parser<StartT>::convert(start_),
            *anychar_p,
            as_parser<EndT>::convert(end_)
        );
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  Predefined non_nested comment parser generator
//
///////////////////////////////////////////////////////////////////////////////

const comment_parser_gen<non_nested> comment_p =
    comment_parser_gen<non_nested>();

///////////////////////////////////////////////////////////////////////////////
//
//  comment_nest_parser class
//
//      Parses a nested comments.
//      Example: nested PASCAL-comments:
//
//      { This is a { nested } PASCAL-comment }
//
///////////////////////////////////////////////////////////////////////////////

template<typename OpenT, typename CloseT>
struct comment_nest_parser:
    public parser<comment_nest_parser<OpenT, CloseT> >
{
    typedef comment_nest_parser<OpenT, CloseT> self_t;

    comment_nest_parser(OpenT const &open_, CloseT const &close_):
        open(open_), close(close_)
    {}

    template<typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const &scan) const
    {
        return do_parse(
            open >> *(*this | (anychar_p - close)) >> close,
            scan);
    }

private:
    template<typename ParserT, typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
        do_parse(ParserT const &p, ScannerT const &scan) const
    {
        return
            impl::contiguous_parser_parse<
                typename parser_result<ParserT, ScannerT>::type
            >(p, scan, scan);
    }

    typename as_parser<OpenT>::type::embed_t open;
    typename as_parser<CloseT>::type::embed_t close;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Predefined nested comment parser generator
//
///////////////////////////////////////////////////////////////////////////////

template<typename OpenT, typename CloseT>
struct comment_nest_p_result
{
    typedef comment_nest_parser<
        typename as_parser<OpenT>::type,
        typename as_parser<CloseT>::type
    > type;
};

template<typename OpenT, typename CloseT>
inline typename comment_nest_p_result<OpenT,CloseT>::type 
comment_nest_p(OpenT const &open, CloseT const &close)
{
    typedef typename comment_nest_p_result<OpenT,CloseT>::type
        result_t;

    return result_t(
        as_parser<OpenT>::convert(open),
        as_parser<CloseT>::convert(close)
    );
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
