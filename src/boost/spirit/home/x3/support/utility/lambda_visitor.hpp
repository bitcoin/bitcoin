/*=============================================================================
    Copyright (c) 2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_LAMBDA_VISITOR_MAY_19_2014_1116AM)
#define BOOST_SPIRIT_X3_LAMBDA_VISITOR_MAY_19_2014_1116AM

namespace boost { namespace spirit { namespace x3
{
    template <typename RT, typename... Lambdas>
    struct lambda_visitor;

    template <typename RT, typename F, typename... Lambdas>
    struct lambda_visitor<RT, F, Lambdas...> : F, lambda_visitor<RT, Lambdas...>
    {
        typedef lambda_visitor<RT , Lambdas...> base_type;
        using F::operator();
        using base_type::operator();
        lambda_visitor(F f, Lambdas... lambdas)
          : F(f), base_type(lambdas...)
        {}
    };

    template <typename RT, typename F>
    struct lambda_visitor<RT, F> : F
    {
        typedef RT result_type;
        using F::operator();
        lambda_visitor(F f)
          : F(f)
        {}
    };

    template <typename RT>
    struct lambda_visitor<RT>
    {
        typedef RT result_type;
    };

    template <typename RT, typename... Lambdas>
    lambda_visitor<RT, Lambdas...> make_lambda_visitor(Lambdas... lambdas)
    {
        return { lambdas... };
    }
}}}

#endif
