#ifndef DATE_DURATION_TYPES_HPP___
#define DATE_DURATION_TYPES_HPP___

/* Copyright (c) 2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or 
 * http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/int_adapter.hpp>
#include <boost/date_time/special_defs.hpp>
#include <boost/date_time/date_duration.hpp>

namespace boost {
namespace date_time {


  //! Additional duration type that represents a number of n*7 days
  template <class duration_config>
  class BOOST_SYMBOL_VISIBLE weeks_duration : public date_duration<duration_config> {
  public:
    BOOST_CXX14_CONSTEXPR weeks_duration(typename duration_config::impl_type w) 
      : date_duration<duration_config>(w * 7) {}
    BOOST_CXX14_CONSTEXPR weeks_duration(special_values sv) 
      : date_duration<duration_config>(sv) {}
  };

  // predeclare
  template<class t>
  class BOOST_SYMBOL_VISIBLE years_duration;

  //! additional duration type that represents a logical month
  /*! A logical month enables things like: "date(2002,Mar,2) + months(2) -> 
   * 2002-May2". If the date is a last day-of-the-month, the result will 
   * also be a last-day-of-the-month.
   */
  template<class base_config>
  class BOOST_SYMBOL_VISIBLE months_duration
  {
    private:
      typedef typename base_config::int_rep int_rep;
      typedef typename int_rep::int_type int_type;
      typedef typename base_config::date_type date_type;
      typedef typename date_type::duration_type duration_type;
      typedef typename base_config::month_adjustor_type month_adjustor_type;
      typedef months_duration<base_config> months_type;
      typedef years_duration<base_config> years_type;
    public:
      BOOST_CXX14_CONSTEXPR months_duration(int_rep num) : _m(num) {}
      BOOST_CXX14_CONSTEXPR months_duration(special_values sv) : _m(sv) 
      {
        _m = int_rep::from_special(sv);
      }
      int_rep number_of_months() const { return _m; }
      //! returns a negative duration
      BOOST_CXX14_CONSTEXPR duration_type get_neg_offset(const date_type& d) const
      {
        month_adjustor_type m_adj(_m.as_number());
        return duration_type(m_adj.get_neg_offset(d));
      }
      BOOST_CXX14_CONSTEXPR duration_type get_offset(const date_type& d) const
      {
        month_adjustor_type m_adj(_m.as_number());
        return duration_type(m_adj.get_offset(d));
      }
      BOOST_CONSTEXPR bool operator==(const months_type& rhs) const
      {
        return(_m == rhs._m);
      }
      BOOST_CONSTEXPR bool operator!=(const months_type& rhs) const
      {
        return(_m != rhs._m);
      }
      BOOST_CXX14_CONSTEXPR months_type operator+(const months_type& rhs)const
      {
        return months_type(_m + rhs._m);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator+=(const months_type& rhs)
      {
        _m = _m + rhs._m;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator-(const months_type& rhs)const
      {
        return months_type(_m - rhs._m);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator-=(const months_type& rhs)
      {
        _m = _m - rhs._m;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator*(const int_type rhs)const
      {
        return months_type(_m * rhs);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator*=(const int_type rhs)
      {
        _m = _m * rhs;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator/(const int_type rhs)const
      {
        return months_type(_m / rhs);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator/=(const int_type rhs)
      {
        _m = _m / rhs;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator+(const years_type& y)const
      {
        return months_type(y.number_of_years() * 12 + _m);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator+=(const years_type& y)
      {
        _m = y.number_of_years() * 12 + _m;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator-(const years_type& y) const
      {
        return months_type(_m - y.number_of_years() * 12);
      }
      BOOST_CXX14_CONSTEXPR months_type& operator-=(const years_type& y)
      {
        _m = _m - y.number_of_years() * 12;
        return *this;
      }
      //
      BOOST_CXX14_CONSTEXPR friend date_type operator+(const date_type& d, const months_type& m)
      {
        return d + m.get_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator+=(date_type& d, const months_type& m)
      {
        return d += m.get_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator-(const date_type& d, const months_type& m)
      {
        // get_neg_offset returns a negative duration, so we add
        return d + m.get_neg_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator-=(date_type& d, const months_type& m)
      {
        // get_neg_offset returns a negative duration, so we add
        return d += m.get_neg_offset(d);
      }
    private:
      int_rep _m;
  };

  //! additional duration type that represents a logical year
  /*! A logical year enables things like: "date(2002,Mar,2) + years(2) -> 
   * 2004-Mar-2". If the date is a last day-of-the-month, the result will 
   * also be a last-day-of-the-month (ie date(2001-Feb-28) + years(3) ->
   * 2004-Feb-29).
   */
  template<class base_config>
  class BOOST_SYMBOL_VISIBLE years_duration
  {
    private:
      typedef typename base_config::int_rep int_rep;
      typedef typename int_rep::int_type int_type;
      typedef typename base_config::date_type date_type;
      typedef typename date_type::duration_type duration_type;
      typedef typename base_config::month_adjustor_type month_adjustor_type;
      typedef years_duration<base_config> years_type;
      typedef months_duration<base_config> months_type;
    public:
      BOOST_CXX14_CONSTEXPR years_duration(int_rep num) : _y(num) {}
      BOOST_CXX14_CONSTEXPR years_duration(special_values sv) : _y(sv) 
      {
        _y = int_rep::from_special(sv);
      }
      BOOST_CXX14_CONSTEXPR int_rep number_of_years() const { return _y; }
      //! returns a negative duration
      BOOST_CXX14_CONSTEXPR duration_type get_neg_offset(const date_type& d) const
      {
        month_adjustor_type m_adj(_y.as_number() * 12);
        return duration_type(m_adj.get_neg_offset(d));
      }
      BOOST_CXX14_CONSTEXPR duration_type get_offset(const date_type& d) const
      {
        month_adjustor_type m_adj(_y.as_number() * 12);
        return duration_type(m_adj.get_offset(d));
      }
      BOOST_CXX14_CONSTEXPR bool operator==(const years_type& rhs) const
      {
        return(_y == rhs._y);
      }
      bool operator!=(const years_type& rhs) const
      {
        return(_y != rhs._y);
      }
      BOOST_CXX14_CONSTEXPR years_type operator+(const years_type& rhs)const
      {
        return years_type(_y + rhs._y);
      }
      BOOST_CXX14_CONSTEXPR years_type& operator+=(const years_type& rhs)
      {
        _y = _y + rhs._y;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR years_type operator-(const years_type& rhs)const
      {
        return years_type(_y - rhs._y);
      }
      BOOST_CXX14_CONSTEXPR years_type& operator-=(const years_type& rhs)
      {
        _y = _y - rhs._y;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR years_type operator*(const int_type rhs)const
      {
        return years_type(_y * rhs);
      }
      BOOST_CXX14_CONSTEXPR years_type& operator*=(const int_type rhs)
      {
        _y = _y * rhs;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR years_type operator/(const int_type rhs)const
      {
        return years_type(_y / rhs);
      }
      BOOST_CXX14_CONSTEXPR years_type& operator/=(const int_type rhs)
      {
        _y = _y / rhs;
        return *this;
      }
      BOOST_CXX14_CONSTEXPR months_type operator+(const months_type& m) const
      {
        return(months_type(_y * 12 + m.number_of_months()));
      }
      BOOST_CXX14_CONSTEXPR months_type operator-(const months_type& m) const
      {
        return(months_type(_y * 12 - m.number_of_months()));
      }
      //
      BOOST_CXX14_CONSTEXPR friend date_type operator+(const date_type& d, const years_type& y)
      {
        return d + y.get_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator+=(date_type& d, const years_type& y)
      {
        return d += y.get_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator-(const date_type& d, const years_type& y)
      {
        // get_neg_offset returns a negative duration, so we add
        return d + y.get_neg_offset(d);
      }
      BOOST_CXX14_CONSTEXPR friend date_type operator-=(date_type& d, const years_type& y)
      {
        // get_neg_offset returns a negative duration, so we add
        return d += y.get_neg_offset(d);
      }
    private:
      int_rep _y;
  };
}} // namespace boost::date_time

#endif // DATE_DURATION_TYPES_HPP___
