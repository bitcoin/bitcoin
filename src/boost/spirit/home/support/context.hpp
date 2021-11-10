/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_CONTEXT_OCTOBER_31_2008_0654PM)
#define BOOST_SPIRIT_CONTEXT_OCTOBER_31_2008_0654PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/spirit/home/support/nonterminal/expand_arg.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/spirit/home/support/argument.hpp>
#include <boost/spirit/home/support/limits.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/fusion/include/as_list.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal.hpp>
#include <boost/phoenix/core/v2_eval.hpp>
#include <boost/proto/proto_fwd.hpp> // for transform placeholders

///////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS

#define SPIRIT_DECLARE_ATTRIBUTE(z, n, data)                                   \
    typedef phoenix::actor<attribute<n> >                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(_r, n), _type);                              \
    phoenix::actor<attribute<n> > const                                        \
        BOOST_PP_CAT(_r, n) = BOOST_PP_CAT(BOOST_PP_CAT(_r, n), _type)();
    /***/
#define SPIRIT_USING_ATTRIBUTE(z, n, data)                                     \
    using spirit::BOOST_PP_CAT(BOOST_PP_CAT(_r, n), _type);                    \
    using spirit::BOOST_PP_CAT(_r, n);                                         \
    /***/

#else

#define SPIRIT_DECLARE_ATTRIBUTE(z, n, data)                                   \
    typedef phoenix::actor<attribute<n> >                                      \
        BOOST_PP_CAT(BOOST_PP_CAT(_r, n), _type);                              \
    /***/
#define SPIRIT_USING_ATTRIBUTE(z, n, data)                                     \
    using spirit::BOOST_PP_CAT(BOOST_PP_CAT(_r, n), _type);                    \
    /***/

#endif

namespace boost { namespace spirit
{
    template <int>
    struct attribute;

    template <int>
    struct local_variable;
}}

BOOST_PHOENIX_DEFINE_CUSTOM_TERMINAL(
    template <int N>
  , boost::spirit::attribute<N>
  , mpl::false_                 // is not nullary
  , v2_eval(
        proto::make<
            boost::spirit::attribute<N>()
        >
      , proto::call<
            functional::env(proto::_state)
        >
    )
)

BOOST_PHOENIX_DEFINE_CUSTOM_TERMINAL(
    template <int N>
  , boost::spirit::local_variable<N>
  , mpl::false_                 // is not nullary
  , v2_eval(
        proto::make<
            boost::spirit::local_variable<N>()
        >
      , proto::call<
            functional::env(proto::_state)
        >
    )
)

namespace boost { namespace spirit
{
    template <typename Attributes, typename Locals>
    struct context
    {
        typedef Attributes attributes_type;
        typedef Locals locals_type;

        context(typename Attributes::car_type attribute)
          : attributes(attribute, fusion::nil_()), locals() {}

        template <typename Args, typename Context>
        context(
            typename Attributes::car_type attribute
          , Args const& args
          , Context& caller_context
        ) : attributes(
                attribute
              , fusion::as_list(
                    fusion::transform(
                        args
                      , detail::expand_arg<Context>(caller_context)
                    )
                )
            )
          , locals() {}

        context(Attributes const& attributes_)
          : attributes(attributes_), locals() {}

        Attributes attributes;  // The attributes
        Locals locals;          // Local variables
    };

    template <typename Context>
    struct attributes_of
    {
        typedef typename Context::attributes_type type;
    };

    template <typename Context>
    struct attributes_of<Context const>
    {
        typedef typename Context::attributes_type const type;
    };

    template <typename Context>
    struct attributes_of<Context &>
      : attributes_of<Context>
    {};

    template <typename Context>
    struct locals_of
    {
        typedef typename Context::locals_type type;
    };

    template <typename Context>
    struct locals_of<Context const>
    {
        typedef typename Context::locals_type const type;
    };

    template <typename Context>
    struct locals_of<Context &>
    {
        typedef typename Context::locals_type type;
    };

    template <int N>
    struct attribute
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename
                attributes_of<typename
                    mpl::at_c<typename Env::args_type, 1>::type
                >::type
            attributes_type;

            typedef typename
                fusion::result_of::size<attributes_type>::type
            attributes_size;

            // report invalid argument not found (N is out of bounds)
            BOOST_SPIRIT_ASSERT_MSG(
                (N < attributes_size::value),
                index_is_out_of_bounds, ());

            typedef typename
                fusion::result_of::at_c<attributes_type, N>::type
            type;
        };

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            return fusion::at_c<N>((fusion::at_c<1>(env.args())).attributes);
        }
    };

    template <int N>
    struct local_variable
    {
        typedef mpl::true_ no_nullary;

        template <typename Env>
        struct result
        {
            typedef typename
                locals_of<typename
                    mpl::at_c<typename Env::args_type, 1>::type
                >::type
            locals_type;

            typedef typename
                fusion::result_of::size<locals_type>::type
            locals_size;

            // report invalid argument not found (N is out of bounds)
            BOOST_SPIRIT_ASSERT_MSG(
                (N < locals_size::value),
                index_is_out_of_bounds, ());

            typedef typename
                fusion::result_of::at_c<locals_type, N>::type
            type;
        };

        template <typename Env>
        typename result<Env>::type
        eval(Env const& env) const
        {
            return get_arg<N>((fusion::at_c<1>(env.args())).locals);
        }
    };
    
    typedef phoenix::actor<attribute<0> > _val_type;
    typedef phoenix::actor<attribute<0> > _r0_type;
    typedef phoenix::actor<attribute<1> > _r1_type;
    typedef phoenix::actor<attribute<2> > _r2_type;

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    // _val refers to the 'return' value of a rule (same as _r0)
    // _r1, _r2, ... refer to the rule arguments
    _val_type const _val = _val_type();
    _r0_type const _r0 = _r0_type();
    _r1_type const _r1 = _r1_type();
    _r2_type const _r2 = _r2_type();
#endif

    //  Bring in the rest of the attributes (_r4 .. _rN+1), using PP
    BOOST_PP_REPEAT_FROM_TO(
        3, SPIRIT_ATTRIBUTES_LIMIT, SPIRIT_DECLARE_ATTRIBUTE, _)

    typedef phoenix::actor<local_variable<0> > _a_type;
    typedef phoenix::actor<local_variable<1> > _b_type;
    typedef phoenix::actor<local_variable<2> > _c_type;
    typedef phoenix::actor<local_variable<3> > _d_type;
    typedef phoenix::actor<local_variable<4> > _e_type;
    typedef phoenix::actor<local_variable<5> > _f_type;
    typedef phoenix::actor<local_variable<6> > _g_type;
    typedef phoenix::actor<local_variable<7> > _h_type;
    typedef phoenix::actor<local_variable<8> > _i_type;
    typedef phoenix::actor<local_variable<9> > _j_type;

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    // _a, _b, ... refer to the local variables of a rule
    _a_type const _a = _a_type();
    _b_type const _b = _b_type();
    _c_type const _c = _c_type();
    _d_type const _d = _d_type();
    _e_type const _e = _e_type();
    _f_type const _f = _f_type();
    _g_type const _g = _g_type();
    _h_type const _h = _h_type();
    _i_type const _i = _i_type();
    _j_type const _j = _j_type();
#endif

    // You can bring these in with the using directive
    // without worrying about bringing in too much.
    namespace labels
    {
        BOOST_PP_REPEAT(SPIRIT_ARGUMENTS_LIMIT, SPIRIT_USING_ARGUMENT, _)
        BOOST_PP_REPEAT(SPIRIT_ATTRIBUTES_LIMIT, SPIRIT_USING_ATTRIBUTE, _)

#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
        using spirit::_val;
        using spirit::_a;
        using spirit::_b;
        using spirit::_c;
        using spirit::_d;
        using spirit::_e;
        using spirit::_f;
        using spirit::_g;
        using spirit::_h;
        using spirit::_i;
        using spirit::_j;
#endif
    }
}}

#endif
