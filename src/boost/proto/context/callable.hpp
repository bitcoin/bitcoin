///////////////////////////////////////////////////////////////////////////////
/// \file callable.hpp
/// Definintion of callable_context\<\>, an evaluation context for
/// proto::eval() that explodes each node and calls the derived context
/// type with the expressions constituents. If the derived context doesn't
/// have an overload that handles this node, fall back to some other
/// context.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_CONTEXT_CALLABLE_HPP_EAN_06_23_2007
#define BOOST_PROTO_CONTEXT_CALLABLE_HPP_EAN_06_23_2007

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/selection/max.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp> // for child_c

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Context>
        struct callable_context_wrapper
          : remove_cv<Context>::type
        {
            callable_context_wrapper();
            typedef private_type_ fun_type(...);
            operator fun_type *() const;

            BOOST_DELETED_FUNCTION(callable_context_wrapper &operator =(callable_context_wrapper const &))
        };

        template<typename T>
        yes_type check_is_expr_handled(T const &);

        no_type check_is_expr_handled(private_type_ const &);

        template<typename Expr, typename Context, long Arity = Expr::proto_arity_c>
        struct is_expr_handled;

        template<typename Expr, typename Context>
        struct is_expr_handled<Expr, Context, 0>
        {
            static callable_context_wrapper<Context> &sctx_;
            static Expr &sexpr_;
            static typename Expr::proto_tag &stag_;

            static const bool value =
                sizeof(yes_type) ==
                sizeof(
                    detail::check_is_expr_handled(
                        (sctx_(stag_, proto::value(sexpr_)), 0)
                    )
                );

            typedef mpl::bool_<value> type;
        };
    }

    namespace context
    {
        /// \brief A BinaryFunction that accepts a Proto expression and a
        /// callable context and calls the context with the expression tag
        /// and children as arguments, effectively fanning the expression
        /// out.
        ///
        /// <tt>callable_eval\<\></tt> requires that \c Context is a
        /// PolymorphicFunctionObject that can be invoked with \c Expr's
        /// tag and children as expressions, as follows:
        ///
        /// \code
        /// context(Expr::proto_tag(), child_c<0>(expr), child_c<1>(expr), ...)
        /// \endcode
        template<
            typename Expr
          , typename Context
          , long Arity          // = Expr::proto_arity_c
        >
        struct callable_eval
        {};

        /// \brief A BinaryFunction that accepts a Proto expression and a
        /// callable context and calls the context with the expression tag
        /// and children as arguments, effectively fanning the expression
        /// out.
        ///
        /// <tt>callable_eval\<\></tt> requires that \c Context is a
        /// PolymorphicFunctionObject that can be invoked with \c Expr's
        /// tag and children as expressions, as follows:
        ///
        /// \code
        /// context(Expr::proto_tag(), value(expr))
        /// \endcode
        template<typename Expr, typename Context>
        struct callable_eval<Expr, Context, 0>
        {
            typedef typename proto::result_of::value<Expr const &>::type value_type;

            typedef
                typename BOOST_PROTO_RESULT_OF<
                    Context(typename Expr::proto_tag, value_type)
                >::type
            result_type;

            /// \param expr The current expression
            /// \param context The callable evaluation context
            /// \return <tt>context(Expr::proto_tag(), value(expr))</tt>
            result_type operator ()(Expr &expr, Context &context) const
            {
                return context(typename Expr::proto_tag(), proto::value(expr));
            }
        };

        /// \brief An evaluation context adaptor that makes authoring a
        /// context a simple matter of writing function overloads, rather
        /// then writing template specializations.
        ///
        /// <tt>callable_context\<\></tt> is a base class that implements
        /// the context protocol by passing fanned-out expression nodes to
        /// the derived context, making it easy to customize the handling
        /// of expression types by writing function overloads. Only those
        /// expression types needing special handling require explicit
        /// handling. All others are dispatched to a user-specified
        /// default context, \c DefaultCtx.
        ///
        /// <tt>callable_context\<\></tt> is defined simply as:
        ///
        /// \code
        /// template<typename Context, typename DefaultCtx = default_context>
        /// struct callable_context
        /// {
        ///    template<typename Expr, typename ThisContext = Context>
        ///     struct eval
        ///       : mpl::if_<
        ///             is_expr_handled_<Expr, Context> // For exposition
        ///           , callable_eval<Expr, ThisContext>
        ///           , typename DefaultCtx::template eval<Expr, Context>
        ///         >::type
        ///     {};
        /// };
        /// \endcode
        ///
        /// The Boolean metafunction <tt>is_expr_handled_\<\></tt> uses
        /// metaprogramming tricks to determine whether \c Context has
        /// an overloaded function call operator that accepts the
        /// fanned-out constituents of an expression of type \c Expr.
        /// If so, the handling of the expression is dispatched to
        /// <tt>callable_eval\<\></tt>. If not, it is dispatched to
        /// the user-specified \c DefaultCtx.
        ///
        /// Below is an example of how to use <tt>callable_context\<\></tt>:
        ///
        /// \code
        /// // An evaluation context that increments all
        /// // integer terminals in-place.
        /// struct increment_ints
        ///  : callable_context<
        ///         increment_ints const    // derived context
        ///       , null_context const      // fall-back context
        ///     >
        /// {
        ///     typedef void result_type;
        ///
        ///     // Handle int terminals here:
        ///     void operator()(proto::tag::terminal, int &i) const
        ///     {
        ///        ++i;
        ///     }
        /// };
        /// \endcode
        ///
        /// With \c increment_ints, we can do the following:
        ///
        /// \code
        /// literal<int> i = 0, j = 10;
        /// proto::eval( i - j * 3.14, increment_ints() );
        ///
        /// assert( i.get() == 1 && j.get() == 11 );
        /// \endcode
        template<
            typename Context
          , typename DefaultCtx // = default_context
        >
        struct callable_context
        {
            /// A BinaryFunction that accepts an \c Expr and a
            /// \c Context, and either fans out the expression and passes
            /// it to the context, or else hands off the expression to
            /// \c DefaultCtx.
            ///
            /// If \c Context is a PolymorphicFunctionObject such that
            /// it can be invoked with the tag and children of \c Expr,
            /// as <tt>ctx(Expr::proto_tag(), child_c\<0\>(expr), child_c\<1\>(expr)...)</tt>,
            /// then <tt>eval\<Expr, ThisContext\></tt> inherits from
            /// <tt>callable_eval\<Expr, ThisContext\></tt>. Otherwise,
            /// <tt>eval\<Expr, ThisContext\></tt> inherits from
            /// <tt>DefaultCtx::eval\<Expr, Context\></tt>.
            template<typename Expr, typename ThisContext = Context>
            struct eval
              : mpl::if_c<
                    detail::is_expr_handled<Expr, Context>::value
                  , callable_eval<Expr, ThisContext>
                  , typename DefaultCtx::template eval<Expr, Context>
                >::type
            {};
        };
    }

    #include <boost/proto/context/detail/callable_eval.hpp>

}}

#endif
