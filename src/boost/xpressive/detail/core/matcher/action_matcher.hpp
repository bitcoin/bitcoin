///////////////////////////////////////////////////////////////////////////////
// action_matcher.hpp
//
//  Copyright 2008 Eric Niebler.
//  Copyright 2008 David Jenkins.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ACTION_MATCHER_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_ACTION_MATCHER_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/ref.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/action.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/xpressive/match_results.hpp> // for type_info_less
#include <boost/xpressive/detail/static/transforms/as_action.hpp> // for 'read_attr'
#if BOOST_VERSION >= 103500
# include <boost/proto/fusion.hpp>
# include <boost/fusion/include/transform_view.hpp>
# include <boost/fusion/include/invoke.hpp>
# include <boost/fusion/include/push_front.hpp>
# include <boost/fusion/include/pop_front.hpp>
#endif

#if BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4510) // default constructor could not be generated
#pragma warning(disable : 4512) // assignment operator could not be generated
#pragma warning(disable : 4610) // can never be instantiated - user defined constructor required
#endif

namespace boost { namespace xpressive { namespace detail
{

    #if BOOST_VERSION >= 103500
    struct DataMember
      : proto::mem_ptr<proto::_, proto::terminal<proto::_> >
    {};

    template<typename Expr, long N>
    struct child_
      : remove_reference<
            typename proto::result_of::child_c<Expr &, N>::type
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // mem_ptr_eval
    //  Rewrites expressions of the form x->*foo(a) into foo(x, a) and then
    //  evaluates them.
    template<typename Expr, typename Context, bool IsDataMember = proto::matches<Expr, DataMember>::value>
    struct mem_ptr_eval
    {
        typedef typename child_<Expr, 0>::type left_type;
        typedef typename child_<Expr, 1>::type right_type;

        typedef
            typename proto::result_of::value<
                typename proto::result_of::child_c<right_type, 0>::type
            >::type
        function_type;

        typedef
            fusion::transform_view<
                typename fusion::result_of::push_front<
                    typename fusion::result_of::pop_front<right_type>::type const
                  , reference_wrapper<left_type>
                >::type const
              , proto::eval_fun<Context>
            >
        evaluated_args;

        typedef
            typename fusion::result_of::invoke<function_type, evaluated_args>::type
        result_type;

        result_type operator()(Expr &expr, Context &ctx) const
        {
            return fusion::invoke<function_type>(
                proto::value(proto::child_c<0>(proto::right(expr)))
              , evaluated_args(
                    fusion::push_front(fusion::pop_front(proto::right(expr)), boost::ref(proto::left(expr)))
                  , proto::eval_fun<Context>(ctx)
                )
            );
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // mem_ptr_eval
    //  Rewrites expressions of the form x->*foo into foo(x) and then
    //  evaluates them.
    template<typename Expr, typename Context>
    struct mem_ptr_eval<Expr, Context, true>
    {
        typedef typename child_<Expr, 0>::type left_type;
        typedef typename child_<Expr, 1>::type right_type;

        typedef
            typename proto::result_of::value<right_type>::type
        function_type;

        typedef typename boost::result_of<
            function_type(typename proto::result_of::eval<left_type, Context>::type)
        >::type result_type;

        result_type operator()(Expr &expr, Context &ctx) const
        {
            return proto::value(proto::right(expr))(
                proto::eval(proto::left(expr), ctx)
            );
        }
    };
    #endif

    struct attr_with_default_tag
    {};

    template<typename T>
    struct opt;

    ///////////////////////////////////////////////////////////////////////////////
    // action_context
    //
    struct action_context
    {
        explicit action_context(action_args_type *action_args)
          : action_args_(action_args)
        {}

        action_args_type const &args() const
        {
            return *this->action_args_;
        }

        // eval_terminal
        template<typename Expr, typename Arg>
        struct eval_terminal
          : proto::default_eval<Expr, action_context const>
        {};

        template<typename Expr, typename Arg>
        struct eval_terminal<Expr, reference_wrapper<Arg> >
        {
            typedef Arg &result_type;
            result_type operator()(Expr &expr, action_context const &) const
            {
                return proto::value(expr).get();
            }
        };

        template<typename Expr, typename Arg>
        struct eval_terminal<Expr, opt<Arg> >
        {
            typedef Arg const &result_type;
            result_type operator()(Expr &expr, action_context const &) const
            {
                return proto::value(expr);
            }
        };

        template<typename Expr, typename Type, typename Int>
        struct eval_terminal<Expr, action_arg<Type, Int> >
        {
            typedef typename action_arg<Type, Int>::reference result_type;
            result_type operator()(Expr &expr, action_context const &ctx) const
            {
                action_args_type::const_iterator where_ = ctx.args().find(&typeid(proto::value(expr)));
                if(where_ == ctx.args().end())
                {
                    BOOST_THROW_EXCEPTION(
                        regex_error(
                            regex_constants::error_badarg
                          , "An argument to an action was unspecified"
                        )
                    );
                }
                return proto::value(expr).cast(where_->second);
            }
        };

        // eval
        template<typename Expr, typename Tag = typename Expr::proto_tag>
        struct eval
          : proto::default_eval<Expr, action_context const>
        {};

        template<typename Expr>
        struct eval<Expr, proto::tag::terminal>
          : eval_terminal<Expr, typename proto::result_of::value<Expr>::type>
        {};

        // Evaluate attributes like a1|42
        template<typename Expr>
        struct eval<Expr, attr_with_default_tag>
        {
            typedef
                typename proto::result_of::value<
                    typename proto::result_of::left<
                        typename proto::result_of::child<
                            Expr
                        >::type
                    >::type
                >::type
            temp_type;

            typedef typename temp_type::type result_type;

            result_type operator ()(Expr const &expr, action_context const &ctx) const
            {
                return proto::value(proto::left(proto::child(expr))).t_
                    ? *proto::value(proto::left(proto::child(expr))).t_
                    :  proto::eval(proto::right(proto::child(expr)), ctx);
            }
        };

        #if BOOST_VERSION >= 103500
        template<typename Expr>
        struct eval<Expr, proto::tag::mem_ptr>
          : mem_ptr_eval<Expr, action_context const>
        {};
        #endif

    private:
        action_args_type *action_args_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // action
    //
    template<typename Actor>
    struct action
      : actionable
    {
        action(Actor const &actor)
          : actionable()
          , actor_(actor)
        {
        }

        virtual void execute(action_args_type *action_args) const
        {
            action_context const ctx(action_args);
            proto::eval(this->actor_, ctx);
        }

    private:
        Actor actor_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // subreg_transform
    //
    struct subreg_transform : proto::transform<subreg_transform>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::state state_type;

            typedef
                typename proto::terminal<sub_match<typename state_type::iterator> >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                return result_type::make(state.sub_matches_[ data ]);
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // mark_transform
    //
    struct mark_transform : proto::transform<mark_transform>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::state state_type;
            typedef
                typename proto::terminal<sub_match<typename state_type::iterator> >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param
            ) const
            {
                return result_type::make(state.sub_matches_[ proto::value(expr).mark_number_ ]);
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // opt
    //
    template<typename T>
    struct opt
    {
        typedef T type;
        typedef T const &reference;

        opt(T const *t)
          : t_(t)
        {}

        operator reference() const
        {
            BOOST_XPR_ENSURE_(0 != this->t_, regex_constants::error_badattr, "Use of uninitialized regex attribute");
            return *this->t_;
        }

        T const *t_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // attr_transform
    //
    struct attr_transform : proto::transform<attr_transform>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef typename impl::expr expr_type;

            typedef
                typename expr_type::proto_child0::matcher_type::value_type::second_type
            attr_type;

            typedef
                typename proto::terminal<opt<attr_type> >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param
              , typename impl::state_param state
              , typename impl::data_param
            ) const
            {
                int slot = typename expr_type::proto_child0::nbr_type();
                attr_type const *attr = static_cast<attr_type const *>(state.attr_context_.attr_slots_[slot-1]);
                return result_type::make(opt<attr_type>(attr));
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // attr_with_default_transform
    //
    template<typename Grammar, typename Callable = proto::callable>
    struct attr_with_default_transform : proto::transform<attr_with_default_transform<Grammar, Callable> >
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::unary_expr<
                    attr_with_default_tag
                  , typename Grammar::template impl<Expr, State, Data>::result_type
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                result_type that = {
                    typename Grammar::template impl<Expr, State, Data>()(expr, state, data)
                };
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // by_ref_transform
    //
    struct by_ref_transform : proto::transform<by_ref_transform>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::result_of::value<typename impl::expr_param>::type
            reference;

            typedef
                typename proto::terminal<reference>::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param
              , typename impl::data_param
            ) const
            {
                return result_type::make(proto::value(expr));
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////////
    // BindActionArgs
    //
    struct BindActionArgs
      : proto::or_<
            proto::when<proto::terminal<any_matcher>,                      subreg_transform>
          , proto::when<proto::terminal<mark_placeholder>,                 mark_transform>
          , proto::when<proto::terminal<read_attr<proto::_, proto::_> >,   attr_transform>
          , proto::when<proto::terminal<proto::_>,                         by_ref_transform>
          , proto::when<
                proto::bitwise_or<proto::terminal<read_attr<proto::_, proto::_> >, BindActionArgs>
              , attr_with_default_transform<proto::bitwise_or<attr_transform, BindActionArgs> >
            >
          , proto::otherwise<proto::nary_expr<proto::_, proto::vararg<BindActionArgs> > >
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // action_matcher
    //
    template<typename Actor>
    struct action_matcher
      : quant_style<quant_none, 0, false>
    {
        int sub_;
        Actor actor_;

        action_matcher(Actor const &actor, int sub)
          : sub_(sub)
          , actor_(actor)
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            // Bind the arguments
            typedef
                typename boost::result_of<BindActionArgs(
                    Actor const &
                  , match_state<BidiIter> &
                  , int const &
                )>::type
            action_type;

            action<action_type> actor(BindActionArgs()(this->actor_, state, this->sub_));

            // Put the action in the action list
            actionable const **action_list_tail = state.action_list_tail_;
            *state.action_list_tail_ = &actor;
            state.action_list_tail_ = &actor.next;

            // Match the rest of the pattern
            if(next.match(state))
            {
                return true;
            }

            BOOST_ASSERT(0 == actor.next);
            // remove action from list
            *action_list_tail = 0;
            state.action_list_tail_ = action_list_tail;
            return false;
        }
    };

}}}

#if BOOST_MSVC
#pragma warning(pop)
#endif

#endif
