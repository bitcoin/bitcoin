//  ratio.hpp  ---------------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*

This code was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.4 Compile-time rational arithmetic [ratio], of the C++ committee working
paper N2798.
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

// The way overflow is managed for ratio_less is taken from llvm/libcxx/include/ratio

#ifndef BOOST_RATIO_DETAIL_RATIO_OPERATIONS_HPP
#define BOOST_RATIO_DETAIL_RATIO_OPERATIONS_HPP

#include <boost/ratio/config.hpp>
#include <boost/ratio/detail/mpl/abs.hpp>
#include <boost/ratio/detail/mpl/sign.hpp>
#include <cstdlib>
#include <climits>
#include <limits>
#include <boost/cstdint.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/integer_traits.hpp>

//
// We simply cannot include this header on gcc without getting copious warnings of the kind:
//
// boost/integer.hpp:77:30: warning: use of C99 long long integer constant
//
// And yet there is no other reasonable implementation, so we declare this a system header
// to suppress these warnings.
//
#if defined(__GNUC__) && (__GNUC__ >= 4)
#pragma GCC system_header
#endif

namespace boost
{

//----------------------------------------------------------------------------//
//                                 helpers                                    //
//----------------------------------------------------------------------------//

namespace ratio_detail
{

  template <boost::intmax_t X, boost::intmax_t Y, boost::intmax_t = mpl::sign_c<boost::intmax_t, Y>::value>
  class br_add;

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_add<X, Y, 1>
  {
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      BOOST_RATIO_STATIC_ASSERT(X <= max - Y , BOOST_RATIO_OVERFLOW_IN_ADD, ());
  public:
      static const boost::intmax_t value = X + Y;
  };

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_add<X, Y, 0>
  {
  public:
      static const boost::intmax_t value = X;
  };

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_add<X, Y, -1>
  {
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      BOOST_RATIO_STATIC_ASSERT(min - Y <= X, BOOST_RATIO_OVERFLOW_IN_ADD, ());
  public:
      static const boost::intmax_t value = X + Y;
  };

  template <boost::intmax_t X, boost::intmax_t Y, boost::intmax_t = mpl::sign_c<boost::intmax_t, Y>::value>
  class br_sub;

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_sub<X, Y, 1>
  {
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      BOOST_RATIO_STATIC_ASSERT(min + Y <= X, BOOST_RATIO_OVERFLOW_IN_SUB, ());
  public:
      static const boost::intmax_t value = X - Y;
  };

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_sub<X, Y, 0>
  {
  public:
      static const boost::intmax_t value = X;
  };

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_sub<X, Y, -1>
  {
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      BOOST_RATIO_STATIC_ASSERT(X <= max + Y, BOOST_RATIO_OVERFLOW_IN_SUB, ());
  public:
      static const boost::intmax_t value = X - Y;
  };

  template <boost::intmax_t X, boost::intmax_t Y>
  class br_mul
  {
      static const boost::intmax_t nan =
          boost::intmax_t(BOOST_RATIO_UINTMAX_C(1) << (sizeof(boost::intmax_t) * CHAR_BIT - 1));
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      static const boost::intmax_t a_x = mpl::abs_c<boost::intmax_t, X>::value;
      static const boost::intmax_t a_y = mpl::abs_c<boost::intmax_t, Y>::value;

      BOOST_RATIO_STATIC_ASSERT(X != nan, BOOST_RATIO_OVERFLOW_IN_MUL, ());
      BOOST_RATIO_STATIC_ASSERT(Y != nan, BOOST_RATIO_OVERFLOW_IN_MUL, ());
      BOOST_RATIO_STATIC_ASSERT(a_x <= max / a_y, BOOST_RATIO_OVERFLOW_IN_MUL, ());
  public:
      static const boost::intmax_t value = X * Y;
  };

  template <boost::intmax_t Y>
  class br_mul<0, Y>
  {
  public:
      static const boost::intmax_t value = 0;
  };

  template <boost::intmax_t X>
  class br_mul<X, 0>
  {
  public:
      static const boost::intmax_t value = 0;
  };

  template <>
  class br_mul<0, 0>
  {
  public:
      static const boost::intmax_t value = 0;
  };

  // Not actually used but left here in case needed in future maintenance
  template <boost::intmax_t X, boost::intmax_t Y>
  class br_div
  {
      static const boost::intmax_t nan = boost::intmax_t(BOOST_RATIO_UINTMAX_C(1) << (sizeof(boost::intmax_t) * CHAR_BIT - 1));
      static const boost::intmax_t min = boost::integer_traits<boost::intmax_t>::const_min;
      static const boost::intmax_t max = boost::integer_traits<boost::intmax_t>::const_max;

      BOOST_RATIO_STATIC_ASSERT(X != nan, BOOST_RATIO_OVERFLOW_IN_DIV, ());
      BOOST_RATIO_STATIC_ASSERT(Y != nan, BOOST_RATIO_OVERFLOW_IN_DIV, ());
      BOOST_RATIO_STATIC_ASSERT(Y != 0, BOOST_RATIO_DIVIDE_BY_0, ());
  public:
      static const boost::intmax_t value = X / Y;
  };

  // ratio arithmetic
  template <class R1, class R2> struct ratio_add;
  template <class R1, class R2> struct ratio_subtract;
  template <class R1, class R2> struct ratio_multiply;
  template <class R1, class R2> struct ratio_divide;

  template <class R1, class R2>
  struct ratio_add
  {
      //The nested typedef type shall be a synonym for ratio<T1, T2>::type where T1 has the value R1::num *
      //R2::den + R2::num * R1::den and T2 has the value R1::den * R2::den.
      // As the preceding doesn't works because of overflow on boost::intmax_t we need something more elaborated.
  private:
      static const boost::intmax_t gcd_n1_n2 = mpl::gcd_c<boost::intmax_t, R1::num, R2::num>::value;
      static const boost::intmax_t gcd_d1_d2 = mpl::gcd_c<boost::intmax_t, R1::den, R2::den>::value;
  public:
      // No need to normalize as ratio_multiply is already normalized
      typedef typename ratio_multiply
         <
             ratio<gcd_n1_n2, R1::den / gcd_d1_d2>,
             ratio
             <
                 boost::ratio_detail::br_add
                 <
                     boost::ratio_detail::br_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value,
                     boost::ratio_detail::br_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value
                 >::value,
                 R2::den
             >
         >::type type;
  };
  template <class R, boost::intmax_t D>
  struct ratio_add<R, ratio<0,D> >
  {
    typedef R type;
  };

  template <class R1, class R2>
  struct ratio_subtract
  {
      //The nested typedef type shall be a synonym for ratio<T1, T2>::type where T1 has the value
      // R1::num *R2::den - R2::num * R1::den and T2 has the value R1::den * R2::den.
      // As the preceding doesn't works because of overflow on boost::intmax_t we need something more elaborated.
  private:
      static const boost::intmax_t gcd_n1_n2 = mpl::gcd_c<boost::intmax_t, R1::num, R2::num>::value;
      static const boost::intmax_t gcd_d1_d2 = mpl::gcd_c<boost::intmax_t, R1::den, R2::den>::value;
  public:
      // No need to normalize as ratio_multiply is already normalized
      typedef typename ratio_multiply
         <
             ratio<gcd_n1_n2, R1::den / gcd_d1_d2>,
             ratio
             <
                 boost::ratio_detail::br_sub
                 <
                     boost::ratio_detail::br_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value,
                     boost::ratio_detail::br_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value
                 >::value,
                 R2::den
             >
         >::type type;
  };

  template <class R, boost::intmax_t D>
  struct ratio_subtract<R, ratio<0,D> >
  {
    typedef R type;
  };

  template <class R1, class R2>
  struct ratio_multiply
  {
      // The nested typedef type  shall be a synonym for ratio<R1::num * R2::den - R2::num * R1::den, R1::den * R2::den>::type.
      // As the preceding doesn't works because of overflow on boost::intmax_t we need something more elaborated.
  private:
     static const boost::intmax_t gcd_n1_d2 = mpl::gcd_c<boost::intmax_t, R1::num, R2::den>::value;
     static const boost::intmax_t gcd_d1_n2 = mpl::gcd_c<boost::intmax_t, R1::den, R2::num>::value;
  public:
      typedef typename ratio
         <
             boost::ratio_detail::br_mul<R1::num / gcd_n1_d2, R2::num / gcd_d1_n2>::value,
             boost::ratio_detail::br_mul<R2::den / gcd_n1_d2, R1::den / gcd_d1_n2>::value
         >::type type;
  };

  template <class R1, class R2>
  struct ratio_divide
  {
      // The nested typedef type  shall be a synonym for ratio<R1::num * R2::den, R2::num * R1::den>::type.
      // As the preceding doesn't works because of overflow on boost::intmax_t we need something more elaborated.
  private:
      static const boost::intmax_t gcd_n1_n2 = mpl::gcd_c<boost::intmax_t, R1::num, R2::num>::value;
      static const boost::intmax_t gcd_d1_d2 = mpl::gcd_c<boost::intmax_t, R1::den, R2::den>::value;
  public:
      typedef typename ratio
         <
             boost::ratio_detail::br_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value,
             boost::ratio_detail::br_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value
         >::type type;
  };
  template <class R1, class R2>
  struct is_evenly_divisible_by
  {
  private:
      static const boost::intmax_t gcd_n1_n2 = mpl::gcd_c<boost::intmax_t, R1::num, R2::num>::value;
      static const boost::intmax_t gcd_d1_d2 = mpl::gcd_c<boost::intmax_t, R1::den, R2::den>::value;
  public:
      typedef integral_constant<bool,
             ((R2::num / gcd_n1_n2 ==1) && (R1::den / gcd_d1_d2)==1)
      > type;
  };

  template <class T>
  struct is_ratio : public boost::false_type
  {};
  template <boost::intmax_t N, boost::intmax_t D>
  struct is_ratio<ratio<N, D> > : public boost::true_type
  {};

  template <class R1, class R2,
            boost::intmax_t Q1 = R1::num / R1::den, boost::intmax_t M1 = R1::num % R1::den,
            boost::intmax_t Q2 = R2::num / R2::den, boost::intmax_t M2 = R2::num % R2::den>
  struct ratio_less1
  {
    static const bool value = Q1 < Q2;
  };

  template <class R1, class R2, boost::intmax_t Q>
  struct ratio_less1<R1, R2, Q, 0, Q, 0>
  {
    static const bool value = false;
  };

  template <class R1, class R2, boost::intmax_t Q, boost::intmax_t M2>
  struct ratio_less1<R1, R2, Q, 0, Q, M2>
  {
    static const bool value = true;
  };

  template <class R1, class R2, boost::intmax_t Q, boost::intmax_t M1>
  struct ratio_less1<R1, R2, Q, M1, Q, 0>
  {
    static const bool value = false;
  };

  template <class R1, class R2, boost::intmax_t Q, boost::intmax_t M1, boost::intmax_t M2>
  struct ratio_less1<R1, R2, Q, M1, Q, M2>
  {
    static const bool value = ratio_less1<ratio<R2::den, M2>, ratio<R1::den, M1>
                                            >::value;
  };

  template <
      class R1,
      class R2,
      boost::intmax_t S1 = mpl::sign_c<boost::intmax_t, R1::num>::value,
    boost::intmax_t S2 = mpl::sign_c<boost::intmax_t, R2::num>::value
>
  struct ratio_less
  {
      static const bool value = S1 < S2;
  };

  template <class R1, class R2>
  struct ratio_less<R1, R2, 1LL, 1LL>
  {
      static const bool value = ratio_less1<R1, R2>::value;
  };

  template <class R1, class R2>
  struct ratio_less<R1, R2, -1LL, -1LL>
  {
      static const bool value = ratio_less1<ratio<-R2::num, R2::den>,
                                            ratio<-R1::num, R1::den> >::value;
  };


}  // namespace ratio_detail

}  // namespace boost

#endif  // BOOST_RATIO_HPP
