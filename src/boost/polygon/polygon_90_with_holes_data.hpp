/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_POLYGON_90_WITH_HOLES_DATA_HPP
#define BOOST_POLYGON_POLYGON_90_WITH_HOLES_DATA_HPP
namespace boost { namespace polygon{
#include "isotropy.hpp"
#include "polygon_90_data.hpp"
struct polygon_90_with_holes_concept;
template <typename T>
class polygon_90_with_holes_data {
public:
  typedef polygon_90_with_holes_concept geometry_type;
  typedef T coordinate_type;
  typedef typename polygon_90_data<T>::iterator_type iterator_type;
  typedef typename polygon_90_data<T>::compact_iterator_type compact_iterator_type;
  typedef typename std::list<polygon_90_data<coordinate_type> >::const_iterator iterator_holes_type;
  typedef polygon_90_data<coordinate_type> hole_type;
  typedef typename coordinate_traits<T>::area_type area_type;
  typedef point_data<T> point_type;

  // default constructor of point does not initialize x and y
  inline polygon_90_with_holes_data() : self_(), holes_() {} //do nothing default constructor

  // initialize a polygon from x,y values, it is assumed that the first is an x
  // and that the input is a well behaved polygon
  template<class iT>
  inline polygon_90_with_holes_data& set(iT input_begin, iT input_end) {
    self_.set(input_begin, input_end);
    return *this;
  }

  // initialize a polygon from x,y values, it is assumed that the first is an x
  // and that the input is a well behaved polygon
  template<class iT>
  inline polygon_90_with_holes_data& set_compact(iT input_begin, iT input_end) {
    self_.set_compact(input_begin, input_end);
    return *this;
  }

  // initialize a polygon from x,y values, it is assumed that the first is an x
  // and that the input is a well behaved polygon
  template<class iT>
  inline polygon_90_with_holes_data& set_holes(iT input_begin, iT input_end) {
    holes_.clear();  //just in case there was some old data there
    for( ; input_begin != input_end; ++ input_begin) {
       holes_.push_back(hole_type());
       holes_.back().set_compact((*input_begin).begin_compact(), (*input_begin).end_compact());
    }
    return *this;
  }

  // copy constructor (since we have dynamic memory)
  inline polygon_90_with_holes_data(const polygon_90_with_holes_data& that) : self_(that.self_),
                                                                  holes_(that.holes_) {}

  // assignment operator (since we have dynamic memory do a deep copy)
  inline polygon_90_with_holes_data& operator=(const polygon_90_with_holes_data& that) {
    self_ = that.self_;
    holes_ = that.holes_;
    return *this;
  }

  template <typename T2>
  inline polygon_90_with_holes_data& operator=(const T2& rvalue);

  // get begin iterator, returns a pointer to a const coordinate_type
  inline const iterator_type begin() const {
    return self_.begin();
  }

  // get end iterator, returns a pointer to a const coordinate_type
  inline const iterator_type end() const {
    return self_.end();
  }

  // get begin iterator, returns a pointer to a const coordinate_type
  inline const compact_iterator_type begin_compact() const {
    return self_.begin_compact();
  }

  // get end iterator, returns a pointer to a const coordinate_type
  inline const compact_iterator_type end_compact() const {
    return self_.end_compact();
  }

  inline std::size_t size() const {
    return self_.size();
  }

  // get begin iterator, returns a pointer to a const polygon
  inline const iterator_holes_type begin_holes() const {
    return holes_.begin();
  }

  // get end iterator, returns a pointer to a const polygon
  inline const iterator_holes_type end_holes() const {
    return holes_.end();
  }

  inline std::size_t size_holes() const {
    return holes_.size();
  }

private:
  polygon_90_data<coordinate_type> self_;
  std::list<hole_type> holes_;
};
}
}
#endif
