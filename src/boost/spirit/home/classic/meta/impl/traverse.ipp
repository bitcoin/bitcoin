/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_TRAVERSE_IPP)
#define BOOST_SPIRIT_TRAVERSE_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/meta/fundamental.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
namespace impl
{

    template <typename CategoryT>
    struct traverse_post_order_return_category;

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
//
//  Environment class for post_order_traversal
//
///////////////////////////////////////////////////////////////////////////////

template <int Level, int Node, int Index, int LastLeft>
struct traverse_post_order_env {

    BOOST_STATIC_CONSTANT(int, level = Level);
    BOOST_STATIC_CONSTANT(int, node = Node);
    BOOST_STATIC_CONSTANT(int, index = Index);
    BOOST_STATIC_CONSTANT(int, lastleft = LastLeft);
};

///////////////////////////////////////////////////////////////////////////////
//
//  traverse_post_order_return template
//
//      This template is a helper for dispatching the calculation of a parser
//      type result for a traversal level to the corresponding parser_category
//      based specialization.
//
///////////////////////////////////////////////////////////////////////////////

template <typename MetaT, typename ParserT, typename EnvT>
struct traverse_post_order_return {

    typedef typename ParserT::parser_category_t parser_category_t;
    typedef typename impl::traverse_post_order_return_category<parser_category_t>
        ::template result<MetaT, ParserT, EnvT>::type type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  parser_traversal_..._result templates
//
//      These are metafunctions, which calculate the resulting parser type
//      for all subparsers and feed these types to the user supplied
//      metafunctions to get back the resulting parser type of this traversal
//      level.
//
///////////////////////////////////////////////////////////////////////////////

template <typename MetaT, typename ParserT, typename EnvT>
struct parser_traversal_plain_result {

    typedef typename MetaT::template plain_result<ParserT, EnvT>::type type;
};

///////////////////////////////////////////////////////////////////////////////
template <typename MetaT, typename UnaryT, typename SubjectT, typename EnvT>
struct parser_traversal_unary_result {

    typedef typename MetaT
        ::template unary_result<UnaryT, SubjectT, EnvT>::type type;
};

///////////////////////////////////////////////////////////////////////////////
template <typename MetaT, typename ActionT, typename SubjectT, typename EnvT>
struct parser_traversal_action_result {

    typedef typename MetaT
        ::template action_result<ActionT, SubjectT, EnvT>::type type;
};

///////////////////////////////////////////////////////////////////////////////
template <
    typename MetaT, typename BinaryT, typename LeftT,
    typename RightT, typename EnvT
>
struct parser_traversal_binary_result {

    BOOST_STATIC_CONSTANT(int,
        thisnum = (node_count<BinaryT>::value + EnvT::lastleft-1));
    BOOST_STATIC_CONSTANT(int,
        leftnum = (node_count<LeftT>::value + EnvT::lastleft-1));
    BOOST_STATIC_CONSTANT(int,
        leafnum = (leaf_count<LeftT>::value + EnvT::index));

    typedef parser_traversal_binary_result self_t;

    // left traversal environment and resulting parser type
    typedef traverse_post_order_env<
                (EnvT::level+1), (self_t::leftnum), (EnvT::index), (EnvT::lastleft)
            > left_sub_env_t;
    typedef typename traverse_post_order_return<
                MetaT, LeftT, left_sub_env_t
            >::type
        left_t;

    // right traversal environment and resulting parser type
    typedef traverse_post_order_env<
                (EnvT::level+1), (self_t::thisnum-1), (self_t::leafnum), (self_t::leftnum+1)
            > right_sub_env_t;
    typedef typename traverse_post_order_return<
                MetaT, RightT, right_sub_env_t
            >::type
        right_t;

    typedef typename MetaT::template binary_result<
                BinaryT, left_t, right_t, EnvT
            >::type
        type;
};

///////////////////////////////////////////////////////////////////////////////
namespace impl
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  Meta functions, which dispatch the calculation of the return type of
    //  of the post_order traverse function to the result template of the
    //  corresponding parser_category based metafunction template.
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename CategoryT>
    struct traverse_post_order_return_category;

    template <>
    struct traverse_post_order_return_category<plain_parser_category> {

        template <typename MetaT, typename ParserT, typename EnvT>
        struct result {

            typedef typename parser_traversal_plain_result<
                        MetaT, ParserT, EnvT
                    >::type
                type;
        };
    };

    template <>
    struct traverse_post_order_return_category<unary_parser_category> {

        template <typename MetaT, typename ParserT, typename EnvT>
        struct result {

            typedef typename parser_traversal_unary_result<
                        MetaT, ParserT, typename ParserT::subject_t, EnvT
                    >::type
                type;
        };
    };

    template <>
    struct traverse_post_order_return_category<action_parser_category> {

        template <typename MetaT, typename ParserT, typename EnvT>
        struct result {

            typedef typename parser_traversal_action_result<
                        MetaT, ParserT, typename ParserT::subject_t, EnvT
                    >::type
                type;
        };
    };

    template <>
    struct traverse_post_order_return_category<binary_parser_category> {

        template <typename MetaT, typename ParserT, typename EnvT>
        struct result {

            typedef typename parser_traversal_binary_result<
                        MetaT, ParserT, typename ParserT::left_t,
                        typename ParserT::right_t, EnvT
                    >::type
                type;
        };
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Post-order parser traversal
    //
    //      The following templates contain the parser_category based code for
    //
    //        - calculating the type of the resulting parser, which is to be
    //          returned from a level of traversal
    //        - traversing down the composite parser structure, this traversal
    //          returnes a new parser object
    //
    //      Both tasks are delegated to the MetaT metafunction supplied by the
    //      user.
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename CategoryT>
    struct traverse_post_order;

    template <>
    struct traverse_post_order<plain_parser_category> {

        template <typename MetaT, typename ParserT, typename EnvT>
        struct result {

            typedef
                typename parser_traversal_plain_result<MetaT, ParserT, EnvT>::type
                type;
        };

        template <typename MetaT, typename ParserT, typename EnvT>
        static
        typename parser_traversal_plain_result<MetaT, ParserT, EnvT>::type
        generate(MetaT const &meta_, ParserT const &parser_, EnvT const &env)
        {
            return meta_.generate_plain(parser_, env);
        }
    };

    template <>
    struct traverse_post_order<unary_parser_category> {

        template <
            typename MetaT, typename ParserT, typename SubjectT, typename EnvT
        >
        struct result {

            typedef typename parser_traversal_unary_result<
                        MetaT, ParserT, SubjectT, EnvT
                    >::type
                type;
        };

        template <typename MetaT, typename ParserT, typename EnvT>
        static
        typename parser_traversal_unary_result<
            MetaT, ParserT, 
            typename traverse_post_order_return<
                MetaT, typename ParserT::subject_t, EnvT
            >::type,
            EnvT
        >::type
        generate(MetaT const &meta_, ParserT const &unary_, EnvT const &env)
        {
            typedef typename ParserT::subject_t             subject_t;
            typedef typename subject_t::parser_category_t   subject_category_t;

            return meta_.generate_unary(
                unary_,
                traverse_post_order<subject_category_t>::generate(meta_,
                    unary_.subject(),
                    traverse_post_order_env<
                        EnvT::level+1, EnvT::node-1, EnvT::index, EnvT::lastleft
                    >()
                ),
                env
            );
        }
    };

    template <>
    struct traverse_post_order<action_parser_category> {

        template <
            typename MetaT, typename ParserT, typename SubjectT, typename EnvT
        >
        struct result {

            typedef typename parser_traversal_action_result<
                        MetaT, ParserT, SubjectT, EnvT
                    >::type
                type;
        };

        template <typename MetaT, typename ParserT, typename EnvT>
        static
        typename parser_traversal_action_result<
            MetaT, ParserT, 
            typename traverse_post_order_return<
                MetaT, typename ParserT::subject_t, EnvT
            >::type,
            EnvT
        >::type
        generate(MetaT const &meta_, ParserT const &action_, EnvT const &env)
        {
            typedef typename ParserT::subject_t             subject_t;
            typedef typename subject_t::parser_category_t   subject_category_t;

            return meta_.generate_action(
                action_,
                traverse_post_order<subject_category_t>::generate(meta_,
                    action_.subject(),
                    traverse_post_order_env<
                        EnvT::level+1, EnvT::node-1, EnvT::index, EnvT::lastleft
                    >()
                ),
                env
            );
        }
    };

    template <>
    struct traverse_post_order<binary_parser_category> {

        template <
            typename MetaT, typename ParserT, typename LeftT,
            typename RightT, typename EnvT
        >
        struct result {

            typedef typename parser_traversal_binary_result<
                        MetaT, ParserT, LeftT, RightT, EnvT
                    >::type
                type;
        };

        template <typename MetaT, typename ParserT, typename EnvT>
        static
        typename parser_traversal_binary_result<
            MetaT, ParserT, 
            typename traverse_post_order_return<
                MetaT, typename ParserT::left_t, EnvT
            >::type,
            typename traverse_post_order_return<
                MetaT, typename ParserT::right_t, EnvT
            >::type,
            EnvT
        >::type
        generate(MetaT const &meta_, ParserT const &binary_, EnvT const& /*env*/)
        {
            typedef typename ParserT::left_t                left_t;
            typedef typename ParserT::right_t               right_t;
            typedef typename left_t::parser_category_t      left_category_t;
            typedef typename right_t::parser_category_t     right_category_t;

            enum {
                leftnum = (node_count<left_t>::value + EnvT::lastleft-1),
                thisnum = (node_count<ParserT>::value + EnvT::lastleft-1),
                rightnum = (thisnum-1),
                leafnum = (leaf_count<left_t>::value + EnvT::index)
            };

            return meta_.generate_binary(
                binary_,
                traverse_post_order<left_category_t>::generate(
                    meta_, binary_.left(),
                    traverse_post_order_env<
                        EnvT::level+1, leftnum, EnvT::index, EnvT::lastleft
                    >()
                ),
                traverse_post_order<right_category_t>::generate(
                    meta_, binary_.right(),
                    traverse_post_order_env<
                        EnvT::level+1, rightnum, leafnum, leftnum+1
                    >()
                ),
                traverse_post_order_env<
                    EnvT::level, thisnum, EnvT::index, EnvT::lastleft
                >()
            );
        }
    };

}   //  namespace impl

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif // !defined(BOOST_SPIRIT_TRAVERSE_IPP)
