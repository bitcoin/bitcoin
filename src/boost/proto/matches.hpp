///////////////////////////////////////////////////////////////////////////////
/// \file matches.hpp
/// Contains definition of matches\<\> metafunction for determining if
/// a given expression matches a given pattern.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_MATCHES_HPP_EAN_11_03_2006
#define BOOST_PROTO_MATCHES_HPP_EAN_11_03_2006

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/config.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/proto/detail/template_arity.hpp>
#include <boost/utility/enable_if.hpp>
#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#include <boost/type_traits/is_array.hpp>
#endif
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/when.hpp>
#include <boost/proto/transform/impl.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable:4305) // 'specialization' : truncation from 'const int' to 'bool'
#endif

#define BOOST_PROTO_LOGICAL_typename_G  BOOST_PP_ENUM_PARAMS(BOOST_PROTO_MAX_LOGICAL_ARITY, typename G)
#define BOOST_PROTO_LOGICAL_G           BOOST_PP_ENUM_PARAMS(BOOST_PROTO_MAX_LOGICAL_ARITY, G)

namespace boost { namespace proto
{

    namespace detail
    {
        template<typename Expr, typename BasicExpr, typename Grammar>
        struct matches_;

        template<bool B, typename Pred>
        struct and_2;

        template<typename And, typename Expr, typename State, typename Data>
        struct _and_impl;

        template<typename T, typename U>
        struct array_matches
          : mpl::false_
        {};

        template<typename T, std::size_t M>
        struct array_matches<T[M], T *>
          : mpl::true_
        {};

        template<typename T, std::size_t M>
        struct array_matches<T[M], T const *>
          : mpl::true_
        {};

        template<typename T, std::size_t M>
        struct array_matches<T[M], T[proto::N]>
          : mpl::true_
        {};

        template<typename T, typename U
            BOOST_PROTO_TEMPLATE_ARITY_PARAM(long Arity = detail::template_arity<U>::value)
        >
        struct lambda_matches
          : mpl::false_
        {};

        template<typename T>
        struct lambda_matches<T, proto::_ BOOST_PROTO_TEMPLATE_ARITY_PARAM(-1)>
          : mpl::true_
        {};

        template<typename T>
        struct lambda_matches<T, T BOOST_PROTO_TEMPLATE_ARITY_PARAM(-1)>
          : mpl::true_
        {};

        template<typename T, std::size_t M, typename U>
        struct lambda_matches<T[M], U BOOST_PROTO_TEMPLATE_ARITY_PARAM(-1)>
          : array_matches<T[M], U>
        {};

        template<typename T, std::size_t M>
        struct lambda_matches<T[M], _ BOOST_PROTO_TEMPLATE_ARITY_PARAM(-1)>
          : mpl::true_
        {};

        template<typename T, std::size_t M>
        struct lambda_matches<T[M], T[M] BOOST_PROTO_TEMPLATE_ARITY_PARAM(-1)>
          : mpl::true_
        {};

        template<template<typename> class T, typename Expr0, typename Grammar0>
        struct lambda_matches<T<Expr0>, T<Grammar0> BOOST_PROTO_TEMPLATE_ARITY_PARAM(1) >
          : lambda_matches<Expr0, Grammar0>
        {};

        // vararg_matches_impl
        template<typename Args1, typename Back, long From, long To>
        struct vararg_matches_impl;

        // vararg_matches
        template<typename Expr, typename Args1, typename Args2, typename Back, bool Can, bool Zero, typename Void = void>
        struct vararg_matches
          : mpl::false_
        {};

        template<typename Expr, typename Args1, typename Args2, typename Back>
        struct vararg_matches<Expr, Args1, Args2, Back, true, true, typename Back::proto_is_vararg_>
          : matches_<
                Expr
              , proto::basic_expr<ignore, Args1, Args1::arity>
              , proto::basic_expr<ignore, Args2, Args1::arity>
            >
        {};

        template<typename Expr, typename Args1, typename Args2, typename Back>
        struct vararg_matches<Expr, Args1, Args2, Back, true, false, typename Back::proto_is_vararg_>
          : and_2<
                matches_<
                    Expr
                  , proto::basic_expr<ignore, Args1, Args2::arity>
                  , proto::basic_expr<ignore, Args2, Args2::arity>
                >::value
              , vararg_matches_impl<Args1, typename Back::proto_grammar, Args2::arity + 1, Args1::arity>
            >
        {};

        // How terminal_matches<> handles references and cv-qualifiers.
        // The cv and ref matter *only* if the grammar has a top-level ref.
        //
        // Expr       |   Grammar    |  Matches?
        // -------------------------------------
        // T              T             yes
        // T &            T             yes
        // T const &      T             yes
        // T              T &           no
        // T &            T &           yes
        // T const &      T &           no
        // T              T const &     no
        // T &            T const &     no
        // T const &      T const &     yes

        template<typename T, typename U>
        struct is_cv_ref_compatible
          : mpl::true_
        {};

        template<typename T, typename U>
        struct is_cv_ref_compatible<T, U &>
          : mpl::false_
        {};

        template<typename T, typename U>
        struct is_cv_ref_compatible<T &, U &>
          : mpl::bool_<is_const<T>::value == is_const<U>::value>
        {};

    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        // MSVC-7.1 has lots of problems with array types that have been
        // deduced. Partially specializing terminal_matches<> on array types
        // doesn't seem to work.
        template<
            typename T
          , typename U
          , bool B = is_array<BOOST_PROTO_UNCVREF(T)>::value
        >
        struct terminal_array_matches
          : mpl::false_
        {};

        template<typename T, typename U, std::size_t M>
        struct terminal_array_matches<T, U(&)[M], true>
          : is_convertible<T, U(&)[M]>
        {};

        template<typename T, typename U>
        struct terminal_array_matches<T, U(&)[proto::N], true>
          : is_convertible<T, U *>
        {};

        template<typename T, typename U>
        struct terminal_array_matches<T, U *, true>
          : is_convertible<T, U *>
        {};

        // terminal_matches
        template<typename T, typename U>
        struct terminal_matches
          : mpl::or_<
                mpl::and_<
                    is_cv_ref_compatible<T, U>
                  , lambda_matches<
                        BOOST_PROTO_UNCVREF(T)
                      , BOOST_PROTO_UNCVREF(U)
                    >
                >
              , terminal_array_matches<T, U>
            >
        {};
    #else
        // terminal_matches
        template<typename T, typename U>
        struct terminal_matches
          : mpl::and_<
                is_cv_ref_compatible<T, U>
              , lambda_matches<
                    BOOST_PROTO_UNCVREF(T)
                  , BOOST_PROTO_UNCVREF(U)
                >
            >
        {};

        template<typename T, std::size_t M>
        struct terminal_matches<T(&)[M], T(&)[proto::N]>
          : mpl::true_
        {};

        template<typename T, std::size_t M>
        struct terminal_matches<T(&)[M], T *>
          : mpl::true_
        {};

        // Avoid ambiguity errors on MSVC
        #if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1500))
        template<typename T, std::size_t M>
        struct terminal_matches<T const (&)[M], T const[M]>
          : mpl::true_
        {};
        #endif
    #endif

        template<typename T>
        struct terminal_matches<T, T>
          : mpl::true_
        {};

        template<typename T>
        struct terminal_matches<T &, T>
          : mpl::true_
        {};

        template<typename T>
        struct terminal_matches<T const &, T>
          : mpl::true_
        {};

        template<typename T>
        struct terminal_matches<T, proto::_>
          : mpl::true_
        {};

        template<typename T>
        struct terminal_matches<T, exact<T> >
          : mpl::true_
        {};

        template<typename T, typename U>
        struct terminal_matches<T, proto::convertible_to<U> >
          : is_convertible<T, U>
        {};

        // matches_
        template<typename Expr, typename BasicExpr, typename Grammar>
        struct matches_
          : mpl::false_
        {};

        template<typename Expr, typename BasicExpr>
        struct matches_< Expr, BasicExpr, proto::_ >
          : mpl::true_
        {};

        template<typename Expr, typename Tag, typename Args1, long N1, typename Args2, long N2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, N1>, proto::basic_expr<Tag, Args2, N2> >
          : vararg_matches< Expr, Args1, Args2, typename Args2::back_, (N1+2 > N2), (N2 > N1) >
        {};

        template<typename Expr, typename Tag, typename Args1, long N1, typename Args2, long N2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, N1>, proto::basic_expr<proto::_, Args2, N2> >
          : vararg_matches< Expr, Args1, Args2, typename Args2::back_, (N1+2 > N2), (N2 > N1) >
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, 0>, proto::basic_expr<Tag, Args2, 0> >
          : terminal_matches<typename Args1::child0, typename Args2::child0>
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2, long N2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, 0>, proto::basic_expr<proto::_, Args2, N2> >
          : mpl::false_
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, 0>, proto::basic_expr<proto::_, Args2, 0> >
          : terminal_matches<typename Args1::child0, typename Args2::child0>
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, 1>, proto::basic_expr<Tag, Args2, 1> >
          : matches_<
                typename detail::expr_traits<typename Args1::child0>::value_type::proto_derived_expr
              , typename detail::expr_traits<typename Args1::child0>::value_type::proto_grammar
              , typename Args2::child0::proto_grammar
            >
        {};

        template<typename Expr, typename Tag, typename Args1, typename Args2>
        struct matches_< Expr, proto::basic_expr<Tag, Args1, 1>, proto::basic_expr<proto::_, Args2, 1> >
          : matches_<
                typename detail::expr_traits<typename Args1::child0>::value_type::proto_derived_expr
              , typename detail::expr_traits<typename Args1::child0>::value_type::proto_grammar
              , typename Args2::child0::proto_grammar
            >
        {};

        #include <boost/proto/detail/and_n.hpp>
        #include <boost/proto/detail/or_n.hpp>
        #include <boost/proto/detail/matches_.hpp>
        #include <boost/proto/detail/vararg_matches_impl.hpp>
        #include <boost/proto/detail/lambda_matches.hpp>

        // handle proto::if_
        template<typename Expr, typename Tag, typename Args, long Arity, typename If, typename Then, typename Else>
        struct matches_<Expr, proto::basic_expr<Tag, Args, Arity>, proto::if_<If, Then, Else> >
          : mpl::eval_if_c<
                static_cast<bool>(
                    remove_reference<
                        typename when<_, If>::template impl<Expr, int, int>::result_type
                    >::type::value
                )
              , matches_<Expr, proto::basic_expr<Tag, Args, Arity>, typename Then::proto_grammar>
              , matches_<Expr, proto::basic_expr<Tag, Args, Arity>, typename Else::proto_grammar>
            >::type
        {
            typedef
                typename mpl::if_c<
                    static_cast<bool>(
                        remove_reference<
                            typename when<_, If>::template impl<Expr, int, int>::result_type
                        >::type::value
                    )
                  , Then
                  , Else
                >::type
            which;
        };

        // handle degenerate cases of proto::or_
        template<typename Expr, typename BasicExpr>
        struct matches_<Expr, BasicExpr, or_<> >
          : mpl::false_
        {
            typedef not_<_> which;
        };

        template<typename Expr, typename BasicExpr, typename G0>
        struct matches_<Expr, BasicExpr, or_<G0> >
          : matches_<Expr, BasicExpr, typename G0::proto_grammar>
        {
            typedef G0 which;
        };

        // handle degenerate cases of proto::and_
        template<typename Expr, typename BasicExpr>
        struct matches_<Expr, BasicExpr, and_<> >
          : mpl::true_
        {};

        template<typename Expr, typename BasicExpr, typename G0>
        struct matches_<Expr, BasicExpr, and_<G0> >
          : matches_<Expr, BasicExpr, typename G0::proto_grammar>
        {};

        // handle proto::not_
        template<typename Expr, typename BasicExpr, typename Grammar>
        struct matches_<Expr, BasicExpr, not_<Grammar> >
          : mpl::not_<matches_<Expr, BasicExpr, typename Grammar::proto_grammar> >
        {};

        // handle proto::switch_
        template<typename Expr, typename Tag, typename Args, long Arity, typename Cases, typename Transform>
        struct matches_<Expr, proto::basic_expr<Tag, Args, Arity>, switch_<Cases, Transform> >
          : matches_<
                Expr
              , proto::basic_expr<Tag, Args, Arity>
              , typename Cases::template case_<
                    typename when<_,Transform>::template impl<Expr,int,int>::result_type
                >::proto_grammar
            >
        {
            typedef
                typename Cases::template case_<
                    typename when<_, Transform>::template impl<Expr, int, int>::result_type
                >
            which;
        };

        // handle proto::switch_ with the default Transform for specially for better compile times
        template<typename Expr, typename Tag, typename Args, long Arity, typename Cases>
        struct matches_<Expr, proto::basic_expr<Tag, Args, Arity>, switch_<Cases> >
          : matches_<
                Expr
              , proto::basic_expr<Tag, Args, Arity>
              , typename Cases::template case_<Tag>::proto_grammar
            >
        {
            typedef typename Cases::template case_<Tag> which;
        };
    }

    /// \brief A Boolean metafunction that evaluates whether a given
    /// expression type matches a grammar.
    ///
    /// <tt>matches\<Expr,Grammar\></tt> inherits (indirectly) from
    /// \c mpl::true_ if <tt>Expr::proto_grammar</tt> matches
    /// <tt>Grammar::proto_grammar</tt>, and from \c mpl::false_
    /// otherwise.
    ///
    /// Non-terminal expressions are matched against a grammar
    /// according to the following rules:
    ///
    /// \li The wildcard pattern, \c _, matches any expression.
    /// \li An expression <tt>expr\<AT, listN\<A0,A1,...An\> \></tt>
    ///     matches a grammar <tt>expr\<BT, listN\<B0,B1,...Bn\> \></tt>
    ///     if \c BT is \c _ or \c AT, and if \c Ax matches \c Bx for
    ///     each \c x in <tt>[0,n)</tt>.
    /// \li An expression <tt>expr\<AT, listN\<A0,...An,U0,...Um\> \></tt>
    ///     matches a grammar <tt>expr\<BT, listM\<B0,...Bn,vararg\<V\> \> \></tt>
    ///     if \c BT is \c _ or \c AT, and if \c Ax matches \c Bx
    ///     for each \c x in <tt>[0,n)</tt> and if \c Ux matches \c V
    ///     for each \c x in <tt>[0,m)</tt>.
    /// \li An expression \c E matches <tt>or_\<B0,B1,...Bn\></tt> if \c E
    ///     matches some \c Bx for \c x in <tt>[0,n)</tt>.
    /// \li An expression \c E matches <tt>and_\<B0,B1,...Bn\></tt> if \c E
    ///     matches all \c Bx for \c x in <tt>[0,n)</tt>.
    /// \li An expression \c E matches <tt>if_\<T,U,V\></tt> if
    ///     <tt>boost::result_of\<when\<_,T\>(E,int,int)\>::type::value</tt>
    ///     is \c true and \c E matches \c U; or, if
    ///     <tt>boost::result_of\<when\<_,T\>(E,int,int)\>::type::value</tt>
    ///     is \c false and \c E matches \c V. (Note: \c U defaults to \c _
    ///     and \c V defaults to \c not_\<_\>.)
    /// \li An expression \c E matches <tt>not_\<T\></tt> if \c E does
    ///     not match \c T.
    /// \li An expression \c E matches <tt>switch_\<C,T\></tt> if
    ///     \c E matches <tt>C::case_\<boost::result_of\<T(E)\>::type\></tt>.
    ///     (Note: T defaults to <tt>tag_of\<_\>()</tt>.)
    ///
    /// A terminal expression <tt>expr\<AT,term\<A\> \></tt> matches
    /// a grammar <tt>expr\<BT,term\<B\> \></tt> if \c BT is \c AT or
    /// \c proto::_ and if one of the following is true:
    ///
    /// \li \c B is the wildcard pattern, \c _
    /// \li \c A is \c B
    /// \li \c A is <tt>B &</tt>
    /// \li \c A is <tt>B const &</tt>
    /// \li \c B is <tt>exact\<A\></tt>
    /// \li \c B is <tt>convertible_to\<X\></tt> and
    ///     <tt>is_convertible\<A,X\>::value</tt> is \c true.
    /// \li \c A is <tt>X[M]</tt> or <tt>X(&)[M]</tt> and
    ///     \c B is <tt>X[proto::N]</tt>.
    /// \li \c A is <tt>X(&)[M]</tt> and \c B is <tt>X(&)[proto::N]</tt>.
    /// \li \c A is <tt>X[M]</tt> or <tt>X(&)[M]</tt> and
    ///     \c B is <tt>X*</tt>.
    /// \li \c B lambda-matches \c A (see below).
    ///
    /// A type \c B lambda-matches \c A if one of the following is true:
    ///
    /// \li \c B is \c A
    /// \li \c B is the wildcard pattern, \c _
    /// \li \c B is <tt>T\<B0,B1,...Bn\></tt> and \c A is
    ///     <tt>T\<A0,A1,...An\></tt> and for each \c x in
    ///     <tt>[0,n)</tt>, \c Ax and \c Bx are types
    ///     such that \c Ax lambda-matches \c Bx
    template<typename Expr, typename Grammar>
    struct matches
      : detail::matches_<
            typename Expr::proto_derived_expr
          , typename Expr::proto_grammar
          , typename Grammar::proto_grammar
        >
    {};

    /// INTERNAL ONLY
    ///
    template<typename Expr, typename Grammar>
    struct matches<Expr &, Grammar>
      : detail::matches_<
            typename Expr::proto_derived_expr
          , typename Expr::proto_grammar
          , typename Grammar::proto_grammar
        >
    {};

    /// \brief A wildcard grammar element that matches any expression,
    /// and a transform that returns the current expression unchanged.
    ///
    /// The wildcard type, \c _, is a grammar element such that
    /// <tt>matches\<E,_\>::value</tt> is \c true for any expression
    /// type \c E.
    ///
    /// The wildcard can also be used as a stand-in for a template
    /// argument when matching terminals. For instance, the following
    /// is a grammar that will match any <tt>std::complex\<\></tt>
    /// terminal:
    ///
    /// \code
    /// BOOST_MPL_ASSERT((
    ///     matches<
    ///         terminal<std::complex<double> >::type
    ///       , terminal<std::complex< _ > >
    ///     >
    /// ));
    /// \endcode
    ///
    /// When used as a transform, \c _ returns the current expression
    /// unchanged. For instance, in the following, \c _ is used with
    /// the \c fold\<\> transform to fold the children of a node:
    ///
    /// \code
    /// struct CountChildren
    ///   : or_<
    ///         // Terminals have no children
    ///         when<terminal<_>, mpl::int_<0>()>
    ///         // Use fold<> to count the children of non-terminals
    ///       , otherwise<
    ///             fold<
    ///                 _ // <-- fold the current expression
    ///               , mpl::int_<0>()
    ///               , mpl::plus<_state, mpl::int_<1> >()
    ///             >
    ///         >
    ///     >
    /// {};
    /// \endcode
    struct _ : transform<_>
    {
        typedef _ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef Expr result_type;

            /// \param expr An expression
            /// \return \c e
            BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, typename impl::expr_param)
            operator()(
                typename impl::expr_param e
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                return e;
            }
        };
    };

    namespace detail
    {
        template<typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<>, Expr, State, Data>
          : proto::_::impl<Expr, State, Data>
        {};

        template<typename G0, typename Expr, typename State, typename Data>
        struct _and_impl<proto::and_<G0>, Expr, State, Data>
          : proto::when<proto::_, G0>::template impl<Expr, State, Data>
        {};
    }

    /// \brief Inverts the set of expressions matched by a grammar. When
    /// used as a transform, \c not_\<\> returns the current expression
    /// unchanged.
    ///
    /// If an expression type \c E does not match a grammar \c G, then
    /// \c E \e does match <tt>not_\<G\></tt>. For example,
    /// <tt>not_\<terminal\<_\> \></tt> will match any non-terminal.
    template<typename Grammar>
    struct not_ : transform<not_<Grammar> >
    {
        typedef not_ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef Expr result_type;

            /// \param e An expression
            /// \pre <tt>matches\<Expr,not_\>::value</tt> is \c true.
            /// \return \c e
            BOOST_PROTO_RETURN_TYPE_STRICT_LOOSE(result_type, typename impl::expr_param)
            operator()(
                typename impl::expr_param e
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                return e;
            }
        };
    };

    /// \brief Used to select one grammar or another based on the result
    /// of a compile-time Boolean. When used as a transform, \c if_\<\>
    /// selects between two transforms based on a compile-time Boolean.
    ///
    /// When <tt>if_\<If,Then,Else\></tt> is used as a grammar, \c If
    /// must be a Proto transform and \c Then and \c Else must be grammars.
    /// An expression type \c E matches <tt>if_\<If,Then,Else\></tt> if
    /// <tt>boost::result_of\<when\<_,If\>(E,int,int)\>::type::value</tt>
    /// is \c true and \c E matches \c U; or, if
    /// <tt>boost::result_of\<when\<_,If\>(E,int,int)\>::type::value</tt>
    /// is \c false and \c E matches \c V.
    ///
    /// The template parameter \c Then defaults to \c _
    /// and \c Else defaults to \c not\<_\>, so an expression type \c E
    /// will match <tt>if_\<If\></tt> if and only if
    /// <tt>boost::result_of\<when\<_,If\>(E,int,int)\>::type::value</tt>
    /// is \c true.
    ///
    /// \code
    /// // A grammar that only matches integral terminals,
    /// // using is_integral<> from Boost.Type_traits.
    /// struct IsIntegral
    ///   : and_<
    ///         terminal<_>
    ///       , if_< is_integral<_value>() >
    ///     >
    /// {};
    /// \endcode
    ///
    /// When <tt>if_\<If,Then,Else\></tt> is used as a transform, \c If,
    /// \c Then and \c Else must be Proto transforms. When applying
    /// the transform to an expression \c E, state \c S and data \c V,
    /// if <tt>boost::result_of\<when\<_,If\>(E,S,V)\>::type::value</tt>
    /// is \c true then the \c Then transform is applied; otherwise
    /// the \c Else transform is applied.
    ///
    /// \code
    /// // Match a terminal. If the terminal is integral, return
    /// // mpl::true_; otherwise, return mpl::false_.
    /// struct IsIntegral2
    ///   : when<
    ///         terminal<_>
    ///       , if_<
    ///             is_integral<_value>()
    ///           , mpl::true_()
    ///           , mpl::false_()
    ///         >
    ///     >
    /// {};
    /// \endcode
    template<
        typename If
      , typename Then   // = _
      , typename Else   // = not_<_>
    >
    struct if_ : transform<if_<If, Then, Else> >
    {
        typedef if_ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            typedef
                typename when<_, If>::template impl<Expr, State, Data>::result_type
            condition;

            typedef
                typename mpl::if_c<
                    static_cast<bool>(remove_reference<condition>::type::value)
                  , when<_, Then>
                  , when<_, Else>
                >::type
            which;

            typedef typename which::template impl<Expr, State, Data>::result_type result_type;

            /// \param e An expression
            /// \param s The current state
            /// \param d A data of arbitrary type
            /// \return <tt>which::impl<Expr, State, Data>()(e, s, d)</tt>
            result_type operator ()(
                typename impl::expr_param e
              , typename impl::state_param s
              , typename impl::data_param d
            ) const
            {
                return typename which::template impl<Expr, State, Data>()(e, s, d);
            }
        };
    };

    /// \brief For matching one of a set of alternate grammars. Alternates
    /// tried in order to avoid ambiguity. When used as a transform, \c or_\<\>
    /// applies the transform associated with the first grammar that matches
    /// the expression.
    ///
    /// An expression type \c E matches <tt>or_\<B0,B1,...Bn\></tt> if \c E
    /// matches any \c Bx for \c x in <tt>[0,n)</tt>.
    ///
    /// When applying <tt>or_\<B0,B1,...Bn\></tt> as a transform with an
    /// expression \c e of type \c E, state \c s and data \c d, it is
    /// equivalent to <tt>Bx()(e, s, d)</tt>, where \c x is the lowest
    /// number such that <tt>matches\<E,Bx\>::value</tt> is \c true.
    template<BOOST_PROTO_LOGICAL_typename_G>
    struct or_ : transform<or_<BOOST_PROTO_LOGICAL_G> >
    {
        typedef or_ proto_grammar;

        /// \param e An expression
        /// \param s The current state
        /// \param d A data of arbitrary type
        /// \pre <tt>matches\<Expr,or_\>::value</tt> is \c true.
        /// \return <tt>which()(e, s, d)</tt>, where <tt>which</tt> is the
        /// sub-grammar that matched <tt>Expr</tt>.

        template<typename Expr, typename State, typename Data>
        struct impl
          : detail::matches_<
                typename Expr::proto_derived_expr
              , typename Expr::proto_grammar
              , or_
            >::which::template impl<Expr, State, Data>
        {};

        template<typename Expr, typename State, typename Data>
        struct impl<Expr &, State, Data>
          : detail::matches_<
                typename Expr::proto_derived_expr
              , typename Expr::proto_grammar
              , or_
            >::which::template impl<Expr &, State, Data>
        {};
    };

    /// \brief For matching all of a set of grammars. When used as a
    /// transform, \c and_\<\> applies the transforms associated with
    /// the each grammar in the set, and returns the result of the last.
    ///
    /// An expression type \c E matches <tt>and_\<B0,B1,...Bn\></tt> if \c E
    /// matches all \c Bx for \c x in <tt>[0,n)</tt>.
    ///
    /// When applying <tt>and_\<B0,B1,...Bn\></tt> as a transform with an
    /// expression \c e, state \c s and data \c d, it is
    /// equivalent to <tt>(B0()(e, s, d),B1()(e, s, d),...Bn()(e, s, d))</tt>.
    template<BOOST_PROTO_LOGICAL_typename_G>
    struct and_ : transform<and_<BOOST_PROTO_LOGICAL_G> >
    {
        typedef and_ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl
          : detail::_and_impl<and_, Expr, State, Data>
        {};
    };

    /// \brief For matching one of a set of alternate grammars, which
    /// are looked up based on some property of an expression. The
    /// property on which to dispatch is specified by the \c Transform
    /// template parameter, which defaults to <tt>tag_of\<_\>()</tt>.
    /// That is, when the \c Trannsform is not specified, the alternate
    /// grammar is looked up using the tag type of the current expression.
    ///
    /// When used as a transform, \c switch_\<\> applies the transform
    /// associated with the grammar that matches the expression.
    ///
    /// \note \c switch_\<\> is functionally identical to \c or_\<\> but
    /// is often more efficient. It does a fast, O(1) lookup using the
    /// result of the specified transform to find a sub-grammar that may
    /// potentially match the expression.
    ///
    /// An expression type \c E matches <tt>switch_\<C,T\></tt> if \c E
    /// matches <tt>C::case_\<boost::result_of\<T(E)\>::type\></tt>.
    ///
    /// When applying <tt>switch_\<C,T\></tt> as a transform with an
    /// expression \c e of type \c E, state \c s of type \S and data
    /// \c d of type \c D, it is equivalent to
    /// <tt>C::case_\<boost::result_of\<T(E,S,D)\>::type\>()(e, s, d)</tt>.
    template<typename Cases, typename Transform>
    struct switch_ : transform<switch_<Cases, Transform> >
    {
        typedef switch_ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl
          : Cases::template case_<
                typename when<_, Transform>::template impl<Expr, State, Data>::result_type
            >::template impl<Expr, State, Data>
        {};
    };

    /// INTERNAL ONLY (This is merely a compile-time optimization for the common case)
    ///
    template<typename Cases>
    struct switch_<Cases> : transform<switch_<Cases> >
    {
        typedef switch_ proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl
          : Cases::template case_<typename Expr::proto_tag>::template impl<Expr, State, Data>
        {};

        template<typename Expr, typename State, typename Data>
        struct impl<Expr &, State, Data>
          : Cases::template case_<typename Expr::proto_tag>::template impl<Expr &, State, Data>
        {};
    };

    /// \brief For forcing exact matches of terminal types.
    ///
    /// By default, matching terminals ignores references and
    /// cv-qualifiers. For instance, a terminal expression of
    /// type <tt>terminal\<int const &\>::type</tt> will match
    /// the grammar <tt>terminal\<int\></tt>. If that is not
    /// desired, you can force an exact match with
    /// <tt>terminal\<exact\<int\> \></tt>. This will only
    /// match integer terminals where the terminal is held by
    /// value.
    template<typename T>
    struct exact
    {};

    /// \brief For matching terminals that are convertible to
    /// a type.
    ///
    /// Use \c convertible_to\<\> to match a terminal that is
    /// convertible to some type. For example, the grammar
    /// <tt>terminal\<convertible_to\<int\> \></tt> will match
    /// any terminal whose argument is convertible to an integer.
    ///
    /// \note The trait \c is_convertible\<\> from Boost.Type_traits
    /// is used to determinal convertibility.
    template<typename T>
    struct convertible_to
    {};

    /// \brief For matching a Grammar to a variable number of
    /// sub-expressions.
    ///
    /// An expression type <tt>expr\<AT, listN\<A0,...An,U0,...Um\> \></tt>
    /// matches a grammar <tt>expr\<BT, listM\<B0,...Bn,vararg\<V\> \> \></tt>
    /// if \c BT is \c _ or \c AT, and if \c Ax matches \c Bx
    /// for each \c x in <tt>[0,n)</tt> and if \c Ux matches \c V
    /// for each \c x in <tt>[0,m)</tt>.
    ///
    /// For example:
    ///
    /// \code
    /// // Match any function call expression, irregardless
    /// // of the number of function arguments:
    /// struct Function
    ///   : function< vararg<_> >
    /// {};
    /// \endcode
    ///
    /// When used as a transform, <tt>vararg\<G\></tt> applies
    /// <tt>G</tt>'s transform.
    template<typename Grammar>
    struct vararg
      : Grammar
    {
        /// INTERNAL ONLY
        typedef void proto_is_vararg_;
    };

    /// INTERNAL ONLY
    ///
    template<BOOST_PROTO_LOGICAL_typename_G>
    struct is_callable<or_<BOOST_PROTO_LOGICAL_G> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<BOOST_PROTO_LOGICAL_typename_G>
    struct is_callable<and_<BOOST_PROTO_LOGICAL_G> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Grammar>
    struct is_callable<not_<Grammar> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename If, typename Then, typename Else>
    struct is_callable<if_<If, Then, Else> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Grammar>
    struct is_callable<vararg<Grammar> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Cases, typename Transform>
    struct is_callable<switch_<Cases, Transform> >
      : mpl::true_
    {};

}}

#undef BOOST_PROTO_LOGICAL_typename_G
#undef BOOST_PROTO_LOGICAL_G

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
