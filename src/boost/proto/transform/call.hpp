///////////////////////////////////////////////////////////////////////////////
/// \file call.hpp
/// Contains definition of the call<> transform.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_CALL_HPP_EAN_11_02_2007
#define BOOST_PROTO_TRANSFORM_CALL_HPP_EAN_11_02_2007

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4714) // function 'xxx' marked as __forceinline not inlined
#endif

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/ref.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/proto/detail/as_lvalue.hpp>
#include <boost/proto/detail/poly_function.hpp>
#include <boost/proto/transform/detail/pack.hpp>

namespace boost { namespace proto
{
    /// \brief Wrap \c PrimitiveTransform so that <tt>when\<\></tt> knows
    /// it is callable. Requires that the parameter is actually a
    /// PrimitiveTransform.
    ///
    /// This form of <tt>call\<\></tt> is useful for annotating an
    /// arbitrary PrimitiveTransform as callable when using it with
    /// <tt>when\<\></tt>. Consider the following transform, which
    /// is parameterized with another transform.
    ///
    /// \code
    /// template<typename Grammar>
    /// struct Foo
    ///   : when<
    ///         unary_plus<Grammar>
    ///       , Grammar(_child)   // May or may not work.
    ///     >
    /// {};
    /// \endcode
    ///
    /// The problem with the above is that <tt>when\<\></tt> may or
    /// may not recognize \c Grammar as callable, depending on how
    /// \c Grammar is implemented. (See <tt>is_callable\<\></tt> for
    /// a discussion of this issue.) You can guard against
    /// the issue by wrapping \c Grammar in <tt>call\<\></tt>, such
    /// as:
    ///
    /// \code
    /// template<typename Grammar>
    /// struct Foo
    ///   : when<
    ///         unary_plus<Grammar>
    ///       , call<Grammar>(_child)   // OK, this works
    ///     >
    /// {};
    /// \endcode
    ///
    /// The above could also have been written as:
    ///
    /// \code
    /// template<typename Grammar>
    /// struct Foo
    ///   : when<
    ///         unary_plus<Grammar>
    ///       , call<Grammar(_child)>   // OK, this works, too
    ///     >
    /// {};
    /// \endcode
    template<typename PrimitiveTransform>
    struct call
      : PrimitiveTransform
    {};

    /// \brief A specialization that treats function pointer Transforms as
    /// if they were function type Transforms.
    ///
    /// This specialization requires that \c Fun is actually a function type.
    ///
    /// This specialization is required for nested transforms such as
    /// <tt>call\<T0(T1(_))\></tt>. In C++, functions that are used as
    /// parameters to other functions automatically decay to funtion
    /// pointer types. In other words, the type <tt>T0(T1(_))</tt> is
    /// indistinguishable from <tt>T0(T1(*)(_))</tt>. This specialization
    /// is required to handle these nested function pointer type transforms
    /// properly.
    template<typename Fun>
    struct call<Fun *>
      : call<Fun>
    {};

    /// INTERNAL ONLY
    template<typename Fun>
    struct call<detail::msvc_fun_workaround<Fun> >
      : call<Fun>
    {};

    /// \brief Either call the PolymorphicFunctionObject with 0
    /// arguments, or invoke the PrimitiveTransform with 3
    /// arguments.
    template<typename Fun>
    struct call<Fun()> : transform<call<Fun()> >
    {
        /// INTERNAL ONLY
        template<typename Expr, typename State, typename Data, bool B>
        struct impl2
          : transform_impl<Expr, State, Data>
        {
            typedef typename BOOST_PROTO_RESULT_OF<Fun()>::type result_type;

            BOOST_FORCEINLINE
            result_type operator()(
                typename impl2::expr_param
              , typename impl2::state_param
              , typename impl2::data_param
            ) const
            {
                return Fun()();
            }
        };

        /// INTERNAL ONLY
        template<typename Expr, typename State, typename Data>
        struct impl2<Expr, State, Data, true>
          : Fun::template impl<Expr, State, Data>
        {};

        /// Either call the PolymorphicFunctionObject \c Fun with 0 arguments; or
        /// invoke the PrimitiveTransform \c Fun with 3 arguments: the current
        /// expression, state, and data.
        ///
        /// If \c Fun is a nullary PolymorphicFunctionObject, return <tt>Fun()()</tt>.
        /// Otherwise, return <tt>Fun()(e, s, d)</tt>.
        ///
        /// \param e The current expression
        /// \param s The current state
        /// \param d An arbitrary data

        /// If \c Fun is a nullary PolymorphicFunctionObject, \c type is a typedef
        /// for <tt>boost::result_of\<Fun()\>::type</tt>. Otherwise, it is
        /// a typedef for <tt>boost::result_of\<Fun(Expr, State, Data)\>::type</tt>.
        template<typename Expr, typename State, typename Data>
        struct impl
          : impl2<Expr, State, Data, detail::is_transform_<Fun>::value>
        {};
    };

    /// \brief Either call the PolymorphicFunctionObject with 1
    /// argument, or invoke the PrimitiveTransform with 3
    /// arguments.
    template<typename Fun, typename A0>
    struct call<Fun(A0)> : transform<call<Fun(A0)> >
    {
        template<typename Expr, typename State, typename Data, bool B>
        struct impl2
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename detail::poly_function_traits<Fun, Fun(a0)>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename detail::poly_function_traits<Fun, Fun(a0)>::function_type()(
                    detail::as_lvalue(typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d))
                );
            }
        };

        template<typename Expr, typename State, typename Data>
        struct impl2<Expr, State, Data, true>
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename Fun::template impl<a0, State, Data>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename Fun::template impl<a0, State, Data>()(
                    typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d)
                  , s
                  , d
                );
            }
        };
        /// Let \c x be <tt>when\<_, A0\>()(e, s, d)</tt> and \c X
        /// be the type of \c x.
        /// If \c Fun is a unary PolymorphicFunctionObject that accepts \c x,
        /// then \c type is a typedef for <tt>boost::result_of\<Fun(X)\>::type</tt>.
        /// Otherwise, it is a typedef for <tt>boost::result_of\<Fun(X, State, Data)\>::type</tt>.

        /// Either call the PolymorphicFunctionObject with 1 argument:
        /// the result of applying the \c A0 transform; or
        /// invoke the PrimitiveTransform with 3 arguments:
        /// result of applying the \c A0 transform, the state, and the
        /// data.
        ///
        /// Let \c x be <tt>when\<_, A0\>()(e, s, d)</tt>.
        /// If \c Fun is a unary PolymorphicFunctionObject that accepts \c x,
        /// then return <tt>Fun()(x)</tt>. Otherwise, return
        /// <tt>Fun()(x, s, d)</tt>.
        ///
        /// \param e The current expression
        /// \param s The current state
        /// \param d An arbitrary data
        template<typename Expr, typename State, typename Data>
        struct impl
          : impl2<Expr, State, Data, detail::is_transform_<Fun>::value>
        {};
    };

    /// \brief Either call the PolymorphicFunctionObject with 2
    /// arguments, or invoke the PrimitiveTransform with 3
    /// arguments.
    template<typename Fun, typename A0, typename A1>
    struct call<Fun(A0, A1)> : transform<call<Fun(A0, A1)> >
    {
        template<typename Expr, typename State, typename Data, bool B>
        struct impl2
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename when<_, A1>::template impl<Expr, State, Data>::result_type a1;
            typedef typename detail::poly_function_traits<Fun, Fun(a0, a1)>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename detail::poly_function_traits<Fun, Fun(a0, a1)>::function_type()(
                    detail::as_lvalue(typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d))
                  , detail::as_lvalue(typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d))
                );
            }
        };

        template<typename Expr, typename State, typename Data>
        struct impl2<Expr, State, Data, true>
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename when<_, A1>::template impl<Expr, State, Data>::result_type a1;
            typedef typename Fun::template impl<a0, a1, Data>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename Fun::template impl<a0, a1, Data>()(
                    typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d)
                  , typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d)
                  , d
                );
            }
        };

            /// Let \c x be <tt>when\<_, A0\>()(e, s, d)</tt> and \c X
            /// be the type of \c x.
            /// Let \c y be <tt>when\<_, A1\>()(e, s, d)</tt> and \c Y
            /// be the type of \c y.
            /// If \c Fun is a binary PolymorphicFunction object that accepts \c x
            /// and \c y, then \c type is a typedef for
            /// <tt>boost::result_of\<Fun(X, Y)\>::type</tt>. Otherwise, it is
            /// a typedef for <tt>boost::result_of\<Fun(X, Y, Data)\>::type</tt>.

        /// Either call the PolymorphicFunctionObject with 2 arguments:
        /// the result of applying the \c A0 transform, and the
        /// result of applying the \c A1 transform; or invoke the
        /// PrimitiveTransform with 3 arguments: the result of applying
        /// the \c A0 transform, the result of applying the \c A1
        /// transform, and the data.
        ///
        /// Let \c x be <tt>when\<_, A0\>()(e, s, d)</tt>.
        /// Let \c y be <tt>when\<_, A1\>()(e, s, d)</tt>.
        /// If \c Fun is a binary PolymorphicFunction object that accepts \c x
        /// and \c y, return <tt>Fun()(x, y)</tt>. Otherwise, return
        /// <tt>Fun()(x, y, d)</tt>.
        ///
        /// \param e The current expression
        /// \param s The current state
        /// \param d An arbitrary data
        template<typename Expr, typename State, typename Data>
        struct impl
          : impl2<Expr, State, Data, detail::is_transform_<Fun>::value>
        {};
    };

    /// \brief Call the PolymorphicFunctionObject or the
    /// PrimitiveTransform with the current expression, state
    /// and data, transformed according to \c A0, \c A1, and
    /// \c A2, respectively.
    template<typename Fun, typename A0, typename A1, typename A2>
    struct call<Fun(A0, A1, A2)> : transform<call<Fun(A0, A1, A2)> >
    {
        template<typename Expr, typename State, typename Data, bool B>
        struct impl2
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename when<_, A1>::template impl<Expr, State, Data>::result_type a1;
            typedef typename when<_, A2>::template impl<Expr, State, Data>::result_type a2;
            typedef typename detail::poly_function_traits<Fun, Fun(a0, a1, a2)>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename detail::poly_function_traits<Fun, Fun(a0, a1, a2)>::function_type()(
                    detail::as_lvalue(typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d))
                  , detail::as_lvalue(typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d))
                  , detail::as_lvalue(typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d))
                );
            }
        };

        template<typename Expr, typename State, typename Data>
        struct impl2<Expr, State, Data, true>
          : transform_impl<Expr, State, Data>
        {
            typedef typename when<_, A0>::template impl<Expr, State, Data>::result_type a0;
            typedef typename when<_, A1>::template impl<Expr, State, Data>::result_type a1;
            typedef typename when<_, A2>::template impl<Expr, State, Data>::result_type a2;
            typedef typename Fun::template impl<a0, a1, a2>::result_type result_type;
            
            BOOST_FORCEINLINE
            result_type operator ()(
                typename impl2::expr_param   e
              , typename impl2::state_param  s
              , typename impl2::data_param   d
            ) const
            {
                return typename Fun::template impl<a0, a1, a2>()(
                    typename when<_, A0>::template impl<Expr, State, Data>()(e, s, d)
                  , typename when<_, A1>::template impl<Expr, State, Data>()(e, s, d)
                  , typename when<_, A2>::template impl<Expr, State, Data>()(e, s, d)
                );
            }
        };

        /// Let \c x be <tt>when\<_, A0\>()(e, s, d)</tt>.
        /// Let \c y be <tt>when\<_, A1\>()(e, s, d)</tt>.
        /// Let \c z be <tt>when\<_, A2\>()(e, s, d)</tt>.
        /// Return <tt>Fun()(x, y, z)</tt>.
        ///
        /// \param e The current expression
        /// \param s The current state
        /// \param d An arbitrary data

        template<typename Expr, typename State, typename Data>
        struct impl
          : impl2<Expr, State, Data, detail::is_transform_<Fun>::value>
        {};
    };

    #include <boost/proto/transform/detail/call.hpp>

    /// INTERNAL ONLY
    ///
    template<typename Fun>
    struct is_callable<call<Fun> >
      : mpl::true_
    {};

}} // namespace boost::proto

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
