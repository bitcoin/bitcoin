//  mac/chrono.cpp  --------------------------------------------------------------//

//  Copyright Beman Dawes 2008
//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


//----------------------------------------------------------------------------//
//                                 Mac                                        //
//----------------------------------------------------------------------------//

#include <sys/time.h> //for gettimeofday and timeval
#include <mach/mach_time.h>  // mach_absolute_time, mach_timebase_info_data_t
#include <boost/assert.hpp>

namespace boost
{
namespace chrono
{

// system_clock

// gettimeofday is the most precise "system time" available on this platform.
// It returns the number of microseconds since New Years 1970 in a struct called timeval
// which has a field for seconds and a field for microseconds.
//    Fill in the timeval and then convert that to the time_point
system_clock::time_point
system_clock::now() BOOST_NOEXCEPT
{
    timeval tv;
    gettimeofday(&tv, 0);
    return time_point(seconds(tv.tv_sec) + microseconds(tv.tv_usec));
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
system_clock::time_point
system_clock::now(system::error_code & ec)
{
    timeval tv;
    gettimeofday(&tv, 0);
    if (!::boost::chrono::is_throws(ec))
    {
        ec.clear();
    }
    return time_point(seconds(tv.tv_sec) + microseconds(tv.tv_usec));
}
#endif
// Take advantage of the fact that on this platform time_t is nothing but
//    an integral count of seconds since New Years 1970 (same epoch as timeval).
//    Just get the duration out of the time_point and truncate it to seconds.
time_t
system_clock::to_time_t(const time_point& t) BOOST_NOEXCEPT
{
    return time_t(duration_cast<seconds>(t.time_since_epoch()).count());
}

// Just turn the time_t into a count of seconds and construct a time_point with it.
system_clock::time_point
system_clock::from_time_t(time_t t) BOOST_NOEXCEPT
{
    return system_clock::time_point(seconds(t));
}

namespace chrono_detail
{

// steady_clock

// Note, in this implementation steady_clock and high_resolution_clock
//   are the same clock.  They are both based on mach_absolute_time().
//   mach_absolute_time() * MachInfo.numer / MachInfo.denom is the number of
//   nanoseconds since the computer booted up.  MachInfo.numer and MachInfo.denom
//   are run time constants supplied by the OS.  This clock has no relationship
//   to the Gregorian calendar.  It's main use is as a high resolution timer.

// MachInfo.numer / MachInfo.denom is often 1 on the latest equipment.  Specialize
//   for that case as an optimization.
BOOST_CHRONO_STATIC
steady_clock::rep
steady_simplified()
{
    return mach_absolute_time();
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
BOOST_CHRONO_STATIC
steady_clock::rep
steady_simplified_ec(system::error_code & ec)
{
    if (!::boost::chrono::is_throws(ec))
    {
        ec.clear();
    }
    return mach_absolute_time();
}
#endif

BOOST_CHRONO_STATIC
double
compute_steady_factor(kern_return_t& err)
{
    mach_timebase_info_data_t MachInfo;
    err = mach_timebase_info(&MachInfo);
    if ( err != 0  ) {
        return 0;
    }
    return static_cast<double>(MachInfo.numer) / MachInfo.denom;
}

BOOST_CHRONO_STATIC
steady_clock::rep
steady_full()
{
    kern_return_t err;
    const double factor = chrono_detail::compute_steady_factor(err);
    if (err != 0)
    {
      BOOST_ASSERT(0 && "Boost::Chrono - Internal Error");
    }
    return static_cast<steady_clock::rep>(mach_absolute_time() * factor);
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
BOOST_CHRONO_STATIC
steady_clock::rep
steady_full_ec(system::error_code & ec)
{
    kern_return_t err;
    const double factor = chrono_detail::compute_steady_factor(err);
    if (err != 0)
    {
        if (::boost::chrono::is_throws(ec))
        {
            boost::throw_exception(
                    system::system_error(
                            err,
                            ::boost::system::system_category(),
                            "chrono::steady_clock" ));
        }
        else
        {
            ec.assign( errno, ::boost::system::system_category() );
            return steady_clock::rep();
        }
    }
    if (!::boost::chrono::is_throws(ec))
    {
        ec.clear();
    }
    return static_cast<steady_clock::rep>(mach_absolute_time() * factor);
}
#endif

typedef steady_clock::rep (*FP)();
#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
typedef steady_clock::rep (*FP_ec)(system::error_code &);
#endif

BOOST_CHRONO_STATIC
FP
init_steady_clock(kern_return_t & err)
{
    mach_timebase_info_data_t MachInfo;
    err = mach_timebase_info(&MachInfo);
    if ( err != 0  )
    {
        return 0;
    }

    if (MachInfo.numer == MachInfo.denom)
    {
        return &chrono_detail::steady_simplified;
    }
    return &chrono_detail::steady_full;
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
BOOST_CHRONO_STATIC
FP_ec
init_steady_clock_ec(kern_return_t & err)
{
    mach_timebase_info_data_t MachInfo;
    err = mach_timebase_info(&MachInfo);
    if ( err != 0  )
    {
        return 0;
    }

    if (MachInfo.numer == MachInfo.denom)
    {
        return &chrono_detail::steady_simplified_ec;
    }
    return &chrono_detail::steady_full_ec;
}
#endif
}

steady_clock::time_point
steady_clock::now() BOOST_NOEXCEPT
{
    kern_return_t err;
    chrono_detail::FP fp = chrono_detail::init_steady_clock(err);
    if ( err != 0  )
    {
      BOOST_ASSERT(0 && "Boost::Chrono - Internal Error");
    }
    return time_point(duration(fp()));
}

#if !defined BOOST_CHRONO_DONT_PROVIDE_HYBRID_ERROR_HANDLING
steady_clock::time_point
steady_clock::now(system::error_code & ec)
{
    kern_return_t err;
    chrono_detail::FP_ec fp = chrono_detail::init_steady_clock_ec(err);
    if ( err != 0  )
    {
        if (::boost::chrono::is_throws(ec))
        {
            boost::throw_exception(
                    system::system_error(
                            err,
                            ::boost::system::system_category(),
                            "chrono::steady_clock" ));
        }
        else
        {
            ec.assign( err, ::boost::system::system_category() );
            return time_point();
        }
    }
    if (!::boost::chrono::is_throws(ec))
    {
        ec.clear();
    }
    return time_point(duration(fp(ec)));
}
#endif
}  // namespace chrono
}  // namespace boost
