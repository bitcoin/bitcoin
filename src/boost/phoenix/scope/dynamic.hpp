/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_SCOPE_DYNAMIC_HPP
#define BOOST_PHOENIX_SCOPE_DYNAMIC_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/call.hpp>
#include <boost/phoenix/support/iterate.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#define BOOST_PHOENIX_DYNAMIC_TEMPLATE_PARAMS(R, DATA, I, ELEM)                 \
      BOOST_PP_COMMA_IF(I) BOOST_PP_TUPLE_ELEM(2, 0, ELEM)                      \
/**/

#define BOOST_PHOENIX_DYNAMIC_CTOR_INIT(R, DATA, I, ELEM)                       \
    BOOST_PP_COMMA_IF(I) BOOST_PP_TUPLE_ELEM(2, 1, ELEM)(init<I>(this))         \
/**/

#define BOOST_PHOENIX_DYNAMIC_MEMBER(R, DATA, I, ELEM)                          \
    BOOST_PP_CAT(member, BOOST_PP_INC(I)) BOOST_PP_TUPLE_ELEM(2, 1, ELEM);      \
/**/

#define BOOST_PHOENIX_DYNAMIC_FILLER_0(X, Y)                                    \
    ((X, Y)) BOOST_PHOENIX_DYNAMIC_FILLER_1                                     \
/**/

#define BOOST_PHOENIX_DYNAMIC_FILLER_1(X, Y)                                    \
    ((X, Y)) BOOST_PHOENIX_DYNAMIC_FILLER_0                                     \
/**/

#define BOOST_PHOENIX_DYNAMIC_FILLER_0_END
#define BOOST_PHOENIX_DYNAMIC_FILLER_1_END

#define BOOST_PHOENIX_DYNAMIC_BASE(NAME, MEMBER)                                \
struct NAME                                                                     \
    : ::boost::phoenix::dynamic<                                                \
        BOOST_PP_SEQ_FOR_EACH_I(                                                \
                BOOST_PHOENIX_DYNAMIC_TEMPLATE_PARAMS                           \
              , _                                                               \
              , MEMBER)                                                         \
    >                                                                           \
{                                                                               \
    NAME()                                                                      \
        : BOOST_PP_SEQ_FOR_EACH_I(BOOST_PHOENIX_DYNAMIC_CTOR_INIT, _, MEMBER)   \
    {}                                                                          \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_PHOENIX_DYNAMIC_MEMBER, _, MEMBER)            \
}                                                                               \
/**/

#define BOOST_PHOENIX_DYNAMIC(NAME, MEMBER)                                     \
    BOOST_PHOENIX_DYNAMIC_BASE(                                                 \
        NAME                                                                    \
      , BOOST_PP_CAT(BOOST_PHOENIX_DYNAMIC_FILLER_0 MEMBER,_END)                \
    )                                                                           \
/**/

BOOST_PHOENIX_DEFINE_EXPRESSION(
    (boost)(phoenix)(dynamic_member)
  , (proto::terminal<proto::_>)
    (proto::terminal<proto::_>)
)

namespace boost { namespace phoenix
{
    template <typename DynamicScope>
    struct dynamic_frame : noncopyable
    {
        typedef typename DynamicScope::tuple_type tuple_type;

        dynamic_frame(DynamicScope const& s)
            : tuple()
            , save(s.frame)
            , scope(s)
        {
            scope.frame = this;
        }

        template <typename Tuple>
        dynamic_frame(DynamicScope const& s, Tuple const& init)
            : tuple(init)
            , save(s.frame)
            , scope(s)
        {
            scope.frame = this;
        }

        ~dynamic_frame()
        {
            scope.frame = save;
        }

        tuple_type& data() { return tuple; }
        tuple_type const& data() const { return tuple; }

        private:
            tuple_type tuple;
            dynamic_frame *save;
            DynamicScope const& scope;
    };

    struct dynamic_member_eval
    {
        template <typename Sig>
        struct result;

        template <typename This, typename N, typename Scope, typename Context>
        struct result<This(N, Scope, Context)>
        {
            typedef
                typename boost::remove_pointer<
                    typename proto::detail::uncvref<
                        typename proto::result_of::value<Scope>::type
                    >::type
                >::type
                scope_type;
            typedef 
                typename scope_type::dynamic_frame_type::tuple_type
                tuple_type;

            typedef
                typename fusion::result_of::at_c<
                    tuple_type
                  , proto::detail::uncvref<
                        typename proto::result_of::value<N>::type
                    >::type::value
                >::type
                type;

        };

        template <typename N, typename Scope, typename Context>
        typename result<dynamic_member_eval(N, Scope, Context)>::type
        operator()(N, Scope s, Context const &) const
        {
            return
                fusion::at_c<
                    proto::detail::uncvref<
                        typename proto::result_of::value<N>::type
                    >::type::value
                >(
                    proto::value(s)->frame->data()
                );
        }
    };

    template <typename Dummy>
    struct default_actions::when<rule::dynamic_member, Dummy>
        : call<dynamic_member_eval>
    {};

//#if defined(BOOST_PHOENIX_NO_VARIADIC_SCOPE)
    template <
        BOOST_PHOENIX_typename_A_void(BOOST_PHOENIX_DYNAMIC_LIMIT)
      , typename Dummy = void
    >
    struct dynamic;

    // Bring in the rest ...
    #include <boost/phoenix/scope/detail/cpp03/dynamic.hpp>
//#else
//    // TODO:
//#endif
}}

#endif
