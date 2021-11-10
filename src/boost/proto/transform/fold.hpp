///////////////////////////////////////////////////////////////////////////////
/// \file fold.hpp
/// Contains definition of the fold<> and reverse_fold<> transforms.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_FOLD_HPP_EAN_11_04_2007
#define BOOST_PROTO_TRANSFORM_FOLD_HPP_EAN_11_04_2007

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/reverse_fold.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/proto/transform/when.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Transform, typename Data>
        struct as_callable
        {
            as_callable(Data d)
              : d_(d)
            {}

            template<typename Sig>
            struct result;

            template<typename This, typename State, typename Expr>
            struct result<This(State, Expr)>
            {
                typedef
                    typename when<_, Transform>::template impl<Expr, State, Data>::result_type
                type;
            };

            template<typename State, typename Expr>
            typename when<_, Transform>::template impl<Expr &, State const &, Data>::result_type
            operator ()(State const &s, Expr &e) const
            {
                return typename when<_, Transform>::template impl<Expr &, State const &, Data>()(e, s, this->d_);
            }

        private:
            Data d_;
        };

        template<
            typename State0
          , typename Fun
          , typename Expr
          , typename State
          , typename Data
          , long Arity = arity_of<Expr>::value
        >
        struct fold_impl
        {};

        template<
            typename State0
          , typename Fun
          , typename Expr
          , typename State
          , typename Data
          , long Arity = arity_of<Expr>::value
        >
        struct reverse_fold_impl
        {};

        #include <boost/proto/transform/detail/fold_impl.hpp>

    } // namespace detail

    /// \brief A PrimitiveTransform that invokes the <tt>fusion::fold\<\></tt>
    /// algorithm to accumulate
    template<typename Sequence, typename State0, typename Fun>
    struct fold : transform<fold<Sequence, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            /// \brief A Fusion sequence.
            typedef
                typename remove_reference<
                    typename when<_, Sequence>::template impl<Expr, State, Data>::result_type
                >::type
            sequence;

            /// \brief An initial state for the fold.
            typedef
                typename remove_reference<
                    typename when<_, State0>::template impl<Expr, State, Data>::result_type
                >::type
            state0;

            /// \brief <tt>fun(d)(e,s) == when\<_,Fun\>()(e,s,d)</tt>
            typedef
                detail::as_callable<Fun, Data>
            fun;

            typedef
                typename fusion::result_of::fold<
                    sequence
                  , state0
                  , fun
                >::type
            result_type;

            /// Let \c seq be <tt>when\<_, Sequence\>()(e, s, d)</tt>, let
            /// \c state0 be <tt>when\<_, State0\>()(e, s, d)</tt>, and
            /// let \c fun(d) be an object such that <tt>fun(d)(e, s)</tt>
            /// is equivalent to <tt>when\<_, Fun\>()(e, s, d)</tt>. Then, this
            /// function returns <tt>fusion::fold(seq, state0, fun(d))</tt>.
            ///
            /// \param e The current expression
            /// \param s The current state
            /// \param d An arbitrary data
            result_type operator ()(
                typename impl::expr_param   e
              , typename impl::state_param  s
              , typename impl::data_param   d
            ) const
            {
                typename when<_, Sequence>::template impl<Expr, State, Data> seq;
                detail::as_callable<Fun, Data> f(d);
                return fusion::fold(
                    seq(e, s, d)
                  , typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d)
                  , f
                );
            }
        };
    };

    /// \brief A PrimitiveTransform that is the same as the
    /// <tt>fold\<\></tt> transform, except that it folds
    /// back-to-front instead of front-to-back.
    template<typename Sequence, typename State0, typename Fun>
    struct reverse_fold  : transform<reverse_fold<Sequence, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : transform_impl<Expr, State, Data>
        {
            /// \brief A Fusion sequence.
            typedef
                typename remove_reference<
                    typename when<_, Sequence>::template impl<Expr, State, Data>::result_type
                >::type
            sequence;

            /// \brief An initial state for the fold.
            typedef
                typename remove_reference<
                    typename when<_, State0>::template impl<Expr, State, Data>::result_type
                >::type
            state0;

            /// \brief <tt>fun(d)(e,s) == when\<_,Fun\>()(e,s,d)</tt>
            typedef
                detail::as_callable<Fun, Data>
            fun;

            typedef
                typename fusion::result_of::reverse_fold<
                    sequence
                  , state0
                  , fun
                >::type
            result_type;

            /// Let \c seq be <tt>when\<_, Sequence\>()(e, s, d)</tt>, let
            /// \c state0 be <tt>when\<_, State0\>()(e, s, d)</tt>, and
            /// let \c fun(d) be an object such that <tt>fun(d)(e, s)</tt>
            /// is equivalent to <tt>when\<_, Fun\>()(e, s, d)</tt>. Then, this
            /// function returns <tt>fusion::fold(seq, state0, fun(d))</tt>.
            ///
            /// \param e The current expression
            /// \param s The current state
            /// \param d An arbitrary data
            result_type operator ()(
                typename impl::expr_param   e
              , typename impl::state_param  s
              , typename impl::data_param   d
            ) const
            {
                typename when<_, Sequence>::template impl<Expr, State, Data> seq;
                detail::as_callable<Fun, Data> f(d);
                return fusion::reverse_fold(
                    seq(e, s, d)
                  , typename when<_, State0>::template impl<Expr, State, Data>()(e, s, d)
                  , f
                );
            }
        };
    };

    // This specialization is only for improved compile-time performance
    // in the commom case when the Sequence transform is \c proto::_.
    //
    /// INTERNAL ONLY
    ///
    template<typename State0, typename Fun>
    struct fold<_, State0, Fun> : transform<fold<_, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : detail::fold_impl<State0, Fun, Expr, State, Data>
        {};
    };

    // This specialization is only for improved compile-time performance
    // in the commom case when the Sequence transform is \c proto::_.
    //
    /// INTERNAL ONLY
    ///
    template<typename State0, typename Fun>
    struct reverse_fold<_, State0, Fun> : transform<reverse_fold<_, State0, Fun> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl
          : detail::reverse_fold_impl<State0, Fun, Expr, State, Data>
        {};
    };

    /// INTERNAL ONLY
    ///
    template<typename Sequence, typename State, typename Fun>
    struct is_callable<fold<Sequence, State, Fun> >
      : mpl::true_
    {};

    /// INTERNAL ONLY
    ///
    template<typename Sequence, typename State, typename Fun>
    struct is_callable<reverse_fold<Sequence, State, Fun> >
      : mpl::true_
    {};

}}

#endif
