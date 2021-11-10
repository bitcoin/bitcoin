//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_LAMBDA_FUNCTIONAL_HPP
#define BOOST_COMPUTE_LAMBDA_FUNCTIONAL_HPP

#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/proto/core.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

#include <boost/compute/functional/get.hpp>
#include <boost/compute/lambda/result_of.hpp>
#include <boost/compute/lambda/placeholder.hpp>

#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/type_traits/scalar_type.hpp>
#include <boost/compute/type_traits/vector_size.hpp>
#include <boost/compute/type_traits/make_vector_type.hpp>

namespace boost {
namespace compute {
namespace lambda {

namespace mpl = boost::mpl;
namespace proto = boost::proto;

// wraps a unary boolean function whose result type is an int_ when the argument
// type is a scalar, and intN_ if the argument type is a vector of size N
#define BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg; \
                typedef typename ::boost::compute::lambda::result_of<Arg, Args>::type result_type; \
                typedef typename ::boost::compute::make_vector_type< \
                    ::boost::compute::int_, \
                    ::boost::compute::vector_size<result_type>::value \
                >::type type; \
            }; \
            \
            template<class Context, class Arg> \
            static void apply(Context &ctx, const Arg &arg) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg& \
    >::type const \
    name(const Arg &arg) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg) \
        ); \
    }

// wraps a unary function whose return type is the same as the argument type
#define BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::lambda::result_of<Arg1, Args>::type type; \
            }; \
            \
            template<class Context, class Arg> \
            static void apply(Context &ctx, const Arg &arg) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg& \
    >::type const \
    name(const Arg &arg) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg) \
        ); \
    }

// wraps a unary function whose result type is the scalar type of the first argument
#define BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_ST(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg; \
                typedef typename ::boost::compute::lambda::result_of<Arg, Args>::type result_type; \
                typedef typename ::boost::compute::scalar_type<result_type>::type type; \
            }; \
            \
            template<class Context, class Arg> \
            static void apply(Context &ctx, const Arg &arg) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg& \
    >::type const \
    name(const Arg &arg) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg) \
        ); \
    }

// wraps a binary boolean function whose result type is an int_ when the first
// argument type is a scalar, and intN_ if the first argument type is a vector
// of size N
#define BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::make_vector_type< \
                    ::boost::compute::int_, \
                    ::boost::compute::vector_size<Arg1>::value \
                >::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2) \
        ); \
    }

// wraps a binary function whose result type is the type of the first argument
#define BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::lambda::result_of<Arg1, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2) \
        ); \
    }

// wraps a binary function whose result type is the type of the second argument
#define BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_2(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 2>::type Arg2; \
                typedef typename ::boost::compute::lambda::result_of<Arg2, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2) \
        ); \
    }

// wraps a binary function who's result type is the scalar type of the first argument
#define BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_ST(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::lambda::result_of<Arg1, Args>::type result_type; \
                typedef typename ::boost::compute::scalar_type<result_type>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2) \
        ); \
    }

// wraps a binary function whose result type is the type of the first argument
// and the second argument is a pointer
#define BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_PTR(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::lambda::result_of<Arg1, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", &"; \
                proto::eval(arg2, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2) \
        ); \
    }

// wraps a ternary function
#define BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 1>::type Arg1; \
                typedef typename ::boost::compute::lambda::result_of<Arg1, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2, class Arg3> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg3, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2, class Arg3> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2&, const Arg3& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2), ::boost::ref(arg3) \
        ); \
    }

// wraps a ternary function whose result type is the type of the third argument
#define BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION_3(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 3>::type Arg3; \
                typedef typename ::boost::compute::lambda::result_of<Arg3, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2, class Arg3> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg3, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2, class Arg3> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2&, const Arg3& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2), ::boost::ref(arg3) \
        ); \
    }

// wraps a ternary function whose result type is the type of the first argument
// and the third argument of the function is a pointer
#define BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION_PTR(name) \
    namespace detail { \
        struct BOOST_PP_CAT(name, _func) \
        { \
            template<class Expr, class Args> \
            struct lambda_result \
            { \
                typedef typename proto::result_of::child_c<Expr, 3>::type Arg3; \
                typedef typename ::boost::compute::lambda::result_of<Arg3, Args>::type type; \
            }; \
            \
            template<class Context, class Arg1, class Arg2, class Arg3> \
            static void apply(Context &ctx, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
            { \
                ctx.stream << #name << "("; \
                proto::eval(arg1, ctx); \
                ctx.stream << ", "; \
                proto::eval(arg2, ctx); \
                ctx.stream << ", &"; \
                proto::eval(arg3, ctx); \
                ctx.stream << ")"; \
            } \
        }; \
    } \
    template<class Arg1, class Arg2, class Arg3> \
    inline typename proto::result_of::make_expr< \
        proto::tag::function, BOOST_PP_CAT(detail::name, _func), const Arg1&, const Arg2&, const Arg3& \
    >::type const \
    name(const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3) \
    { \
        return proto::make_expr<proto::tag::function>( \
            BOOST_PP_CAT(detail::name, _func)(), ::boost::ref(arg1), ::boost::ref(arg2), ::boost::ref(arg3) \
        ); \
    }

// Common Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(clamp)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(degrees)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(min)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(max)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(mix)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(radians)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sign)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_2(step)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION_3(smoothstep)

// Geometric Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(cross)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_ST(dot)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_ST(distance)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_ST(length)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(normalize)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_ST(fast_distance)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_ST(fast_length)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(fast_normalize)

// Integer Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(abs)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(abs_diff)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(add_sat)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(hadd)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(rhadd)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(clz)
#ifdef BOOST_COMPUTE_CL_VERSION_2_0
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(ctz)
#endif
// clamp() (since 1.1) already defined in common
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(mad_hi)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(mad24)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(mad_sat)
// max() and min() functions are defined in common
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(mul_hi)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(mul24)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(rotate)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(sub_sat)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(upsample)
#ifdef BOOST_COMPUTE_CL_VERSION_1_2
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(popcount)
#endif

// Math Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(acos)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(acosh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(acospi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(asin)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(asinh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(asinpi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(atan)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(atan2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(atanh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(atanpi)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(atan2pi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(cbrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(ceil)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(copysign)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(cos)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(cosh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(cospi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(erfc)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(erf)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(exp)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(exp2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(exp10)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(expm1)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(fabs)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(fdim)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(floor)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(fma)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(fmax)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(fmin)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(fmod)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_PTR(fract)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_PTR(frexp)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(hypot)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(ilogb) // ilogb returns intN_
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(ldexp)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(lgamma)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_PTR(lgamma_r)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(log)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(log2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(log10)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(log1p)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(logb)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(mad)
#ifdef BOOST_COMPUTE_CL_VERSION_1_1
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(maxmag)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(minmag)
#endif
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION_PTR(modf)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(nan)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(nextafter)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(pow)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(pown)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(powr)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(remainder)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION_PTR(remquo)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(rint)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(rootn)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(round)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(rsqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sin)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sincos)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sinh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sinpi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(sqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(tan)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(tanh)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(tanpi)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(tgamma)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(trunc)

// Native Math Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_cos)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(native_divide)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_exp)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_exp2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_exp10)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_log)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_log2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_log10)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(native_powr)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_recip)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_rsqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_sin)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_sqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(native_tan)

// Half Math Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_cos)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(half_divide)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_exp)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_exp2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_exp10)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_log)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_log2)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_log10)
BOOST_COMPUTE_LAMBDA_WRAP_BINARY_FUNCTION(half_powr)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_recip)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_rsqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_sin)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_sqrt)
BOOST_COMPUTE_LAMBDA_WRAP_UNARY_FUNCTION_T(half_tan)

// Relational Built-In Functions
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isequal)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isnotequal)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isgreater)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isgreaterequal)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isless)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(islessequal)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(islessgreater)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(isfinite)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(isinf)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(isnan)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(isnormal)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isordered)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_BINARY_FUNCTION(isunordered)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(singbit)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(all)
BOOST_COMPUTE_LAMBDA_WRAP_BOOLEAN_UNARY_FUNCTION(any)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(bitselect)
BOOST_COMPUTE_LAMBDA_WRAP_TERNARY_FUNCTION(select)

} // end lambda namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_LAMBDA_FUNCTIONAL_HPP
