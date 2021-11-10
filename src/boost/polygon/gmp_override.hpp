/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_GMP_OVERRIDE_HPP
#define BOOST_POLYGON_GMP_OVERRIDE_HPP
#include <gmpxx.h>
namespace boost { namespace polygon {

  class gmp_int {
  private:
    inline gmp_int(const mpq_class& input) : v_(input) {}
  public:
    inline gmp_int() {}
    explicit inline gmp_int(long input) : v_(input) {}
    inline gmp_int(const gmp_int& input) : v_(input.v_) {}
    inline gmp_int& operator=(const gmp_int& that) {
      v_ = that.v_;
      return (*this);
    }
    inline gmp_int& operator=(long that) {
      v_ = that;
      return (*this);
    }
    inline operator int() const {
      std::cout << "cast\n";
      mpz_class num = v_.get_num();
      mpz_class den = v_.get_den();
      num /= den;
      return num.get_si();
    }
    inline double get_d() const {
      return v_.get_d();
    }
    inline int get_num() const {
      return v_.get_num().get_si();
    }
    inline int get_den() const {
      return v_.get_den().get_si();
    }
    inline bool operator==(const gmp_int& that) const {
      return v_ == that.v_;
    }
    inline bool operator!=(const gmp_int& that) const {
      return v_ != that.v_;
    }
    inline bool operator<(const gmp_int& that) const {
      bool retval = v_ < that.v_;
      return retval;
    }
    inline bool operator<=(const gmp_int& that) const {
      return v_ <= that.v_;
    }
    inline bool operator>(const gmp_int& that) const {
      return v_ > that.v_;
    }
    inline bool operator>=(const gmp_int& that) const {
      return v_ >= that.v_;
    }
    inline gmp_int operator+(const gmp_int& b) {
      return gmp_int((*this).v_ + b.v_);
    }
    inline gmp_int operator-(const gmp_int& b) {
      return gmp_int((*this).v_ - b.v_);
    }
    inline gmp_int operator*(const gmp_int& b) {
      return gmp_int((*this).v_ * b.v_);
    }
    inline gmp_int operator/(const gmp_int& b) {
      return gmp_int((*this).v_ / b.v_);
    }
    inline gmp_int& operator+=(const gmp_int& b) {
      (*this).v_ += b.v_;
      return (*this);
    }
    inline gmp_int& operator-=(const gmp_int& b) {
      (*this).v_ -= b.v_;
      return (*this);
    }
    inline gmp_int& operator*=(const gmp_int& b) {
      (*this).v_ *= b.v_;
      return (*this);
    }
    inline gmp_int& operator/=(const gmp_int& b) {
      (*this).v_ /= b.v_;
      return (*this);
    }
    inline gmp_int& operator++() {
      ++v_;
      return (*this);
    }
    inline gmp_int& operator--() {
      --v_;
      return (*this);
    }
    inline gmp_int operator++(int) {
      gmp_int retval(*this);
      ++(*this);
      return retval;
    }
    inline gmp_int operator--(int) {
      gmp_int retval(*this);
      --(*this);
      return retval;
    }
  private:
    mpq_class v_;
  };

  template <>
  struct high_precision_type<int> {
    typedef mpq_class type;
  };

  template <>
  int convert_high_precision_type<int>(const mpq_class& v) {
    mpz_class num = v.get_num();
    mpz_class den = v.get_den();
    num /= den;
    return num.get_si();
  };

}
}
#endif
