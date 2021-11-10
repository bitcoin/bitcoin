///////////////////////////////////////////////////////////////////////////////
/// \file functional_fwd.hpp
///
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_NUMERIC_FUNCTIONAL_FWD_HPP_EAN_08_12_2005
#define BOOST_NUMERIC_FUNCTIONAL_FWD_HPP_EAN_08_12_2005

#include <boost/mpl/if.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_const.hpp>

namespace boost { namespace numeric
{
    // For using directives -- this namespace may be re-opened elsewhere
    namespace operators
    {}

    namespace op
    {
        using mpl::_;
        using mpl::_1;
        using mpl::_2;
    }

    namespace functional
    {
        using namespace operators;

        template<typename T>
        struct tag
        {
            typedef void type;
        };

        template<typename T>
        struct tag<T const>
          : tag<T>
        {};

        template<typename T>
        struct tag<T volatile>
          : tag<T>
        {};

        template<typename T>
        struct tag<T const volatile>
          : tag<T>
        {};

        template<typename T>
        struct static_;

        template<typename A0, typename A1>
        struct are_integral;
    }

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP(Name, Op)                                     \
    namespace functional                                                                        \
    {                                                                                           \
        template<typename Arg, typename EnableIf = void>                                        \
        struct Name ## _base;                                                                   \
        template<typename Arg, typename ArgTag = typename tag<Arg>::type>                       \
        struct Name;                                                                            \
    }                                                                                           \
    namespace op                                                                                \
    {                                                                                           \
        struct Name;                                                                            \
    }                                                                                           \
    namespace                                                                                   \
    {                                                                                           \
        extern op::Name const &Name;                                                            \
    }

    /// INTERNAL ONLY
    ///
#define BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(Name)                                        \
    namespace functional                                                                        \
    {                                                                                           \
        template<typename Left, typename Right, typename EnableIf = void>                       \
        struct result_of_ ## Name;                                                              \
        template<typename Left, typename Right, typename EnableIf = void>                       \
        struct Name ## _base;                                                                   \
        template<                                                                               \
            typename Left                                                                       \
          , typename Right                                                                      \
          , typename LeftTag = typename tag<Left>::type                                         \
          , typename RightTag = typename tag<Right>::type                                       \
        >                                                                                       \
        struct Name;                                                                            \
    }                                                                                           \
    namespace op                                                                                \
    {                                                                                           \
        struct Name;                                                                            \
    }                                                                                           \
    namespace                                                                                   \
    {                                                                                           \
        extern op::Name const &Name;                                                            \
    }

    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(plus)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(minus)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(multiplies)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(divides)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(modulus)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(greater)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(greater_equal)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(less)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(less_equal)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(equal_to)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(not_equal_to)

    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(assign)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(plus_assign)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(minus_assign)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(multiplies_assign)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(divides_assign)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP(modulus_assign)

    BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP(unary_plus, +)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP(unary_minus, -)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP(complement, ~)
    BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP(logical_not, !)

#undef BOOST_NUMERIC_FUNCTIONAL_DECLARE_UNARY_OP
#undef BOOST_NUMERIC_FUNCTIONAL_DECLARE_BINARY_OP


    namespace functional
    {
        template<typename To, typename From, typename EnableIf = void>
        struct promote_base;
        template<typename Left, typename Right, typename EnableIf = void>
        struct min_assign_base;
        template<typename Left, typename Right, typename EnableIf = void>
        struct max_assign_base;
        template<typename Left, typename Right, typename EnableIf = void>
        struct fdiv_base;
        template<typename Arg, typename EnableIf = void>
        struct as_min_base;
        template<typename Arg, typename EnableIf = void>
        struct as_max_base;
        template<typename Arg, typename EnableIf = void>
        struct as_zero_base;
        template<typename Arg, typename EnableIf = void>
        struct as_one_base;

        template<typename To, typename From, typename ToTag = typename tag<To>::type, typename FromTag = typename tag<From>::type>
        struct promote;
        template<typename Left, typename Right, typename LeftTag = typename tag<Left>::type, typename RightTag = typename tag<Right>::type>
        struct min_assign;
        template<typename Left, typename Right, typename LeftTag = typename tag<Left>::type, typename RightTag = typename tag<Right>::type>
        struct max_assign;
        template<typename Left, typename Right, typename LeftTag = typename tag<Left>::type, typename RightTag = typename tag<Right>::type>
        struct fdiv;
        template<typename Arg, typename Tag = typename tag<Arg>::type>
        struct as_min;
        template<typename Arg, typename Tag = typename tag<Arg>::type>
        struct as_max;
        template<typename Arg, typename Tag = typename tag<Arg>::type>
        struct as_zero;
        template<typename Arg, typename Tag = typename tag<Arg>::type>
        struct as_one;
    }

    namespace op
    {
        template<typename To>
        struct promote;
        struct min_assign;
        struct max_assign;
        struct fdiv;
        struct as_min;
        struct as_max;
        struct as_zero;
        struct as_one;
    }

    namespace
    {
        extern op::min_assign const &min_assign;
        extern op::max_assign const &max_assign;
        extern op::fdiv const &fdiv;
        extern op::as_min const &as_min;
        extern op::as_max const &as_max;
        extern op::as_zero const &as_zero;
        extern op::as_one const &as_one;
    }

    template<typename To, typename From>
    typename lazy_disable_if<is_const<From>, mpl::if_<is_same<To, From>, To &, To> >::type
    promote(From &from);

    template<typename To, typename From>
    typename mpl::if_<is_same<To const, From const>, To const &, To const>::type
    promote(From const &from);

    template<typename T>
    struct default_;

    template<typename T>
    struct one;

    template<typename T>
    struct zero;

    template<typename T>
    struct one_or_default;

    template<typename T>
    struct zero_or_default;

}} // namespace boost::numeric

#endif
