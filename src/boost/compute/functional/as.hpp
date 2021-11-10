//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_AS_HPP
#define BOOST_COMPUTE_FUNCTIONAL_AS_HPP

namespace boost {
namespace compute {
namespace detail {

template<class T, class Arg>
struct invoked_as
{
    invoked_as(const Arg &arg)
        : m_arg(arg)
    {
    }

    Arg m_arg;
};

} // end detail namespace

/// The \ref as function converts its argument to type \c T (similar to
/// reinterpret_cast<T>).
///
/// \see \ref convert "convert<T>"
template<class T>
struct as
{
    typedef T result_type;

    /// \internal_
    template<class Arg>
    detail::invoked_as<T, Arg> operator()(const Arg &arg) const
    {
        return detail::invoked_as<T, Arg>(arg);
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_AS_HPP
