// Boost.Polygon library segment_data.hpp header file

// Copyright (c) Intel Corporation 2008.
// Copyright (c) 2008-2012 Simonson Lucanus.
// Copyright (c) 2012-2012 Andrii Sydorchuk.

// See http://www.boost.org for updates, documentation, and revision history.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_POLYGON_SEGMENT_DATA_HPP
#define BOOST_POLYGON_SEGMENT_DATA_HPP

#include "isotropy.hpp"
#include "segment_concept.hpp"

namespace boost {
namespace polygon {

template <typename T>
class segment_data {
 public:
  typedef T coordinate_type;
  typedef point_data<T> point_type;

  segment_data()
#ifndef BOOST_POLYGON_MSVC
    : points_()
#endif
  {}

  segment_data(const point_type& low, const point_type& high) {
    points_[LOW] = low;
    points_[HIGH] = high;
  }

  segment_data(const segment_data& that) {
    points_[0] = that.points_[0];
    points_[1] = that.points_[1];
  }

  segment_data& operator=(const segment_data& that) {
    points_[0] = that.points_[0];
    points_[1] = that.points_[1];
    return *this;
  }

  template <typename SegmentType>
  segment_data& operator=(const SegmentType& that) {
    assign(*this, that);
    return *this;
  }

  point_type get(direction_1d dir) const {
    return points_[dir.to_int()];
  }

  void set(direction_1d dir, const point_type& point) {
    points_[dir.to_int()] = point;
  }

  point_type low() const {
    return points_[LOW];
  }

  segment_data& low(const point_type& point) {
    points_[LOW] = point;
    return *this;
  }

  point_type high() const {
    return points_[HIGH];
  }

  segment_data& high(const point_type& point) {
    points_[HIGH] = point;
    return *this;
  }

  bool operator==(const segment_data& that) const {
    return (points_[0] == that.points_[0]) &&
           (points_[1] == that.points_[1]);
  }

  bool operator!=(const segment_data& that) const {
    return (points_[0] != that.points_[0]) ||
           (points_[1] != that.points_[1]);
  }

  bool operator<(const segment_data& that) const {
    if (points_[0] != that.points_[0]) {
      return points_[0] < that.points_[0];
    }
    return points_[1] < that.points_[1];
  }

  bool operator<=(const segment_data& that) const {
    return !(that < *this);
  }

  bool operator>(const segment_data& that) const {
    return that < *this;
  }

  bool operator>=(const segment_data& that) const {
    return !((*this) < that);
  }

 private:
  point_type points_[2];
};

template <typename CType>
struct geometry_concept<segment_data<CType> > {
  typedef segment_concept type;
};
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_SEGMENT_DATA_HPP
