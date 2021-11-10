///////////////////////////////////////////////////////////////////////////////
// predicate_matcher.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_PREDICATE_MATCHER_HPP_EAN_03_22_2007
#define BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_PREDICATE_MATCHER_HPP_EAN_03_22_2007

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/mpl/not.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/quant_style.hpp>
#include <boost/xpressive/detail/core/matcher/action_matcher.hpp>
#include <boost/xpressive/detail/core/state.hpp>
#include <boost/proto/core.hpp>

namespace boost { namespace xpressive { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // predicate_context
    //
    template<typename BidiIter>
    struct predicate_context
    {
        explicit predicate_context(int sub, sub_match_impl<BidiIter> const *sub_matches, action_args_type *action_args)
          : sub_(sub)
          , sub_matches_(sub_matches)
          , action_args_(action_args)
        {}

        action_args_type const &args() const
        {
            return *this->action_args_;
        }

        // eval_terminal
        template<typename Expr, typename Arg>
        struct eval_terminal
          : proto::default_eval<Expr, predicate_context const>
        {};

        template<typename Expr, typename Arg>
        struct eval_terminal<Expr, reference_wrapper<Arg> >
        {
            typedef Arg &result_type;
            result_type operator()(Expr &expr, predicate_context const &) const
            {
                return proto::value(expr).get();
            }
        };

        template<typename Expr>
        struct eval_terminal<Expr, any_matcher>
        {
            typedef sub_match<BidiIter> const &result_type;
            result_type operator()(Expr &, predicate_context const &ctx) const
            {
                return ctx.sub_matches_[ctx.sub_];
            }
        };

        template<typename Expr>
        struct eval_terminal<Expr, mark_placeholder>
        {
            typedef sub_match<BidiIter> const &result_type;
            result_type operator()(Expr &expr, predicate_context const &ctx) const
            {
                return ctx.sub_matches_[proto::value(expr).mark_number_];
            }
        };

        template<typename Expr, typename Type, typename Int>
        struct eval_terminal<Expr, action_arg<Type, Int> >
        {
            typedef typename action_arg<Type, Int>::reference result_type;
            result_type operator()(Expr &expr, predicate_context const &ctx) const
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
          : proto::default_eval<Expr, predicate_context const>
        {};

        template<typename Expr>
        struct eval<Expr, proto::tag::terminal>
          : eval_terminal<Expr, typename proto::result_of::value<Expr>::type>
        {};

        #if BOOST_VERSION >= 103500
        template<typename Expr>
        struct eval<Expr, proto::tag::mem_ptr>
          : mem_ptr_eval<Expr, predicate_context const>
        {};
        #endif

        int sub_;
        sub_match_impl<BidiIter> const *sub_matches_;
        action_args_type *action_args_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // AssertionFunctor
    //
    struct AssertionFunctor
      : proto::function<
            proto::terminal<check_tag>
          , proto::terminal<proto::_>
        >
    {};

    ///////////////////////////////////////////////////////////////////////////////
    // predicate_matcher
    //
    template<typename Predicate>
    struct predicate_matcher
      : quant_style_assertion
    {
        int sub_;
        Predicate predicate_;

        predicate_matcher(Predicate const &pred, int sub)
          : sub_(sub)
          , predicate_(pred)
        {
        }

        template<typename BidiIter, typename Next>
        bool match(match_state<BidiIter> &state, Next const &next) const
        {
            // Predicate is check(assertion), where assertion can be
            // a lambda or a function object.
            return this->match_(state, next, proto::matches<Predicate, AssertionFunctor>());
        }

    private:
        template<typename BidiIter, typename Next>
        bool match_(match_state<BidiIter> &state, Next const &next, mpl::true_) const
        {
            sub_match<BidiIter> const &sub = state.sub_match(this->sub_);
            return proto::value(proto::child_c<1>(this->predicate_))(sub) && next.match(state);
        }

        template<typename BidiIter, typename Next>
        bool match_(match_state<BidiIter> &state, Next const &next, mpl::false_) const
        {
            predicate_context<BidiIter> ctx(this->sub_, state.sub_matches_, state.action_args_);
            return proto::eval(proto::child_c<1>(this->predicate_), ctx) && next.match(state);
        }
    };

}}}

#endif // BOOST_XPRESSIVE_DETAIL_CORE_MATCHER_PREDICATE_MATCHER_HPP_EAN_03_22_2007
