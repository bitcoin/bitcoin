
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2018 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_ITERATORS_HPP
#define BOOST_MATH_BESSEL_ITERATORS_HPP

#include <boost/math/tools/recurrence.hpp>

namespace boost {
   namespace math {
      namespace detail {

         template <class T>
         struct bessel_jy_recurrence
         {
            bessel_jy_recurrence(T v, T z) : v(v), z(z) {}
            boost::math::tuple<T, T, T> operator()(int k)
            {
               return boost::math::tuple<T, T, T>(1, -2 * (v + k) / z, 1);
            }

            T v, z;
         };
         template <class T>
         struct bessel_ik_recurrence
         {
            bessel_ik_recurrence(T v, T z) : v(v), z(z) {}
            boost::math::tuple<T, T, T> operator()(int k)
            {
               return boost::math::tuple<T, T, T>(1, -2 * (v + k) / z, -1);
            }

            T v, z;
         };
      } // namespace detail

      template <class T, class Policy = boost::math::policies::policy<> >
      struct bessel_j_backwards_iterator
      {
         typedef std::ptrdiff_t difference_type;
         typedef T value_type;
         typedef T* pointer;
         typedef T& reference;
         typedef std::input_iterator_tag iterator_category;

         bessel_j_backwards_iterator(const T& v, const T& x)
            : it(detail::bessel_jy_recurrence<T>(v, x), boost::math::cyl_bessel_j(v, x, Policy())) 
         {
            if(v < 0)
               boost::math::policies::raise_domain_error("bessel_j_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }

         bessel_j_backwards_iterator(const T& v, const T& x, const T& J_v)
            : it(detail::bessel_jy_recurrence<T>(v, x), J_v) 
         {
            if(v < 0)
               boost::math::policies::raise_domain_error("bessel_j_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }
         bessel_j_backwards_iterator(const T& v, const T& x, const T& J_v_plus_1, const T& J_v)
            : it(detail::bessel_jy_recurrence<T>(v, x), J_v_plus_1, J_v)
         {
            if (v < -1)
               boost::math::policies::raise_domain_error("bessel_j_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }

         bessel_j_backwards_iterator& operator++()
         {
            ++it;
            return *this;
         }

         bessel_j_backwards_iterator operator++(int)
         {
            bessel_j_backwards_iterator t(*this);
            ++(*this);
            return t;
         }

         T operator*() { return *it; }

      private:
         boost::math::tools::backward_recurrence_iterator< detail::bessel_jy_recurrence<T> > it;
      };

      template <class T, class Policy = boost::math::policies::policy<> >
      struct bessel_i_backwards_iterator
      {
         typedef std::ptrdiff_t difference_type;
         typedef T value_type;
         typedef T* pointer;
         typedef T& reference;
         typedef std::input_iterator_tag iterator_category;

         bessel_i_backwards_iterator(const T& v, const T& x)
            : it(detail::bessel_ik_recurrence<T>(v, x), boost::math::cyl_bessel_i(v, x, Policy()))
         {
            if(v < -1)
               boost::math::policies::raise_domain_error("bessel_i_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }
         bessel_i_backwards_iterator(const T& v, const T& x, const T& I_v)
            : it(detail::bessel_ik_recurrence<T>(v, x), I_v) 
         {
            if(v < -1)
               boost::math::policies::raise_domain_error("bessel_i_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }
         bessel_i_backwards_iterator(const T& v, const T& x, const T& I_v_plus_1, const T& I_v)
            : it(detail::bessel_ik_recurrence<T>(v, x), I_v_plus_1, I_v)
         {
            if(v < -1)
               boost::math::policies::raise_domain_error("bessel_i_backwards_iterator<%1%>", "Order must be > 0 stable backwards recurrence but got %1%", v, Policy());
         }

         bessel_i_backwards_iterator& operator++()
         {
            ++it;
            return *this;
         }

         bessel_i_backwards_iterator operator++(int)
         {
            bessel_i_backwards_iterator t(*this);
            ++(*this);
            return t;
         }

         T operator*() { return *it; }

      private:
         boost::math::tools::backward_recurrence_iterator< detail::bessel_ik_recurrence<T> > it;
      };

      template <class T, class Policy = boost::math::policies::policy<> >
      struct bessel_i_forwards_iterator
      {
         typedef std::ptrdiff_t difference_type;
         typedef T value_type;
         typedef T* pointer;
         typedef T& reference;
         typedef std::input_iterator_tag iterator_category;

         bessel_i_forwards_iterator(const T& v, const T& x)
            : it(detail::bessel_ik_recurrence<T>(v, x), boost::math::cyl_bessel_i(v, x, Policy()))
         {
            if(v > 1)
               boost::math::policies::raise_domain_error("bessel_i_forwards_iterator<%1%>", "Order must be < 0 stable forwards recurrence but got %1%", v, Policy());
         }
         bessel_i_forwards_iterator(const T& v, const T& x, const T& I_v)
            : it(detail::bessel_ik_recurrence<T>(v, x), I_v) 
         {
            if (v > 1)
               boost::math::policies::raise_domain_error("bessel_i_forwards_iterator<%1%>", "Order must be < 0 stable forwards recurrence but got %1%", v, Policy());
         }
         bessel_i_forwards_iterator(const T& v, const T& x, const T& I_v_plus_1, const T& I_v)
            : it(detail::bessel_ik_recurrence<T>(v, x), I_v_plus_1, I_v)
         {
            if (v > 1)
               boost::math::policies::raise_domain_error("bessel_i_forwards_iterator<%1%>", "Order must be < 0 stable forwards recurrence but got %1%", v, Policy());
         }

         bessel_i_forwards_iterator& operator++()
         {
            ++it;
            return *this;
         }

         bessel_i_forwards_iterator operator++(int)
         {
            bessel_i_forwards_iterator t(*this);
            ++(*this);
            return t;
         }

         T operator*() { return *it; }

      private:
         boost::math::tools::forward_recurrence_iterator< detail::bessel_ik_recurrence<T> > it;
      };

   }
} // namespaces

#endif // BOOST_MATH_BESSEL_ITERATORS_HPP
