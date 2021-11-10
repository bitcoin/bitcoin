//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_DURATION_HPP
#define BOOST_COMPUTE_DETAIL_DURATION_HPP

#include <boost/config.hpp>

#ifndef BOOST_COMPUTE_NO_HDR_CHRONO
#include <chrono>
#endif

#ifndef BOOST_COMPUTE_NO_BOOST_CHRONO
#include <boost/chrono/duration.hpp>
#endif

namespace boost {
namespace compute {
namespace detail {

#ifndef BOOST_COMPUTE_NO_HDR_CHRONO
template<class Rep, class Period>
inline std::chrono::duration<Rep, Period>
make_duration_from_nanoseconds(std::chrono::duration<Rep, Period>, size_t nanoseconds)
{
    return std::chrono::duration_cast<std::chrono::duration<Rep, Period> >(
        std::chrono::nanoseconds(nanoseconds)
    );
}
#endif // BOOST_COMPUTE_NO_HDR_CHRONO

#ifndef BOOST_COMPUTE_NO_BOOST_CHRONO
template<class Rep, class Period>
inline boost::chrono::duration<Rep, Period>
make_duration_from_nanoseconds(boost::chrono::duration<Rep, Period>, size_t nanoseconds)
{
    return boost::chrono::duration_cast<boost::chrono::duration<Rep, Period> >(
        boost::chrono::nanoseconds(nanoseconds)
    );
}
#endif // BOOST_COMPUTE_NO_BOOST_CHRONO

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_DURATION_HPP
