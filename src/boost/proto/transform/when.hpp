///////////////////////////////////////////////////////////////////////////////
/// \file when.hpp
/// Definition of when transform.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_WHEN_HPP_EAN_10_29_2007
#define BOOST_PROTO_TRANSFORM_WHEN_HPP_EAN_10_29_2007

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/call.hpp>
#include <boost/proto/transform/make.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/proto/transform/env.hpp>

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable : 4714) // function 'xxx' marked as __forceinline not inlined
#endif

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Grammar, typename R, typename Fun>
        struct when_impl
          : transform<when<Grammar, Fun> >
        {
            typedef Grammar first;
            typedef Fun second;
            typedef typename Grammar::proto_grammar proto_grammar;

            // Note: do not evaluate is_callable<R> in this scope.
            // R may be an incomplete type at this point.

            template<typename Expr, typename State, typename Data>
            struct impl : transform_impl<Expr, State, Data>
            {
                // OK to evaluate is_callable<R> here. R should be compete by now.
                typedef
                    typename mpl::if_c<
                        is_callable<R>::value
                      , proto::call<Fun> // "R" is a function to call
                      , proto::make<Fun> // "R" is an object to construct
                    >::type
                which;

                typedef typename which::template impl<Expr, State, Data>::result_type result_type;

                /// Evaluate <tt>R(A0,A1,...)</tt> as a transform either with
                /// <tt>call\<\></tt> or with <tt>make\<\></tt> depending on
                /// whether <tt>is_callable\<R\>::value</tt> is \c true or
                /// \c false.
                ///
                /// \param e The current expression
                /// \param s The current state
                /// \param d An arbitrary data
                /// \pre <tt>matches\<Expr, Grammar\>::value</tt> is \c true
                /// \return <tt>which()(e, s, d)</tt>
                BOOST_FORCEINLINE
                result_type operator ()(
                    typename impl::expr_param   e
                  , typename impl::state_param  s
                  , typename impl::data_param   d
                ) const
                {
                    return typename which::template impl<Expr, State, Data>()(e, s, d);
                }
            };
        };
    }

    /// \brief A grammar element and a PrimitiveTransform that associates
    /// a transform with the grammar.
    ///
    /// Use <tt>when\<\></tt> to override a grammar's default transform
    /// with a custom transform. It is for used when composing larger
    /// transforms by associating smaller transforms with individual
    /// rules in your grammar, as in the following transform which
    /// counts the number of terminals in an expression.
    ///
    /// \code
    /// // Count the terminals in an expression tree.
    /// // Must be invoked with initial state == mpl::int_<0>().
    /// struct CountLeaves
    ///   : or_<
    ///         when<terminal<_>, mpl::next<_state>()>
    ///       , otherwise<fold<_, _state, CountLeaves> >
    ///     >
    /// {};
    /// \endcode
    ///
    /// In <tt>when\<G, T\></tt>, when \c T is a class type it is a
    /// PrimitiveTransform and the following equivalencies hold:
    ///
    /// <tt>boost::result_of\<when\<G,T\>(E,S,V)\>::type</tt> is the same as
    /// <tt>boost::result_of\<T(E,S,V)\>::type</tt>.
    ///
    /// <tt>when\<G,T\>()(e,s,d)</tt> is the same as
    /// <tt>T()(e,s,d)</tt>.
    template<typename Grammar, typename PrimitiveTransform /*= Grammar*/>
    struct when
      : PrimitiveTransform
    {
        typedef Grammar first;
        typedef PrimitiveTransform second;
        typedef typename Grammar::proto_grammar proto_grammar;
    };

    /// \brief A specialization that treats function pointer Transforms as
    /// if they were function type Transforms.
    ///
    /// This specialization requires that \c Fun is actually a function type.
    ///
    /// This specialization is required for nested transforms such as
    /// <tt>when\<G, T0(T1(_))\></tt>. In C++, functions that are used as
    /// parameters to other functions automatically decay to funtion
    /// pointer types. In other words, the type <tt>T0(T1(_))</tt> is
    /// indistinguishable from <tt>T0(T1(*)(_))</tt>. This specialization
    /// is required to handle these nested function pointer type transforms
    /// properly.
    template<typename Grammar, typename Fun>
    struct when<Grammar, Fun *>
      : when<Grammar, Fun>
    {};

    /// \brief Syntactic sugar for <tt>when\<_, Fun\></tt>, for use
    /// in grammars to handle all the cases not yet handled.
    ///
    /// Use <tt>otherwise\<T\></tt> in your grammars as a synonym for
    /// <tt>when\<_, T\></tt> as in the following transform which
    /// counts the number of terminals in an expression.
    ///
    /// \code
    /// // Count the terminals in an expression tree.
    /// // Must be invoked with initial state == mpl::int_<0>().
    /// struct CountLeaves
    ///   : or_<
    ///         when<terminal<_>, mpl::next<_state>()>
    ///       , otherwise<fold<_, _state, CountLeaves> >
    ///     >
    /// {};
    /// \endcode
    template<typename Fun>
    struct otherwise
      : when<_, Fun>
    {};

    namespace envns_
    {
        // Define the transforms global
        BOOST_PROTO_DEFINE_ENV_VAR(transforms_type, transforms);
    }

    using envns_::transforms;

    /// \brief This specialization uses the Data parameter as a collection
    /// of transforms that can be indexed by the specified rule.
    ///
    /// Use <tt>when\<T, external_transform\></tt> in your code when you would like
    /// to define a grammar once and use it to evaluate expressions with
    /// many different sets of transforms. The transforms are found by
    /// using the Data parameter as a map from rules to transforms.
    ///
    /// See \c action_map for an example.
    template<typename Grammar>
    struct when<Grammar, external_transform>
      : proto::transform<when<Grammar, external_transform> >
    {
        typedef Grammar first;
        typedef external_transform second;
        typedef typename Grammar::proto_grammar proto_grammar;

        template<typename Expr, typename State, typename Data>
        struct impl
          : remove_reference<
                typename mpl::eval_if_c<
                    proto::result_of::has_env_var<Data, transforms_type>::value
                  , proto::result_of::env_var<Data, transforms_type>
                  , proto::result_of::env_var<Data, data_type>
                >::type
            >::type::template when<Grammar>::template impl<Expr, State, Data>
        {};
    };

    /// \brief For defining a map of Rule/Transform pairs for use with
    /// <tt>when\<T, external_transform\></tt> to make transforms external to the grammar
    ///
    /// The following code defines a grammar with a couple of external transforms.
    /// It also defines an action_map that maps from rules to transforms. It then
    /// passes that transforms map at the Data parameter to the grammar. In this way,
    /// the behavior of the grammar can be modified post-hoc by passing a different
    /// action_map.
    ///
    /// \code
    /// struct int_terminal
    ///   : proto::terminal<int>
    /// {};
    /// 
    /// struct char_terminal
    ///   : proto::terminal<char>
    /// {};
    /// 
    /// struct my_grammar
    ///   : proto::or_<
    ///         proto::when< int_terminal, proto::external_transform >
    ///       , proto::when< char_terminal, proto::external_transform >
    ///       , proto::when<
    ///             proto::plus< my_grammar, my_grammar >
    ///           , proto::fold< _, int(), my_grammar >
    ///         >
    ///     >
    /// {};
    /// 
    /// struct my_transforms
    ///   : proto::external_transforms<
    ///         proto::when<int_terminal, print(proto::_value)>
    ///       , proto::when<char_terminal, print(proto::_value)>
    ///     >
    /// {};
    ///
    /// proto::literal<int> i(1);
    /// proto::literal<char> c('a');
    /// my_transforms trx;
    ///
    /// // Evaluate "i+c" using my_grammar with the specified transforms:
    /// my_grammar()(i + c, 0, trx);
    /// \endcode
    template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_MPL_LIMIT_MAP_SIZE, typename T, mpl::na)>
    struct external_transforms
    {
        typedef mpl::map<BOOST_PP_ENUM_PARAMS(BOOST_MPL_LIMIT_MAP_SIZE, T)> map_type;

        template<typename Rule>
        struct when
          : proto::when<_, typename mpl::at<map_type, Rule>::type>
        {};
    };

    // Other specializations of proto::when are generated by the preprocessor...
    #include <boost/proto/transform/detail/when.hpp>

    /// INTERNAL ONLY
    ///
    template<typename Grammar, typename Transform>
    struct is_callable<when<Grammar, Transform> >
      : mpl::true_
    {};

}} // namespace boost::proto

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
