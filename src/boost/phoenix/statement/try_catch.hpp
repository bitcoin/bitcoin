/*==============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_PHOENIX_STATEMENT_TRY_CATCH_HPP
#define BOOST_PHOENIX_STATEMENT_TRY_CATCH_HPP

#include <boost/phoenix/config.hpp>
#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/is_nullary.hpp>
#include <boost/phoenix/scope/local_variable.hpp>
#include <boost/phoenix/scope/scoped_environment.hpp>
#include <boost/proto/functional/fusion/pop_front.hpp>
#include <boost/core/enable_if.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct try_catch_actor;

    template <typename Exception>
    struct catch_exception
    {
        typedef Exception type;
    };

    namespace tag
    {
        struct try_catch {};
        struct catch_ {};
        struct catch_all {};
    }

    namespace expression
    {
        template <
            typename Try
          , BOOST_PHOENIX_typename_A_void(BOOST_PHOENIX_CATCH_LIMIT)
          , typename Dummy = void
        >
        struct try_catch;

        // bring in the expression definitions
        #include <boost/phoenix/statement/detail/try_catch_expression.hpp>

        template <typename A0, typename A1, typename A2 = void>
        struct catch_
            : proto::nary_expr<tag::catch_, A0, A1, A2>
        {};

        template <typename A0, typename A1>
        struct catch_<A0, A1, void>
            : proto::binary_expr<tag::catch_, A0, A1>
        {};
        
        template <typename A0>
        struct catch_all
            : proto::unary_expr<tag::catch_all, A0>
        {};
    }

    namespace rule
    {
        typedef
            expression::catch_<
                proto::terminal<catch_exception<proto::_> >
              , local_variable
              , meta_grammar
            >
        captured_catch;

        typedef
            expression::catch_<
                proto::terminal<catch_exception<proto::_> >
              , meta_grammar
            >
        non_captured_catch;

        struct catch_
            : proto::or_<
                captured_catch
              , non_captured_catch
            >
        {};
        
        struct catch_all
            : expression::catch_all<
                meta_grammar
            >
        {};

        struct try_catch
            : proto::or_<
                expression::try_catch<
                     meta_grammar
                   , proto::vararg<rule::catch_>
                >
              , expression::try_catch<
                     meta_grammar
                   , rule::catch_all
                >
              , expression::try_catch<
                     meta_grammar
                   , proto::vararg<rule::catch_>
                   , rule::catch_all
                >
            >
        {};
    }

    template <typename Dummy>
    struct meta_grammar::case_<tag::try_catch, Dummy>
        : enable_rule<rule::try_catch, Dummy>
    {};

    struct try_catch_eval
    {
        typedef void result_type;

        template <typename Try, typename Context>
        void operator()(Try const &, Context const &) const
        {}

        template <typename Catch, typename Exception, typename Context>
        typename enable_if<proto::matches<Catch, rule::non_captured_catch> >::type
        eval_catch_body(Catch const &c, Exception & /*unused*/, Context const &ctx
            BOOST_PHOENIX_SFINAE_AND_OVERLOADS) const
        {
            phoenix::eval(proto::child_c<1>(c), ctx);
        }

        template <typename Catch, typename Exception, typename Context>
        typename enable_if<proto::matches<Catch, rule::captured_catch> >::type
        eval_catch_body(Catch const &c, Exception &e, Context const &ctx) const
        {
            typedef
                typename proto::detail::uncvref<
                    typename proto::result_of::value<
                        typename proto::result_of::child_c<Catch, 1>::type
                    >::type
                >::type
            capture_type;
            typedef
                typename proto::detail::uncvref<
                    typename result_of::env<Context>::type
                >::type
            env_type;
            typedef vector1<Exception &> local_type;
            typedef detail::map_local_index_to_tuple<capture_type> map_type;

            typedef
                phoenix::scoped_environment<
                    env_type
                  , env_type
                  , local_type
                  , map_type
                >
            scoped_env_tpe;

            local_type local = {e};

            scoped_env_tpe env(phoenix::env(ctx), phoenix::env(ctx), local);

            phoenix::eval(proto::child_c<2>(c), phoenix::context(env, phoenix::actions(ctx)));
        }

        // bring in the operator overloads
        #include <boost/phoenix/statement/detail/try_catch_eval.hpp>
    };
    
    template <typename Dummy>
    struct default_actions::when<rule::try_catch, Dummy>
        : call<try_catch_eval, Dummy>
    {};

    namespace detail
    {
        struct try_catch_is_nullary
            : proto::or_<
                proto::when<
                    phoenix::rule::catch_all
                  , proto::call<
                        evaluator(
                            proto::_child_c<0>
                          , proto::_data
                          , proto::make<proto::empty_env()>
                        )
                    >
                >
              , proto::when<
                    phoenix::rule::catch_
                  , proto::or_<
                        proto::when<
                            phoenix::rule::captured_catch
                          , proto::call<
                                evaluator(
                                    proto::_child_c<2>
                                  , proto::call<
                                        phoenix::functional::context(
                                            proto::make<mpl::true_()>
                                          , proto::make<detail::scope_is_nullary_actions()>
                                        )
                                    >
                                  , proto::make<proto::empty_env()>
                                )
                            >
                        >
                      , proto::otherwise<
                            proto::call<
                                evaluator(
                                    proto::_child_c<1>
                                  , proto::_data
                                  , proto::make<proto::empty_env()>
                                )
                            >
                        >
                    >
                >
              , proto::when<
                    phoenix::rule::try_catch
                  , proto::make<
                        mpl::and_<
                            proto::call<
                                evaluator(
                                    proto::_child_c<0>
                                  , proto::_data
                                  , proto::make<proto::empty_env()>
                                )
                            >
                          , proto::fold<
                                proto::call<
                                    proto::functional::pop_front(proto::_)
                                >
                              , proto::make<mpl::true_()>
                              , proto::make<
                                    mpl::and_<
                                        proto::_state
                                      , proto::call<
                                            try_catch_is_nullary(
                                                proto::_
                                              , proto::make<proto::empty_env()>
                                              , proto::_data
                                            )
                                        >
                                    >()
                                >
                            >
                        >()
                    >
                >
            >
        {};

        template <
            typename TryCatch
          , typename Exception
          , typename Capture
          , typename Expr
          , long Arity = proto::arity_of<TryCatch>::value
        >
        struct catch_push_back;

        template <typename TryCatch, typename Exception, typename Capture, typename Expr>
        struct catch_push_back<TryCatch, Exception, Capture, Expr, 1>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_
                  , proto::basic_default_domain
                  , catch_exception<Exception>
                  , Capture
                  , Expr
                >::type
                catch_expr;

            typedef
                phoenix::expression::try_catch<
                    TryCatch
                  , catch_expr
                >
                gen_type;
            typedef typename gen_type::type type;

            static type make(TryCatch const & try_catch, Capture const & capture, Expr const & catch_)
            {
                return
                    gen_type::make(
                        try_catch
                      , proto::make_expr<
                            phoenix::tag::catch_
                          , proto::basic_default_domain
                        >(catch_exception<Exception>(), capture, catch_)
                    );
            }
        };

        template <typename TryCatch, typename Exception, typename Expr>
        struct catch_push_back<TryCatch, Exception, void, Expr, 1>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_
                  , proto::basic_default_domain
                  , catch_exception<Exception>
                  , Expr
                >::type
                catch_expr;
            
            typedef
                phoenix::expression::try_catch<
                    TryCatch
                  , catch_expr
                >
                gen_type;
            typedef typename gen_type::type type;

            static type make(TryCatch const & try_catch, Expr const & catch_)
            {
                return
                    gen_type::make(
                        try_catch
                      , proto::make_expr<
                            phoenix::tag::catch_
                          , proto::basic_default_domain
                        >(catch_exception<Exception>(), catch_)
                    );
            }
        };
        
        template <
            typename TryCatch
          , typename Expr
          , long Arity = proto::arity_of<TryCatch>::value
        >
        struct catch_all_push_back;

        template <typename TryCatch, typename Expr>
        struct catch_all_push_back<TryCatch, Expr, 1>
        {
            typedef
                typename proto::result_of::make_expr<
                    phoenix::tag::catch_all
                  , proto::basic_default_domain
                  , Expr
                >::type
                catch_expr;
            
            typedef
                phoenix::expression::try_catch<
                    TryCatch
                  , catch_expr
                >
                gen_type;
            typedef typename gen_type::type type;

            static type make(TryCatch const& try_catch, Expr const& catch_)
            {
                return
                    gen_type::make(
                        try_catch
                      , proto::make_expr<
                            phoenix::tag::catch_all
                          , proto::basic_default_domain
                        >(catch_)
                    );
            }
        };
        #include <boost/phoenix/statement/detail/catch_push_back.hpp>
    }

    template <typename Dummy>
    struct is_nullary::when<rule::try_catch, Dummy>
        : proto::call<
            detail::try_catch_is_nullary(
                proto::_
              , proto::make<proto::empty_env()>
              , _context
            )
        >
    {};

    template <typename TryCatch, typename Exception, typename Capture = void>
    struct catch_gen
    {
        catch_gen(TryCatch const& try_catch_, Capture const& capture)
            : try_catch(try_catch_)
            , capture(capture) {}

        template <typename Expr>
        typename boost::disable_if<
            proto::matches<
                typename proto::result_of::child_c<
                    TryCatch
                  , proto::arity_of<TryCatch>::value - 1
                >::type
              , rule::catch_all
            >
          , typename detail::catch_push_back<TryCatch, Exception, Capture, Expr>::type
        >::type
        operator[](Expr const& expr) const
        {
            return
                detail::catch_push_back<TryCatch, Exception, Capture, Expr>::make(
                    try_catch, capture, expr
                );
        }

        TryCatch try_catch;
        Capture capture;
    };

    template <typename TryCatch, typename Exception>
    struct catch_gen<TryCatch, Exception, void>
    {
        catch_gen(TryCatch const& try_catch_) : try_catch(try_catch_) {}

        template <typename Expr>
        typename boost::disable_if<
            proto::matches<
                typename proto::result_of::child_c<
                    TryCatch
                  , proto::arity_of<TryCatch>::value - 1
                >::type
              , rule::catch_all
            >
          , typename detail::catch_push_back<TryCatch, Exception, void, Expr>::type
        >::type
        operator[](Expr const& expr) const
        {
            return
                detail::catch_push_back<TryCatch, Exception, void, Expr>::make(
                    try_catch, expr
                );
        }

        TryCatch try_catch;
    };

    template <typename TryCatch>
    struct catch_all_gen
    {
        catch_all_gen(TryCatch const& try_catch_) : try_catch(try_catch_) {}

        template <typename Expr>
        typename boost::disable_if<
            proto::matches<
                typename proto::result_of::child_c<
                    TryCatch
                  , proto::arity_of<TryCatch>::value - 1
                >::type
              , rule::catch_all
            >
          , typename detail::catch_all_push_back<TryCatch, Expr>::type
        >::type
        operator[](Expr const& expr) const
        {
            return detail::catch_all_push_back<TryCatch, Expr>::make(
                try_catch, expr
            );
        }

        TryCatch try_catch;
    };

    template <
        typename Expr
    >
    struct try_catch_actor;

    template <typename Expr>
    struct try_catch_actor
        : actor<Expr>
    {
        typedef actor<Expr> base_type;

        try_catch_actor(base_type const& expr)
            : base_type(expr)
            , catch_all(*this)
        {
        }

        template <typename Exception>
        catch_gen<base_type, Exception> const
        catch_() const
        {
            return catch_gen<base_type, Exception>(*this);
        }

        template <typename Exception, typename Capture>
        catch_gen<base_type, Exception, Capture> const
        catch_(Capture const &expr) const
        {
            return catch_gen<base_type, Exception, Capture>(*this, expr);
        }

        catch_all_gen<base_type> const catch_all;
    };

    template <typename Expr>
    struct is_actor<try_catch_actor<Expr> >
        : mpl::true_
    {};

    struct try_gen
    {
        template <typename Try>
        typename expression::try_catch<Try>::type const
        operator[](Try const & try_) const
        {
            return expression::try_catch<Try>::make(try_);
        }
    };

#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
    try_gen const try_ = {};
#endif
}}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
