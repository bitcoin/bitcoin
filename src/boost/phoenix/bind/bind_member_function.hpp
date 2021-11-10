/*=============================================================================
    Copyright (c) 2016 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_BIND_BIND_MEMBER_FUNCTION_HPP
#define PHOENIX_BIND_BIND_MEMBER_FUNCTION_HPP

#include <boost/phoenix/core/limits.hpp>

#if defined(BOOST_PHOENIX_NO_VARIADIC_BIND)
# include <boost/phoenix/bind/detail/cpp03/bind_member_function.hpp>
#else

#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/core/detail/function_eval.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <typename RT, typename FP>
        struct member_function_ptr
        {
            typedef RT result_type;

            member_function_ptr(FP fp_)
                : fp(fp_) {}

            template <typename Class, typename... A>
            result_type operator()(Class& obj, A&... a) const
            {
                BOOST_PROTO_USE_GET_POINTER();

                typedef typename proto::detail::class_member_traits<FP>::class_type class_type;
                return (BOOST_PROTO_GET_POINTER(class_type, obj)->*fp)(a...);
            }

            template <typename Class, typename... A>
            result_type operator()(Class* obj, A&... a) const
            {
                return (obj->*fp)(a...);
            }

            bool operator==(member_function_ptr const& rhs) const
            {
                return fp == rhs.fp;
            }

            template <int M, typename RhsRT, typename RhsFP>
            bool operator==(member_function_ptr<RhsRT, RhsFP> const& /*rhs*/) const
            {
                return false;
            }

            FP fp;
        };
    } // namespace boost::phoenix::detail

    template <typename RT, typename ClassT, typename... T, typename ClassA, typename... A>
    inline
    typename detail::expression::function_eval<
        detail::member_function_ptr<RT, RT(ClassT::*)(T...)>
      , ClassA
      , A...
    >::type const
    bind(RT (ClassT::*f)(T...), ClassA const & obj, A const&... a)
    {
        typedef detail::member_function_ptr<RT, RT (ClassT::*)(T...)> fp_type;
        return detail::expression::function_eval<fp_type, ClassA, A...>::make(fp_type(f), obj, a...);
    }

    template <typename RT, typename ClassT, typename... T, typename ClassA, typename... A>
    inline
    typename detail::expression::function_eval<
        detail::member_function_ptr<RT, RT (ClassT::*)(T...) const>
      , ClassA
      , A...
    >::type const
    bind(RT (ClassT::*f)(T...) const, ClassA const & obj, A const&... a)
    {
        typedef detail::member_function_ptr<RT, RT(ClassT::*)(T...) const> fp_type;
        return detail::expression::function_eval<fp_type, ClassA, A...>::make(fp_type(f), obj, a...);
    }

    template <typename RT, typename ClassT, typename... T, typename... A>
    inline
    typename detail::expression::function_eval<
        detail::member_function_ptr<RT, RT(ClassT::*)(T...)>
      , ClassT
      , A...
    >::type const
    bind(RT (ClassT::*f)(T...), ClassT & obj, A const&... a)
    {
        typedef detail::member_function_ptr<RT, RT(ClassT::*)(T...)> fp_type;
        return detail::expression::function_eval<fp_type, ClassT, A...>::make(fp_type(f), obj, a...);
    }

    template <typename RT, typename ClassT, typename... T, typename... A>
    inline
    typename detail::expression::function_eval<
        detail::member_function_ptr<RT, RT(ClassT::*)(T...) const>
      , ClassT
      , A...
    >::type const
    bind(RT (ClassT::*f)(T...) const, ClassT const& obj, A const&... a)
    {
        typedef detail::member_function_ptr<RT, RT(ClassT::*)(T...) const> fp_type;
        return detail::expression::function_eval<fp_type, ClassT, A...>::make(fp_type(f), obj, a...);
    }
}} // namespace boost::phoenix

#endif
#endif
