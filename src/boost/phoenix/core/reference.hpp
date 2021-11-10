/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_REFERENCE_HPP
#define BOOST_PHOENIX_CORE_REFERENCE_HPP

#include <boost/phoenix/core/limits.hpp>
#include <boost/ref.hpp>
#include <boost/phoenix/core/actor.hpp>
#include <boost/phoenix/core/terminal.hpp>
#include <boost/utility/result_of.hpp>

namespace boost { namespace phoenix
{
    /////////////////////////////////////////////////////////////////////////////
    //
    // reference
    //
    //      function for evaluating references, e.g. ref(123)
    //
    /////////////////////////////////////////////////////////////////////////////
    namespace expression
    {
        template <typename T>
        struct reference
            : expression::terminal<reference_wrapper<T> >
        {
            typedef
                typename expression::terminal<reference_wrapper<T> >::type
                type;

            static const type make(T & t)
            {
                typename reference<T>::type const e = {{boost::ref(t)}};
                return e;
            }
        };
        
        template <typename T>
        struct reference<T const>
            : expression::terminal<reference_wrapper<T const> >
        {
            typedef
                typename expression::terminal<reference_wrapper<T const> >::type
                type;

            static const type make(T const & t)
            {
                typename reference<T const>::type const e = {{boost::cref(t)}};
                return e;
            }
        };
    }

    namespace rule
    {
        struct reference
            : expression::reference<proto::_>
        {};
    }

    template <typename T>
    inline
    typename expression::reference<T>::type const
    ref(T & t)
    {
        return expression::reference<T>::make(t);
    }

    template <typename T>
    inline
    typename expression::reference<T const>::type const
    cref(T const & t)
    {
        return expression::reference<T const>::make(t);
    }

    // Call out boost::reference_wrapper for special handling
    template<typename T>
    struct is_custom_terminal<boost::reference_wrapper<T> >
      : mpl::true_
    {};

    // Special handling for boost::reference_wrapper
    template<typename T>
    struct custom_terminal<boost::reference_wrapper<T> >
    {
        typedef T &result_type;

        template <typename Context>
        T &operator()(boost::reference_wrapper<T> r, Context &) const
        {
            return r;
        }
    };
    
    template<typename Expr>
    struct custom_terminal<boost::reference_wrapper<actor<Expr> > >
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Context>
        struct result<This(boost::reference_wrapper<actor<Expr> > const &, Context)>
            : boost::result_of<evaluator(actor<Expr> &, Context)>
        {};

        template <typename This, typename Context>
        struct result<This(boost::reference_wrapper<actor<Expr> > &, Context)>
            : boost::result_of<evaluator(actor<Expr> &, Context)>
        {};

        template <typename Context>
        typename boost::result_of<evaluator(actor<Expr> &, Context const &)>::type
        operator()(boost::reference_wrapper<actor<Expr> > & r, Context const & ctx) const
        {
            return boost::phoenix::eval(r, ctx);
        }
    };
    
    template<typename Expr>
    struct custom_terminal<boost::reference_wrapper<actor<Expr> const> >
    {
        template <typename Sig>
        struct result;

        template <typename This, typename Context>
        struct result<This(boost::reference_wrapper<actor<Expr> const> const &, Context)>
            : boost::result_of<evaluator(actor<Expr> const&, Context)>
        {};

        template <typename This, typename Context>
        struct result<This(boost::reference_wrapper<actor<Expr> const> &, Context)>
            : boost::result_of<evaluator(actor<Expr> const&, Context)>
        {};

        template <typename Context>
        typename boost::result_of<evaluator(actor<Expr> const&, Context const &)>::type
        operator()(boost::reference_wrapper<actor<Expr> const> const & r, Context & ctx) const
        {
            return boost::phoenix::eval(unwrap_ref(r), ctx);
        }
    };
}}

#endif
