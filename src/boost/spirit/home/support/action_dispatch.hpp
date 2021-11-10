/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ACTION_DISPATCH_APRIL_18_2008_0720AM)
#define BOOST_SPIRIT_ACTION_DISPATCH_APRIL_18_2008_0720AM

#if defined(_MSC_VER)
#pragma once
#endif

#include<boost/config.hpp>

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_LAMBDAS) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_DECLTYPE)
#include <utility>
#include <type_traits>
#endif


#include <boost/spirit/home/support/attributes.hpp>

namespace boost { namespace phoenix
{
    template <typename Expr>
    struct actor;
}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Component>
    struct action_dispatch
    {
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_LAMBDAS) && \
    !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_DECLTYPE)
        // omit function parameters without specializing for each possible
        // type of callable entity
        // many thanks to Eelis/##iso-c++ for this contribution

    private:
        // this will be used to pass around POD types which are safe
        // to go through the ellipsis operator (if ever used)
        template <typename>
        struct fwd_tag {};

        // the first parameter is a placeholder to obtain SFINAE when
        // doing overload resolution, the second one is the actual
        // forwarder, where we can apply our implementation
        template <typename, typename T>
        struct fwd_storage { typedef T type; };

        // gcc should accept fake<T>() but it prints a sorry, needs
        // a check once the bug is sorted out, use a FAKE_CALL macro for now
        template <typename T>
        T fake_call();

#define BOOST_SPIRIT_FAKE_CALL(T) (*(T*)0)

        // the forwarders, here we could tweak the implementation of
        // how parameters are passed to the functions, if needed
        struct fwd_none
        {
            template<typename F, typename... Rest>
            auto operator()(F && f, Rest&&...) -> decltype(f())
            {
                return f();
            }
        };

        struct fwd_attrib
        {
            template<typename F, typename A, typename... Rest>
            auto operator()(F && f, A && a, Rest&&...) -> decltype(f(a))
            {
                 return f(a);
            }
        };

        struct fwd_attrib_context
        {
             template<typename F, typename A, typename B, typename... Rest>
             auto operator()(F && f, A && a, B && b, Rest&&...)
                -> decltype(f(a, b))
             {
                 return f(a, b);
             }
        };

        struct fwd_attrib_context_pass
        {
            template<typename F, typename A, typename B, typename C
              , typename... Rest>
            auto operator()(F && f, A && a, B && b, C && c, Rest&&...)
               -> decltype(f(a, b, c))
            {
                return f(a, b, c);
            }
        };

        // SFINAE for our calling syntax, the forwarders are stored based
        // on what function call gives a proper result
        // this code can probably be more generic once implementations are
        // steady
        template <typename F>
        static auto do_call(F && f, ...)
           -> typename fwd_storage<decltype(f()), fwd_none>::type
        {
            return {};
        }

        template <typename F, typename A>
        static auto do_call(F && f, fwd_tag<A>, ...)
           -> typename fwd_storage<decltype(f(BOOST_SPIRIT_FAKE_CALL(A)))
                 , fwd_attrib>::type
        {
            return {};
        }

        template <typename F, typename A, typename B>
        static auto do_call(F && f, fwd_tag<A>, fwd_tag<B>, ...)
           -> typename fwd_storage<
                    decltype(f(BOOST_SPIRIT_FAKE_CALL(A), BOOST_SPIRIT_FAKE_CALL(B)))
                , fwd_attrib_context>::type
        {
            return {};
        }

        template <typename F, typename A, typename B, typename C>
        static auto do_call(F && f, fwd_tag<A>, fwd_tag<B>, fwd_tag<C>, ...)
           -> typename fwd_storage<
                  decltype(f(BOOST_SPIRIT_FAKE_CALL(A), BOOST_SPIRIT_FAKE_CALL(B)
                    , BOOST_SPIRIT_FAKE_CALL(C)))
                , fwd_attrib_context_pass>::type
        {
            return {};
        }

        // this function calls the forwarder and is responsible for
        // stripping the tail of the parameters
        template <typename F, typename... A>
        static void caller(F && f, A && ... a)
        {
            do_call(f, fwd_tag<typename std::remove_reference<A>::type>()...)
                (std::forward<F>(f), std::forward<A>(a)...);
        }

#undef BOOST_SPIRIT_FAKE_CALL

    public:
        template <typename F, typename Attribute, typename Context>
        bool operator()(F const& f, Attribute& attr, Context& context)
        {
            bool pass = true;
            caller(f, attr, context, pass);
            return pass;
        }
#else
        // general handler for everything not explicitly specialized below
        template <typename F, typename Attribute, typename Context>
        bool operator()(F const& f, Attribute& attr, Context& context)
        {
            bool pass = true;
            f(attr, context, pass);
            return pass;
        }
#endif

        // handler for phoenix actors

        // If the component this action has to be invoked for is a tuple, we
        // wrap any non-fusion tuple into a fusion tuple (done by pass_attribute)
        // and pass through any fusion tuple.
        template <typename Eval, typename Attribute, typename Context>
        bool operator()(phoenix::actor<Eval> const& f
          , Attribute& attr, Context& context)
        {
            bool pass = true;
            typename pass_attribute<Component, Attribute>::type attr_wrap(attr);
            f(attr_wrap, context, pass);
            return pass;
        }

        // specializations for plain function pointers taking different number of
        // arguments
        template <typename RT, typename A0, typename A1, typename A2
          , typename Attribute, typename Context>
        bool operator()(RT(*f)(A0, A1, A2), Attribute& attr, Context& context)
        {
            bool pass = true;
            f(attr, context, pass);
            return pass;
        }

        template <typename RT, typename A0, typename A1
          , typename Attribute, typename Context>
        bool operator()(RT(*f)(A0, A1), Attribute& attr, Context& context)
        {
            f(attr, context);
            return true;
        }

        template <typename RT, typename A0, typename Attribute, typename Context>
        bool operator()(RT(*f)(A0), Attribute& attr, Context&)
        {
            f(attr);
            return true;
        }

        template <typename RT, typename Attribute, typename Context>
        bool operator()(RT(*f)(), Attribute&, Context&)
        {
            f();
            return true;
        }
    };
}}}

#endif
