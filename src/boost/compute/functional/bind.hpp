//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_BIND_HPP
#define BOOST_COMPUTE_FUNCTIONAL_BIND_HPP

#include <boost/mpl/int.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/conditional.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/detail/meta_kernel.hpp>

namespace boost {
namespace compute {
namespace placeholders {

/// \internal_
template<int I>
struct placeholder : boost::integral_constant<int, I>
{
    placeholder() { }
};

placeholder<0> const _1;
placeholder<1> const _2;

} // end placeholders namespace

/// Meta-function returning \c true if \c T is a placeholder type.
template<class T>
struct is_placeholder : boost::false_type
{
};

/// \internal_
template<int I>
struct is_placeholder<placeholders::placeholder<I> > : boost::true_type
{
};

namespace detail {

template<class Function, class BoundArgs, class Args>
struct invoked_bound_function
{
    invoked_bound_function(Function f, BoundArgs bound_args, Args args)
        : m_function(f),
          m_bound_args(bound_args),
          m_args(args)
    {
    }

    // meta-function returning true if the N'th argument is a placeholder
    template<int N>
    struct is_placeholder_arg
    {
        typedef typename boost::tuples::element<N, BoundArgs>::type nth_bound_arg;

        typedef typename is_placeholder<nth_bound_arg>::type type;
        static const bool value = is_placeholder<nth_bound_arg>::value;
    };

    template<class Arg>
    struct get_arg_type
    {
        typedef Arg type;
    };

    template<int I>
    struct get_arg_type<placeholders::placeholder<I> >
    {
        typedef typename boost::tuples::element<I, Args>::type type;
    };

    // meta-function returning the type of the N'th argument when invoked
    template<int N>
    struct get_nth_arg_type
    {
        typedef typename boost::tuples::element<N, BoundArgs>::type nth_bound_arg;

        typedef typename get_arg_type<nth_bound_arg>::type type;
    };

    template<int N>
    typename get_nth_arg_type<N>::type get_nth_arg(
        typename boost::enable_if_c<is_placeholder_arg<N>::value>::type* = 0
    ) const
    {
        typedef typename boost::tuples::element<N, BoundArgs>::type nth_bound_arg;

        return boost::get<nth_bound_arg::value>(m_args);
    }

    template<int N>
    typename get_nth_arg_type<N>::type get_nth_arg(
        typename boost::disable_if_c<is_placeholder_arg<N>::value>::type* = 0
    ) const
    {
        return boost::get<N>(m_bound_args);
    }

    Function m_function;
    BoundArgs m_bound_args;
    Args m_args;
};

template<class Function, class BoundArgs, class Args>
inline meta_kernel& apply_invoked_bound_function(
    meta_kernel &k,
    const invoked_bound_function<Function, BoundArgs, Args> &expr,
    typename boost::enable_if_c<
        boost::tuples::length<BoundArgs>::value == 1
    >::type* = 0
)
{
    return k << expr.m_function(expr.template get_nth_arg<0>());
}

template<class Function, class BoundArgs, class Args>
inline meta_kernel& apply_invoked_bound_function(
    meta_kernel &k,
    const invoked_bound_function<Function, BoundArgs, Args> &expr,
    typename boost::enable_if_c<
        boost::tuples::length<BoundArgs>::value == 2
    >::type* = 0
)
{
    return k << expr.m_function(expr.template get_nth_arg<0>(),
                                expr.template get_nth_arg<1>());
}

template<class Function, class BoundArgs, class Args>
inline meta_kernel& apply_invoked_bound_function(
    meta_kernel &k,
    const invoked_bound_function<Function, BoundArgs, Args> &expr,
    typename boost::enable_if_c<
        boost::tuples::length<BoundArgs>::value == 3
    >::type* = 0
)
{
    return k << expr.m_function(expr.template get_nth_arg<0>(),
                                expr.template get_nth_arg<1>(),
                                expr.template get_nth_arg<2>());
}

template<class Function, class BoundArgs, class Args>
inline meta_kernel& operator<<(
    meta_kernel &k,
    const invoked_bound_function<Function, BoundArgs, Args> &expr
)
{
    return apply_invoked_bound_function(k, expr);
}

template<class Function, class BoundArgs>
struct bound_function
{
    typedef int result_type;

    bound_function(Function f, BoundArgs args)
        : m_function(f),
          m_args(args)
    {
    }

    template<class Arg1>
    detail::invoked_bound_function<
        Function,
        BoundArgs,
        boost::tuple<Arg1>
    >
    operator()(const Arg1 &arg1) const
    {
        return detail::invoked_bound_function<
                   Function,
                   BoundArgs,
                   boost::tuple<Arg1>
               >(m_function, m_args, boost::make_tuple(arg1));
    }

    template<class Arg1, class Arg2>
    detail::invoked_bound_function<
        Function,
        BoundArgs,
        boost::tuple<Arg1, Arg2>
    >
    operator()(const Arg1 &arg1, const Arg2 &arg2) const
    {
        return detail::invoked_bound_function<
                   Function,
                   BoundArgs,
                   boost::tuple<Arg1, Arg2>
               >(m_function, m_args, boost::make_tuple(arg1, arg2));
    }

    Function m_function;
    BoundArgs m_args;
};

} // end detail namespace

#if !defined(BOOST_COMPUTE_NO_VARIADIC_TEMPLATES) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
/// Returns a function wrapper which invokes \p f with \p args when called.
///
/// For example, to generate a unary function object which returns \c true
/// when its argument is less than \c 7:
/// \code
/// using boost::compute::less;
/// using boost::compute::placeholders::_1;
///
/// auto less_than_seven = boost::compute::bind(less<int>(), _1, 7);
/// \endcode
template<class F, class... Args>
inline detail::bound_function<F, boost::tuple<Args...> >
bind(F f, Args... args)
{
    typedef typename boost::tuple<Args...> ArgsTuple;

    return detail::bound_function<F, ArgsTuple>(f, boost::make_tuple(args...));
}
#else
template<class F, class A1>
inline detail::bound_function<F, boost::tuple<A1> >
bind(F f, A1 a1)
{
    typedef typename boost::tuple<A1> Args;

    return detail::bound_function<F, Args>(f, boost::make_tuple(a1));
}

template<class F, class A1, class A2>
inline detail::bound_function<F, boost::tuple<A1, A2> >
bind(F f, A1 a1, A2 a2)
{
    typedef typename boost::tuple<A1, A2> Args;

    return detail::bound_function<F, Args>(f, boost::make_tuple(a1, a2));
}

template<class F, class A1, class A2, class A3>
inline detail::bound_function<F, boost::tuple<A1, A2, A3> >
bind(F f, A1 a1, A2 a2, A3 a3)
{
    typedef typename boost::tuple<A1, A2, A3> Args;

    return detail::bound_function<F, Args>(f, boost::make_tuple(a1, a2, a3));
}
#endif // BOOST_COMPUTE_NO_VARIADIC_TEMPLATES

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_BIND_HPP
