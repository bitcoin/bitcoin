#ifndef DATE_TIME_DATE_DURATION__
#define DATE_TIME_DATE_DURATION__

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


#include <boost/operators.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/int_adapter.hpp>

namespace boost {
namespace date_time {


  //! Duration type with date level resolution
  template<class duration_rep_traits>
  class BOOST_SYMBOL_VISIBLE date_duration : private
              boost::less_than_comparable1< date_duration< duration_rep_traits >
            , boost::equality_comparable1< date_duration< duration_rep_traits >
            , boost::addable1< date_duration< duration_rep_traits >
            , boost::subtractable1< date_duration< duration_rep_traits >
            , boost::dividable2< date_duration< duration_rep_traits >, int
            > > > > >
  {
  public:
    typedef typename duration_rep_traits::int_type duration_rep_type;
    typedef typename duration_rep_traits::impl_type duration_rep;

    //! Construct from a day count
    BOOST_CXX14_CONSTEXPR explicit date_duration(duration_rep day_count) : days_(day_count) {}

    /*! construct from special_values - only works when
     * instantiated with duration_traits_adapted */
    BOOST_CXX14_CONSTEXPR date_duration(special_values sv) :
            days_(duration_rep::from_special(sv))
    {}

    //! returns days_ as it's instantiated type - used for streaming
    BOOST_CXX14_CONSTEXPR duration_rep get_rep()const
    {
        return days_;
    }
    BOOST_CXX14_CONSTEXPR special_values as_special() const
    {
        return days_.as_special();
    }
    BOOST_CXX14_CONSTEXPR bool is_special()const
    {
        return days_.is_special();
    }
    //! returns days as value, not object.
    BOOST_CXX14_CONSTEXPR duration_rep_type days() const
    {
        return duration_rep_traits::as_number(days_);
    }
    //! Returns the smallest duration -- used by to calculate 'end'
    static BOOST_CXX14_CONSTEXPR date_duration unit()
    {
        return date_duration<duration_rep_traits>(1);
    }
    //! Equality
    BOOST_CXX14_CONSTEXPR bool operator==(const date_duration& rhs) const
    {
        return days_ == rhs.days_;
    }
    //! Less
    BOOST_CXX14_CONSTEXPR bool operator<(const date_duration& rhs) const
    {
        return days_ < rhs.days_;
    }

    /* For shortcut operators (+=, -=, etc) simply using
     * "days_ += days_" may not work. If instantiated with
     * an int_adapter, shortcut operators are not present,
     * so this will not compile */

    //! Subtract another duration -- result is signed
    BOOST_CXX14_CONSTEXPR date_duration& operator-=(const date_duration& rhs)
    {
        //days_ -= rhs.days_;
        days_ = days_ - rhs.days_;
        return *this;
    }
    //! Add a duration -- result is signed
    BOOST_CXX14_CONSTEXPR date_duration& operator+=(const date_duration& rhs)
    {
        days_ = days_ + rhs.days_;
        return *this;
    }

    //! unary- Allows for dd = -date_duration(2); -> dd == -2
    BOOST_CXX14_CONSTEXPR date_duration operator-() const
    {
        return date_duration<duration_rep_traits>(get_rep() * (-1));
    }
    //! Division operations on a duration with an integer.
    BOOST_CXX14_CONSTEXPR date_duration& operator/=(int divisor)
    {
        days_ = days_ / divisor;
        return *this;
    }

    //! return sign information
    BOOST_CXX14_CONSTEXPR bool is_negative() const
    {
        return days_ < 0;
    }

  private:
    duration_rep days_;
  };


  /*! Struct for instantiating date_duration with <b>NO</b> special values
   * functionality. Allows for transparent implementation of either
   * date_duration<long> or date_duration<int_adapter<long> > */
  struct BOOST_SYMBOL_VISIBLE duration_traits_long
  {
    typedef long int_type;
    typedef long impl_type;
    static BOOST_CXX14_CONSTEXPR int_type as_number(impl_type i) { return i; }
  };

  /*! Struct for instantiating date_duration <b>WITH</b> special values
   * functionality. Allows for transparent implementation of either
   * date_duration<long> or date_duration<int_adapter<long> > */
  struct BOOST_SYMBOL_VISIBLE duration_traits_adapted
  {
    typedef long int_type;
    typedef boost::date_time::int_adapter<long> impl_type;
    static BOOST_CXX14_CONSTEXPR int_type as_number(impl_type i) { return i.as_number(); }
  };


} } //namspace date_time


#endif

