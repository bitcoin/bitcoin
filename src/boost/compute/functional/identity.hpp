//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_IDENTITY_HPP
#define BOOST_COMPUTE_FUNCTIONAL_IDENTITY_HPP

namespace boost {
namespace compute {
namespace detail {

template<class T, class Arg>
struct invoked_identity
{
    typedef T result_type;

    invoked_identity(const Arg &arg)
        : m_arg(arg)
    {
    }

    Arg m_arg;
};

} // end detail namespace

/// Identity function which simply returns its input.
///
/// For example, to directly copy values using the transform() algorithm:
/// \code
/// transform(input.begin(), input.end(), output.begin(), identity<int>(), queue);
/// \endcode
///
/// \see \ref as "as<T>", \ref convert "convert<T>"
template<class T>
class identity
{
public:
    /// Identity function result type.
    typedef T result_type;

    /// Creates a new identity function.
    identity()
    {
    }

    /// \internal_
    template<class Arg>
    detail::invoked_identity<T, Arg> operator()(const Arg &arg) const
    {
        return detail::invoked_identity<T, Arg>(arg);
    }
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_IDENTITY_HPP
