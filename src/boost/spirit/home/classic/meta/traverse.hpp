/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TRAVERSE_HPP)
#define BOOST_SPIRIT_TRAVERSE_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/meta/impl/traverse.ipp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Post-order traversal of auxiliary parsers.
    //
    ///////////////////////////////////////////////////////////////////////////
    struct post_order
    {
        //  Return the parser type, which is generated as the result of the
        //  traverse function below.

        template <typename MetaT, typename ParserT>
        struct result
        {
            typedef typename
                traverse_post_order_return<
                    MetaT
                  , ParserT
                  , traverse_post_order_env<0, 0, 0, 0>
                >::type
            type;
        };

        //  Traverse a given parser and refactor it with the help of the given
        //  MetaT metafunction template.

        template <typename MetaT, typename ParserT>
        static typename result<MetaT, ParserT>::type
        traverse(MetaT const &meta_, ParserT const &parser_)
        {
            typedef typename ParserT::parser_category_t parser_category_t;
            return impl::traverse_post_order<parser_category_t>::generate(
                meta_, parser_, traverse_post_order_env<0, 0, 0, 0>());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Transform policies
    //
    //      The following policy classes could be used to assemble some new
    //      transformation metafunction which uses identity transformations
    //      for some parser_category type parsers.
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //  transform plain parsers
    template <typename TransformT>
    struct plain_identity_policy
    {
        template <typename ParserT, typename EnvT>
        struct plain_result
        {
            // plain parsers should be embedded and returned correctly
            typedef typename ParserT::embed_t type;
        };

        template <typename ParserT, typename EnvT>
        typename parser_traversal_plain_result<TransformT, ParserT, EnvT>::type
        generate_plain(ParserT const &parser_, EnvT const& /*env*/) const
        {
            return parser_;
        }
    };

    //////////////////////////////////
    //  transform unary parsers
    template <typename UnaryT, typename SubjectT>
    struct unary_identity_policy_return
    {
        typedef typename UnaryT::parser_generator_t parser_generator_t;
        typedef typename parser_generator_t
            ::template result<SubjectT>::type type;
    };

    template <typename TransformT>
    struct unary_identity_policy
    {
        template <typename UnaryT, typename SubjectT, typename EnvT>
        struct unary_result
        {
            typedef
                typename unary_identity_policy_return<UnaryT, SubjectT>::type
            type;
        };

        template <typename UnaryT, typename SubjectT, typename EnvT>
        typename parser_traversal_unary_result<
            TransformT, UnaryT, SubjectT, EnvT>::type
        generate_unary(
            UnaryT const &, SubjectT const &subject_, EnvT const& /*env*/) const
        {
            typedef typename UnaryT::parser_generator_t parser_generator_t;
            return parser_generator_t::template generate<SubjectT>(subject_);
        }
    };

    //////////////////////////////////
    //  transform action parsers
    template <typename TransformT>
    struct action_identity_policy
    {
        template <typename ActionT, typename SubjectT, typename EnvT>
        struct action_result
        {
            typedef action<SubjectT, typename ActionT::predicate_t> type;
        };

        template <typename ActionT, typename SubjectT, typename EnvT>
        typename parser_traversal_action_result<
            TransformT, ActionT, SubjectT, EnvT
        >::type
        generate_action(ActionT const &action_, SubjectT const &subject_,
            EnvT const& /*env*/) const
        {
            return subject_[action_.predicate()];
        }
    };

    //////////////////////////////////
    //  transform binary parsers
    template <typename BinaryT, typename LeftT, typename RightT>
    struct binary_identity_policy_return
    {
        typedef typename BinaryT::parser_generator_t parser_generator_t;
        typedef typename parser_generator_t
            ::template result<LeftT, RightT>::type type;
    };

    template <typename TransformT>
    struct binary_identity_policy
    {
        template <typename BinaryT, typename LeftT
            , typename RightT, typename EnvT>
        struct binary_result {

            typedef typename
                binary_identity_policy_return<BinaryT, LeftT, RightT>::type
            type;
        };

        template <typename BinaryT, typename LeftT
            , typename RightT, typename EnvT>
        typename parser_traversal_binary_result<
            TransformT, BinaryT, LeftT, RightT, EnvT
        >::type
        generate_binary(
            BinaryT const &, LeftT const& left_
          , RightT const& right_, EnvT const& /*env*/) const
        {
            typedef typename BinaryT::parser_generator_t parser_generator_t;
            return parser_generator_t::
                template generate<LeftT, RightT>(left_, right_);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  transform_policies template
    //
    //      The transform_policies template metafunction could serve as a
    //      base class for new metafunctions to be passed to the traverse meta
    //      template (see above), where only minimal parts have to be
    //      overwritten.
    //
    ///////////////////////////////////////////////////////////////////////////

    template <
        typename TransformT,
        typename PlainPolicyT = plain_identity_policy<TransformT>,
        typename UnaryPolicyT = unary_identity_policy<TransformT>,
        typename ActionPolicyT = action_identity_policy<TransformT>,
        typename BinaryPolicyT = binary_identity_policy<TransformT>
    >
    struct transform_policies :
        public PlainPolicyT,
        public UnaryPolicyT,
        public ActionPolicyT,
        public BinaryPolicyT
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Identity transformation
    //
    //      The identity_transform metafunction supplied to the traverse
    //      template will generate a new parser, which will be exactly
    //      identical to the parser given as the parameter to the traverse
    //      metafunction. I.e. the following conceptual 'equation' will be
    //      always true:
    //
    //      some_parser ==
    //          post_order::traverse(identity_transform(), some_parser)
    //
    ///////////////////////////////////////////////////////////////////////////

    struct identity_transform : transform_policies<identity_transform> {};

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif // !defined(BOOST_SPIRIT_TRAVERSE_HPP)
