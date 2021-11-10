//  (C) Copyright Anton Bikineev 2014
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_RECURRENCE_HPP_
#define BOOST_MATH_TOOLS_RECURRENCE_HPP_

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/tuple.hpp>
#include <boost/math/tools/fraction.hpp>
#include <boost/math/tools/cxx03_warn.hpp>
#include <boost/math/tools/assert.hpp>

namespace boost {
   namespace math {
      namespace tools {
         namespace detail{

            //
            // Function ratios directly from recurrence relations:
            // H. Shintan, Note on Miller's recurrence algorithm, J. Sci. Hiroshima Univ. Ser. A-I
            // Math., 29 (1965), pp. 121 - 133.
            // and:
            // COMPUTATIONAL ASPECTS OF THREE-TERM RECURRENCE RELATIONS
            // WALTER GAUTSCHI
            // SIAM REVIEW Vol. 9, No. 1, January, 1967
            //
            template <class Recurrence>
            struct function_ratio_from_backwards_recurrence_fraction
            {
               typedef typename boost::remove_reference<decltype(boost::math::get<0>(std::declval<Recurrence&>()(0)))>::type value_type;
               typedef std::pair<value_type, value_type> result_type;
               function_ratio_from_backwards_recurrence_fraction(const Recurrence& r) : r(r), k(0) {}

               result_type operator()()
               {
                  value_type a, b, c;
                  boost::math::tie(a, b, c) = r(k);
                  ++k;
                  // an and bn defined as per Gauchi 1.16, not the same
                  // as the usual continued fraction a' and b's.
                  value_type bn = a / c;
                  value_type an = b / c;
                  return result_type(-bn, an);
               }

            private:
               function_ratio_from_backwards_recurrence_fraction operator=(const function_ratio_from_backwards_recurrence_fraction&);

               Recurrence r;
               int k;
            };

            template <class R, class T>
            struct recurrence_reverser
            {
               recurrence_reverser(const R& r) : r(r) {}
               boost::math::tuple<T, T, T> operator()(int i)
               {
                  using std::swap;
                  boost::math::tuple<T, T, T> t = r(-i);
                  swap(boost::math::get<0>(t), boost::math::get<2>(t));
                  return t;
               }
               R r;
            };

            template <class Recurrence>
            struct recurrence_offsetter
            {
               typedef decltype(std::declval<Recurrence&>()(0)) result_type;
               recurrence_offsetter(Recurrence const& rr, int offset) : r(rr), k(offset) {}
               result_type operator()(int i)
               {
                  return r(i + k);
               }
            private:
               Recurrence r;
               int k;
            };



         }  // namespace detail

         //
         // Given a stable backwards recurrence relation:
         // a f_n-1 + b f_n + c f_n+1 = 0
         // returns the ratio f_n / f_n-1
         //
         // Recurrence: a functor that returns a tuple of the factors (a,b,c).
         // factor:     Convergence criteria, should be no less than machine epsilon.
         // max_iter:   Maximum iterations to use solving the continued fraction.
         //
         template <class Recurrence, class T>
         T function_ratio_from_backwards_recurrence(const Recurrence& r, const T& factor, std::uintmax_t& max_iter)
         {
            detail::function_ratio_from_backwards_recurrence_fraction<Recurrence> f(r);
            return boost::math::tools::continued_fraction_a(f, factor, max_iter);
         }

         //
         // Given a stable forwards recurrence relation:
         // a f_n-1 + b f_n + c f_n+1 = 0
         // returns the ratio f_n / f_n+1
         //
         // Note that in most situations where this would be used, we're relying on
         // pseudo-convergence, as in most cases f_n will not be minimal as N -> -INF
         // as long as we reach convergence on the continued-fraction before f_n
         // switches behaviour, we should be fine.
         //
         // Recurrence: a functor that returns a tuple of the factors (a,b,c).
         // factor:     Convergence criteria, should be no less than machine epsilon.
         // max_iter:   Maximum iterations to use solving the continued fraction.
         //
         template <class Recurrence, class T>
         T function_ratio_from_forwards_recurrence(const Recurrence& r, const T& factor, std::uintmax_t& max_iter)
         {
            boost::math::tools::detail::function_ratio_from_backwards_recurrence_fraction<boost::math::tools::detail::recurrence_reverser<Recurrence, T> > f(r);
            return boost::math::tools::continued_fraction_a(f, factor, max_iter);
         }



         // solves usual recurrence relation for homogeneous
         // difference equation in stable forward direction
         // a(n)w(n-1) + b(n)w(n) + c(n)w(n+1) = 0
         //
         // Params:
         // get_coefs: functor returning a tuple, where
         //            get<0>() is a(n); get<1>() is b(n); get<2>() is c(n);
         // last_index: index N to be found;
         // first: w(-1);
         // second: w(0);
         //
         template <class NextCoefs, class T>
         inline T apply_recurrence_relation_forward(const NextCoefs& get_coefs, unsigned number_of_steps, T first, T second, long long* log_scaling = 0, T* previous = 0)
         {
            BOOST_MATH_STD_USING
            using boost::math::tuple;
            using boost::math::get;

            T third;
            T a, b, c;

            for (unsigned k = 0; k < number_of_steps; ++k)
            {
               tie(a, b, c) = get_coefs(k);

               if ((log_scaling) &&
                  ((fabs(tools::max_value<T>() * (c / (a * 2048))) < fabs(first))
                     || (fabs(tools::max_value<T>() * (c / (b * 2048))) < fabs(second))
                     || (fabs(tools::min_value<T>() * (c * 2048 / a)) > fabs(first))
                     || (fabs(tools::min_value<T>() * (c * 2048 / b)) > fabs(second))
                     ))

               {
                  // Rescale everything:
                  long long log_scale = lltrunc(log(fabs(second)));
                  T scale = exp(T(-log_scale));
                  second *= scale;
                  first *= scale;
                  *log_scaling += log_scale;
               }
               // scale each part separately to avoid spurious overflow:
               third = (a / -c) * first + (b / -c) * second;
               BOOST_MATH_ASSERT((boost::math::isfinite)(third));


               swap(first, second);
               swap(second, third);
            }

            if (previous)
               *previous = first;

            return second;
         }

         // solves usual recurrence relation for homogeneous
         // difference equation in stable backward direction
         // a(n)w(n-1) + b(n)w(n) + c(n)w(n+1) = 0
         //
         // Params:
         // get_coefs: functor returning a tuple, where
         //            get<0>() is a(n); get<1>() is b(n); get<2>() is c(n);
         // number_of_steps: index N to be found;
         // first: w(1);
         // second: w(0);
         //
         template <class T, class NextCoefs>
         inline T apply_recurrence_relation_backward(const NextCoefs& get_coefs, unsigned number_of_steps, T first, T second, long long* log_scaling = 0, T* previous = 0)
         {
            BOOST_MATH_STD_USING
            using boost::math::tuple;
            using boost::math::get;

            T next;
            T a, b, c;

            for (unsigned k = 0; k < number_of_steps; ++k)
            {
               tie(a, b, c) = get_coefs(-static_cast<int>(k));

               if ((log_scaling) && 
                  ( (fabs(tools::max_value<T>() * (a / b) / 2048) < fabs(second))
                     || (fabs(tools::max_value<T>() * (a / c) / 2048) < fabs(first))
                     || (fabs(tools::min_value<T>() * (a / b) * 2048) > fabs(second))
                     || (fabs(tools::min_value<T>() * (a / c) * 2048) > fabs(first))
                  ))
               {
                  // Rescale everything:
                  int log_scale = itrunc(log(fabs(second)));
                  T scale = exp(T(-log_scale));
                  second *= scale;
                  first *= scale;
                  *log_scaling += log_scale;
               }
               // scale each part separately to avoid spurious overflow:
               next = (b / -a) * second + (c / -a) * first;
               BOOST_MATH_ASSERT((boost::math::isfinite)(next));

               swap(first, second);
               swap(second, next);
            }

            if (previous)
               *previous = first;

            return second;
         }

         template <class Recurrence>
         struct forward_recurrence_iterator
         {
            typedef typename boost::remove_reference<decltype(std::get<0>(std::declval<Recurrence&>()(0)))>::type value_type;

            forward_recurrence_iterator(const Recurrence& r, value_type f_n_minus_1, value_type f_n)
               : f_n_minus_1(f_n_minus_1), f_n(f_n), coef(r), k(0) {}

            forward_recurrence_iterator(const Recurrence& r, value_type f_n)
               : f_n(f_n), coef(r), k(0)
            {
               std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<boost::math::policies::policy<> >();
               f_n_minus_1 = f_n * boost::math::tools::function_ratio_from_forwards_recurrence(detail::recurrence_offsetter<Recurrence>(r, -1), value_type(boost::math::tools::epsilon<value_type>() * 2), max_iter);
               boost::math::policies::check_series_iterations<value_type>("forward_recurrence_iterator<>::forward_recurrence_iterator", max_iter, boost::math::policies::policy<>());
            }

            forward_recurrence_iterator& operator++()
            {
               using std::swap;
               value_type a, b, c;
               boost::math::tie(a, b, c) = coef(k);
               value_type f_n_plus_1 = a * f_n_minus_1 / -c + b * f_n / -c;
               swap(f_n_minus_1, f_n);
               swap(f_n, f_n_plus_1);
               ++k;
               return *this;
            }

            forward_recurrence_iterator operator++(int)
            {
               forward_recurrence_iterator t(*this);
               ++(*this);
               return t;
            }

            value_type operator*() { return f_n; }

            value_type f_n_minus_1, f_n;
            Recurrence coef;
            int k;
         };

         template <class Recurrence>
         struct backward_recurrence_iterator
         {
            typedef typename boost::remove_reference<decltype(std::get<0>(std::declval<Recurrence&>()(0)))>::type value_type;

            backward_recurrence_iterator(const Recurrence& r, value_type f_n_plus_1, value_type f_n)
               : f_n_plus_1(f_n_plus_1), f_n(f_n), coef(r), k(0) {}

            backward_recurrence_iterator(const Recurrence& r, value_type f_n)
               : f_n(f_n), coef(r), k(0)
            {
               std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<boost::math::policies::policy<> >();
               f_n_plus_1 = f_n * boost::math::tools::function_ratio_from_backwards_recurrence(detail::recurrence_offsetter<Recurrence>(r, 1), value_type(boost::math::tools::epsilon<value_type>() * 2), max_iter);
               boost::math::policies::check_series_iterations<value_type>("backward_recurrence_iterator<>::backward_recurrence_iterator", max_iter, boost::math::policies::policy<>());
            }

            backward_recurrence_iterator& operator++()
            {
               using std::swap;
               value_type a, b, c;
               boost::math::tie(a, b, c) = coef(k);
               value_type f_n_minus_1 = c * f_n_plus_1 / -a + b * f_n / -a;
               swap(f_n_plus_1, f_n);
               swap(f_n, f_n_minus_1);
               --k;
               return *this;
            }

            backward_recurrence_iterator operator++(int)
            {
               backward_recurrence_iterator t(*this);
               ++(*this);
               return t;
            }

            value_type operator*() { return f_n; }

            value_type f_n_plus_1, f_n;
            Recurrence coef;
            int k;
         };

      }
   }
} // namespaces

#endif // BOOST_MATH_TOOLS_RECURRENCE_HPP_
