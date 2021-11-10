#ifndef DATE_TIME_PERIOD_HPP___
#define DATE_TIME_PERIOD_HPP___

/* Copyright (c) 2002,2003 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst 
 * $Date$
 */

/*! \file period.hpp
  This file contain the implementation of the period abstraction. This is
  basically the same idea as a range.  Although this class is intended for
  use in the time library, it is pretty close to general enough for other
  numeric uses.

*/

#include <boost/operators.hpp>
#include <boost/date_time/compiler_config.hpp>


namespace boost {
namespace date_time {
  //!Provides generalized period type useful in date-time systems
  /*!This template uses a class to represent a time point within the period
    and another class to represent a duration.  As a result, this class is
    not appropriate for use when the number and duration representation 
    are the same (eg: in the regular number domain).
    
    A period can be specified by providing either the begining point and 
    a duration or the begining point and the end point( end is NOT part 
    of the period but 1 unit past it. A period will be "invalid" if either
    end_point <= begin_point or the given duration is <= 0. Any valid period 
    will return false for is_null().
    
    Zero length periods are also considered invalid. Zero length periods are
    periods where the begining and end points are the same, or, the given 
    duration is zero. For a zero length period, the last point will be one 
    unit less than the begining point.

    In the case that the begin and last are the same, the period has a 
    length of one unit.
    
    The best way to handle periods is usually to provide a begining point and
    a duration.  So, day1 + 7 days is a week period which includes all of the
    first day and 6 more days (eg: Sun to Sat).

   */
  template<class point_rep, class duration_rep>
  class BOOST_SYMBOL_VISIBLE period : private
      boost::less_than_comparable<period<point_rep, duration_rep> 
    , boost::equality_comparable< period<point_rep, duration_rep> 
    > >
  {
  public:
    typedef point_rep point_type;
    typedef duration_rep duration_type;

    BOOST_CXX14_CONSTEXPR period(point_rep first_point, point_rep end_point);
    BOOST_CXX14_CONSTEXPR period(point_rep first_point, duration_rep len);
    BOOST_CXX14_CONSTEXPR point_rep begin() const;
    BOOST_CXX14_CONSTEXPR point_rep end() const;
    BOOST_CXX14_CONSTEXPR point_rep last() const;
    BOOST_CXX14_CONSTEXPR duration_rep length() const;
    BOOST_CXX14_CONSTEXPR bool is_null() const;
    BOOST_CXX14_CONSTEXPR bool operator==(const period& rhs) const;
    BOOST_CXX14_CONSTEXPR bool operator<(const period& rhs) const;
    BOOST_CXX14_CONSTEXPR void shift(const duration_rep& d);
    BOOST_CXX14_CONSTEXPR void expand(const duration_rep& d);
    BOOST_CXX14_CONSTEXPR bool contains(const point_rep& point) const;
    BOOST_CXX14_CONSTEXPR bool contains(const period& other) const;
    BOOST_CXX14_CONSTEXPR bool intersects(const period& other) const;
    BOOST_CXX14_CONSTEXPR bool is_adjacent(const period& other) const;
    BOOST_CXX14_CONSTEXPR bool is_before(const point_rep& point) const;
    BOOST_CXX14_CONSTEXPR bool is_after(const point_rep& point) const;
    BOOST_CXX14_CONSTEXPR period intersection(const period& other) const;
    BOOST_CXX14_CONSTEXPR period merge(const period& other) const;
    BOOST_CXX14_CONSTEXPR period span(const period& other) const;
  private:
    point_rep begin_;
    point_rep last_;
  };

  //! create a period from begin to last eg: [begin,end)
  /*! If end <= begin then the period will be invalid
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  period<point_rep,duration_rep>::period(point_rep first_point, 
                                         point_rep end_point) : 
    begin_(first_point), 
    last_(end_point - duration_rep::unit())
  {}

  //! create a period as [begin, begin+len)
  /*! If len is <= 0 then the period will be invalid
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  period<point_rep,duration_rep>::period(point_rep first_point, duration_rep len) :
    begin_(first_point), 
    last_(first_point + len-duration_rep::unit())
  { }


  //! Return the first element in the period
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  point_rep period<point_rep,duration_rep>::begin() const 
  {
    return begin_;
  }

  //! Return one past the last element 
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  point_rep period<point_rep,duration_rep>::end() const 
  {
    return last_ + duration_rep::unit();
  }

  //! Return the last item in the period
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  point_rep period<point_rep,duration_rep>::last() const 
  {
    return last_;
  }

  //! True if period is ill formed (length is zero or less)
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::is_null() const 
  {
    return end() <= begin_;
  }

  //! Return the length of the period
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  duration_rep period<point_rep,duration_rep>::length() const
  {
    if(last_ < begin_){ // invalid period
      return last_+duration_rep::unit() - begin_;
    }
    else{
      return end() - begin_; // normal case
    }
  }

  //! Equality operator
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::operator==(const period& rhs) const 
  {
    return  ((begin_ == rhs.begin_) && 
             (last_ == rhs.last_));
  }

  //! Strict as defined by rhs.last <= lhs.last
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::operator<(const period& rhs) const 
  {
    return (last_ < rhs.begin_);
  } 


  //! Shift the start and end by the specified amount
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  void period<point_rep,duration_rep>::shift(const duration_rep& d)
  {
    begin_ = begin_ + d;
    last_  = last_  + d;
  }

  /** Expands the size of the period by the duration on both ends.
   *
   *So before expand 
   *@code
   *
   *         [-------]
   * ^   ^   ^   ^   ^   ^  ^
   * 1   2   3   4   5   6  7
   * 
   *@endcode
   * After expand(2)
   *@code
   *
   * [----------------------]
   * ^   ^   ^   ^   ^   ^  ^
   * 1   2   3   4   5   6  7
   * 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  void period<point_rep,duration_rep>::expand(const duration_rep& d)
  {
    begin_ = begin_ - d;
    last_  = last_  + d;
  }

  //! True if the point is inside the period, zero length periods contain no points
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::contains(const point_rep& point) const 
  {
    return ((point >= begin_) &&
            (point <= last_));
  }


  //! True if this period fully contains (or equals) the other period
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::contains(const period<point_rep,duration_rep>& other) const
  {
    return ((begin_ <= other.begin_) && (last_ >= other.last_));
  }


  //! True if periods are next to each other without a gap.
  /* In the example below, p1 and p2 are adjacent, but p3 is not adjacent
   * with either of p1 or p2.
   *@code
   *   [-p1-)
   *        [-p2-)
   *          [-p3-) 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::is_adjacent(const period<point_rep,duration_rep>& other) const
  {
    return (other.begin() == end() ||
            begin_ == other.end());
  }


  //! True if all of the period is prior or t < start
  /* In the example below only point 1 would evaluate to true.
   *@code
   *     [---------])
   * ^   ^    ^     ^   ^
   * 1   2    3     4   5
   * 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::is_after(const point_rep& t) const
  { 
    if (is_null()) 
    {
      return false; //null period isn't after
    }
    
    return t < begin_;
  }

  //! True if all of the period is prior to the passed point or end <= t
  /* In the example below points 4 and 5 return true.
   *@code
   *     [---------])
   * ^   ^    ^     ^   ^
   * 1   2    3     4   5
   * 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::is_before(const point_rep& t) const
  { 
    if (is_null()) 
    {
      return false;  //null period isn't before anything
    }
    
    return last_ < t;
  }


  //! True if the periods overlap in any way
  /* In the example below p1 intersects with p2, p4, and p6.
   *@code
   *       [---p1---)
   *             [---p2---)
   *                [---p3---) 
   *  [---p4---) 
   * [-p5-) 
   *         [-p6-) 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  bool period<point_rep,duration_rep>::intersects(const period<point_rep,duration_rep>& other) const
  { 
    return ( contains(other.begin_) ||
             other.contains(begin_) ||
             ((other.begin_ < begin_) && (other.last_ >= begin_)));
  }

  //! Returns the period of intersection or invalid range no intersection
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  period<point_rep,duration_rep>
  period<point_rep,duration_rep>::intersection(const period<point_rep,duration_rep>& other) const 
  {
    if (begin_ > other.begin_) {
      if (last_ <= other.last_) { //case2
        return *this;  
      }
      //case 1
      return period<point_rep,duration_rep>(begin_, other.end());
    }
    else {
      if (last_ <= other.last_) { //case3
        return period<point_rep,duration_rep>(other.begin_, this->end());
      }
      //case4
      return other;
    }
    //unreachable
  }

  //! Returns the union of intersecting periods -- or null period
  /*! 
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  period<point_rep,duration_rep>
  period<point_rep,duration_rep>::merge(const period<point_rep,duration_rep>& other) const 
  {
    if (this->intersects(other)) {      
      if (begin_ < other.begin_) {
        return period<point_rep,duration_rep>(begin_, last_ > other.last_ ? this->end() : other.end());
      }
      
      return period<point_rep,duration_rep>(other.begin_, last_ > other.last_ ? this->end() : other.end());
      
    }
    return period<point_rep,duration_rep>(begin_,begin_); // no intersect return null
  }

  //! Combine two periods with earliest start and latest end.
  /*! Combines two periods and any gap between them such that 
   *  start = min(p1.start, p2.start)
   *  end   = max(p1.end  , p2.end)
   *@code
   *        [---p1---)
   *                       [---p2---)
   * result:
   *        [-----------p3----------) 
   *@endcode
   */
  template<class point_rep, class duration_rep>
  inline BOOST_CXX14_CONSTEXPR
  period<point_rep,duration_rep>
  period<point_rep,duration_rep>::span(const period<point_rep,duration_rep>& other) const 
  {
    point_rep start((begin_ < other.begin_) ? begin() : other.begin());
    point_rep newend((last_  < other.last_)  ? other.end() : this->end());
    return period<point_rep,duration_rep>(start, newend);
  }


} } //namespace date_time



#endif
