//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_DETAIL_UNPACK_HPP
#define BOOST_COMPUTE_FUNCTIONAL_DETAIL_UNPACK_HPP

#include <boost/compute/functional/get.hpp>
#include <boost/compute/type_traits/is_vector_type.hpp>
#include <boost/compute/type_traits/result_of.hpp>
#include <boost/compute/type_traits/vector_size.hpp>
#include <boost/compute/detail/meta_kernel.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Function, class Arg, size_t Arity>
struct invoked_unpacked
{
    invoked_unpacked(const Function &f, const Arg &arg)
        : m_function(f),
          m_arg(arg)
    {
    }

    Function m_function;
    Arg m_arg;
};

template<class Function, class Arg, size_t Arity>
inline meta_kernel& operator<<(meta_kernel &k, const invoked_unpacked<Function, Arg, Arity> &expr);

template<class Function, class Arg>
inline meta_kernel& operator<<(meta_kernel &k, const invoked_unpacked<Function, Arg, 1> &expr)
{
    return k << expr.m_function(get<0>()(expr.m_arg));
}

template<class Function, class Arg>
inline meta_kernel& operator<<(meta_kernel &k, const invoked_unpacked<Function, Arg, 2> &expr)
{
    return k << expr.m_function(get<0>()(expr.m_arg), get<1>()(expr.m_arg));
}

template<class Function, class Arg>
inline meta_kernel& operator<<(meta_kernel &k, const invoked_unpacked<Function, Arg, 3> &expr)
{
    return k << expr.m_function(get<0>()(expr.m_arg), get<1>()(expr.m_arg), get<2>()(expr.m_arg));
}

template<class Function>
struct unpacked
{
    template<class T, class Enable = void>
    struct aggregate_length
    {
        BOOST_STATIC_CONSTANT(size_t, value = boost::tuples::length<T>::value);
    };

    template<class T>
    struct aggregate_length<T, typename enable_if<is_vector_type<T> >::type>
    {
        BOOST_STATIC_CONSTANT(size_t, value = vector_size<T>::value);
    };

    template<class TupleArg, size_t TupleSize>
    struct result_impl {};

    template<class TupleArg>
    struct result_impl<TupleArg, 1>
    {
        typedef typename detail::get_result_type<0, TupleArg>::type T1;

        typedef typename boost::compute::result_of<Function(T1)>::type type;
    };

    template<class TupleArg>
    struct result_impl<TupleArg, 2>
    {
        typedef typename detail::get_result_type<0, TupleArg>::type T1;
        typedef typename detail::get_result_type<1, TupleArg>::type T2;

        typedef typename boost::compute::result_of<Function(T1, T2)>::type type;
    };

    template<class TupleArg>
    struct result_impl<TupleArg, 3>
    {
        typedef typename detail::get_result_type<0, TupleArg>::type T1;
        typedef typename detail::get_result_type<1, TupleArg>::type T2;
        typedef typename detail::get_result_type<2, TupleArg>::type T3;

        typedef typename boost::compute::result_of<Function(T1, T2, T3)>::type type;
    };

    template<class Signature>
    struct result {};

    template<class This, class Arg>
    struct result<This(Arg)>
    {
        typedef typename result_impl<Arg, aggregate_length<Arg>::value>::type type;
    };

    unpacked(const Function &f)
        : m_function(f)
    {
    }

    template<class Arg>
    detail::invoked_unpacked<
        Function, Arg, aggregate_length<typename Arg::result_type>::value
    >
    operator()(const Arg &arg) const
    {
        return detail::invoked_unpacked<
                   Function,
                   Arg,
                   aggregate_length<typename Arg::result_type>::value
                >(m_function, arg);
    }

    Function m_function;
};

template<class Function>
inline unpacked<Function> unpack(const Function &f)
{
    return unpacked<Function>(f);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_DETAIL_UNPACK_HPP
