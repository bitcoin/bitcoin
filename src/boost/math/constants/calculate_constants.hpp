//  Copyright John Maddock 2010, 2012.
//  Copyright Paul A. Bristow 2011, 2012.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_CALCULATE_CONSTANTS_CONSTANTS_INCLUDED
#define BOOST_MATH_CALCULATE_CONSTANTS_CONSTANTS_INCLUDED
#include <type_traits>

namespace boost{ namespace math{ namespace constants{ namespace detail{

template <class T>
template<int N>
inline T constant_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING

   return ldexp(acos(T(0)), 1);

   /*
   // Although this code works well, it's usually more accurate to just call acos
   // and access the number types own representation of PI which is usually calculated
   // at slightly higher precision...

   T result;
   T a = 1;
   T b;
   T A(a);
   T B = 0.5f;
   T D = 0.25f;

   T lim;
   lim = boost::math::tools::epsilon<T>();

   unsigned k = 1;

   do
   {
      result = A + B;
      result = ldexp(result, -2);
      b = sqrt(B);
      a += b;
      a = ldexp(a, -1);
      A = a * a;
      B = A - result;
      B = ldexp(B, 1);
      result = A - B;
      bool neg = boost::math::sign(result) < 0;
      if(neg)
         result = -result;
      if(result <= lim)
         break;
      if(neg)
         result = -result;
      result = ldexp(result, k - 1);
      D -= result;
      ++k;
      lim = ldexp(lim, 1);
   }
   while(true);

   result = B / D;
   return result;
   */
}

template <class T>
template<int N>
inline T constant_two_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 2 * pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T> // 2 / pi
template<int N>
inline T constant_two_div_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 2 / pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T>  // sqrt(2/pi)
template <int N>
inline T constant_root_two_div_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt((2 / pi<T, policies::policy<policies::digits2<N> > >()));
}

template <class T>
template<int N>
inline T constant_one_div_two_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 1 / two_pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_root_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(pi<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_root_half_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(pi<T, policies::policy<policies::digits2<N> > >() / 2);
}

template <class T>
template<int N>
inline T constant_root_two_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(two_pi<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_log_root_two_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return log(root_two_pi<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_root_ln_four<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(log(static_cast<T>(4)));
}

template <class T>
template<int N>
inline T constant_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   //
   // Although we can clearly calculate this from first principles, this hooks into
   // T's own notion of e, which hopefully will more accurate than one calculated to
   // a few epsilon:
   //
   BOOST_MATH_STD_USING
   return exp(static_cast<T>(1));
}

template <class T>
template<int N>
inline T constant_half<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return static_cast<T>(1) / static_cast<T>(2);
}

template <class T>
template<int M>
inline T constant_euler<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, M>)))
{
   BOOST_MATH_STD_USING
   //
   // This is the method described in:
   // "Some New Algorithms for High-Precision Computation of Euler's Constant"
   // Richard P Brent and Edwin M McMillan.
   // Mathematics of Computation, Volume 34, Number 149, Jan 1980, pages 305-312.
   // See equation 17 with p = 2.
   //
   T n = 3 + (M ? (std::min)(M, tools::digits<T>()) : tools::digits<T>()) / 4;
   T lim = M ? ldexp(T(1), 1 - (std::min)(M, tools::digits<T>())) : tools::epsilon<T>();
   T lnn = log(n);
   T term = 1;
   T N = -lnn;
   T D = 1;
   T Hk = 0;
   T one = 1;

   for(unsigned k = 1;; ++k)
   {
      term *= n * n;
      term /= k * k;
      Hk += one / k;
      N += term * (Hk - lnn);
      D += term;

      if(term < D * lim)
         break;
   }
   return N / D;
}

template <class T>
template<int N>
inline T constant_euler_sqr<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  BOOST_MATH_STD_USING
  return euler<T, policies::policy<policies::digits2<N> > >()
     * euler<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_one_div_euler<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  BOOST_MATH_STD_USING
  return static_cast<T>(1)
     / euler<T, policies::policy<policies::digits2<N> > >();
}


template <class T>
template<int N>
inline T constant_root_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(static_cast<T>(2));
}


template <class T>
template<int N>
inline T constant_root_three<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(static_cast<T>(3));
}

template <class T>
template<int N>
inline T constant_half_root_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(static_cast<T>(2)) / 2;
}

template <class T>
template<int N>
inline T constant_ln_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   //
   // Although there are good ways to calculate this from scratch, this hooks into
   // T's own notion of log(2) which will hopefully be accurate to the full precision
   // of T:
   //
   BOOST_MATH_STD_USING
   return log(static_cast<T>(2));
}

template <class T>
template<int N>
inline T constant_ln_ten<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return log(static_cast<T>(10));
}

template <class T>
template<int N>
inline T constant_ln_ln_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return log(log(static_cast<T>(2)));
}

template <class T>
template<int N>
inline T constant_third<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(1) / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_twothirds<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(2) / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_two_thirds<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(2) / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_three_quarters<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(3) / static_cast<T>(4);
}

template <class T>
template<int N>
inline T constant_sixth<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  BOOST_MATH_STD_USING
  return static_cast<T>(1) / static_cast<T>(6);
}

// Pi and related constants.
template <class T>
template<int N>
inline T constant_pi_minus_three<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return pi<T, policies::policy<policies::digits2<N> > >() - static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_four_minus_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return static_cast<T>(4) - pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_exp_minus_half<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return exp(static_cast<T>(-0.5));
}

template <class T>
template<int N>
inline T constant_exp_minus_one<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  BOOST_MATH_STD_USING
  return exp(static_cast<T>(-1.));
}

template <class T>
template<int N>
inline T constant_one_div_root_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return static_cast<T>(1) / root_two<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_one_div_root_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return static_cast<T>(1) / root_pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_one_div_root_two_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return static_cast<T>(1) / root_two_pi<T, policies::policy<policies::digits2<N> > >();
}

template <class T>
template<int N>
inline T constant_root_one_div_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(static_cast<T>(1) / pi<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_four_thirds_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >() * static_cast<T>(4) / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_half_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()  / static_cast<T>(2);
}

template <class T>
template<int N>
inline T constant_third_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()  / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_sixth_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()  / static_cast<T>(6);
}

template <class T>
template<int N>
inline T constant_two_thirds_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >() * static_cast<T>(2) / static_cast<T>(3);
}

template <class T>
template<int N>
inline T constant_three_quarters_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >() * static_cast<T>(3) / static_cast<T>(4);
}

template <class T>
template<int N>
inline T constant_pi_pow_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pow(pi<T, policies::policy<policies::digits2<N> > >(), e<T, policies::policy<policies::digits2<N> > >()); //
}

template <class T>
template<int N>
inline T constant_pi_sqr<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()
   *      pi<T, policies::policy<policies::digits2<N> > >() ; //
}

template <class T>
template<int N>
inline T constant_pi_sqr_div_six<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()
   *      pi<T, policies::policy<policies::digits2<N> > >()
   / static_cast<T>(6); //
}

template <class T>
template<int N>
inline T constant_pi_cubed<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()
   *      pi<T, policies::policy<policies::digits2<N> > >()
   *      pi<T, policies::policy<policies::digits2<N> > >()
   ; //
}

template <class T>
template<int N>
inline T constant_cbrt_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pow(pi<T, policies::policy<policies::digits2<N> > >(), static_cast<T>(1)/ static_cast<T>(3));
}

template <class T>
template<int N>
inline T constant_one_div_cbrt_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(1)
   / pow(pi<T, policies::policy<policies::digits2<N> > >(), static_cast<T>(1)/ static_cast<T>(3));
}

// Euler's e

template <class T>
template<int N>
inline T constant_e_pow_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pow(e<T, policies::policy<policies::digits2<N> > >(), pi<T, policies::policy<policies::digits2<N> > >()); //
}

template <class T>
template<int N>
inline T constant_root_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sqrt(e<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_log10_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return log10(e<T, policies::policy<policies::digits2<N> > >());
}

template <class T>
template<int N>
inline T constant_one_div_log10_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return  static_cast<T>(1) /
     log10(e<T, policies::policy<policies::digits2<N> > >());
}

// Trigonometric

template <class T>
template<int N>
inline T constant_degree<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return pi<T, policies::policy<policies::digits2<N> > >()
   / static_cast<T>(180)
   ; //
}

template <class T>
template<int N>
inline T constant_radian<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(180)
   / pi<T, policies::policy<policies::digits2<N> > >()
   ; //
}

template <class T>
template<int N>
inline T constant_sin_one<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sin(static_cast<T>(1)) ; //
}

template <class T>
template<int N>
inline T constant_cos_one<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return cos(static_cast<T>(1)) ; //
}

template <class T>
template<int N>
inline T constant_sinh_one<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return sinh(static_cast<T>(1)) ; //
}

template <class T>
template<int N>
inline T constant_cosh_one<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return cosh(static_cast<T>(1)) ; //
}

template <class T>
template<int N>
inline T constant_phi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return (static_cast<T>(1) + sqrt(static_cast<T>(5)) )/static_cast<T>(2) ; //
}

template <class T>
template<int N>
inline T constant_ln_phi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return log((static_cast<T>(1) + sqrt(static_cast<T>(5)) )/static_cast<T>(2) );
}

template <class T>
template<int N>
inline T constant_one_div_ln_phi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING
   return static_cast<T>(1) /
     log((static_cast<T>(1) + sqrt(static_cast<T>(5)) )/static_cast<T>(2) );
}

// Zeta

template <class T>
template<int N>
inline T constant_zeta_two<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   BOOST_MATH_STD_USING

     return pi<T, policies::policy<policies::digits2<N> > >()
     *  pi<T, policies::policy<policies::digits2<N> > >()
     /static_cast<T>(6);
}

template <class T>
template<int N>
inline T constant_zeta_three<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   // http://mathworld.wolfram.com/AperysConstant.html
   // http://en.wikipedia.org/wiki/Mathematical_constant

   // http://oeis.org/A002117/constant
   //T zeta3("1.20205690315959428539973816151144999076"
   // "4986292340498881792271555341838205786313"
   // "09018645587360933525814619915");

  //"1.202056903159594285399738161511449990, 76498629234049888179227155534183820578631309018645587360933525814619915"  A002117
  // 1.202056903159594285399738161511449990, 76498629234049888179227155534183820578631309018645587360933525814619915780, +00);
  //"1.2020569031595942 double
  // http://www.spaennare.se/SSPROG/ssnum.pdf  // section 11, Algorithm for Apery's constant zeta(3).
  // Programs to Calculate some Mathematical Constants to Large Precision, Document Version 1.50

  // by Stefan Spannare  September 19, 2007
  // zeta(3) = 1/64 * sum
   BOOST_MATH_STD_USING
   T n_fact=static_cast<T>(1); // build n! for n = 0.
   T sum = static_cast<double>(77); // Start with n = 0 case.
   // for n = 0, (77/1) /64 = 1.203125
   //double lim = std::numeric_limits<double>::epsilon();
   T lim = N ? ldexp(T(1), 1 - (std::min)(N, tools::digits<T>())) : tools::epsilon<T>();
   for(unsigned int n = 1; n < 40; ++n)
   { // three to five decimal digits per term, so 40 should be plenty for 100 decimal digits.
      //cout << "n = " << n << endl;
      n_fact *= n; // n!
      T n_fact_p10 = n_fact * n_fact * n_fact * n_fact * n_fact * n_fact * n_fact * n_fact * n_fact * n_fact; // (n!)^10
      T num = ((205 * n * n) + (250 * n) + 77) * n_fact_p10; // 205n^2 + 250n + 77
      // int nn = (2 * n + 1);
      // T d = factorial(nn); // inline factorial.
      T d = 1;
      for(unsigned int i = 1; i <= (n+n + 1); ++i) // (2n + 1)
      {
        d *= i;
      }
      T den = d * d * d * d * d; // [(2n+1)!]^5
      //cout << "den = " << den << endl;
      T term = num/den;
      if (n % 2 != 0)
      { //term *= -1;
        sum -= term;
      }
      else
      {
        sum += term;
      }
      //cout << "term = " << term << endl;
      //cout << "sum/64  = " << sum/64 << endl;
      if(abs(term) < lim)
      {
         break;
      }
   }
   return sum / 64;
}

template <class T>
template<int N>
inline T constant_catalan<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{ // http://oeis.org/A006752/constant
  //T c("0.915965594177219015054603514932384110774"
 //"149374281672134266498119621763019776254769479356512926115106248574");

  // 9.159655941772190150546035149323841107, 74149374281672134266498119621763019776254769479356512926115106248574422619, -01);

   // This is equation (entry) 31 from
   // http://www-2.cs.cmu.edu/~adamchik/articles/catalan/catalan.htm
   // See also http://www.mpfr.org/algorithms.pdf
   BOOST_MATH_STD_USING
   T k_fact = 1;
   T tk_fact = 1;
   T sum = 1;
   T term;
   T lim = N ? ldexp(T(1), 1 - (std::min)(N, tools::digits<T>())) : tools::epsilon<T>();

   for(unsigned k = 1;; ++k)
   {
      k_fact *= k;
      tk_fact *= (2 * k) * (2 * k - 1);
      term = k_fact * k_fact / (tk_fact * (2 * k + 1) * (2 * k + 1));
      sum += term;
      if(term < lim)
      {
         break;
      }
   }
   return boost::math::constants::pi<T, boost::math::policies::policy<> >()
      * log(2 + boost::math::constants::root_three<T, boost::math::policies::policy<> >())
       / 8
      + 3 * sum / 8;
}

namespace khinchin_detail{

template <class T>
T zeta_polynomial_series(T s, T sc, int digits)
{
   BOOST_MATH_STD_USING
   //
   // This is algorithm 3 from:
   //
   // "An Efficient Algorithm for the Riemann Zeta Function", P. Borwein,
   // Canadian Mathematical Society, Conference Proceedings, 2000.
   // See: http://www.cecm.sfu.ca/personal/pborwein/PAPERS/P155.pdf
   //
   BOOST_MATH_STD_USING
   int n = (digits * 19) / 53;
   T sum = 0;
   T two_n = ldexp(T(1), n);
   int ej_sign = 1;
   for(int j = 0; j < n; ++j)
   {
      sum += ej_sign * -two_n / pow(T(j + 1), s);
      ej_sign = -ej_sign;
   }
   T ej_sum = 1;
   T ej_term = 1;
   for(int j = n; j <= 2 * n - 1; ++j)
   {
      sum += ej_sign * (ej_sum - two_n) / pow(T(j + 1), s);
      ej_sign = -ej_sign;
      ej_term *= 2 * n - j;
      ej_term /= j - n + 1;
      ej_sum += ej_term;
   }
   return -sum / (two_n * (1 - pow(T(2), sc)));
}

template <class T>
T khinchin(int digits)
{
   BOOST_MATH_STD_USING
   T sum = 0;
   T term;
   T lim = ldexp(T(1), 1-digits);
   T factor = 0;
   unsigned last_k = 1;
   T num = 1;
   for(unsigned n = 1;; ++n)
   {
      for(unsigned k = last_k; k <= 2 * n - 1; ++k)
      {
         factor += num / k;
         num = -num;
      }
      last_k = 2 * n;
      term = (zeta_polynomial_series(T(2 * n), T(1 - T(2 * n)), digits) - 1) * factor / n;
      sum += term;
      if(term < lim)
         break;
   }
   return exp(sum / boost::math::constants::ln_two<T, boost::math::policies::policy<> >());
}

}

template <class T>
template<int N>
inline T constant_khinchin<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   int n = N ? (std::min)(N, tools::digits<T>()) : tools::digits<T>();
   return khinchin_detail::khinchin<T>(n);
}

template <class T>
template<int N>
inline T constant_extreme_value_skewness<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{  // N[12 Sqrt[6]  Zeta[3]/Pi^3, 1101]
   BOOST_MATH_STD_USING
   T ev(12 * sqrt(static_cast<T>(6)) * zeta_three<T, policies::policy<policies::digits2<N> > >()
    / pi_cubed<T, policies::policy<policies::digits2<N> > >() );

//T ev(
//"1.1395470994046486574927930193898461120875997958365518247216557100852480077060706857071875468869385150"
//"1894272048688553376986765366075828644841024041679714157616857834895702411080704529137366329462558680"
//"2015498788776135705587959418756809080074611906006528647805347822929577145038743873949415294942796280"
//"0895597703063466053535550338267721294164578901640163603544404938283861127819804918174973533694090594"
//"3094963822672055237678432023017824416203652657301470473548274848068762500300316769691474974950757965"
//"8640779777748741897542093874605477776538884083378029488863880220988107155275203245233994097178778984"
//"3488995668362387892097897322246698071290011857605809901090220903955815127463328974447572119951192970"
//"3684453635456559086126406960279692862247058250100678008419431185138019869693206366891639436908462809"
//"9756051372711251054914491837034685476095423926553367264355374652153595857163724698198860485357368964"
//"3807049634423621246870868566707915720704996296083373077647528285782964567312903914752617978405994377"
//"9064157147206717895272199736902453130842229559980076472936976287378945035706933650987259357729800315");

  return ev;
}

namespace detail{
//
// Calculation of the Glaisher constant depends upon calculating the
// derivative of the zeta function at 2, we can then use the relation:
// zeta'(2) = 1/6 pi^2 [euler + ln(2pi)-12ln(A)]
// To get the constant A.
// See equation 45 at http://mathworld.wolfram.com/RiemannZetaFunction.html.
//
// The derivative of the zeta function is computed by direct differentiation
// of the relation:
// (1-2^(1-s))zeta(s) = SUM(n=0, INF){ (-n)^n / (n+1)^s  }
// Which gives us 2 slowly converging but alternating sums to compute,
// for this we use Algorithm 1 from "Convergent Acceleration of Alternating Series",
// Henri Cohen, Fernando Rodriguez Villegas and Don Zagier, Experimental Mathematics 9:1 (1999).
// See http://www.math.utexas.edu/users/villegas/publications/conv-accel.pdf
//
template <class T>
T zeta_series_derivative_2(unsigned digits)
{
   // Derivative of the series part, evaluated at 2:
   BOOST_MATH_STD_USING
   int n = digits * 301 * 13 / 10000;
   T d = pow(3 + sqrt(T(8)), n);
   d = (d + 1 / d) / 2;
   T b = -1;
   T c = -d;
   T s = 0;
   for(int k = 0; k < n; ++k)
   {
      T a = -log(T(k+1)) / ((k+1) * (k+1));
      c = b - c;
      s = s + c * a;
      b = (k + n) * (k - n) * b / ((k + T(0.5f)) * (k + 1));
   }
   return s / d;
}

template <class T>
T zeta_series_2(unsigned digits)
{
   // Series part of zeta at 2:
   BOOST_MATH_STD_USING
   int n = digits * 301 * 13 / 10000;
   T d = pow(3 + sqrt(T(8)), n);
   d = (d + 1 / d) / 2;
   T b = -1;
   T c = -d;
   T s = 0;
   for(int k = 0; k < n; ++k)
   {
      T a = T(1) / ((k + 1) * (k + 1));
      c = b - c;
      s = s + c * a;
      b = (k + n) * (k - n) * b / ((k + T(0.5f)) * (k + 1));
   }
   return s / d;
}

template <class T>
inline T zeta_series_lead_2()
{
   // lead part at 2:
   return 2;
}

template <class T>
inline T zeta_series_derivative_lead_2()
{
   // derivative of lead part at 2:
   return -2 * boost::math::constants::ln_two<T>();
}

template <class T>
inline T zeta_derivative_2(unsigned n)
{
   // zeta derivative at 2:
   return zeta_series_derivative_2<T>(n) * zeta_series_lead_2<T>()
      + zeta_series_derivative_lead_2<T>() * zeta_series_2<T>(n);
}

}  // namespace detail

template <class T>
template<int N>
inline T constant_glaisher<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{

   BOOST_MATH_STD_USING
   typedef policies::policy<policies::digits2<N> > forwarding_policy;
   int n = N ? (std::min)(N, tools::digits<T>()) : tools::digits<T>();
   T v = detail::zeta_derivative_2<T>(n);
   v *= 6;
   v /= boost::math::constants::pi<T, forwarding_policy>() * boost::math::constants::pi<T, forwarding_policy>();
   v -= boost::math::constants::euler<T, forwarding_policy>();
   v -= log(2 * boost::math::constants::pi<T, forwarding_policy>());
   v /= -12;
   return exp(v);

   /*
   // from http://mpmath.googlecode.com/svn/data/glaisher.txt
     // 20,000 digits of the Glaisher-Kinkelin constant A = exp(1/2 - zeta'(-1))
     // Computed using A = exp((6 (-zeta'(2))/pi^2 + log 2 pi + gamma)/12)
   // with Euler-Maclaurin summation for zeta'(2).
   T g(
   "1.282427129100622636875342568869791727767688927325001192063740021740406308858826"
   "46112973649195820237439420646120399000748933157791362775280404159072573861727522"
   "14334327143439787335067915257366856907876561146686449997784962754518174312394652"
   "76128213808180219264516851546143919901083573730703504903888123418813674978133050"
   "93770833682222494115874837348064399978830070125567001286994157705432053927585405"
   "81731588155481762970384743250467775147374600031616023046613296342991558095879293"
   "36343887288701988953460725233184702489001091776941712153569193674967261270398013"
   "52652668868978218897401729375840750167472114895288815996668743164513890306962645"
   "59870469543740253099606800842447417554061490189444139386196089129682173528798629"
   "88434220366989900606980888785849587494085307347117090132667567503310523405221054"
   "14176776156308191919997185237047761312315374135304725819814797451761027540834943"
   "14384965234139453373065832325673954957601692256427736926358821692159870775858274"
   "69575162841550648585890834128227556209547002918593263079373376942077522290940187");

   return g;
   */
}

template <class T>
template<int N>
inline T constant_rayleigh_skewness<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{  // 1100 digits of the Rayleigh distribution skewness
   // N[2 Sqrt[Pi] (Pi - 3)/((4 - Pi)^(3/2)), 1100]

   BOOST_MATH_STD_USING
   T rs(2 * root_pi<T, policies::policy<policies::digits2<N> > >()
      * pi_minus_three<T, policies::policy<policies::digits2<N> > >()
      / pow(four_minus_pi<T, policies::policy<policies::digits2<N> > >(), static_cast<T>(3./2))
      );
   //   6.31110657818937138191899351544227779844042203134719497658094585692926819617473725459905027032537306794400047264,

   //"0.6311106578189371381918993515442277798440422031347194976580945856929268196174737254599050270325373067"
   //"9440004726436754739597525250317640394102954301685809920213808351450851396781817932734836994829371322"
   //"5797376021347531983451654130317032832308462278373358624120822253764532674177325950686466133508511968"
   //"2389168716630349407238090652663422922072397393006683401992961569208109477307776249225072042971818671"
   //"4058887072693437217879039875871765635655476241624825389439481561152126886932506682176611183750503553"
   //"1218982627032068396407180216351425758181396562859085306247387212297187006230007438534686340210168288"
   //"8956816965453815849613622117088096547521391672977226658826566757207615552041767516828171274858145957"
   //"6137539156656005855905288420585194082284972984285863898582313048515484073396332610565441264220790791"
   //"0194897267890422924599776483890102027823328602965235306539844007677157873140562950510028206251529523"
   //"7428049693650605954398446899724157486062545281504433364675815915402937209673727753199567661561209251"
   //"4695589950526053470201635372590001578503476490223746511106018091907936826431407434894024396366284848");  ;
   return rs;
}

template <class T>
template<int N>
inline T constant_rayleigh_kurtosis_excess<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{ // - (6 Pi^2 - 24 Pi + 16)/((Pi - 4)^2)
    // Might provide and calculate this using pi_minus_four.
   BOOST_MATH_STD_USING
   return - (((static_cast<T>(6) * pi<T, policies::policy<policies::digits2<N> > >()
        * pi<T, policies::policy<policies::digits2<N> > >())
   - (static_cast<T>(24) * pi<T, policies::policy<policies::digits2<N> > >()) + static_cast<T>(16) )
   /
   ((pi<T, policies::policy<policies::digits2<N> > >() - static_cast<T>(4))
   * (pi<T, policies::policy<policies::digits2<N> > >() - static_cast<T>(4)))
   );
}

template <class T>
template<int N>
inline T constant_rayleigh_kurtosis<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{ // 3 - (6 Pi^2 - 24 Pi + 16)/((Pi - 4)^2)
  // Might provide and calculate this using pi_minus_four.
   BOOST_MATH_STD_USING
   return static_cast<T>(3) - (((static_cast<T>(6) * pi<T, policies::policy<policies::digits2<N> > >()
        * pi<T, policies::policy<policies::digits2<N> > >())
   - (static_cast<T>(24) * pi<T, policies::policy<policies::digits2<N> > >()) + static_cast<T>(16) )
   /
   ((pi<T, policies::policy<policies::digits2<N> > >() - static_cast<T>(4))
   * (pi<T, policies::policy<policies::digits2<N> > >() - static_cast<T>(4)))
   );
}

template <class T>
template<int N>
inline T constant_log2_e<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 1 / boost::math::constants::ln_two<T>();
}

template <class T>
template<int N>
inline T constant_quarter_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return boost::math::constants::pi<T>() / 4;
}

template <class T>
template<int N>
inline T constant_one_div_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 1 / boost::math::constants::pi<T>();
}

template <class T>
template<int N>
inline T constant_two_div_root_pi<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   return 2 * boost::math::constants::one_div_root_pi<T>();
}

#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1900)
template <class T>
template<int N>
inline T constant_first_feigenbaum<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   // We know the constant to 1018 decimal digits.
   // See:  http://www.plouffe.fr/simon/constants/feigenbaum.txt
   // Also: https://oeis.org/A006890
   // N is in binary digits; so we multiply by log_2(10)

   static_assert(N < 3.321*1018, "\nThe first Feigenbaum constant cannot be computed at runtime; it is too expensive. It is known to 1018 decimal digits; you must request less than that.");
   T alpha{"4.6692016091029906718532038204662016172581855774757686327456513430041343302113147371386897440239480138171659848551898151344086271420279325223124429888908908599449354632367134115324817142199474556443658237932020095610583305754586176522220703854106467494942849814533917262005687556659523398756038256372256480040951071283890611844702775854285419801113440175002428585382498335715522052236087250291678860362674527213399057131606875345083433934446103706309452019115876972432273589838903794946257251289097948986768334611626889116563123474460575179539122045562472807095202198199094558581946136877445617396074115614074243754435499204869180982648652368438702799649017397793425134723808737136211601860128186102056381818354097598477964173900328936171432159878240789776614391395764037760537119096932066998361984288981837003229412030210655743295550388845849737034727532121925706958414074661841981961006129640161487712944415901405467941800198133253378592493365883070459999938375411726563553016862529032210862320550634510679399023341675"};
   return alpha;
}

template <class T>
template<int N>
inline T constant_plastic<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   using std::cbrt;
   using std::sqrt;
   return (cbrt(9-sqrt(T(69))) + cbrt(9+sqrt(T(69))))/cbrt(T(18));
}


template <class T>
template<int N>
inline T constant_gauss<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
   using std::sqrt;
   T a = sqrt(T(2));
   T g = 1;
   const T scale = sqrt(std::numeric_limits<T>::epsilon())/512;
   while (a-g > scale*g)
   {
      T anp1 = (a + g)/2;
      g = sqrt(a*g);
      a = anp1;
   }

   return 2/(a + g);
}

template <class T>
template<int N>
inline T constant_dottie<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  // Error analysis: cos(x(1+d)) - x(1+d) = -(sin(x)+1)xd; plug in x = 0.739 gives -1.236d; take d as half an ulp gives the termination criteria we want.
  using std::cos;
  using std::abs;
  using std::sin;
  T x{".739085133215160641655312087673873404013411758900757464965680635773284654883547594599376106931766531849801246"};
  T residual = cos(x) - x;
  do {
    x += residual/(sin(x)+1);
    residual = cos(x) - x;
  } while(abs(residual) > std::numeric_limits<T>::epsilon());
  return x;
}


template <class T>
template<int N>
inline T constant_reciprocal_fibonacci<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  // Wikipedia says Gosper has deviced a faster algorithm for this, but I read the linked paper and couldn't see it!
  // In any case, k bits per iteration is fine, though it would be better to sum from smallest to largest.
  // That said, the condition number is unity, so it should be fine.
  T x0 = 1;
  T x1 = 1;
  T sum = 2;
  T diff = 1;
  while (diff > std::numeric_limits<T>::epsilon()) {
    T tmp = x1 + x0;
    diff = 1/tmp;
    sum += diff;
    x0 = x1;
    x1 = tmp;
  }
  return sum;
}

template <class T>
template<int N>
inline T constant_laplace_limit<T>::compute(BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC((std::integral_constant<int, N>)))
{
  // If x is the exact root, then the approximate root is given by x(1+delta).
  // Plugging this into the equation for the Laplace limit gives the residual of approximately
  // 2.6389delta. Take delta as half an epsilon and give some leeway so we don't get caught in an infinite loop,
  // gives a termination condition as 2eps.
  using std::abs;
  using std::exp;
  using std::sqrt;
  T x{"0.66274341934918158097474209710925290705623354911502241752039253499097185308651127724965480259895818168"};
  T tmp = sqrt(1+x*x);
  T etmp = exp(tmp);
  T residual = x*exp(tmp) - 1 - tmp;
  T df = etmp -x/tmp + etmp*x*x/tmp;
  do {
    x -= residual/df;
    tmp = sqrt(1+x*x);
    etmp = exp(tmp);
    residual = x*exp(tmp) - 1 - tmp;
    df = etmp -x/tmp + etmp*x*x/tmp;    
  } while(abs(residual) > 2*std::numeric_limits<T>::epsilon());
  return x;
}

#endif

}
}
}
} // namespaces

#endif // BOOST_MATH_CALCULATE_CONSTANTS_CONSTANTS_INCLUDED
