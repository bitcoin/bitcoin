// Boost.Polygon library transform.hpp header file

// Copyright (c) Intel Corporation 2008.
// Copyright (c) 2008-2012 Simonson Lucanus.
// Copyright (c) 2012-2012 Andrii Sydorchuk.

// See http://www.boost.org for updates, documentation, and revision history.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_POLYGON_TRANSFORM_HPP
#define BOOST_POLYGON_TRANSFORM_HPP

#include "isotropy.hpp"

namespace boost {
namespace polygon {
// Transformation of Coordinate System.
// Enum meaning:
// Select which direction_2d to change the positive direction of each
// axis in the old coordinate system to map it to the new coordiante system.
// The first direction_2d listed for each enum is the direction to map the
// positive horizontal direction to.
// The second direction_2d listed for each enum is the direction to map the
// positive vertical direction to.
// The zero position bit (LSB) indicates whether the horizontal axis flips
// when transformed.
// The 1st postion bit indicates whether the vertical axis flips when
// transformed.
// The 2nd position bit indicates whether the horizontal and vertical axis
// swap positions when transformed.
// Enum Values:
//   000 EAST NORTH
//   001 WEST NORTH
//   010 EAST SOUTH
//   011 WEST SOUTH
//   100 NORTH EAST
//   101 SOUTH EAST
//   110 NORTH WEST
//   111 SOUTH WEST
class axis_transformation {
 public:
  enum ATR {
#ifdef BOOST_POLYGON_ENABLE_DEPRECATED
    EN = 0,
    WN = 1,
    ES = 2,
    WS = 3,
    NE = 4,
    SE = 5,
    NW = 6,
    SW = 7,
#endif
    NULL_TRANSFORM = 0,
    BEGIN_TRANSFORM = 0,
    EAST_NORTH = 0,
    WEST_NORTH = 1, FLIP_X       = 1,
    EAST_SOUTH = 2, FLIP_Y       = 2,
    WEST_SOUTH = 3, FLIP_XY      = 3,
    NORTH_EAST = 4, SWAP_XY      = 4,
    SOUTH_EAST = 5, ROTATE_LEFT  = 5,
    NORTH_WEST = 6, ROTATE_RIGHT = 6,
    SOUTH_WEST = 7, FLIP_SWAP_XY = 7,
    END_TRANSFORM = 7
  };

  // Individual axis enum values indicate which axis an implicit individual
  // axis will be mapped to.
  // The value of the enum paired with an axis provides the information
  // about what the axis will transform to.
  // Three individual axis values, one for each axis, are equivalent to one
  // ATR enum value, but easier to work with because they are independent.
  // Converting to and from the individual axis values from the ATR value
  // is a convenient way to implement tranformation related functionality.
  // Enum meanings:
  // PX: map to positive x axis
  // NX: map to negative x axis
  // PY: map to positive y axis
  // NY: map to negative y axis
  enum INDIVIDUAL_AXIS {
    PX = 0,
    NX = 1,
    PY = 2,
    NY = 3
  };

  axis_transformation() : atr_(NULL_TRANSFORM) {}
  explicit axis_transformation(ATR atr) : atr_(atr) {}
  axis_transformation(const axis_transformation& atr) : atr_(atr.atr_) {}

  explicit axis_transformation(const orientation_2d& orient) {
    const ATR tmp[2] = {
      NORTH_EAST,  // sort x, then y
      EAST_NORTH   // sort y, then x
    };
    atr_ = tmp[orient.to_int()];
  }

  explicit axis_transformation(const direction_2d& dir) {
    const ATR tmp[4] = {
      SOUTH_EAST,  // sort x, then y
      NORTH_EAST,  // sort x, then y
      EAST_SOUTH,  // sort y, then x
      EAST_NORTH   // sort y, then x
    };
    atr_ = tmp[dir.to_int()];
  }

  // assignment operator
  axis_transformation& operator=(const axis_transformation& a) {
    atr_ = a.atr_;
    return *this;
  }

  // assignment operator
  axis_transformation& operator=(const ATR& atr) {
    atr_ = atr;
    return *this;
  }

  // equivalence operator
  bool operator==(const axis_transformation& a) const {
    return atr_ == a.atr_;
  }

  // inequivalence operator
  bool operator!=(const axis_transformation& a) const {
    return !(*this == a);
  }

  // ordering
  bool operator<(const axis_transformation& a) const {
    return atr_ < a.atr_;
  }

  // concatenate this with that
  axis_transformation& operator+=(const axis_transformation& a) {
    bool abit2 = (a.atr_ & 4) != 0;
    bool abit1 = (a.atr_ & 2) != 0;
    bool abit0 = (a.atr_ & 1) != 0;
    bool bit2 = (atr_ & 4) != 0;
    bool bit1 = (atr_ & 2) != 0;
    bool bit0 = (atr_ & 1) != 0;
    int indexes[2][2] = {
      { (int)bit2, (int)(!bit2) },
      { (int)abit2, (int)(!abit2) }
    };
    int zero_bits[2][2] = {
      {bit0, bit1}, {abit0, abit1}
    };
    int nbit1 = zero_bits[0][1] ^ zero_bits[1][indexes[0][1]];
    int nbit0 = zero_bits[0][0] ^ zero_bits[1][indexes[0][0]];
    indexes[0][0] = indexes[1][indexes[0][0]];
    indexes[0][1] = indexes[1][indexes[0][1]];
    int nbit2 = indexes[0][0] & 1;  // swap xy
    atr_ = (ATR)((nbit2 << 2) + (nbit1 << 1) + nbit0);
    return *this;
  }

  // concatenation operator
  axis_transformation operator+(const axis_transformation& a) const {
    axis_transformation retval(*this);
    return retval+=a;
  }

  // populate_axis_array writes the three INDIVIDUAL_AXIS values that the
  // ATR enum value of 'this' represent into axis_array
  void populate_axis_array(INDIVIDUAL_AXIS axis_array[]) const {
    bool bit2 = (atr_ & 4) != 0;
    bool bit1 = (atr_ & 2) != 0;
    bool bit0 = (atr_ & 1) != 0;
    axis_array[1] = (INDIVIDUAL_AXIS)(((int)(!bit2) << 1) + bit1);
    axis_array[0] = (INDIVIDUAL_AXIS)(((int)(bit2) << 1) + bit0);
  }

  // it is recommended that the directions stored in an array
  // in the caller code for easier isotropic access by orientation value
  void get_directions(direction_2d& horizontal_dir,
                      direction_2d& vertical_dir) const {
    bool bit2 = (atr_ & 4) != 0;
    bool bit1 = (atr_ & 2) != 0;
    bool bit0 = (atr_ & 1) != 0;
    vertical_dir = direction_2d((direction_2d_enum)(((int)(!bit2) << 1) + !bit1));
    horizontal_dir = direction_2d((direction_2d_enum)(((int)(bit2) << 1) + !bit0));
  }

  // combine_axis_arrays concatenates this_array and that_array overwriting
  // the result into this_array
  static void combine_axis_arrays(INDIVIDUAL_AXIS this_array[],
                                  const INDIVIDUAL_AXIS that_array[]) {
    int indexes[2] = { this_array[0] >> 1, this_array[1] >> 1 };
    int zero_bits[2][2] = {
      { this_array[0] & 1, this_array[1] & 1 },
      { that_array[0] & 1, that_array[1] & 1 }
    };
    this_array[0] = (INDIVIDUAL_AXIS)((int)this_array[0] |
                                      ((int)zero_bits[0][0] ^
                                       (int)zero_bits[1][indexes[0]]));
    this_array[1] = (INDIVIDUAL_AXIS)((int)this_array[1] |
                                      ((int)zero_bits[0][1] ^
                                       (int)zero_bits[1][indexes[1]]));
  }

  // write_back_axis_array converts an array of three INDIVIDUAL_AXIS values
  // to the ATR enum value and sets 'this' to that value
  void write_back_axis_array(const INDIVIDUAL_AXIS this_array[]) {
    int bit2 = ((int)this_array[0] & 2) != 0;  // swap xy
    int bit1 = ((int)this_array[1] & 1);
    int bit0 = ((int)this_array[0] & 1);
    atr_ = ATR((bit2 << 2) + (bit1 << 1) + bit0);
  }

  // behavior is deterministic but undefined in the case where illegal
  // combinations of directions are passed in.
  axis_transformation& set_directions(const direction_2d& horizontal_dir,
                                      const direction_2d& vertical_dir) {
    int bit2 = (static_cast<orientation_2d>(horizontal_dir).to_int()) != 0;
    int bit1 = !(vertical_dir.to_int() & 1);
    int bit0 = !(horizontal_dir.to_int() & 1);
    atr_ = ATR((bit2 << 2) + (bit1 << 1) + bit0);
    return *this;
  }

  // transform the three coordinates by reference
  template <typename coordinate_type>
  void transform(coordinate_type& x, coordinate_type& y) const {
    int bit2 = (atr_ & 4) != 0;
    int bit1 = (atr_ & 2) != 0;
    int bit0 = (atr_ & 1) != 0;
    x *= -((bit0 << 1) - 1);
    y *= -((bit1 << 1) - 1);
    predicated_swap(bit2 != 0, x, y);
  }

  // invert this axis_transformation
  axis_transformation& invert() {
    int bit2 = ((atr_ & 4) != 0);
    int bit1 = ((atr_ & 2) != 0);
    int bit0 = ((atr_ & 1) != 0);
    // swap bit 0 and bit 1 if bit2 is 1
    predicated_swap(bit2 != 0, bit0, bit1);
    bit1 = bit1 << 1;
    atr_ = (ATR)(atr_ & (32+16+8+4));  // mask away bit0 and bit1
    atr_ = (ATR)(atr_ | bit0 | bit1);
    return *this;
  }

  // get the inverse axis_transformation of this
  axis_transformation inverse() const {
    axis_transformation retval(*this);
    return retval.invert();
  }

 private:
  ATR atr_;
};

// Scaling object to be used to store the scale factor for each axis.
// For use by the transformation object, in that context the scale factor
// is the amount that each axis scales by when transformed.
template <typename scale_factor_type>
class anisotropic_scale_factor {
 public:
  anisotropic_scale_factor() {
    scale_[0] = 1;
    scale_[1] = 1;
  }
  anisotropic_scale_factor(scale_factor_type xscale,
                           scale_factor_type yscale) {
    scale_[0] = xscale;
    scale_[1] = yscale;
  }

  // get a component of the anisotropic_scale_factor by orientation
  scale_factor_type get(orientation_2d orient) const {
    return scale_[orient.to_int()];
  }

  // set a component of the anisotropic_scale_factor by orientation
  void set(orientation_2d orient, scale_factor_type value) {
    scale_[orient.to_int()] = value;
  }

  scale_factor_type x() const {
    return scale_[HORIZONTAL];
  }

  scale_factor_type y() const {
    return scale_[VERTICAL];
  }

  void x(scale_factor_type value) {
    scale_[HORIZONTAL] = value;
  }

  void y(scale_factor_type value) {
    scale_[VERTICAL] = value;
  }

  // concatination operator (convolve scale factors)
  anisotropic_scale_factor operator+(const anisotropic_scale_factor& s) const {
    anisotropic_scale_factor<scale_factor_type> retval(*this);
    return retval += s;
  }

  // concatinate this with that
  const anisotropic_scale_factor& operator+=(
      const anisotropic_scale_factor& s) {
    scale_[0] *= s.scale_[0];
    scale_[1] *= s.scale_[1];
    return *this;
  }

  // transform this scale with an axis_transform
  anisotropic_scale_factor& transform(axis_transformation atr) {
    direction_2d dirs[2];
    atr.get_directions(dirs[0], dirs[1]);
    scale_factor_type tmp[2] = {scale_[0], scale_[1]};
    for (int i = 0; i < 2; ++i) {
      scale_[orientation_2d(dirs[i]).to_int()] = tmp[i];
    }
    return *this;
  }

  // scale the two coordinates
  template <typename coordinate_type>
  void scale(coordinate_type& x, coordinate_type& y) const {
    x = scaling_policy<coordinate_type>::round(
        (scale_factor_type)x * get(HORIZONTAL));
    y = scaling_policy<coordinate_type>::round(
        (scale_factor_type)y * get(HORIZONTAL));
  }

  // invert this scale factor to give the reverse scale factor
  anisotropic_scale_factor& invert() {
    x(1/x());
    y(1/y());
    return *this;
  }

 private:
  scale_factor_type scale_[2];
};

// Transformation object, stores and provides services for transformations.
// Consits of axis transformation, scale factor and translation.
// The tranlation is the position of the origin of the new coordinate system of
// in the old system. Coordinates are scaled before they are transformed.
template <typename coordinate_type>
class transformation {
 public:
  transformation() : atr_(), p_(0, 0) {}
  explicit transformation(axis_transformation atr) : atr_(atr), p_(0, 0) {}
  explicit transformation(axis_transformation::ATR atr) : atr_(atr), p_(0, 0) {}
  transformation(const transformation& tr) : atr_(tr.atr_), p_(tr.p_) {}

  template <typename point_type>
  explicit transformation(const point_type& p) : atr_(), p_(0, 0) {
    set_translation(p);
  }

  template <typename point_type>
  transformation(axis_transformation atr,
                 const point_type& p) : atr_(atr), p_(0, 0) {
    set_translation(p);
  }

  template <typename point_type>
  transformation(axis_transformation atr,
                 const point_type& referencePt,
                 const point_type& destinationPt) : atr_(), p_(0, 0) {
    transformation<coordinate_type> tmp(referencePt);
    transformation<coordinate_type> rotRef(atr);
    transformation<coordinate_type> tmpInverse = tmp.inverse();
    point_type decon(referencePt);
    deconvolve(decon, destinationPt);
    transformation<coordinate_type> displacement(decon);
    tmp += rotRef;
    tmp += tmpInverse;
    tmp += displacement;
    (*this) = tmp;
  }

  // equivalence operator
  bool operator==(const transformation& tr) const {
    return (atr_ == tr.atr_) && (p_ == tr.p_);
  }

  // inequivalence operator
  bool operator!=(const transformation& tr) const {
    return !(*this == tr);
  }

  // ordering
  bool operator<(const transformation& tr) const {
    return (atr_ < tr.atr_) || ((atr_ == tr.atr_) && (p_ < tr.p_));
  }

  // concatenation operator
  transformation operator+(const transformation& tr) const {
    transformation<coordinate_type> retval(*this);
    return retval+=tr;
  }

  // concatenate this with that
  const transformation& operator+=(const transformation& tr) {
    coordinate_type x, y;
    transformation<coordinate_type> inv = inverse();
    inv.transform(x, y);
    p_.set(HORIZONTAL, p_.get(HORIZONTAL) + x);
    p_.set(VERTICAL, p_.get(VERTICAL) + y);
    // concatenate axis transforms
    atr_ += tr.atr_;
    return *this;
  }

  // get the axis_transformation portion of this
  axis_transformation get_axis_transformation() const {
    return atr_;
  }

  // set the axis_transformation portion of this
  void set_axis_transformation(const axis_transformation& atr) {
    atr_ = atr;
  }

  // get the translation
  template <typename point_type>
  void get_translation(point_type& p) const {
    assign(p, p_);
  }

  // set the translation
  template <typename point_type>
  void set_translation(const point_type& p) {
    assign(p_, p);
  }

  // apply the 2D portion of this transformation to the two coordinates given
  void transform(coordinate_type& x, coordinate_type& y) const {
    y -= p_.get(VERTICAL);
    x -= p_.get(HORIZONTAL);
    atr_.transform(x, y);
  }

  // invert this transformation
  transformation& invert() {
    coordinate_type x = p_.get(HORIZONTAL), y = p_.get(VERTICAL);
    atr_.transform(x, y);
    x *= -1;
    y *= -1;
    p_ = point_data<coordinate_type>(x, y);
    atr_.invert();
    return *this;
  }

  // get the inverse of this transformation
  transformation inverse() const {
    transformation<coordinate_type> ret_val(*this);
    return ret_val.invert();
  }

  void get_directions(direction_2d& horizontal_dir,
                      direction_2d& vertical_dir) const {
    return atr_.get_directions(horizontal_dir, vertical_dir);
  }

 private:
  axis_transformation atr_;
  point_data<coordinate_type> p_;
};
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_TRANSFORM_HPP
