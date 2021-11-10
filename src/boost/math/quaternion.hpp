//  boost quaternion.hpp header file

//  (C) Copyright Hubert Holin 2001.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_QUATERNION_HPP
#define BOOST_QUATERNION_HPP

#include <boost/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <locale>                                    // for the "<<" operator

#include <complex>
#include <iosfwd>                                    // for the "<<" and ">>" operators
#include <sstream>                                    // for the "<<" operator

#include <boost/math/special_functions/sinc.hpp>    // for the Sinus cardinal
#include <boost/math/special_functions/sinhc.hpp>    // for the Hyperbolic Sinus cardinal
#include <boost/math/tools/cxx03_warn.hpp>

#include <type_traits>

namespace boost
{
   namespace math
   {

      namespace detail {

         template <class T>
         struct is_trivial_arithmetic_type_imp
         {
            typedef std::integral_constant<bool,
               noexcept(std::declval<T&>() += std::declval<T>())
               && noexcept(std::declval<T&>() -= std::declval<T>())
               && noexcept(std::declval<T&>() *= std::declval<T>())
               && noexcept(std::declval<T&>() /= std::declval<T>())
            > type;
         };

         template <class T>
         struct is_trivial_arithmetic_type : public is_trivial_arithmetic_type_imp<T>::type {};
      }

#ifndef BOOST_NO_CXX14_CONSTEXPR
      namespace constexpr_detail
      {
         template <class T>
         constexpr void swap(T& a, T& b)
         {
            T t(a);
            a = b;
            b = t;
         }
       }
#endif

       template<typename T>
        class quaternion
        {
        public:
            
            typedef T value_type;
            
            
            // constructor for H seen as R^4
            // (also default constructor)
            
            constexpr explicit            quaternion( T const & requested_a = T(),
                                            T const & requested_b = T(),
                                            T const & requested_c = T(),
                                            T const & requested_d = T())
            :   a(requested_a),
                b(requested_b),
                c(requested_c),
                d(requested_d)
            {
                // nothing to do!
            }
            
            
            // constructor for H seen as C^2
                
            constexpr explicit            quaternion( ::std::complex<T> const & z0,
                                            ::std::complex<T> const & z1 = ::std::complex<T>())
            :   a(z0.real()),
                b(z0.imag()),
                c(z1.real()),
                d(z1.imag())
            {
                // nothing to do!
            }
            
            
            // UNtemplated copy constructor
            constexpr quaternion(quaternion const & a_recopier)
               : a(a_recopier.R_component_1()),
               b(a_recopier.R_component_2()),
               c(a_recopier.R_component_3()),
               d(a_recopier.R_component_4()) {}

            constexpr quaternion(quaternion && a_recopier)
               : a(std::move(a_recopier.R_component_1())),
               b(std::move(a_recopier.R_component_2())),
               c(std::move(a_recopier.R_component_3())),
               d(std::move(a_recopier.R_component_4())) {}
            
            // templated copy constructor
            
            template<typename X>
            constexpr explicit            quaternion(quaternion<X> const & a_recopier)
            :   a(static_cast<T>(a_recopier.R_component_1())),
                b(static_cast<T>(a_recopier.R_component_2())),
                c(static_cast<T>(a_recopier.R_component_3())),
                d(static_cast<T>(a_recopier.R_component_4()))
            {
                // nothing to do!
            }
            
            
            // destructor
            // (this is taken care of by the compiler itself)
            
            
            // accessors
            //
            // Note:    Like complex number, quaternions do have a meaningful notion of "real part",
            //            but unlike them there is no meaningful notion of "imaginary part".
            //            Instead there is an "unreal part" which itself is a quaternion, and usually
            //            nothing simpler (as opposed to the complex number case).
            //            However, for practicality, there are accessors for the other components
            //            (these are necessary for the templated copy constructor, for instance).
            
            constexpr T real() const
            {
               return(a);
            }

            constexpr quaternion<T> unreal() const
            {
               return(quaternion<T>(static_cast<T>(0), b, c, d));
            }

            constexpr T R_component_1() const
            {
               return(a);
            }

            constexpr T R_component_2() const
            {
               return(b);
            }

            constexpr T R_component_3() const
            {
               return(c);
            }

            constexpr T R_component_4() const
            {
               return(d);
            }

            constexpr ::std::complex<T> C_component_1() const
            {
               return(::std::complex<T>(a, b));
            }

            constexpr ::std::complex<T> C_component_2() const
            {
               return(::std::complex<T>(c, d));
            }

            BOOST_CXX14_CONSTEXPR void swap(quaternion& o)
            {
#ifndef BOOST_NO_CXX14_CONSTEXPR
               using constexpr_detail::swap;
#else
               using std::swap;
#endif
               swap(a, o.a);
               swap(b, o.b);
               swap(c, o.c);
               swap(d, o.d);
            }

            // assignment operators
            
            template<typename X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator = (quaternion<X> const  & a_affecter)
            {
               a = static_cast<T>(a_affecter.R_component_1());
               b = static_cast<T>(a_affecter.R_component_2());
               c = static_cast<T>(a_affecter.R_component_3());
               d = static_cast<T>(a_affecter.R_component_4());

               return(*this);
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator = (quaternion<T> const & a_affecter)
            {
               a = a_affecter.a;
               b = a_affecter.b;
               c = a_affecter.c;
               d = a_affecter.d;

               return(*this);
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator = (quaternion<T> && a_affecter)
            {
               a = std::move(a_affecter.a);
               b = std::move(a_affecter.b);
               c = std::move(a_affecter.c);
               d = std::move(a_affecter.d);

               return(*this);
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator = (T const & a_affecter)
            {
               a = a_affecter;

               b = c = d = static_cast<T>(0);

               return(*this);
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator = (::std::complex<T> const & a_affecter)
            {
               a = a_affecter.real();
               b = a_affecter.imag();

               c = d = static_cast<T>(0);

               return(*this);
            }

            // other assignment-related operators
            //
            // NOTE:    Quaternion multiplication is *NOT* commutative;
            //            symbolically, "q *= rhs;" means "q = q * rhs;"
            //            and "q /= rhs;" means "q = q * inverse_of(rhs);"
            //
            // Note2:   Each operator comes in 2 forms - one for the simple case where
            //          type T throws no exceptions, and one exception-safe version
            //          for the case where it might.
         private:
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(T const & rhs, const std::true_type&)
            {
               a += rhs;
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(T const & rhs, const std::false_type&)
            {
               quaternion<T> result(a + rhs, b, c, d); // exception guard
               swap(result);
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(std::complex<T> const & rhs, const std::true_type&)
            {
               a += std::real(rhs);
               b += std::imag(rhs);
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(std::complex<T> const & rhs, const std::false_type&)
            {
               quaternion<T> result(a + std::real(rhs), b + std::imag(rhs), c, d); // exception guard
               swap(result);
               return *this;
            }
            template <class X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(quaternion<X> const & rhs, const std::true_type&)
            {
               a += rhs.R_component_1();
               b += rhs.R_component_2();
               c += rhs.R_component_3();
               d += rhs.R_component_4();
               return *this;
            }
            template <class X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_add(quaternion<X> const & rhs, const std::false_type&)
            {
               quaternion<T> result(a + rhs.R_component_1(), b + rhs.R_component_2(), c + rhs.R_component_3(), d + rhs.R_component_4()); // exception guard
               swap(result);
               return *this;
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(T const & rhs, const std::true_type&)
            {
               a -= rhs;
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(T const & rhs, const std::false_type&)
            {
               quaternion<T> result(a - rhs, b, c, d); // exception guard
               swap(result);
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(std::complex<T> const & rhs, const std::true_type&)
            {
               a -= std::real(rhs);
               b -= std::imag(rhs);
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(std::complex<T> const & rhs, const std::false_type&)
            {
               quaternion<T> result(a - std::real(rhs), b - std::imag(rhs), c, d); // exception guard
               swap(result);
               return *this;
            }
            template <class X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(quaternion<X> const & rhs, const std::true_type&)
            {
               a -= rhs.R_component_1();
               b -= rhs.R_component_2();
               c -= rhs.R_component_3();
               d -= rhs.R_component_4();
               return *this;
            }
            template <class X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_subtract(quaternion<X> const & rhs, const std::false_type&)
            {
               quaternion<T> result(a - rhs.R_component_1(), b - rhs.R_component_2(), c - rhs.R_component_3(), d - rhs.R_component_4()); // exception guard
               swap(result);
               return *this;
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_multiply(T const & rhs, const std::true_type&)
            {
               a *= rhs;
               b *= rhs;
               c *= rhs;
               d *= rhs;
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_multiply(T const & rhs, const std::false_type&)
            {
               quaternion<T> result(a * rhs, b * rhs, c * rhs, d * rhs); // exception guard
               swap(result);
               return *this;
            }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_divide(T const & rhs, const std::true_type&)
            {
               a /= rhs;
               b /= rhs;
               c /= rhs;
               d /= rhs;
               return *this;
            }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        do_divide(T const & rhs, const std::false_type&)
            {
               quaternion<T> result(a / rhs, b / rhs, c / rhs, d / rhs); // exception guard
               swap(result);
               return *this;
            }
         public:

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator += (T const & rhs) { return do_add(rhs, detail::is_trivial_arithmetic_type<T>()); }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator += (::std::complex<T> const & rhs) { return do_add(rhs, detail::is_trivial_arithmetic_type<T>()); }
            template<typename X> BOOST_CXX14_CONSTEXPR quaternion<T> & operator += (quaternion<X> const & rhs) { return do_add(rhs, detail::is_trivial_arithmetic_type<T>()); }

            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator -= (T const & rhs) { return do_subtract(rhs, detail::is_trivial_arithmetic_type<T>()); }
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator -= (::std::complex<T> const & rhs) { return do_subtract(rhs, detail::is_trivial_arithmetic_type<T>()); }
            template<typename X> BOOST_CXX14_CONSTEXPR quaternion<T> & operator -= (quaternion<X> const & rhs) { return do_subtract(rhs, detail::is_trivial_arithmetic_type<T>()); }
            
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator *= (T const & rhs) { return do_multiply(rhs, detail::is_trivial_arithmetic_type<T>()); }
            
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator *= (::std::complex<T> const & rhs)
            {
                T    ar = rhs.real();
                T    br = rhs.imag();
                quaternion<T> result(a*ar - b*br, a*br + b*ar, c*ar + d*br, -c*br+d*ar);
                swap(result);
                return(*this);
            }
            
            template<typename X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator *= (quaternion<X> const & rhs)
            {
                T    ar = static_cast<T>(rhs.R_component_1());
                T    br = static_cast<T>(rhs.R_component_2());
                T    cr = static_cast<T>(rhs.R_component_3());
                T    dr = static_cast<T>(rhs.R_component_4());
                
                quaternion<T> result(a*ar - b*br - c*cr - d*dr, a*br + b*ar + c*dr - d*cr, a*cr - b*dr + c*ar + d*br, a*dr + b*cr - c*br + d*ar);
                swap(result);
                return(*this);
            }
            
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator /= (T const & rhs) { return do_divide(rhs, detail::is_trivial_arithmetic_type<T>()); }
            
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator /= (::std::complex<T> const & rhs)
            {
                T    ar = rhs.real();
                T    br = rhs.imag();
                T    denominator = ar*ar+br*br;
                quaternion<T> result((+a*ar + b*br) / denominator, (-a*br + b*ar) / denominator, (+c*ar - d*br) / denominator, (+c*br + d*ar) / denominator);
                swap(result);
                return(*this);
            }
            
            template<typename X>
            BOOST_CXX14_CONSTEXPR quaternion<T> &        operator /= (quaternion<X> const & rhs)
            {
                T    ar = static_cast<T>(rhs.R_component_1());
                T    br = static_cast<T>(rhs.R_component_2());
                T    cr = static_cast<T>(rhs.R_component_3());
                T    dr = static_cast<T>(rhs.R_component_4());
                
                T    denominator = ar*ar+br*br+cr*cr+dr*dr;
                quaternion<T> result((+a*ar+b*br+c*cr+d*dr)/denominator, (-a*br+b*ar-c*dr+d*cr)/denominator, (-a*cr+b*dr+c*ar-d*br)/denominator, (-a*dr-b*cr+c*br+d*ar)/denominator);
                swap(result);
                return(*this);
            }
        private:
           T a, b, c, d;
            
        };

// swap:
template <class T>
BOOST_CXX14_CONSTEXPR void swap(quaternion<T>& a, quaternion<T>& b) { a.swap(b); }
        
// operator+
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator + (const quaternion<T1>& a, const T2& b)
{
   return quaternion<T1>(static_cast<T1>(a.R_component_1() + b), a.R_component_2(), a.R_component_3(), a.R_component_4());
}
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator + (const T1& a, const quaternion<T2>& b)
{
   return quaternion<T2>(static_cast<T2>(b.R_component_1() + a), b.R_component_2(), b.R_component_3(), b.R_component_4());
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator + (const quaternion<T1>& a, const std::complex<T2>& b)
{
   return quaternion<T1>(a.R_component_1() + std::real(b), a.R_component_2() + std::imag(b), a.R_component_3(), a.R_component_4());
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator + (const std::complex<T1>& a, const quaternion<T2>& b)
{
   return quaternion<T1>(b.R_component_1() + std::real(a), b.R_component_2() + std::imag(a), b.R_component_3(), b.R_component_4());
}
template <class T>
inline constexpr quaternion<T> operator + (const quaternion<T>& a, const quaternion<T>& b)
{
   return quaternion<T>(a.R_component_1() + b.R_component_1(), a.R_component_2() + b.R_component_2(), a.R_component_3() + b.R_component_3(), a.R_component_4() + b.R_component_4());
}
// operator-
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator - (const quaternion<T1>& a, const T2& b)
{
   return quaternion<T1>(static_cast<T1>(a.R_component_1() - b), a.R_component_2(), a.R_component_3(), a.R_component_4());
}
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator - (const T1& a, const quaternion<T2>& b)
{
   return quaternion<T2>(static_cast<T2>(a - b.R_component_1()), -b.R_component_2(), -b.R_component_3(), -b.R_component_4());
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator - (const quaternion<T1>& a, const std::complex<T2>& b)
{
   return quaternion<T1>(a.R_component_1() - std::real(b), a.R_component_2() - std::imag(b), a.R_component_3(), a.R_component_4());
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator - (const std::complex<T1>& a, const quaternion<T2>& b)
{
   return quaternion<T1>(std::real(a) - b.R_component_1(), std::imag(a) - b.R_component_2(), -b.R_component_3(), -b.R_component_4());
}
template <class T>
inline constexpr quaternion<T> operator - (const quaternion<T>& a, const quaternion<T>& b)
{
   return quaternion<T>(a.R_component_1() - b.R_component_1(), a.R_component_2() - b.R_component_2(), a.R_component_3() - b.R_component_3(), a.R_component_4() - b.R_component_4());
}

// operator*
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator * (const quaternion<T1>& a, const T2& b)
{
   return quaternion<T1>(static_cast<T1>(a.R_component_1() * b), a.R_component_2() * b, a.R_component_3() * b, a.R_component_4() * b);
}
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator * (const T1& a, const quaternion<T2>& b)
{
   return quaternion<T2>(static_cast<T2>(a * b.R_component_1()), a * b.R_component_2(), a * b.R_component_3(), a * b.R_component_4());
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator * (const quaternion<T1>& a, const std::complex<T2>& b)
{
   quaternion<T1> result(a);
   result *= b;
   return result;
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator * (const std::complex<T1>& a, const quaternion<T2>& b)
{
   quaternion<T1> result(a);
   result *= b;
   return result;
}
template <class T>
inline BOOST_CXX14_CONSTEXPR quaternion<T> operator * (const quaternion<T>& a, const quaternion<T>& b)
{
   quaternion<T> result(a);
   result *= b;
   return result;
}

// operator/
template <class T1, class T2>
inline constexpr typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator / (const quaternion<T1>& a, const T2& b)
{
   return quaternion<T1>(a.R_component_1() / b, a.R_component_2() / b, a.R_component_3() / b, a.R_component_4() / b);
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator / (const T1& a, const quaternion<T2>& b)
{
   quaternion<T2> result(a);
   result /= b;
   return result;
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T2, T1>::value, quaternion<T1> >::type
operator / (const quaternion<T1>& a, const std::complex<T2>& b)
{
   quaternion<T1> result(a);
   result /= b;
   return result;
}
template <class T1, class T2>
inline BOOST_CXX14_CONSTEXPR typename std::enable_if<std::is_convertible<T1, T2>::value, quaternion<T2> >::type
operator / (const std::complex<T1>& a, const quaternion<T2>& b)
{
   quaternion<T2> result(a);
   result /= b;
   return result;
}
template <class T>
inline BOOST_CXX14_CONSTEXPR quaternion<T> operator / (const quaternion<T>& a, const quaternion<T>& b)
{
   quaternion<T> result(a);
   result /= b;
   return result;
}
        
        
        template<typename T>
        inline constexpr const quaternion<T>&             operator + (quaternion<T> const & q)
        {
            return q;
        }
        
        
        template<typename T>
        inline constexpr quaternion<T>                    operator - (quaternion<T> const & q)
        {
            return(quaternion<T>(-q.R_component_1(),-q.R_component_2(),-q.R_component_3(),-q.R_component_4()));
        }
        
        
        template<typename R, typename T>
        inline constexpr typename std::enable_if<std::is_convertible<R, T>::value, bool>::type operator == (R const & lhs, quaternion<T> const & rhs)
        {
            return    (
                        (rhs.R_component_1() == lhs)&&
                        (rhs.R_component_2() == static_cast<T>(0))&&
                        (rhs.R_component_3() == static_cast<T>(0))&&
                        (rhs.R_component_4() == static_cast<T>(0))
                    );
        }
        
        
        template<typename T, typename R>
        inline constexpr typename std::enable_if<std::is_convertible<R, T>::value, bool>::type operator == (quaternion<T> const & lhs, R const & rhs)
        {
           return rhs == lhs;
        }
        
        
        template<typename T>
        inline constexpr bool                                operator == (::std::complex<T> const & lhs, quaternion<T> const & rhs)
        {
            return    (
                        (rhs.R_component_1() == lhs.real())&&
                        (rhs.R_component_2() == lhs.imag())&&
                        (rhs.R_component_3() == static_cast<T>(0))&&
                        (rhs.R_component_4() == static_cast<T>(0))
                    );
        }
        
        
        template<typename T>
        inline constexpr bool                                operator == (quaternion<T> const & lhs, ::std::complex<T> const & rhs)
        {
           return rhs == lhs;
        }
        
        
        template<typename T>
        inline constexpr bool                                operator == (quaternion<T> const & lhs, quaternion<T> const & rhs)
        {
            return    (
                        (rhs.R_component_1() == lhs.R_component_1())&&
                        (rhs.R_component_2() == lhs.R_component_2())&&
                        (rhs.R_component_3() == lhs.R_component_3())&&
                        (rhs.R_component_4() == lhs.R_component_4())
                    );
        }
                
        template<typename R, typename T> inline constexpr bool operator != (R const & lhs, quaternion<T> const & rhs) { return !(lhs == rhs); }
        template<typename T, typename R> inline constexpr bool operator != (quaternion<T> const & lhs, R const & rhs) { return !(lhs == rhs); }
        template<typename T> inline constexpr bool operator != (::std::complex<T> const & lhs, quaternion<T> const & rhs) { return !(lhs == rhs); }
        template<typename T> inline constexpr bool operator != (quaternion<T> const & lhs, ::std::complex<T> const & rhs) { return !(lhs == rhs); }
        template<typename T> inline constexpr bool operator != (quaternion<T> const & lhs, quaternion<T> const & rhs) { return !(lhs == rhs); }
        
        
        // Note:    we allow the following formats, with a, b, c, and d reals
        //            a
        //            (a), (a,b), (a,b,c), (a,b,c,d)
        //            (a,(c)), (a,(c,d)), ((a)), ((a),c), ((a),(c)), ((a),(c,d)), ((a,b)), ((a,b),c), ((a,b),(c)), ((a,b),(c,d))
        template<typename T, typename charT, class traits>
        ::std::basic_istream<charT,traits> &    operator >> (    ::std::basic_istream<charT,traits> & is,
                                                                quaternion<T> & q)
        {
            const ::std::ctype<charT> & ct = ::std::use_facet< ::std::ctype<charT> >(is.getloc());
            
            T    a = T();
            T    b = T();
            T    c = T();
            T    d = T();
            
            ::std::complex<T>    u = ::std::complex<T>();
            ::std::complex<T>    v = ::std::complex<T>();
            
            charT    ch = charT();
            char    cc;
            
            is >> ch;                                        // get the first lexeme
            
            if    (!is.good())    goto finish;
            
            cc = ct.narrow(ch, char());
            
            if    (cc == '(')                            // read "(", possible: (a), (a,b), (a,b,c), (a,b,c,d), (a,(c)), (a,(c,d)), ((a)), ((a),c), ((a),(c)), ((a),(c,d)), ((a,b)), ((a,b),c), ((a,b),(c)), ((a,b,),(c,d,))
            {
                is >> ch;                                    // get the second lexeme
                
                if    (!is.good())    goto finish;
                
                cc = ct.narrow(ch, char());
                
                if    (cc == '(')                        // read "((", possible: ((a)), ((a),c), ((a),(c)), ((a),(c,d)), ((a,b)), ((a,b),c), ((a,b),(c)), ((a,b,),(c,d,))
                {
                    is.putback(ch);
                    
                    is >> u;                                // we extract the first and second components
                    a = u.real();
                    b = u.imag();
                    
                    if    (!is.good())    goto finish;
                    
                    is >> ch;                                // get the next lexeme
                    
                    if    (!is.good())    goto finish;
                    
                    cc = ct.narrow(ch, char());
                    
                    if        (cc == ')')                    // format: ((a)) or ((a,b))
                    {
                        q = quaternion<T>(a,b);
                    }
                    else if    (cc == ',')                // read "((a)," or "((a,b),", possible: ((a),c), ((a),(c)), ((a),(c,d)), ((a,b),c), ((a,b),(c)), ((a,b,),(c,d,))
                    {
                        is >> v;                            // we extract the third and fourth components
                        c = v.real();
                        d = v.imag();
                        
                        if    (!is.good())    goto finish;
                        
                        is >> ch;                                // get the last lexeme
                        
                        if    (!is.good())    goto finish;
                        
                        cc = ct.narrow(ch, char());
                        
                        if    (cc == ')')                    // format: ((a),c), ((a),(c)), ((a),(c,d)), ((a,b),c), ((a,b),(c)) or ((a,b,),(c,d,))
                        {
                            q = quaternion<T>(a,b,c,d);
                        }
                        else                            // error
                        {
                            is.setstate(::std::ios_base::failbit);
                        }
                    }
                    else                                // error
                    {
                        is.setstate(::std::ios_base::failbit);
                    }
                }
                else                                // read "(a", possible: (a), (a,b), (a,b,c), (a,b,c,d), (a,(c)), (a,(c,d))
                {
                    is.putback(ch);
                    
                    is >> a;                                // we extract the first component
                    
                    if    (!is.good())    goto finish;
                    
                    is >> ch;                                // get the third lexeme
                    
                    if    (!is.good())    goto finish;
                    
                    cc = ct.narrow(ch, char());
                    
                    if        (cc == ')')                    // format: (a)
                    {
                        q = quaternion<T>(a);
                    }
                    else if    (cc == ',')                // read "(a,", possible: (a,b), (a,b,c), (a,b,c,d), (a,(c)), (a,(c,d))
                    {
                        is >> ch;                            // get the fourth lexeme
                        
                        if    (!is.good())    goto finish;
                        
                        cc = ct.narrow(ch, char());
                        
                        if    (cc == '(')                // read "(a,(", possible: (a,(c)), (a,(c,d))
                        {
                            is.putback(ch);
                            
                            is >> v;                        // we extract the third and fourth component
                            
                            c = v.real();
                            d = v.imag();
                            
                            if    (!is.good())    goto finish;
                            
                            is >> ch;                        // get the ninth lexeme
                            
                            if    (!is.good())    goto finish;
                            
                            cc = ct.narrow(ch, char());
                            
                            if    (cc == ')')                // format: (a,(c)) or (a,(c,d))
                            {
                                q = quaternion<T>(a,b,c,d);
                            }
                            else                        // error
                            {
                                is.setstate(::std::ios_base::failbit);
                            }
                        }
                        else                        // read "(a,b", possible: (a,b), (a,b,c), (a,b,c,d)
                        {
                            is.putback(ch);
                            
                            is >> b;                        // we extract the second component
                            
                            if    (!is.good())    goto finish;
                            
                            is >> ch;                        // get the fifth lexeme
                            
                            if    (!is.good())    goto finish;
                            
                            cc = ct.narrow(ch, char());
                            
                            if    (cc == ')')                // format: (a,b)
                            {
                                q = quaternion<T>(a,b);
                            }
                            else if    (cc == ',')        // read "(a,b,", possible: (a,b,c), (a,b,c,d)
                            {
                                is >> c;                    // we extract the third component
                                
                                if    (!is.good())    goto finish;
                                
                                is >> ch;                    // get the seventh lexeme
                                
                                if    (!is.good())    goto finish;
                                
                                cc = ct.narrow(ch, char());
                                
                                if        (cc == ')')        // format: (a,b,c)
                                {
                                    q = quaternion<T>(a,b,c);
                                }
                                else if    (cc == ',')    // read "(a,b,c,", possible: (a,b,c,d)
                                {
                                    is >> d;                // we extract the fourth component
                                    
                                    if    (!is.good())    goto finish;
                                    
                                    is >> ch;                // get the ninth lexeme
                                    
                                    if    (!is.good())    goto finish;
                                    
                                    cc = ct.narrow(ch, char());
                                    
                                    if    (cc == ')')        // format: (a,b,c,d)
                                    {
                                        q = quaternion<T>(a,b,c,d);
                                    }
                                    else                // error
                                    {
                                        is.setstate(::std::ios_base::failbit);
                                    }
                                }
                                else                    // error
                                {
                                    is.setstate(::std::ios_base::failbit);
                                }
                            }
                            else                        // error
                            {
                                is.setstate(::std::ios_base::failbit);
                            }
                        }
                    }
                    else                                // error
                    {
                        is.setstate(::std::ios_base::failbit);
                    }
                }
            }
            else                                        // format:    a
            {
                is.putback(ch);
                
                is >> a;                                    // we extract the first component
                
                if    (!is.good())    goto finish;
                
                q = quaternion<T>(a);
            }
            
            finish:
            return(is);
        }
        
        
        template<typename T, typename charT, class traits>
        ::std::basic_ostream<charT,traits> &    operator << (    ::std::basic_ostream<charT,traits> & os,
                                                                quaternion<T> const & q)
        {
            ::std::basic_ostringstream<charT,traits>    s;

            s.flags(os.flags());
            s.imbue(os.getloc());
            s.precision(os.precision());
            
            s << '('    << q.R_component_1() << ','
                        << q.R_component_2() << ','
                        << q.R_component_3() << ','
                        << q.R_component_4() << ')';
            
            return os << s.str();
        }
        
        
        // values
        
        template<typename T>
        inline constexpr T real(quaternion<T> const & q)
        {
            return(q.real());
        }
        
        
        template<typename T>
        inline constexpr quaternion<T> unreal(quaternion<T> const & q)
        {
            return(q.unreal());
        }
                
        template<typename T>
        inline T sup(quaternion<T> const & q)
        {
            using    ::std::abs;
            return (std::max)((std::max)(abs(q.R_component_1()), abs(q.R_component_2())), (std::max)(abs(q.R_component_3()), abs(q.R_component_4())));
        }
        
        
        template<typename T>
        inline T l1(quaternion<T> const & q)
        {
           using    ::std::abs;
           return abs(q.R_component_1()) + abs(q.R_component_2()) + abs(q.R_component_3()) + abs(q.R_component_4());
        }
        
        
        template<typename T>
        inline T abs(quaternion<T> const & q)
        {
            using    ::std::abs;
            using    ::std::sqrt;
            
            T            maxim = sup(q);    // overflow protection
            
            if    (maxim == static_cast<T>(0))
            {
                return(maxim);
            }
            else
            {
                T    mixam = static_cast<T>(1)/maxim;    // prefer multiplications over divisions
                
                T a = q.R_component_1() * mixam;
                T b = q.R_component_2() * mixam;
                T c = q.R_component_3() * mixam;
                T d = q.R_component_4() * mixam;

                a *= a;
                b *= b;
                c *= c;
                d *= d;
                
                return(maxim * sqrt(a + b + c + d));
            }
            
            //return(sqrt(norm(q)));
        }
        
        
        // Note:    This is the Cayley norm, not the Euclidean norm...
        
        template<typename T>
        inline BOOST_CXX14_CONSTEXPR T norm(quaternion<T>const  & q)
        {
            return(real(q*conj(q)));
        }
        
        
        template<typename T>
        inline constexpr quaternion<T> conj(quaternion<T> const & q)
        {
            return(quaternion<T>(   +q.R_component_1(),
                                    -q.R_component_2(),
                                    -q.R_component_3(),
                                    -q.R_component_4()));
        }
        
        
        template<typename T>
        inline quaternion<T>                    spherical(  T const & rho,
                                                            T const & theta,
                                                            T const & phi1,
                                                            T const & phi2)
        {
            using ::std::cos;
            using ::std::sin;
            
            //T    a = cos(theta)*cos(phi1)*cos(phi2);
            //T    b = sin(theta)*cos(phi1)*cos(phi2);
            //T    c = sin(phi1)*cos(phi2);
            //T    d = sin(phi2);
            
            T    courrant = static_cast<T>(1);
            
            T    d = sin(phi2);
            
            courrant *= cos(phi2);
            
            T    c = sin(phi1)*courrant;
            
            courrant *= cos(phi1);
            
            T    b = sin(theta)*courrant;
            T    a = cos(theta)*courrant;
            
            return(rho*quaternion<T>(a,b,c,d));
        }
        
        
        template<typename T>
        inline quaternion<T>                    semipolar(  T const & rho,
                                                            T const & alpha,
                                                            T const & theta1,
                                                            T const & theta2)
        {
            using ::std::cos;
            using ::std::sin;
            
            T    a = cos(alpha)*cos(theta1);
            T    b = cos(alpha)*sin(theta1);
            T    c = sin(alpha)*cos(theta2);
            T    d = sin(alpha)*sin(theta2);
            
            return(rho*quaternion<T>(a,b,c,d));
        }
        
        
        template<typename T>
        inline quaternion<T>                    multipolar( T const & rho1,
                                                            T const & theta1,
                                                            T const & rho2,
                                                            T const & theta2)
        {
            using ::std::cos;
            using ::std::sin;
            
            T    a = rho1*cos(theta1);
            T    b = rho1*sin(theta1);
            T    c = rho2*cos(theta2);
            T    d = rho2*sin(theta2);
            
            return(quaternion<T>(a,b,c,d));
        }
        
        
        template<typename T>
        inline quaternion<T>                    cylindrospherical(  T const & t,
                                                                    T const & radius,
                                                                    T const & longitude,
                                                                    T const & latitude)
        {
            using ::std::cos;
            using ::std::sin;
            
            
            
            T    b = radius*cos(longitude)*cos(latitude);
            T    c = radius*sin(longitude)*cos(latitude);
            T    d = radius*sin(latitude);
            
            return(quaternion<T>(t,b,c,d));
        }
        
        
        template<typename T>
        inline quaternion<T>                    cylindrical(T const & r,
                                                            T const & angle,
                                                            T const & h1,
                                                            T const & h2)
        {
            using ::std::cos;
            using ::std::sin;
            
            T    a = r*cos(angle);
            T    b = r*sin(angle);
            
            return(quaternion<T>(a,b,h1,h2));
        }
        
        
        // transcendentals
        // (please see the documentation)
        
        
        template<typename T>
        inline quaternion<T>                    exp(quaternion<T> const & q)
        {
            using    ::std::exp;
            using    ::std::cos;
            
            using    ::boost::math::sinc_pi;
            
            T    u = exp(real(q));
            
            T    z = abs(unreal(q));
            
            T    w = sinc_pi(z);
            
            return(u*quaternion<T>(cos(z),
                w*q.R_component_2(), w*q.R_component_3(),
                w*q.R_component_4()));
        }
        
        
        template<typename T>
        inline quaternion<T>                    cos(quaternion<T> const & q)
        {
            using    ::std::sin;
            using    ::std::cos;
            using    ::std::cosh;
            
            using    ::boost::math::sinhc_pi;
            
            T    z = abs(unreal(q));
            
            T    w = -sin(q.real())*sinhc_pi(z);
            
            return(quaternion<T>(cos(q.real())*cosh(z),
                w*q.R_component_2(), w*q.R_component_3(),
                w*q.R_component_4()));
        }
        
        
        template<typename T>
        inline quaternion<T>                    sin(quaternion<T> const & q)
        {
            using    ::std::sin;
            using    ::std::cos;
            using    ::std::cosh;
            
            using    ::boost::math::sinhc_pi;
            
            T    z = abs(unreal(q));
            
            T    w = +cos(q.real())*sinhc_pi(z);
            
            return(quaternion<T>(sin(q.real())*cosh(z),
                w*q.R_component_2(), w*q.R_component_3(),
                w*q.R_component_4()));
        }
        
        
        template<typename T>
        inline quaternion<T>                    tan(quaternion<T> const & q)
        {
            return(sin(q)/cos(q));
        }
        
        
        template<typename T>
        inline quaternion<T>                    cosh(quaternion<T> const & q)
        {
            return((exp(+q)+exp(-q))/static_cast<T>(2));
        }
        
        
        template<typename T>
        inline quaternion<T>                    sinh(quaternion<T> const & q)
        {
            return((exp(+q)-exp(-q))/static_cast<T>(2));
        }
        
        
        template<typename T>
        inline quaternion<T>                    tanh(quaternion<T> const & q)
        {
            return(sinh(q)/cosh(q));
        }
        
        
        template<typename T>
        quaternion<T>                            pow(quaternion<T> const & q,
                                                    int n)
        {
            if        (n > 1)
            {
                int    m = n>>1;
                
                quaternion<T>    result = pow(q, m);
                
                result *= result;
                
                if    (n != (m<<1))
                {
                    result *= q; // n odd
                }
                
                return(result);
            }
            else if    (n == 1)
            {
                return(q);
            }
            else if    (n == 0)
            {
                return(quaternion<T>(static_cast<T>(1)));
            }
            else    /* n < 0 */
            {
                return(pow(quaternion<T>(static_cast<T>(1))/q,-n));
            }
        }
    }
}

#endif /* BOOST_QUATERNION_HPP */
