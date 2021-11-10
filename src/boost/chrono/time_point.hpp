//  duration.hpp  --------------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009-2012 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*

This code was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.9 Time utilities [time] of the C++ committee's working paper N2798.
See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2798.pdf.

time2_demo contained this comment:

    Much thanks to Andrei Alexandrescu,
                   Walter Brown,
                   Peter Dimov,
                   Jeff Garland,
                   Terry Golubiewski,
                   Daniel Krugler,
                   Anthony Williams.
*/


#ifndef BOOST_CHRONO_TIME_POINT_HPP
#define BOOST_CHRONO_TIME_POINT_HPP

#include <boost/chrono/duration.hpp>

#ifndef BOOST_CHRONO_HEADER_ONLY
// this must occur after all of the includes and before any code appears:
#include <boost/config/abi_prefix.hpp> // must be the last #include
#endif

//----------------------------------------------------------------------------//
//                                                                            //
//                        20.9 Time utilities [time]                          //
//                                 synopsis                                   //
//                                                                            //
//----------------------------------------------------------------------------//

namespace boost {
namespace chrono {

  template <class Clock, class Duration = typename Clock::duration>
    class time_point;


} // namespace chrono


// common_type trait specializations

template <class Clock, class Duration1, class Duration2>
  struct common_type<chrono::time_point<Clock, Duration1>,
                     chrono::time_point<Clock, Duration2> >;


//----------------------------------------------------------------------------//
//      20.9.2.3 Specializations of common_type [time.traits.specializations] //
//----------------------------------------------------------------------------//


template <class Clock, class Duration1, class Duration2>
struct common_type<chrono::time_point<Clock, Duration1>,
                   chrono::time_point<Clock, Duration2> >
{
  typedef chrono::time_point<Clock,
    typename common_type<Duration1, Duration2>::type> type;
};



namespace chrono {

    // time_point arithmetic
    template <class Clock, class Duration1, class Rep2, class Period2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<Duration1, duration<Rep2, Period2> >::type>
    operator+(
            const time_point<Clock, Duration1>& lhs,
            const duration<Rep2, Period2>& rhs);
    template <class Rep1, class Period1, class Clock, class Duration2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<duration<Rep1, Period1>, Duration2>::type>
    operator+(
            const duration<Rep1, Period1>& lhs,
            const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Rep2, class Period2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<Duration1, duration<Rep2, Period2> >::type>
    operator-(
            const time_point<Clock, Duration1>& lhs,
            const duration<Rep2, Period2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    typename common_type<Duration1, Duration2>::type
    operator-(
            const time_point<Clock, Duration1>& lhs,
            const time_point<Clock,
            Duration2>& rhs);

    // time_point comparisons
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator==(
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator!=(
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator< (
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator<=(
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator> (
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);
    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool operator>=(
          const time_point<Clock, Duration1>& lhs,
          const time_point<Clock, Duration2>& rhs);

    // time_point_cast
    template <class ToDuration, class Clock, class Duration>
    inline BOOST_CONSTEXPR
    time_point<Clock, ToDuration> time_point_cast(const time_point<Clock, Duration>& t);

//----------------------------------------------------------------------------//
//                                                                            //
//      20.9.4 Class template time_point [time.point]                         //
//                                                                            //
//----------------------------------------------------------------------------//

    template <class Clock, class Duration>
    class time_point
    {
        BOOST_CHRONO_STATIC_ASSERT(boost::chrono::detail::is_duration<Duration>::value,
                BOOST_CHRONO_SECOND_TEMPLATE_PARAMETER_OF_TIME_POINT_MUST_BE_A_BOOST_CHRONO_DURATION, (Duration));
    public:
        typedef Clock                     clock;
        typedef Duration                  duration;
        typedef typename duration::rep    rep;
        typedef typename duration::period period;
        typedef Duration                  difference_type;

    private:
        duration d_;

    public:
        BOOST_FORCEINLINE BOOST_CONSTEXPR
        time_point() : d_(duration::zero())
        {}
        BOOST_FORCEINLINE BOOST_CONSTEXPR
        explicit time_point(const duration& d)
            : d_(d)
        {}

        // conversions
        template <class Duration2>
        BOOST_FORCEINLINE BOOST_CONSTEXPR
        time_point(const time_point<clock, Duration2>& t
                , typename boost::enable_if
                <
                    boost::is_convertible<Duration2, duration>
                >::type* = 0
        )
            : d_(t.time_since_epoch())
        {
        }
        // observer

        BOOST_CONSTEXPR
        duration time_since_epoch() const
        {
            return d_;
        }

        // arithmetic

#ifdef BOOST_CHRONO_EXTENSIONS
        BOOST_CONSTEXPR
        time_point  operator+() const {return *this;}
        BOOST_CONSTEXPR
        time_point  operator-() const {return time_point(-d_);}
        time_point& operator++()      {++d_; return *this;}
        time_point  operator++(int)   {return time_point(d_++);}
        time_point& operator--()      {--d_; return *this;}
        time_point  operator--(int)   {return time_point(d_--);}

        time_point& operator+=(const rep& r) {d_ += duration(r); return *this;}
        time_point& operator-=(const rep& r) {d_ -= duration(r); return *this;}

#endif

        time_point& operator+=(const duration& d) {d_ += d; return *this;}
        time_point& operator-=(const duration& d) {d_ -= d; return *this;}

        // special values

        static BOOST_CHRONO_LIB_CONSTEXPR time_point
        min BOOST_PREVENT_MACRO_SUBSTITUTION ()
        {
            return time_point((duration::min)());
        }
        static BOOST_CHRONO_LIB_CONSTEXPR time_point
        max BOOST_PREVENT_MACRO_SUBSTITUTION ()
        {
            return time_point((duration::max)());
        }
    };

//----------------------------------------------------------------------------//
//      20.9.4.5 time_point non-member arithmetic [time.point.nonmember]      //
//----------------------------------------------------------------------------//

    // time_point operator+(time_point x, duration y);

    template <class Clock, class Duration1, class Rep2, class Period2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<Duration1, duration<Rep2, Period2> >::type>
    operator+(const time_point<Clock, Duration1>& lhs,
            const duration<Rep2, Period2>& rhs)
    {
      typedef typename common_type<Duration1, duration<Rep2, Period2> >::type CDuration;
      typedef time_point<
          Clock,
          CDuration
      > TimeResult;
        return TimeResult(lhs.time_since_epoch() + CDuration(rhs));
    }

    // time_point operator+(duration x, time_point y);

    template <class Rep1, class Period1, class Clock, class Duration2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<duration<Rep1, Period1>, Duration2>::type>
    operator+(const duration<Rep1, Period1>& lhs,
            const time_point<Clock, Duration2>& rhs)
    {
        return rhs + lhs;
    }

    // time_point operator-(time_point x, duration y);

    template <class Clock, class Duration1, class Rep2, class Period2>
    inline BOOST_CONSTEXPR
    time_point<Clock,
        typename common_type<Duration1, duration<Rep2, Period2> >::type>
    operator-(const time_point<Clock, Duration1>& lhs,
            const duration<Rep2, Period2>& rhs)
    {
        return lhs + (-rhs);
    }

    // duration operator-(time_point x, time_point y);

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    typename common_type<Duration1, Duration2>::type
    operator-(const time_point<Clock, Duration1>& lhs,
            const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() - rhs.time_since_epoch();
    }

//----------------------------------------------------------------------------//
//      20.9.4.6 time_point comparisons [time.point.comparisons]              //
//----------------------------------------------------------------------------//

    // time_point ==

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator==(const time_point<Clock, Duration1>& lhs,
             const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() == rhs.time_since_epoch();
    }

    // time_point !=

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator!=(const time_point<Clock, Duration1>& lhs,
             const time_point<Clock, Duration2>& rhs)
    {
        return !(lhs == rhs);
    }

    // time_point <

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator<(const time_point<Clock, Duration1>& lhs,
            const time_point<Clock, Duration2>& rhs)
    {
        return lhs.time_since_epoch() < rhs.time_since_epoch();
    }

    // time_point >

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator>(const time_point<Clock, Duration1>& lhs,
            const time_point<Clock, Duration2>& rhs)
    {
        return rhs < lhs;
    }

    // time_point <=

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator<=(const time_point<Clock, Duration1>& lhs,
             const time_point<Clock, Duration2>& rhs)
    {
        return !(rhs < lhs);
    }

    // time_point >=

    template <class Clock, class Duration1, class Duration2>
    inline BOOST_CONSTEXPR
    bool
    operator>=(const time_point<Clock, Duration1>& lhs,
             const time_point<Clock, Duration2>& rhs)
    {
        return !(lhs < rhs);
    }

//----------------------------------------------------------------------------//
//      20.9.4.7 time_point_cast [time.point.cast]                            //
//----------------------------------------------------------------------------//

    template <class ToDuration, class Clock, class Duration>
    inline BOOST_CONSTEXPR
    time_point<Clock, ToDuration>
    time_point_cast(const time_point<Clock, Duration>& t)
    {
        return time_point<Clock, ToDuration>(
                duration_cast<ToDuration>(t.time_since_epoch()));
    }

} // namespace chrono
} // namespace boost

#ifndef BOOST_CHRONO_HEADER_ONLY
// the suffix header occurs after all of our code:
#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#endif

#endif // BOOST_CHRONO_TIME_POINT_HPP
