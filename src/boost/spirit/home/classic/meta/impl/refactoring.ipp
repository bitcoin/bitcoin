/*=============================================================================
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_REFACTORING_IPP
#define BOOST_SPIRIT_REFACTORING_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  The struct 'self_nested_refactoring' is used to indicate, that the
//  refactoring algorithm should be 'self-nested'.
//
//  The struct 'non_nested_refactoring' is used to indicate, that no nesting
//  of refactoring algorithms is reqired.
//
///////////////////////////////////////////////////////////////////////////////

struct non_nested_refactoring { typedef non_nested_refactoring embed_t; };
struct self_nested_refactoring { typedef self_nested_refactoring embed_t; };

///////////////////////////////////////////////////////////////////////////////
namespace impl {

///////////////////////////////////////////////////////////////////////////////
//
//  Helper templates for refactoring parsers
//
///////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //
    //  refactor the left unary operand of a binary parser
    //
    //      The refactoring should be done only if the left operand is an
    //      unary_parser_category parser.
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct refactor_unary_nested {

        template <
            typename ParserT, typename NestedT,
            typename ScannerT, typename BinaryT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary,
            NestedT const& /*nested_d*/)
        {
            return binary.parse(scan);
        }
    };

    template <>
    struct refactor_unary_nested<unary_parser_category> {

        template <
            typename ParserT, typename ScannerT, typename BinaryT,
            typename NestedT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary,
            NestedT const& nested_d)
        {
            typedef typename BinaryT::parser_generator_t op_t;
            typedef
                typename BinaryT::left_t::parser_generator_t
                unary_t;

            return
                unary_t::generate(
                    nested_d[
                        op_t::generate(binary.left().subject(), binary.right())
                    ]
                ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct refactor_unary_non_nested {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary)
        {
            return binary.parse(scan);
        }
    };

    template <>
    struct refactor_unary_non_nested<unary_parser_category> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary)
        {
            typedef typename BinaryT::parser_generator_t op_t;
            typedef
                typename BinaryT::left_t::parser_generator_t
                unary_t;

            return unary_t::generate(
                op_t::generate(binary.left().subject(), binary.right())
            ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename NestedT>
    struct refactor_unary_type {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            NestedT const& nested_d)
        {
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;

            return refactor_unary_nested<parser_category_t>::
                    parse(p, scan, binary, nested_d);
        }
    };

    template <>
    struct refactor_unary_type<non_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            non_nested_refactoring const&)
        {
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;

            return refactor_unary_non_nested<parser_category_t>::
                    parse(p, scan, binary);
        }

    };

    template <>
    struct refactor_unary_type<self_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            self_nested_refactoring const &nested_tag)
        {
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;
            typedef typename ParserT::parser_generator_t parser_generator_t;

            parser_generator_t nested_d(nested_tag);
            return refactor_unary_nested<parser_category_t>::
                    parse(p, scan, binary, nested_d);
        }

    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  refactor the action on the left operand of a binary parser
    //
    //      The refactoring should be done only if the left operand is an
    //      action_parser_category parser.
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct refactor_action_nested {

        template <
            typename ParserT, typename ScannerT, typename BinaryT,
            typename NestedT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary,
            NestedT const& nested_d)
        {
            return nested_d[binary].parse(scan);
        }
    };

    template <>
    struct refactor_action_nested<action_parser_category> {

        template <
            typename ParserT, typename ScannerT, typename BinaryT,
            typename NestedT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary,
            NestedT const& nested_d)
        {
            typedef typename BinaryT::parser_generator_t binary_gen_t;

            return (
                nested_d[
                    binary_gen_t::generate(
                        binary.left().subject(),
                        binary.right()
                    )
                ][binary.left().predicate()]
            ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct refactor_action_non_nested {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary)
        {
            return binary.parse(scan);
        }
    };

    template <>
    struct refactor_action_non_nested<action_parser_category> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, BinaryT const& binary)
        {
            typedef typename BinaryT::parser_generator_t binary_gen_t;

            return (
                binary_gen_t::generate(
                    binary.left().subject(),
                    binary.right()
                )[binary.left().predicate()]
            ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename NestedT>
    struct refactor_action_type {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            NestedT const& nested_d)
        {
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;

            return refactor_action_nested<parser_category_t>::
                    parse(p, scan, binary, nested_d);
        }
    };

    template <>
    struct refactor_action_type<non_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            non_nested_refactoring const&)
        {
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;

            return refactor_action_non_nested<parser_category_t>::
                parse(p, scan, binary);
        }
    };

    template <>
    struct refactor_action_type<self_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename BinaryT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, BinaryT const& binary,
            self_nested_refactoring const &nested_tag)
        {
            typedef typename ParserT::parser_generator_t parser_generator_t;
            typedef
                typename BinaryT::left_t::parser_category_t
                parser_category_t;

            parser_generator_t nested_d(nested_tag);
            return refactor_action_nested<parser_category_t>::
                    parse(p, scan, binary, nested_d);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  refactor the action attached to a binary parser
    //
    //      The refactoring should be done only if the given parser is an
    //      binary_parser_category parser.
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct attach_action_nested {

        template <
            typename ParserT, typename ScannerT, typename ActionT,
            typename NestedT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, ActionT const &action,
            NestedT const& /*nested_d*/)
        {
            return action.parse(scan);
        }
    };

    template <>
    struct attach_action_nested<binary_parser_category> {

        template <
            typename ParserT, typename ScannerT, typename ActionT,
            typename NestedT
        >
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, ActionT const &action,
            NestedT const& nested_d)
        {
            typedef
                typename ActionT::subject_t::parser_generator_t
                binary_gen_t;

            return (
                binary_gen_t::generate(
                    nested_d[action.subject().left()[action.predicate()]],
                    nested_d[action.subject().right()[action.predicate()]]
                )
            ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename CategoryT>
    struct attach_action_non_nested {

        template <typename ParserT, typename ScannerT, typename ActionT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, ActionT const &action)
        {
            return action.parse(scan);
        }
    };

    template <>
    struct attach_action_non_nested<binary_parser_category> {

        template <typename ParserT, typename ScannerT, typename ActionT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &, ScannerT const& scan, ActionT const &action)
        {
            typedef
                typename ActionT::subject_t::parser_generator_t
                binary_gen_t;

            return (
                binary_gen_t::generate(
                    action.subject().left()[action.predicate()],
                    action.subject().right()[action.predicate()]
                )
            ).parse(scan);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename NestedT>
    struct attach_action_type {

        template <typename ParserT, typename ScannerT, typename ActionT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, ActionT const& action,
            NestedT const& nested_d)
        {
            typedef
                typename ActionT::subject_t::parser_category_t
                parser_category_t;

            return attach_action_nested<parser_category_t>::
                    parse(p, scan, action, nested_d);
        }
    };

    template <>
    struct attach_action_type<non_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename ActionT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, ActionT const &action,
            non_nested_refactoring const&)
        {
            typedef
                typename ActionT::subject_t::parser_category_t
                parser_category_t;

            return attach_action_non_nested<parser_category_t>::
                parse(p, scan, action);
        }
    };

    template <>
    struct attach_action_type<self_nested_refactoring> {

        template <typename ParserT, typename ScannerT, typename ActionT>
        static typename parser_result<ParserT, ScannerT>::type
        parse(ParserT const &p, ScannerT const& scan, ActionT const &action,
            self_nested_refactoring const& nested_tag)
        {
            typedef typename ParserT::parser_generator_t parser_generator_t;
            typedef
                typename ActionT::subject_t::parser_category_t
                parser_category_t;

            parser_generator_t nested_d(nested_tag);
            return attach_action_nested<parser_category_t>::
                    parse(p, scan, action, nested_d);
        }
    };

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif

