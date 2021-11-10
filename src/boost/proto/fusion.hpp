///////////////////////////////////////////////////////////////////////////////
/// \file fusion.hpp
/// Make any Proto expression a valid Fusion sequence
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_FUSION_HPP_EAN_11_04_2006
#define BOOST_PROTO_FUSION_HPP_EAN_11_04_2006

#include <boost/config.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/long.hpp>
#include <boost/mpl/sequence_tag_fwd.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/is_view.hpp>
#include <boost/fusion/include/tag_of_fwd.hpp>
#include <boost/fusion/include/category_of.hpp>
#include <boost/fusion/include/iterator_base.hpp>
#include <boost/fusion/include/intrinsic.hpp>
#include <boost/fusion/include/single_view.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/as_list.hpp>
#include <boost/fusion/include/is_segmented.hpp>
#include <boost/fusion/sequence/comparison/enable_comparison.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/eval.hpp>
#include <boost/proto/make_expr.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4510) // default constructor could not be generated
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4610) // can never be instantiated - user defined constructor required
#endif

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Expr, long Pos>
        struct expr_iterator
          : fusion::iterator_base<expr_iterator<Expr, Pos> >
        {
            typedef Expr expr_type;
            static const long index = Pos;
            typedef fusion::random_access_traversal_tag category;
            typedef
                tag::proto_expr_iterator<
                    typename Expr::proto_tag
                  , typename Expr::proto_domain
                >
            fusion_tag;

            explicit expr_iterator(Expr &e)
              : expr(e)
            {}

            Expr &expr;
        };

        template<typename Tag>
        struct as_element
        {
            template<typename Sig>
            struct result;

            template<typename This, typename Expr>
            struct result<This(Expr)>
              : result<This(Expr const &)>
            {};

            template<typename This, typename Expr>
            struct result<This(Expr &)>
              : mpl::if_c<
                    is_same<Tag, typename Expr::proto_tag>::value
                  , flat_view<Expr>
                  , fusion::single_view<Expr &>
                >
            {};

            template<typename Expr>
            typename result<as_element(Expr &)>::type const
            operator ()(Expr &e) const
            {
                return typename result<as_element(Expr &)>::type(e);
            }

            template<typename Expr>
            typename result<as_element(Expr const &)>::type const
            operator ()(Expr const &e) const
            {
                return typename result<as_element(Expr const &)>::type(e);
            }
        };

        template<typename Expr>
        struct flat_view
          : fusion::sequence_base<flat_view<Expr> >
        {
            typedef fusion::forward_traversal_tag category;
            typedef
                tag::proto_flat_view<
                    typename Expr::proto_tag
                  , typename Expr::proto_domain
                >
            fusion_tag;
            typedef
                typename fusion::result_of::as_list<
                    typename fusion::result_of::transform<
                        Expr
                      , as_element<typename Expr::proto_tag>
                    >::type
                >::type
            segments_type;

            explicit flat_view(Expr &e)
              : segs_(fusion::as_list(fusion::transform(e, as_element<typename Expr::proto_tag>())))
            {}

            segments_type segs_;
        };
    }

    namespace result_of
    {
        template<typename Expr>
        struct flatten
          : flatten<Expr const &>
        {};

        template<typename Expr>
        struct flatten<Expr &>
        {
            typedef detail::flat_view<Expr> type;
        };
    }

    namespace functional
    {
        /// \brief A PolymorphicFunctionObject type that returns a "flattened"
        /// view of a Proto expression tree.
        ///
        /// A PolymorphicFunctionObject type that returns a "flattened"
        /// view of a Proto expression tree. For a tree with a top-most node
        /// tag of type \c T, the elements of the flattened sequence are
        /// determined by recursing into each child node with the same
        /// tag type and returning those nodes of different type. So for
        /// instance, the Proto expression tree corresponding to the
        /// expression <tt>a | b | c</tt> has a flattened view with elements
        /// [a, b, c], even though the tree is grouped as
        /// <tt>((a | b) | c)</tt>.
        struct flatten
        {
            BOOST_PROTO_CALLABLE()

            template<typename Sig>
            struct result;

            template<typename This, typename Expr>
            struct result<This(Expr)>
              : result<This(Expr const &)>
            {};

            template<typename This, typename Expr>
            struct result<This(Expr &)>
            {
                typedef proto::detail::flat_view<Expr> type;
            };

            template<typename Expr>
            proto::detail::flat_view<Expr> const
            operator ()(Expr &e) const
            {
                return proto::detail::flat_view<Expr>(e);
            }

            template<typename Expr>
            proto::detail::flat_view<Expr const> const
            operator ()(Expr const &e) const
            {
                return proto::detail::flat_view<Expr const>(e);
            }
        };
    }

    /// \brief A function that returns a "flattened"
    /// view of a Proto expression tree.
    ///
    /// For a tree with a top-most node
    /// tag of type \c T, the elements of the flattened sequence are
    /// determined by recursing into each child node with the same
    /// tag type and returning those nodes of different type. So for
    /// instance, the Proto expression tree corresponding to the
    /// expression <tt>a | b | c</tt> has a flattened view with elements
    /// [a, b, c], even though the tree is grouped as
    /// <tt>((a | b) | c)</tt>.
    template<typename Expr>
    proto::detail::flat_view<Expr> const
    flatten(Expr &e)
    {
        return proto::detail::flat_view<Expr>(e);
    }

    /// \overload
    ///
    template<typename Expr>
    proto::detail::flat_view<Expr const> const
    flatten(Expr const &e)
    {
        return proto::detail::flat_view<Expr const>(e);
    }

    /// INTERNAL ONLY
    ///
    template<typename Context>
    struct eval_fun
      : proto::callable
    {
        explicit eval_fun(Context &ctx)
          : ctx_(ctx)
        {}

        template<typename Sig>
        struct result;

        template<typename This, typename Expr>
        struct result<This(Expr)>
          : result<This(Expr const &)>
        {};

        template<typename This, typename Expr>
        struct result<This(Expr &)>
          : proto::result_of::eval<Expr, Context>
        {};

        template<typename Expr>
        typename proto::result_of::eval<Expr, Context>::type
        operator ()(Expr &e) const
        {
            return proto::eval(e, this->ctx_);
        }

        template<typename Expr>
        typename proto::result_of::eval<Expr const, Context>::type
        operator ()(Expr const &e) const
        {
            return proto::eval(e, this->ctx_);
        }

    private:
        Context &ctx_;
    };

    /// INTERNAL ONLY
    ///
    template<typename Context>
    struct is_callable<eval_fun<Context> >
      : mpl::true_
    {};
}}

namespace boost { namespace fusion
{
    namespace extension
    {
        template<typename Tag>
        struct is_sequence_impl;

        template<typename Tag, typename Domain>
        struct is_sequence_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
              : mpl::true_
            {};
        };

        template<typename Tag, typename Domain>
        struct is_sequence_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
              : mpl::true_
            {};
        };

        template<typename Tag>
        struct is_view_impl;

        template<typename Tag, typename Domain>
        struct is_view_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
              : mpl::true_
            {};
        };

        template<typename Tag, typename Domain>
        struct is_view_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
              : mpl::false_
            {};
        };

        template<typename Tag>
        struct value_of_impl;

        template<typename Tag, typename Domain>
        struct value_of_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<
                typename Iterator
              , long Arity = proto::arity_of<typename Iterator::expr_type>::value
            >
            struct apply
            {
                typedef
                    typename proto::result_of::child_c<
                        typename Iterator::expr_type
                      , Iterator::index
                    >::value_type
                type;
            };

            template<typename Iterator>
            struct apply<Iterator, 0>
            {
                typedef
                    typename proto::result_of::value<
                        typename Iterator::expr_type
                    >::value_type
                type;
            };
        };

        template<typename Tag>
        struct deref_impl;

        template<typename Tag, typename Domain>
        struct deref_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<
                typename Iterator
              , long Arity = proto::arity_of<typename Iterator::expr_type>::value
            >
            struct apply
            {
                typedef
                    typename proto::result_of::child_c<
                        typename Iterator::expr_type &
                      , Iterator::index
                    >::type
                type;

                static type call(Iterator const &iter)
                {
                    return proto::child_c<Iterator::index>(iter.expr);
                }
            };

            template<typename Iterator>
            struct apply<Iterator, 0>
            {
                typedef
                    typename proto::result_of::value<
                        typename Iterator::expr_type &
                    >::type
                type;

                static type call(Iterator const &iter)
                {
                    return proto::value(iter.expr);
                }
            };
        };

        template<typename Tag>
        struct advance_impl;

        template<typename Tag, typename Domain>
        struct advance_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<typename Iterator, typename N>
            struct apply
            {
                typedef
                    proto::detail::expr_iterator<
                        typename Iterator::expr_type
                      , Iterator::index + N::value
                    >
                type;

                static type call(Iterator const &iter)
                {
                    return type(iter.expr);
                }
            };
        };

        template<typename Tag>
        struct distance_impl;

        template<typename Tag, typename Domain>
        struct distance_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<typename IteratorFrom, typename IteratorTo>
            struct apply
              : mpl::long_<IteratorTo::index - IteratorFrom::index>
            {};
        };

        template<typename Tag>
        struct next_impl;

        template<typename Tag, typename Domain>
        struct next_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<typename Iterator>
            struct apply
              : advance_impl<proto::tag::proto_expr_iterator<Tag, Domain> >::template apply<Iterator, mpl::long_<1> >
            {};
        };

        template<typename Tag>
        struct prior_impl;

        template<typename Tag, typename Domain>
        struct prior_impl<proto::tag::proto_expr_iterator<Tag, Domain> >
        {
            template<typename Iterator>
            struct apply
              : advance_impl<proto::tag::proto_expr_iterator<Tag, Domain> >::template apply<Iterator, mpl::long_<-1> >
            {};
        };

        template<typename Tag>
        struct category_of_impl;

        template<typename Tag, typename Domain>
        struct category_of_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef random_access_traversal_tag type;
            };
        };

        template<typename Tag>
        struct size_impl;

        template<typename Tag, typename Domain>
        struct size_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
              : mpl::long_<0 == Sequence::proto_arity_c ? 1 : Sequence::proto_arity_c>
            {};
        };

        template<typename Tag>
        struct begin_impl;

        template<typename Tag, typename Domain>
        struct begin_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef proto::detail::expr_iterator<Sequence, 0> type;

                static type call(Sequence &seq)
                {
                    return type(seq);
                }
            };
        };

        template<typename Tag>
        struct end_impl;

        template<typename Tag, typename Domain>
        struct end_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef
                    proto::detail::expr_iterator<
                        Sequence
                      , 0 == Sequence::proto_arity_c ? 1 : Sequence::proto_arity_c
                    >
                type;

                static type call(Sequence &seq)
                {
                    return type(seq);
                }
            };
        };

        template<typename Tag>
        struct value_at_impl;

        template<typename Tag, typename Domain>
        struct value_at_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<
                typename Sequence
              , typename Index
              , long Arity = proto::arity_of<Sequence>::value
            >
            struct apply
            {
                typedef
                    typename proto::result_of::child_c<
                        Sequence
                      , Index::value
                    >::value_type
                type;
            };

            template<typename Sequence, typename Index>
            struct apply<Sequence, Index, 0>
            {
                typedef
                    typename proto::result_of::value<
                        Sequence
                    >::value_type
                type;
            };
        };

        template<typename Tag>
        struct at_impl;

        template<typename Tag, typename Domain>
        struct at_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<
                typename Sequence
              , typename Index
              , long Arity = proto::arity_of<Sequence>::value
            >
            struct apply
            {
                typedef
                    typename proto::result_of::child_c<
                        Sequence &
                      , Index::value
                    >::type
                type;

                static type call(Sequence &seq)
                {
                    return proto::child_c<Index::value>(seq);
                }
            };

            template<typename Sequence, typename Index>
            struct apply<Sequence, Index, 0>
            {
                typedef
                    typename proto::result_of::value<
                        Sequence &
                    >::type
                type;

                static type call(Sequence &seq)
                {
                    return proto::value(seq);
                }
            };
        };

        template<typename Tag>
        struct convert_impl;

        template<typename Tag, typename Domain>
        struct convert_impl<proto::tag::proto_expr<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef
                    typename proto::result_of::unpack_expr<
                        Tag
                      , Domain
                      , Sequence
                    >::type
                type;

                static type call(Sequence& seq)
                {
                    return proto::unpack_expr<Tag, Domain>(seq);
                }
            };
        };

        template<typename Tag, typename Domain>
        struct convert_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef
                    typename proto::result_of::unpack_expr<
                        Tag
                      , Domain
                      , Sequence
                    >::type
                type;

                static type call(Sequence& seq)
                {
                    return proto::unpack_expr<Tag, Domain>(seq);
                }
            };
        };

        template<typename Tag>
        struct is_segmented_impl;

        template<typename Tag, typename Domain>
        struct is_segmented_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Iterator>
            struct apply
              : mpl::true_
            {};
        };

        template<typename Tag>
        struct segments_impl;

        template<typename Tag, typename Domain>
        struct segments_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef typename Sequence::segments_type const &type;
                            
                static type call(Sequence &sequence)
                {
                    return sequence.segs_;
                }
            };
        };

        template<typename Tag, typename Domain>
        struct category_of_impl<proto::tag::proto_flat_view<Tag, Domain> >
        {
            template<typename Sequence>
            struct apply
            {
                typedef forward_traversal_tag type;
            };
        };
    }

    namespace traits
    {
        template<typename Seq1, typename Seq2>
        struct enable_equality<
            Seq1
          , Seq2
          , typename enable_if_c<
                mpl::or_<
                    proto::is_expr<Seq1>
                  , proto::is_expr<Seq2>
                >::value
            >::type
        >
            : mpl::false_
        {};

        template<typename Seq1, typename Seq2>
        struct enable_comparison<
            Seq1
          , Seq2
          , typename enable_if_c<
                mpl::or_<
                    proto::is_expr<Seq1>
                  , proto::is_expr<Seq2>
                >::value
            >::type
        >
          : mpl::false_
        {};
    }
}}

namespace boost { namespace mpl
{
    template<typename Tag, typename Args, long Arity>
    struct sequence_tag< proto::expr<Tag, Args, Arity> >
    {
        typedef fusion::fusion_sequence_tag type;
    };

    template<typename Tag, typename Args, long Arity>
    struct sequence_tag< proto::basic_expr<Tag, Args, Arity> >
    {
        typedef fusion::fusion_sequence_tag type;
    };
}}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
