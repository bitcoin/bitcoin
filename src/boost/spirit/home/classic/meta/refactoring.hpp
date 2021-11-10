/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_REFACTORING_HPP
#define BOOST_SPIRIT_REFACTORING_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/static_assert.hpp>
#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/meta/as_parser.hpp>
#include <boost/spirit/home/classic/core/parser.hpp>
#include <boost/spirit/home/classic/core/composite/composite.hpp>
#include <boost/spirit/home/classic/meta/impl/refactoring.ipp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  refactor_unary_parser class
//
//      This helper template allows to attach an unary operation to a newly
//      constructed parser, which combines the subject of the left operand of
//      the original given parser (BinaryT) with the right operand of the
//      original binary parser through the original binary operation and
//      rewraps the resulting parser with the original unary operator.
//
//      For instance given the parser:
//          *some_parser - another_parser
//
//      will be refactored to:
//          *(some_parser - another_parser)
//
//      If the parser to refactor is not a unary parser, no refactoring is done
//      at all.
//
//      The original parser should be a binary_parser_category parser,
//      else the compilation will fail
//
///////////////////////////////////////////////////////////////////////////////

template <typename NestedT = non_nested_refactoring>
class refactor_unary_gen;

template <typename BinaryT, typename NestedT = non_nested_refactoring>
class refactor_unary_parser :
    public parser<refactor_unary_parser<BinaryT, NestedT> > {

public:
    //  the parser to refactor has to be at least a binary_parser_category
    //  parser
    BOOST_STATIC_ASSERT((
        boost::is_convertible<typename BinaryT::parser_category_t,
            binary_parser_category>::value
    ));

    refactor_unary_parser(BinaryT const& binary_, NestedT const& nested_)
    : binary(binary_), nested(nested_) {}

    typedef refactor_unary_parser<BinaryT, NestedT> self_t;
    typedef refactor_unary_gen<NestedT> parser_generator_t;
    typedef typename BinaryT::left_t::parser_category_t parser_category_t;

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::refactor_unary_type<NestedT>::
            parse(*this, scan, binary, nested);
    }

private:
    typename as_parser<BinaryT>::type::embed_t binary;
    typename NestedT::embed_t nested;
};

//////////////////////////////////
template <typename NestedT>
class refactor_unary_gen {

public:
    typedef refactor_unary_gen<NestedT> embed_t;

    refactor_unary_gen(NestedT const& nested_ = non_nested_refactoring())
    : nested(nested_) {}

    template <typename ParserT>
    refactor_unary_parser<ParserT, NestedT>
    operator[](parser<ParserT> const& subject) const
    {
        return refactor_unary_parser<ParserT, NestedT>
            (subject.derived(), nested);
    }

private:
    typename NestedT::embed_t nested;
};

const refactor_unary_gen<> refactor_unary_d = refactor_unary_gen<>();

///////////////////////////////////////////////////////////////////////////////
//
//  refactor_action_parser class
//
//      This helper template allows to attach an action taken from the left
//      operand of the given binary parser to a newly constructed parser,
//      which combines the subject of the left operand of the original binary
//      parser with the right operand of the original binary parser by means of
//      the original binary operator parser.
//
//      For instance the parser:
//          some_parser[some_attached_functor] - another_parser
//
//      will be refactored to:
//          (some_parser - another_parser)[some_attached_functor]
//
//      If the left operand to refactor is not an action parser, no refactoring
//      is done at all.
//
//      The original parser should be a binary_parser_category parser,
//      else the compilation will fail
//
///////////////////////////////////////////////////////////////////////////////

template <typename NestedT = non_nested_refactoring>
class refactor_action_gen;

template <typename BinaryT, typename NestedT = non_nested_refactoring>
class refactor_action_parser :
    public parser<refactor_action_parser<BinaryT, NestedT> > {

public:
    //  the parser to refactor has to be at least a binary_parser_category
    //  parser
    BOOST_STATIC_ASSERT((
        boost::is_convertible<typename BinaryT::parser_category_t,
            binary_parser_category>::value
    ));

    refactor_action_parser(BinaryT const& binary_, NestedT const& nested_)
    : binary(binary_), nested(nested_) {}

    typedef refactor_action_parser<BinaryT, NestedT> self_t;
    typedef refactor_action_gen<NestedT> parser_generator_t;
    typedef typename BinaryT::left_t::parser_category_t parser_category_t;

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::refactor_action_type<NestedT>::
            parse(*this, scan, binary, nested);
    }

private:
    typename as_parser<BinaryT>::type::embed_t binary;
    typename NestedT::embed_t nested;
};

//////////////////////////////////
template <typename NestedT>
class refactor_action_gen {

public:
    typedef refactor_action_gen<NestedT> embed_t;

    refactor_action_gen(NestedT const& nested_ = non_nested_refactoring())
    : nested(nested_) {}

    template <typename ParserT>
    refactor_action_parser<ParserT, NestedT>
    operator[](parser<ParserT> const& subject) const
    {
        return refactor_action_parser<ParserT, NestedT>
            (subject.derived(), nested);
    }

private:
    typename NestedT::embed_t nested;
};

const refactor_action_gen<> refactor_action_d = refactor_action_gen<>();

///////////////////////////////////////////////////////////////////////////////
//
//  attach_action_parser class
//
//      This helper template allows to attach an action given separately
//      to all parsers, out of which the given parser is constructed and
//      reconstructs a new parser having the same structure.
//
//      For instance the parser:
//          (some_parser >> another_parser)[some_attached_functor]
//
//      will be refactored to:
//          some_parser[some_attached_functor]
//              >> another_parser[some_attached_functor]
//
//      The original parser should be a action_parser_category parser,
//      else the compilation will fail.
//
//      If the parser, to which the action is attached is not an binary parser,
//      no refactoring is done at all.
//
///////////////////////////////////////////////////////////////////////////////

template <typename NestedT = non_nested_refactoring>
class attach_action_gen;

template <typename ActionT, typename NestedT = non_nested_refactoring>
class attach_action_parser :
    public parser<attach_action_parser<ActionT, NestedT> > {

public:
    //  the parser to refactor has to be at least a action_parser_category
    //  parser
    BOOST_STATIC_ASSERT((
        boost::is_convertible<typename ActionT::parser_category_t,
            action_parser_category>::value
    ));

    attach_action_parser(ActionT const& actor_, NestedT const& nested_)
    : actor(actor_), nested(nested_) {}

    typedef attach_action_parser<ActionT, NestedT> self_t;
    typedef attach_action_gen<NestedT> parser_generator_t;
    typedef typename ActionT::parser_category_t parser_category_t;

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        return impl::attach_action_type<NestedT>::
            parse(*this, scan, actor, nested);
    }

private:
    typename as_parser<ActionT>::type::embed_t actor;
    typename NestedT::embed_t nested;
};

//////////////////////////////////
template <typename NestedT>
class attach_action_gen {

public:
    typedef attach_action_gen<NestedT> embed_t;

    attach_action_gen(NestedT const& nested_ = non_nested_refactoring())
    : nested(nested_) {}

    template <typename ParserT, typename ActionT>
    attach_action_parser<action<ParserT, ActionT>, NestedT>
    operator[](action<ParserT, ActionT> const& actor) const
    {
        return attach_action_parser<action<ParserT, ActionT>, NestedT>
            (actor, nested);
    }

private:
    typename NestedT::embed_t nested;
};

const attach_action_gen<> attach_action_d = attach_action_gen<>();

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // BOOST_SPIRIT_REFACTORING_HPP

