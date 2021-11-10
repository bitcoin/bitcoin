// Boost.Polygon library detail/voronoi_robust_fpt.hpp header file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POLYGON_DETAIL_VORONOI_ROBUST_FPT
#define BOOST_POLYGON_DETAIL_VORONOI_ROBUST_FPT

#include <algorithm>
#include <cmath>

// Geometry predicates with floating-point variables usually require
// high-precision predicates to retrieve the correct result.
// Epsilon robust predicates give the result within some epsilon relative
// error, but are a lot faster than high-precision predicates.
// To make algorithm robust and efficient epsilon robust predicates are
// used at the first step. In case of the undefined result high-precision
// arithmetic is used to produce required robustness. This approach
// requires exact computation of epsilon intervals within which epsilon
// robust predicates have undefined value.
// There are two ways to measure an error of floating-point calculations:
// relative error and ULPs (units in the last place).
// Let EPS be machine epsilon, then next inequalities have place:
// 1 EPS <= 1 ULP <= 2 EPS (1), 0.5 ULP <= 1 EPS <= 1 ULP (2).
// ULPs are good for measuring rounding errors and comparing values.
// Relative errors are good for computation of general relative
// error of formulas or expressions. So to calculate epsilon
// interval within which epsilon robust predicates have undefined result
// next schema is used:
//     1) Compute rounding errors of initial variables using ULPs;
//     2) Transform ULPs to epsilons using upper bound of the (1);
//     3) Compute relative error of the formula using epsilon arithmetic;
//     4) Transform epsilon to ULPs using upper bound of the (2);
// In case two values are inside undefined ULP range use high-precision
// arithmetic to produce the correct result, else output the result.
// Look at almost_equal function to see how two floating-point variables
// are checked to fit in the ULP range.
// If A has relative error of r(A) and B has relative error of r(B) then:
//     1) r(A + B) <= max(r(A), r(B)), for A * B >= 0;
//     2) r(A - B) <= B*r(A)+A*r(B)/(A-B), for A * B >= 0;
//     2) r(A * B) <= r(A) + r(B);
//     3) r(A / B) <= r(A) + r(B);
// In addition rounding error should be added, that is always equal to
// 0.5 ULP or at most 1 epsilon. As you might see from the above formulas
// subtraction relative error may be extremely large, that's why
// epsilon robust comparator class is used to store floating point values
// and compute subtraction as the final step of the evaluation.
// For further information about relative errors and ULPs try this link:
// http://docs.sun.com/source/806-3568/ncg_goldberg.html

namespace boost {
namespace polygon {
namespace detail {

template <typename T>
T get_sqrt(const T& that) {
  return (std::sqrt)(that);
}

template <typename T>
bool is_pos(const T& that) {
  return that > 0;
}

template <typename T>
bool is_neg(const T& that) {
  return that < 0;
}

template <typename T>
bool is_zero(const T& that) {
  return that == 0;
}

template <typename _fpt>
class robust_fpt {
 public:
  typedef _fpt floating_point_type;
  typedef _fpt relative_error_type;

  // Rounding error is at most 1 EPS.
  enum {
    ROUNDING_ERROR = 1
  };

  robust_fpt() : fpv_(0.0), re_(0.0) {}
  explicit robust_fpt(floating_point_type fpv) :
      fpv_(fpv), re_(0.0) {}
  robust_fpt(floating_point_type fpv, relative_error_type error) :
      fpv_(fpv), re_(error) {}

  floating_point_type fpv() const { return fpv_; }
  relative_error_type re() const { return re_; }
  relative_error_type ulp() const { return re_; }

  bool has_pos_value() const {
    return is_pos(fpv_);
  }

  bool has_neg_value() const {
    return is_neg(fpv_);
  }

  bool has_zero_value() const {
    return is_zero(fpv_);
  }

  robust_fpt operator-() const {
    return robust_fpt(-fpv_, re_);
  }

  robust_fpt& operator+=(const robust_fpt& that) {
    floating_point_type fpv = this->fpv_ + that.fpv_;
    if ((!is_neg(this->fpv_) && !is_neg(that.fpv_)) ||
        (!is_pos(this->fpv_) && !is_pos(that.fpv_))) {
      this->re_ = (std::max)(this->re_, that.re_) + ROUNDING_ERROR;
    } else {
      floating_point_type temp =
        (this->fpv_ * this->re_ - that.fpv_ * that.re_) / fpv;
      if (is_neg(temp))
        temp = -temp;
      this->re_ = temp + ROUNDING_ERROR;
    }
    this->fpv_ = fpv;
    return *this;
  }

  robust_fpt& operator-=(const robust_fpt& that) {
    floating_point_type fpv = this->fpv_ - that.fpv_;
    if ((!is_neg(this->fpv_) && !is_pos(that.fpv_)) ||
        (!is_pos(this->fpv_) && !is_neg(that.fpv_))) {
       this->re_ = (std::max)(this->re_, that.re_) + ROUNDING_ERROR;
    } else {
      floating_point_type temp =
        (this->fpv_ * this->re_ + that.fpv_ * that.re_) / fpv;
      if (is_neg(temp))
         temp = -temp;
      this->re_ = temp + ROUNDING_ERROR;
    }
    this->fpv_ = fpv;
    return *this;
  }

  robust_fpt& operator*=(const robust_fpt& that) {
    this->re_ += that.re_ + ROUNDING_ERROR;
    this->fpv_ *= that.fpv_;
    return *this;
  }

  robust_fpt& operator/=(const robust_fpt& that) {
    this->re_ += that.re_ + ROUNDING_ERROR;
    this->fpv_ /= that.fpv_;
    return *this;
  }

  robust_fpt operator+(const robust_fpt& that) const {
    floating_point_type fpv = this->fpv_ + that.fpv_;
    relative_error_type re;
    if ((!is_neg(this->fpv_) && !is_neg(that.fpv_)) ||
        (!is_pos(this->fpv_) && !is_pos(that.fpv_))) {
      re = (std::max)(this->re_, that.re_) + ROUNDING_ERROR;
    } else {
      floating_point_type temp =
        (this->fpv_ * this->re_ - that.fpv_ * that.re_) / fpv;
      if (is_neg(temp))
        temp = -temp;
      re = temp + ROUNDING_ERROR;
    }
    return robust_fpt(fpv, re);
  }

  robust_fpt operator-(const robust_fpt& that) const {
    floating_point_type fpv = this->fpv_ - that.fpv_;
    relative_error_type re;
    if ((!is_neg(this->fpv_) && !is_pos(that.fpv_)) ||
        (!is_pos(this->fpv_) && !is_neg(that.fpv_))) {
      re = (std::max)(this->re_, that.re_) + ROUNDING_ERROR;
    } else {
      floating_point_type temp =
        (this->fpv_ * this->re_ + that.fpv_ * that.re_) / fpv;
      if (is_neg(temp))
        temp = -temp;
      re = temp + ROUNDING_ERROR;
    }
    return robust_fpt(fpv, re);
  }

  robust_fpt operator*(const robust_fpt& that) const {
    floating_point_type fpv = this->fpv_ * that.fpv_;
    relative_error_type re = this->re_ + that.re_ + ROUNDING_ERROR;
    return robust_fpt(fpv, re);
  }

  robust_fpt operator/(const robust_fpt& that) const {
    floating_point_type fpv = this->fpv_ / that.fpv_;
    relative_error_type re = this->re_ + that.re_ + ROUNDING_ERROR;
    return robust_fpt(fpv, re);
  }

  robust_fpt sqrt() const {
    return robust_fpt(get_sqrt(fpv_),
                      re_ * static_cast<relative_error_type>(0.5) +
                      ROUNDING_ERROR);
  }

 private:
  floating_point_type fpv_;
  relative_error_type re_;
};

template <typename T>
robust_fpt<T> get_sqrt(const robust_fpt<T>& that) {
  return that.sqrt();
}

template <typename T>
bool is_pos(const robust_fpt<T>& that) {
  return that.has_pos_value();
}

template <typename T>
bool is_neg(const robust_fpt<T>& that) {
  return that.has_neg_value();
}

template <typename T>
bool is_zero(const robust_fpt<T>& that) {
  return that.has_zero_value();
}

// robust_dif consists of two not negative values: value1 and value2.
// The resulting expression is equal to the value1 - value2.
// Subtraction of a positive value is equivalent to the addition to value2
// and subtraction of a negative value is equivalent to the addition to
// value1. The structure implicitly avoids difference computation.
template <typename T>
class robust_dif {
 public:
  robust_dif() :
      positive_sum_(0),
      negative_sum_(0) {}

  explicit robust_dif(const T& value) :
      positive_sum_((value > 0)?value:0),
      negative_sum_((value < 0)?-value:0) {}

  robust_dif(const T& pos, const T& neg) :
      positive_sum_(pos),
      negative_sum_(neg) {}

  T dif() const {
    return positive_sum_ - negative_sum_;
  }

  T pos() const {
    return positive_sum_;
  }

  T neg() const {
    return negative_sum_;
  }

  robust_dif<T> operator-() const {
    return robust_dif(negative_sum_, positive_sum_);
  }

  robust_dif<T>& operator+=(const T& val) {
    if (!is_neg(val))
      positive_sum_ += val;
    else
      negative_sum_ -= val;
    return *this;
  }

  robust_dif<T>& operator+=(const robust_dif<T>& that) {
    positive_sum_ += that.positive_sum_;
    negative_sum_ += that.negative_sum_;
    return *this;
  }

  robust_dif<T>& operator-=(const T& val) {
    if (!is_neg(val))
      negative_sum_ += val;
    else
      positive_sum_ -= val;
    return *this;
  }

  robust_dif<T>& operator-=(const robust_dif<T>& that) {
    positive_sum_ += that.negative_sum_;
    negative_sum_ += that.positive_sum_;
    return *this;
  }

  robust_dif<T>& operator*=(const T& val) {
    if (!is_neg(val)) {
      positive_sum_ *= val;
      negative_sum_ *= val;
    } else {
      positive_sum_ *= -val;
      negative_sum_ *= -val;
      swap();
    }
    return *this;
  }

  robust_dif<T>& operator*=(const robust_dif<T>& that) {
    T positive_sum = this->positive_sum_ * that.positive_sum_ +
                     this->negative_sum_ * that.negative_sum_;
    T negative_sum = this->positive_sum_ * that.negative_sum_ +
                     this->negative_sum_ * that.positive_sum_;
    positive_sum_ = positive_sum;
    negative_sum_ = negative_sum;
    return *this;
  }

  robust_dif<T>& operator/=(const T& val) {
    if (!is_neg(val)) {
      positive_sum_ /= val;
      negative_sum_ /= val;
    } else {
      positive_sum_ /= -val;
      negative_sum_ /= -val;
      swap();
    }
    return *this;
  }

 private:
  void swap() {
    (std::swap)(positive_sum_, negative_sum_);
  }

  T positive_sum_;
  T negative_sum_;
};

template<typename T>
robust_dif<T> operator+(const robust_dif<T>& lhs,
                        const robust_dif<T>& rhs) {
  return robust_dif<T>(lhs.pos() + rhs.pos(), lhs.neg() + rhs.neg());
}

template<typename T>
robust_dif<T> operator+(const robust_dif<T>& lhs, const T& rhs) {
  if (!is_neg(rhs)) {
    return robust_dif<T>(lhs.pos() + rhs, lhs.neg());
  } else {
    return robust_dif<T>(lhs.pos(), lhs.neg() - rhs);
  }
}

template<typename T>
robust_dif<T> operator+(const T& lhs, const robust_dif<T>& rhs) {
  if (!is_neg(lhs)) {
    return robust_dif<T>(lhs + rhs.pos(), rhs.neg());
  } else {
    return robust_dif<T>(rhs.pos(), rhs.neg() - lhs);
  }
}

template<typename T>
robust_dif<T> operator-(const robust_dif<T>& lhs,
                        const robust_dif<T>& rhs) {
  return robust_dif<T>(lhs.pos() + rhs.neg(), lhs.neg() + rhs.pos());
}

template<typename T>
robust_dif<T> operator-(const robust_dif<T>& lhs, const T& rhs) {
  if (!is_neg(rhs)) {
    return robust_dif<T>(lhs.pos(), lhs.neg() + rhs);
  } else {
    return robust_dif<T>(lhs.pos() - rhs, lhs.neg());
  }
}

template<typename T>
robust_dif<T> operator-(const T& lhs, const robust_dif<T>& rhs) {
  if (!is_neg(lhs)) {
    return robust_dif<T>(lhs + rhs.neg(), rhs.pos());
  } else {
    return robust_dif<T>(rhs.neg(), rhs.pos() - lhs);
  }
}

template<typename T>
robust_dif<T> operator*(const robust_dif<T>& lhs,
                        const robust_dif<T>& rhs) {
  T res_pos = lhs.pos() * rhs.pos() + lhs.neg() * rhs.neg();
  T res_neg = lhs.pos() * rhs.neg() + lhs.neg() * rhs.pos();
  return robust_dif<T>(res_pos, res_neg);
}

template<typename T>
robust_dif<T> operator*(const robust_dif<T>& lhs, const T& val) {
  if (!is_neg(val)) {
    return robust_dif<T>(lhs.pos() * val, lhs.neg() * val);
  } else {
    return robust_dif<T>(-lhs.neg() * val, -lhs.pos() * val);
  }
}

template<typename T>
robust_dif<T> operator*(const T& val, const robust_dif<T>& rhs) {
  if (!is_neg(val)) {
    return robust_dif<T>(val * rhs.pos(), val * rhs.neg());
  } else {
    return robust_dif<T>(-val * rhs.neg(), -val * rhs.pos());
  }
}

template<typename T>
robust_dif<T> operator/(const robust_dif<T>& lhs, const T& val) {
  if (!is_neg(val)) {
    return robust_dif<T>(lhs.pos() / val, lhs.neg() / val);
  } else {
    return robust_dif<T>(-lhs.neg() / val, -lhs.pos() / val);
  }
}

// Used to compute expressions that operate with sqrts with predefined
// relative error. Evaluates expressions of the next type:
// sum(i = 1 .. n)(A[i] * sqrt(B[i])), 1 <= n <= 4.
template <typename _int, typename _fpt, typename _converter>
class robust_sqrt_expr {
 public:
  enum MAX_RELATIVE_ERROR {
    MAX_RELATIVE_ERROR_EVAL1 = 4,
    MAX_RELATIVE_ERROR_EVAL2 = 7,
    MAX_RELATIVE_ERROR_EVAL3 = 16,
    MAX_RELATIVE_ERROR_EVAL4 = 25
  };

  // Evaluates expression (re = 4 EPS):
  // A[0] * sqrt(B[0]).
  _fpt eval1(_int* A, _int* B) {
    _fpt a = convert(A[0]);
    _fpt b = convert(B[0]);
    return a * get_sqrt(b);
  }

  // Evaluates expression (re = 7 EPS):
  // A[0] * sqrt(B[0]) + A[1] * sqrt(B[1]).
  _fpt eval2(_int* A, _int* B) {
    _fpt a = eval1(A, B);
    _fpt b = eval1(A + 1, B + 1);
    if ((!is_neg(a) && !is_neg(b)) ||
        (!is_pos(a) && !is_pos(b)))
      return a + b;
    return convert(A[0] * A[0] * B[0] - A[1] * A[1] * B[1]) / (a - b);
  }

  // Evaluates expression (re = 16 EPS):
  // A[0] * sqrt(B[0]) + A[1] * sqrt(B[1]) + A[2] * sqrt(B[2]).
  _fpt eval3(_int* A, _int* B) {
    _fpt a = eval2(A, B);
    _fpt b = eval1(A + 2, B + 2);
    if ((!is_neg(a) && !is_neg(b)) ||
        (!is_pos(a) && !is_pos(b)))
      return a + b;
    tA[3] = A[0] * A[0] * B[0] + A[1] * A[1] * B[1] - A[2] * A[2] * B[2];
    tB[3] = 1;
    tA[4] = A[0] * A[1] * 2;
    tB[4] = B[0] * B[1];
    return eval2(tA + 3, tB + 3) / (a - b);
  }


  // Evaluates expression (re = 25 EPS):
  // A[0] * sqrt(B[0]) + A[1] * sqrt(B[1]) +
  // A[2] * sqrt(B[2]) + A[3] * sqrt(B[3]).
  _fpt eval4(_int* A, _int* B) {
    _fpt a = eval2(A, B);
    _fpt b = eval2(A + 2, B + 2);
    if ((!is_neg(a) && !is_neg(b)) ||
        (!is_pos(a) && !is_pos(b)))
      return a + b;
    tA[0] = A[0] * A[0] * B[0] + A[1] * A[1] * B[1] -
            A[2] * A[2] * B[2] - A[3] * A[3] * B[3];
    tB[0] = 1;
    tA[1] = A[0] * A[1] * 2;
    tB[1] = B[0] * B[1];
    tA[2] = A[2] * A[3] * -2;
    tB[2] = B[2] * B[3];
    return eval3(tA, tB) / (a - b);
  }

 private:
  _int tA[5];
  _int tB[5];
  _converter convert;
};
}  // detail
}  // polygon
}  // boost

#endif  // BOOST_POLYGON_DETAIL_VORONOI_ROBUST_FPT
