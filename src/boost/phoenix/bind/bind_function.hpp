/*=============================================================================
    Copyright (c) 2016 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_BIND_BIND_FUNCTION_HPP
#define PHOENIX_BIND_BIND_FUNCTION_HPP

#include <boost/phoenix/core/limits.hpp>

#if defined(BOOST_PHOENIX_NO_VARIADIC_BIND)
# include <boost/phoenix/bind/detail/cpp03/bind_function.hpp>
#else

#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/detail/function_eval.hpp>

namespace boost { namespace phoenix
{
    namespace detail
    {
        template <typename RT, typename FP>
        struct function_ptr
        {
            typedef RT result_type;

            function_ptr(FP fp_)
                : fp(fp_) {}

            template <typename... A>
            result_type operator()(A&... a) const
            {
                return fp(a...);
            }

            bool operator==(function_ptr const& rhs) const
            {
                return fp == rhs.fp;
            }

            template <typename RhsRT, typename RhsFP>
            bool operator==(function_ptr<RhsRT, RhsFP> const& /*rhs*/) const
            {
                return false;
            }

            FP fp;
        };
    } // namespace boost::phoenix::detail

    template <typename RT, typename... T, typename... A>
    inline typename detail::expression::function_eval<
        detail::function_ptr<RT, RT (*)(T...)>
      , A...
    >::type const
    bind(RT (*f)(T...), A const&... a)
    {
        typedef detail::function_ptr<RT, RT (*)(T...)> fp_type;
        return detail::expression::function_eval<fp_type, A...>::make(fp_type(f), a...);
    }
}} // namespace boost::phoenix

#endif
#endif
