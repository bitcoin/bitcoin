/*=============================================================================
    Copyright (c) 2011 Thomas Heller
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c)      2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ARGUMENT_MARCH_22_2011_0939PM)
#define BOOST_SPIRIT_ARGUMENT_MARCH_22_2011_0939PM

#include <boost/phoenix/core/terminal.hpp>
#include <boost/phoenix/core/v2_eval.hpp>
#include <boost/proto/proto_fwd.hpp> // for transform placeholders

namespace boost { namespace spirit
{
    template <int N>
    struct argument;

    template <typename Dummy>
    struct attribute_context;

    namespace expression
    {
        template <int N>
        struct argument
          : phoenix::expression::terminal<spirit::argument<N> >
        {
            typedef typename phoenix::expression::terminal<
                spirit::argument<N> 
            >::type type;

            static type make()
            {
                type const e = {{{}}};
                return e;
            }
        };
        
        template <typename Dummy>
        struct attribute_context
          : phoenix::expression::terminal<spirit::attribute_context<Dummy> >
        {
            typedef typename phoenix::expression::terminal<
                spirit::attribute_context<Dummy> 
            >::type type;

            static type make()
            {
                type const e = {{{}}};
                return e;
            }
        };
    }
}}

namespace boost { namespace phoenix
{
    namespace result_of
    {
        template <typename Dummy>
        struct is_nullary<custom_terminal<spirit::attribute_context<Dummy> > >
          : mpl::false_
        {};

        template <int N>
        struct is_nullary<custom_terminal<spirit::argument<N> > >
          : mpl::false_
        {};
    }

    template <typename Dummy>
    struct is_custom_terminal<spirit::attribute_context<Dummy> >
      : mpl::true_
    {};

    template <int N>
    struct is_custom_terminal<spirit::argument<N> >
      : mpl::true_
    {};

    template <typename Dummy>
    struct custom_terminal<spirit::attribute_context<Dummy> >
      : proto::call<
            v2_eval(
                proto::make<spirit::attribute_context<Dummy>()>
              , proto::call<
                    functional::env(proto::_state)
                >
            )
        >
    {};

    template <int N>
    struct custom_terminal<spirit::argument<N> >
      : proto::call<
            v2_eval(
                proto::make<spirit::argument<N>()>
              , proto::call<
                    functional::env(proto::_state)
                >
            )
        >
    {};
}}

#endif
