//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_TYPES_COMPLEX_HPP
#define BOOST_COMPUTE_TYPES_COMPLEX_HPP

#include <complex>

#include <boost/compute/functional.hpp>
#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/type_traits/make_vector_type.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/detail/meta_kernel.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class T>
meta_kernel& operator<<(meta_kernel &kernel, const std::complex<T> &x)
{
    typedef typename std::complex<T> value_type;

    kernel << "(" << type_name<value_type>() << ")"
           << "(" << x.real() << ", " << x.imag() << ")";

    return kernel;
}

// get<N>() result type specialization for std::complex<>
template<size_t N, class T>
struct get_result_type<N, std::complex<T> >
{
    typedef T type;
};

// get<N>() specialization for std::complex<>
template<size_t N, class Arg, class T>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_get<N, Arg, std::complex<T> > &expr)
{
    BOOST_STATIC_ASSERT(N < 2);

    return kernel << expr.m_arg << (N == 0 ? ".x" : ".y");
}

} // end detail namespace

// returns the real component of a complex<T>
template<class T>
struct real
{
    typedef T result_type;

    template<class Arg>
    detail::invoked_get<0, Arg, std::complex<T> >
    operator()(const Arg &x) const
    {
        return detail::invoked_get<0, Arg, std::complex<T> >(x);
    }
};

// returns the imaginary component of a complex<T>
template<class T>
struct imag
{
    typedef T result_type;

    template<class Arg>
    detail::invoked_get<1, Arg, std::complex<T> >
    operator()(const Arg &x) const
    {
        return detail::invoked_get<1, Arg, std::complex<T> >(x);
    }
};

namespace detail {

template<class Arg1, class Arg2, class T>
struct invoked_complex_multiplies
{
    typedef typename std::complex<T> result_type;

    invoked_complex_multiplies(const Arg1 &x, const Arg2 &y)
        : m_x(x),
          m_y(y)
    {
    }

    Arg1 m_x;
    Arg2 m_y;
};

template<class Arg1, class Arg2, class T>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_complex_multiplies<Arg1, Arg2, T> &expr)
{
    typedef typename std::complex<T> value_type;

    kernel << "(" << type_name<value_type>() << ")"
           << "(" << expr.m_x << ".x*" << expr.m_y << ".x-"
                  << expr.m_x << ".y*" << expr.m_y << ".y,"
                  << expr.m_x << ".y*" << expr.m_y << ".x+"
                  << expr.m_x << ".x*" << expr.m_y << ".y" << ")";

    return kernel;
}

template<class Arg, class T>
struct invoked_complex_conj
{
    typedef typename std::complex<T> result_type;

    invoked_complex_conj(const Arg &arg)
        : m_arg(arg)
    {
    }

    Arg m_arg;
};

template<class Arg, class T>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const invoked_complex_conj<Arg, T> &expr)
{
    typedef typename std::complex<T> value_type;

    kernel << "(" << type_name<value_type>() << ")"
           << "(" << expr.m_arg << ".x" << ", -" << expr.m_arg << ".y" << ")";

    return kernel;
}

} // end detail namespace

// specialization for multiplies<T>
template<class T>
class multiplies<std::complex<T> > :
    public function<std::complex<T> (std::complex<T>, std::complex<T>)>
{
public:
    multiplies() :
        function<
            std::complex<T> (std::complex<T>, std::complex<T>)
        >("complex_multiplies")
    {
    }

    template<class Arg1, class Arg2>
    detail::invoked_complex_multiplies<Arg1, Arg2, T>
    operator()(const Arg1 &x, const Arg2 &y) const
    {
        return detail::invoked_complex_multiplies<Arg1, Arg2, T>(x, y);
    }
};

// returns the complex conjugate of a complex<T>
template<class T>
struct conj
{
    typedef typename std::complex<T> result_type;

    template<class Arg>
    detail::invoked_complex_conj<Arg, T>
    operator()(const Arg &x) const
    {
        return detail::invoked_complex_conj<Arg, T>(x);
    }
};

namespace detail {

// type_name() specialization for std::complex
template<class T>
struct type_name_trait<std::complex<T> >
{
    static const char* value()
    {
        typedef typename make_vector_type<T, 2>::type vector_type;

        return type_name<vector_type>();
    }
};

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_TYPES_COMPLEX_HPP
