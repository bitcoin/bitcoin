/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_STATEMENT_SWITCH_HPP
#define BOOST_PHOENIX_STATEMENT_SWITCH_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/proto/make_expr.hpp>
#include <boost/proto/fusion.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4065) // switch statement contains 'default' but no 'case' labels
#endif

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(switch_case)
  , (proto::terminal<proto::_>)
    (meta_grammar)
)

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(switch_default_case)
  , (meta_grammar)
)

namespace boost { namespace phoenix
{
    namespace detail
    {
        struct switch_case_grammar;
        struct switch_case_with_default_grammar;
        struct switch_grammar
            : proto::or_<
                proto::when<
                    detail::switch_case_grammar
                  , mpl::false_()
                >
              , proto::when<
                    detail::switch_case_with_default_grammar
                  , mpl::true_()
                >
            >
        {};
    }

    namespace detail
    {
        struct switch_case_is_nullary
            : proto::or_<
                proto::when<
                    proto::comma<
                        switch_case_is_nullary
                      , proto::or_<phoenix::rule::switch_default_case, phoenix::rule::switch_case>
                    >
                  , mpl::and_<
                        switch_case_is_nullary(
                            proto::_child_c<0>
                          , proto::_state
                        )
                      , switch_case_is_nullary(
                            proto::_child_c<1>
                          , proto::_state
                        )
                    >()
                >
              , proto::when<
                    proto::or_<phoenix::rule::switch_default_case, phoenix::rule::switch_case>
                  , evaluator(proto::_child_c<0>, proto::_state)
                >
            >
        {};

        struct switch_case_grammar
            : proto::or_<
                proto::comma<switch_case_grammar, phoenix::rule::switch_case>
              , proto::when<phoenix::rule::switch_case, proto::_>
            >
        {};

        struct switch_case_with_default_grammar
            : proto::or_<
                proto::comma<switch_case_grammar, phoenix::rule::switch_default_case>
              , proto::when<phoenix::rule::switch_default_case, proto::_>
            >
        {};

        struct switch_size
            : proto::or_<
                proto::when<
                    proto::comma<switch_size, proto::_>
                  , mpl::next<switch_size(proto::_left)>()
                >
              , proto::when<proto::_, mpl::int_<1>()>
            >
        {};
    }
}}

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(switch_)
  , (meta_grammar)           // Cond
    (detail::switch_grammar) // Cases
)

namespace boost { namespace phoenix {

    template <typename Dummy>
    struct is_nullary::when<rule::switch_, Dummy>
        : proto::and_<
            evaluator(proto::_child_c<0>, _context)
          , detail::switch_case_is_nullary(proto::_child_c<1>, _context)
        >
    {};

    struct switch_eval
    {
        typedef void result_type;
        
        template <typename Context>
        result_type
        operator()(Context const &) const
        {
        }

        template <typename Cond, typename Cases, typename Context>
        result_type
        operator()(Cond const & cond, Cases const & cases, Context const & ctx) const
        {
            this->evaluate(
                    ctx
                  , cond
                  , cases
                  , typename detail::switch_size::impl<Cases, int, proto::empty_env>::result_type()
                  , typename detail::switch_grammar::impl<Cases, int, proto::empty_env>::result_type()
                );
        }

        private:
            template <typename Context, typename Cond, typename Cases>
            result_type
            evaluate(
                Context const & ctx
              , Cond const & cond
              , Cases const & cases
              , mpl::int_<1>
              , mpl::false_
            ) const
            {
                typedef
                    typename proto::result_of::value<
                        typename proto::result_of::child_c<
                            Cases
                          , 0
                        >::type
                    >::type
                    case_label;

                switch(boost::phoenix::eval(cond, ctx))
                {
                    case case_label::value:
                        boost::phoenix::eval(proto::child_c<1>(cases), ctx);
                }
            }
            
            template <typename Context, typename Cond, typename Cases>
            result_type
            evaluate(
                Context const & ctx
              , Cond const & cond
              , Cases const & cases
              , mpl::int_<1>
              , mpl::true_
            ) const
            {
                switch(boost::phoenix::eval(cond, ctx))
                {
                    default:
                        boost::phoenix::eval(proto::child_c<0>(cases), ctx);
                }
            }

            // Bring in the evaluation functions
            #include <boost/phoenix/statement/detail/switch.hpp>
    };
    
    template <typename Dummy>
    struct default_actions::when<rule::switch_, Dummy>
        : call<switch_eval>
    {};

    template <int N, typename A>
    inline
    typename proto::result_of::make_expr<
        tag::switch_case
      , proto::basic_default_domain
      , mpl::int_<N>
      , A
    >::type const
    case_(A const & a)
    {
        return
            proto::make_expr<
                tag::switch_case
              , proto::basic_default_domain
            >(
                mpl::int_<N>()
              , a
            );
    }

    template <typename A>
    inline
    typename proto::result_of::make_expr<
        tag::switch_default_case
      , proto::basic_default_domain
      , A
    >::type const
    default_(A const& a)
    {
        return
            proto::make_expr<
                tag::switch_default_case
              , proto::basic_default_domain
            >(a);
    }

    template <typename Cond>
    struct switch_gen
    {
        switch_gen(Cond const& cond_) : cond(cond_) {}

        template <typename Cases>
        typename expression::switch_<
            Cond
          , Cases
        >::type
        operator[](Cases const& cases) const
        {
            return
                this->generate(
                    cases
                  , proto::matches<Cases, detail::switch_grammar>()
                );
        }

        private:
            Cond const& cond;

            template <typename Cases>
            typename expression::switch_<
                Cond
              , Cases
            >::type
            generate(Cases const & cases, mpl::true_) const
            {
                return expression::switch_<Cond, Cases>::make(cond, cases);
            }
            
            template <typename Cases>
            typename expression::switch_<
                Cond
              , Cases
            >::type
            generate(Cases const &, mpl::false_) const
            {
                BOOST_MPL_ASSERT_MSG(
                    false
                  , INVALID_SWITCH_CASE_STATEMENT
                  , (Cases)
                );
            }
    };

    template <typename Cond>
    inline
    switch_gen<Cond> const
    switch_(Cond const& cond)
    {
        return switch_gen<Cond>(cond);
    }

}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

