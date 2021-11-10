// Copyright (C) 2001-2003
// Mac Murrett
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_FORCE_CAST_MJM012402_HPP
#define BOOST_FORCE_CAST_MJM012402_HPP

#include <boost/thread/detail/config.hpp>

namespace boost {
namespace detail {
namespace thread {

// force_cast will convert anything to anything.

// general case
template<class Return_Type, class Argument_Type>
inline Return_Type &force_cast(Argument_Type &rSrc)
{
    return(*reinterpret_cast<Return_Type *>(&rSrc));
}

// specialization for const
template<class Return_Type, class Argument_Type>
inline const Return_Type &force_cast(const Argument_Type &rSrc)
{
    return(*reinterpret_cast<const Return_Type *>(&rSrc));
}

} // namespace thread
} // namespace detail
} // namespace boost

#endif // BOOST_FORCE_CAST_MJM012402_HPP
