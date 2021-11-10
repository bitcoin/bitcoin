///////////////////////////////////////////////////////////////////////////////
/// \file fold_tree.hpp
/// Contains definition of the fold_tree<> and reverse_fold_tree<> transforms.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_FOLD_TREE_HPP_EAN_11_05_2007
#define BOOST_PROTO_TRANSFORM_FOLD_TREE_HPP_EAN_11_05_2007

#include <boost/type_traits/is_same.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/matches.hpp>
#include <boost/proto/transform/fold.hpp>
#include <boost/proto/transform/impl.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Tag>
        struct has_tag
        {
            template<typename Expr, typename State, typename Data, typename EnableIf = Tag>
            struct impl
            {
                typedef mpl::false_ result_type;
            };

            template<typename Expr, typename State, typename Data>
            struct impl<Expr, State, Data, typename Expr::proto_tag>
            {
                typedef mpl::true_ result_type;
            };

            template<typename Expr, typename State, typename Data>
            struct impl<Expr &, State, Data, typename Expr::proto_tag>
            {
                typedef mpl::true_ result_type;
            };
        };

        template<typename Tag, typename Fun>
        struct fold_tree_
          : if_<has_tag<Tag>, fold<_, _state, fold_tree_<Tag, Fun> >, Fun>
        {};

        template<typename Tag, typename Fun>
        struct reverse_fold_tree_
          : if_<has_tag<Tag>, reverse_fold<_, _state, reverse_fold_tree_<Tag, Fun> >, Fun>
        {};
    }

    /// \brief A PrimitiveTransform that recursively applies the
    /// <tt>fold\<\></tt> transform to sub-trees that all share a common
    /// tag type.
    ///
    /// <tt>fold_tree\<\></tt> is useful for flattening trees into lists;
    /// for example, you might use <tt>fold_tree\<\></tt> to flatten an
    /// expression tree like <tt>a | b | c</tt> into a Fusion list like
    /// <tt>cons(c, cons(b, cons(a)))</tt>.
    ///
    /// <tt>fold_tree\<\></tt> is easily understood in terms of a
    /// <tt>recurse_if_\<\></tt> helper, defined as follows:
    ///
    /// \code
    /// template<typename Tag, typename Fun>
    /// struct recurse_if_
    ///   : if_<
    ///         // If the current node has type "Tag" ...
    ///         is_same<tag_of<_>, Tag>()
    ///         // ... recurse, otherwise ...
    ///       , fold<_, _state, recurse_if_<Tag, Fun> >
    ///         // ... apply the Fun transform.
    ///       , Fun
    ///     >
    /// {};
    /// \endcode
    ///
    /// With <tt>recurse_if_\<\></tt> as defined above,
    /// <tt>fold_tree\<Sequence, State0, Fun\>()(e, s, d)</tt> is
    /// equivalent to
    /// <tt>fold<Sequence, State0, recurse_if_<Expr::proto_tag, Fun> >()(e, s, d).</tt>
    /// It has the effect of folding a tree front-to-back, recursing into
    /// child nodes that share a tag type with the parent node.
    template<typename Sequence, typename State0, typename Fun>
    struct fold_tree
      : transform<fold_tree<Sequence, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : fold<
                Sequence
              , State0
              , detail::fold_tree_<typename Expr::proto_tag, Fun>
            >::template impl<Expr, State, Data>
        {};

        template<typename Expr, typename State, typename Data>
        struct impl<Expr &, State, Data>
          : fold<
                Sequence
              , State0
              , detail::fold_tree_<typename Expr::proto_tag, Fun>
            >::template impl<Expr &, State, Data>
        {};
    };

    /// \brief A PrimitiveTransform that recursively applies the
    /// <tt>reverse_fold\<\></tt> transform to sub-trees that all share
    /// a common tag type.
    ///
    /// <tt>reverse_fold_tree\<\></tt> is useful for flattening trees into
    /// lists; for example, you might use <tt>reverse_fold_tree\<\></tt> to
    /// flatten an expression tree like <tt>a | b | c</tt> into a Fusion list
    /// like <tt>cons(a, cons(b, cons(c)))</tt>.
    ///
    /// <tt>reverse_fold_tree\<\></tt> is easily understood in terms of a
    /// <tt>recurse_if_\<\></tt> helper, defined as follows:
    ///
    /// \code
    /// template<typename Tag, typename Fun>
    /// struct recurse_if_
    ///   : if_<
    ///         // If the current node has type "Tag" ...
    ///         is_same<tag_of<_>, Tag>()
    ///         // ... recurse, otherwise ...
    ///       , reverse_fold<_, _state, recurse_if_<Tag, Fun> >
    ///         // ... apply the Fun transform.
    ///       , Fun
    ///     >
    /// {};
    /// \endcode
    ///
    /// With <tt>recurse_if_\<\></tt> as defined above,
    /// <tt>reverse_fold_tree\<Sequence, State0, Fun\>()(e, s, d)</tt> is
    /// equivalent to
    /// <tt>reverse_fold<Sequence, State0, recurse_if_<Expr::proto_tag, Fun> >()(e, s, d).</tt>
    /// It has the effect of folding a tree back-to-front, recursing into
    /// child nodes that share a tag type with the parent node.
    template<typename Sequence, typename State0, typename Fun>
    struct reverse_fold_tree
      : transform<reverse_fold_tree<Sequence, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : reverse_fold<
                Sequence
              , State0
              , detail::reverse_fold_tree_<typename Expr::proto_tag, Fun>
            >::template impl<Expr, State, Data>
        {};

        template<typename Expr, typename State, typename Data>
        struct impl<Expr &, State, Data>
          : reverse_fold<
                Sequence
              , State0
              , detail::reverse_fold_tree_<typename Expr::proto_tag, Fun>
            >::template impl<Expr &, State, Data>
        {};
    };

    /// INTERNAL ONLY
    ///
    template<typename Sequence, typename State0, typename Fun>
    struct is_callable<fold_tree<Sequence, State0, Fun> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Sequence, typename State0, typename Fun>
    struct is_callable<reverse_fold_tree<Sequence, State0, Fun> >
      : mpl::true_
    {};

}}

#endif
