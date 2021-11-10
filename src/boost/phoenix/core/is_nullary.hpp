/*=============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_IS_NULLARY_HPP
#define BOOST_PHOENIX_CORE_IS_NULLARY_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/core/environment.hpp>
#include <boost/phoenix/core/is_actor.hpp>
#include <boost/phoenix/core/meta_grammar.hpp>
#include <boost/phoenix/core/terminal_fwd.hpp>
#include <boost/phoenix/support/vector.hpp>
#include <boost/proto/transform/fold.hpp>
#include <boost/proto/transform/lazy.hpp>

namespace boost { namespace phoenix
{
    namespace result_of
    {
        template <typename Expr, typename Enable = void>
        struct is_nullary;
    }

    struct is_nullary
    {
        template <typename Rule, typename Dummy = void>
        struct when
            : proto::fold<
                proto::_
              , mpl::true_()
              , mpl::and_<
                    proto::_state
                  , proto::call<evaluator(proto::_, _context)>
                >()
            >
        {};
    };
    
    template <typename Dummy>
    struct is_nullary::when<rule::argument, Dummy>
    {
        BOOST_PROTO_TRANSFORM(is_nullary::when<rule::argument>)
        template <typename Expr, typename State, typename Data>
        struct impl
        {
            typedef mpl::false_ result_type;
        };
    };
    
    template <
        typename Trait
      , typename Expr
      , typename State
      , typename Data
      , bool IsTransform = proto::is_transform<Trait>::value
    >
    struct is_nullary_custom_terminal_impl
    {
        typedef typename Trait::type result_type;
    };
    
    template <typename Transform, typename Expr, typename State, typename Data>
    struct is_nullary_custom_terminal_impl<Transform, Expr, State, Data, true>
    {
        typedef
            typename Transform::template impl<
                Expr
              , State
              , Data
            >::result_type
            result_type;
    };

    template <typename Dummy>
    struct is_nullary::when<rule::custom_terminal, Dummy>
    {
        BOOST_PROTO_TRANSFORM(is_nullary::when<rule::custom_terminal>)
        
        template <typename Expr, typename State, typename Data>
        struct impl
            : is_nullary_custom_terminal_impl<
                result_of::is_nullary<
                    custom_terminal<
                        typename proto::detail::uncvref<
                            typename proto::result_of::value<Expr>::type
                        >::type
                    >
                >
              , typename proto::result_of::value<Expr>::type
              , State
              , Data
            >
        {};
    };
    
    template <typename Dummy>
    struct is_nullary::when<rule::terminal, Dummy>
    {
        BOOST_PROTO_TRANSFORM(is_nullary::when<rule::terminal>)
        template <typename Expr, typename State, typename Data>
        struct impl
        {
            typedef mpl::true_ result_type;
        };
    };

    namespace result_of
    {
        template <typename Expr, typename Enable>
        struct is_nullary
            : boost::phoenix::evaluator::impl<
                Expr const &
              , vector2<
                    mpl::true_
                  , boost::phoenix::is_nullary
                >
              , proto::empty_env
            >::result_type
        {};
        
        template <typename T>
        struct is_nullary<T & >
            : is_nullary<T>
        {};

        template <typename T>
        struct is_nullary<T const & >
            : is_nullary<T>
        {};

        template <typename T>
        struct is_nullary<T const >
            : is_nullary<T>
        {};

        template <typename T>
        struct is_nullary<custom_terminal<T> >
            : mpl::true_
        {};

        template <typename T>
        struct is_nullary<custom_terminal<actor<T> > >
            : evaluator
        {};
        
        template <typename T>
        struct is_nullary<custom_terminal<boost::reference_wrapper<actor<T> > > >
        {
            BOOST_PROTO_TRANSFORM(is_nullary<custom_terminal<boost::reference_wrapper<actor<T> > > >)
            template <typename Expr, typename State, typename Data>
            struct impl
            {
                typedef typename evaluator::template impl<actor<T>, State, Data>::result_type result_type;
            };
        };
        
        template <typename T>
        struct is_nullary<custom_terminal<boost::reference_wrapper<actor<T> const> > >
        {
            BOOST_PROTO_TRANSFORM(is_nullary<custom_terminal<boost::reference_wrapper<actor<T> const> > >)
            template <typename Expr, typename State, typename Data>
            struct impl
            {
                typedef typename evaluator::template impl<actor<T> const, State, Data>::result_type result_type;
            };
        };
    }

}}

#endif

