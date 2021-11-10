#ifndef DATE_TIME_TIME_DURATION_HPP___
#define DATE_TIME_TIME_DURATION_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <boost/core/enable_if.hpp>
#include <boost/cstdint.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/time_defs.hpp>
#include <boost/operators.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_integral.hpp>

namespace boost {
namespace date_time {


  //! Represents some amount of elapsed time measure to a given resolution
  /*! This class represents a standard set of capabilities for all
      counted time durations.  Time duration implementations should derive
      from this class passing their type as the first template parameter.
      This design allows the subclass duration types to provide custom
      construction policies or other custom features not provided here.

      @tparam T The subclass type
      @tparam rep_type The time resolution traits for this duration type.
  */
  template<class T, typename rep_type>
  class BOOST_SYMBOL_VISIBLE time_duration : private
      boost::less_than_comparable<T
    , boost::equality_comparable<T
    > >
  /* dividable, addable, and subtractable operator templates
   * won't work with this class (MSVC++ 6.0). return type
   * from '+=' is different than expected return type
   * from '+'. multipliable probably wont work
   * either (haven't tried) */
  {
  public:
    // A tag for type categorization. Can be used to detect Boost.DateTime duration types in generic code.
    typedef void _is_boost_date_time_duration;
    typedef T duration_type;  //the subclass
    typedef rep_type traits_type;
    typedef typename rep_type::day_type  day_type;
    typedef typename rep_type::hour_type hour_type;
    typedef typename rep_type::min_type  min_type;
    typedef typename rep_type::sec_type  sec_type;
    typedef typename rep_type::fractional_seconds_type fractional_seconds_type;
    typedef typename rep_type::tick_type tick_type;
    typedef typename rep_type::impl_type impl_type;

    BOOST_CXX14_CONSTEXPR time_duration() : ticks_(0) {}
    BOOST_CXX14_CONSTEXPR time_duration(hour_type hours_in,
                  min_type minutes_in,
                  sec_type seconds_in=0,
                  fractional_seconds_type frac_sec_in = 0) :
      ticks_(rep_type::to_tick_count(hours_in,minutes_in,seconds_in,frac_sec_in))
    {}
    //! Construct from special_values
    BOOST_CXX14_CONSTEXPR time_duration(special_values sv) : ticks_(impl_type::from_special(sv))
    {}
    //! Returns smallest representable duration
    static BOOST_CXX14_CONSTEXPR duration_type unit()
    {
      return duration_type(0,0,0,1);
    }
    //! Return the number of ticks in a second
    static BOOST_CXX14_CONSTEXPR tick_type ticks_per_second()
    {
      return rep_type::res_adjust();
    }
    //! Provide the resolution of this duration type
    static BOOST_CXX14_CONSTEXPR time_resolutions resolution()
    {
      return rep_type::resolution();
    }
    //! Returns number of hours in the duration
    BOOST_CXX14_CONSTEXPR hour_type hours()   const
    {
      return static_cast<hour_type>(ticks() / (3600*ticks_per_second()));
    }
    //! Returns normalized number of minutes
    BOOST_CXX14_CONSTEXPR min_type minutes() const
    {
      return static_cast<min_type>((ticks() / (60*ticks_per_second())) % 60);
    }
    //! Returns normalized number of seconds (0..60)
    BOOST_CXX14_CONSTEXPR sec_type seconds() const
    {
      return static_cast<sec_type>((ticks()/ticks_per_second()) % 60);
    }
    //! Returns total number of seconds truncating any fractional seconds
    BOOST_CXX14_CONSTEXPR sec_type total_seconds() const
    {
      return static_cast<sec_type>(ticks() / ticks_per_second());
    }
    //! Returns total number of milliseconds truncating any fractional seconds
    BOOST_CXX14_CONSTEXPR tick_type total_milliseconds() const
    {
      if (ticks_per_second() < 1000) {
        return ticks() * (static_cast<tick_type>(1000) / ticks_per_second());
      }
      return ticks() / (ticks_per_second() / static_cast<tick_type>(1000)) ;
    }
    //! Returns total number of nanoseconds truncating any sub millisecond values
    BOOST_CXX14_CONSTEXPR tick_type total_nanoseconds() const
    {
      if (ticks_per_second() < 1000000000) {
        return ticks() * (static_cast<tick_type>(1000000000) / ticks_per_second());
      }
      return ticks() / (ticks_per_second() / static_cast<tick_type>(1000000000)) ;
    }
    //! Returns total number of microseconds truncating any sub microsecond values
    BOOST_CXX14_CONSTEXPR tick_type total_microseconds() const
    {
      if (ticks_per_second() < 1000000) {
        return ticks() * (static_cast<tick_type>(1000000) / ticks_per_second());
      }
      return ticks() / (ticks_per_second() / static_cast<tick_type>(1000000)) ;
    }
    //! Returns count of fractional seconds at given resolution
    BOOST_CXX14_CONSTEXPR fractional_seconds_type fractional_seconds() const
    {
      return (ticks() % ticks_per_second());
    }
    //! Returns number of possible digits in fractional seconds
    static BOOST_CXX14_CONSTEXPR unsigned short num_fractional_digits()
    {
      return rep_type::num_fractional_digits();
    }
    BOOST_CXX14_CONSTEXPR duration_type invert_sign() const
    {
      return duration_type(ticks_ * (-1));
    }
    BOOST_CXX14_CONSTEXPR duration_type abs() const
    {
      if ( is_negative() )
      {
        return invert_sign();
      }
      return duration_type(ticks_);
    }
    BOOST_CONSTEXPR bool is_negative() const
    {
      return ticks_ < 0;
    }
    BOOST_CONSTEXPR bool is_zero() const
    {
      return ticks_ == 0;
    }
    BOOST_CONSTEXPR bool is_positive() const
    {
      return ticks_ > 0;
    }
    BOOST_CONSTEXPR bool operator<(const time_duration& rhs)  const
    {
      return ticks_ <  rhs.ticks_;
    }
    BOOST_CONSTEXPR bool operator==(const time_duration& rhs)  const
    {
      return ticks_ ==  rhs.ticks_;
    }
    //! unary- Allows for time_duration td = -td1
    BOOST_CONSTEXPR duration_type operator-()const
    {
      return duration_type(ticks_ * (-1));
    }
    BOOST_CONSTEXPR duration_type operator-(const duration_type& d) const
    {
      return duration_type(ticks_ - d.ticks_);
    }
    BOOST_CONSTEXPR duration_type operator+(const duration_type& d) const
    {
      return duration_type(ticks_ + d.ticks_);
    }
    BOOST_CONSTEXPR duration_type operator/(int divisor) const
    {
      return duration_type(ticks_ / divisor);
    }
    BOOST_CXX14_CONSTEXPR duration_type operator-=(const duration_type& d)
    {
      ticks_ = ticks_ - d.ticks_;
      return duration_type(ticks_);
    }
    BOOST_CXX14_CONSTEXPR duration_type operator+=(const duration_type& d)
    {
      ticks_ = ticks_ + d.ticks_;
      return duration_type(ticks_);
    }
    //! Division operations on a duration with an integer.
    BOOST_CXX14_CONSTEXPR duration_type operator/=(int divisor)
    {
      ticks_ = ticks_ / divisor;
      return duration_type(ticks_);
    }
    //! Multiplication operations an a duration with an integer
    BOOST_CXX14_CONSTEXPR duration_type operator*(int rhs) const
    {
      return duration_type(ticks_ * rhs);
    }
    BOOST_CXX14_CONSTEXPR duration_type operator*=(int divisor)
    {
      ticks_ = ticks_ * divisor;
      return duration_type(ticks_);
    }
    BOOST_CXX14_CONSTEXPR tick_type ticks() const
    {
      return traits_type::as_number(ticks_);
    }

    //! Is ticks_ a special value?
    BOOST_CXX14_CONSTEXPR bool is_special()const
    {
      if(traits_type::is_adapted())
      {
        return ticks_.is_special();
      }
      else{
        return false;
      }
    }
    //! Is duration pos-infinity
    BOOST_CXX14_CONSTEXPR bool is_pos_infinity()const
    {
      if(traits_type::is_adapted())
      {
        return ticks_.is_pos_infinity();
      }
      else{
        return false;
      }
    }
    //! Is duration neg-infinity
    BOOST_CXX14_CONSTEXPR bool is_neg_infinity()const
    {
      if(traits_type::is_adapted())
      {
        return ticks_.is_neg_infinity();
      }
      else{
        return false;
      }
    }
    //! Is duration not-a-date-time
    BOOST_CXX14_CONSTEXPR bool is_not_a_date_time()const
    {
      if(traits_type::is_adapted())
      {
        return ticks_.is_nan();
      }
      else{
        return false;
      }
    }

    //! Used for special_values output
    BOOST_CONSTEXPR impl_type get_rep()const
    {
      return ticks_;
    }

  protected:
    BOOST_CXX14_CONSTEXPR explicit time_duration(impl_type in) : ticks_(in) {}
    impl_type ticks_;
  };



  //! Template for instantiating derived adjusting durations
  /* These templates are designed to work with multiples of
   * 10 for frac_of_second and resolution adjustment
   */
  template<class base_duration, boost::int64_t frac_of_second>
  class BOOST_SYMBOL_VISIBLE subsecond_duration : public base_duration
  {
  public:
    typedef typename base_duration::impl_type impl_type;
    typedef typename base_duration::traits_type traits_type;

  private:
    // To avoid integer overflow we precompute the duration resolution conversion coefficient (ticket #3471)
    BOOST_STATIC_ASSERT_MSG((traits_type::ticks_per_second >= frac_of_second ? traits_type::ticks_per_second % frac_of_second : frac_of_second % traits_type::ticks_per_second) == 0,\
      "The base duration resolution must be a multiple of the subsecond duration resolution");
    BOOST_STATIC_CONSTANT(boost::int64_t, adjustment_ratio = (traits_type::ticks_per_second >= frac_of_second ? traits_type::ticks_per_second / frac_of_second : frac_of_second / traits_type::ticks_per_second));

  public:
    // The argument (ss) must be an integral type
    template <typename T>
    BOOST_CXX14_CONSTEXPR explicit subsecond_duration(T const& ss,
                                                      typename boost::enable_if<boost::is_integral<T>,
                                                        void>::type* = BOOST_DATE_TIME_NULLPTR) :
      base_duration(impl_type(traits_type::ticks_per_second >= frac_of_second ? ss * adjustment_ratio : ss / adjustment_ratio))
    {
    }
  };

} } //namespace date_time




#endif

