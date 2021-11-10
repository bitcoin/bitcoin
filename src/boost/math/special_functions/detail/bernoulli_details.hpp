///////////////////////////////////////////////////////////////////////////////
//  Copyright 2013 John Maddock
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BERNOULLI_DETAIL_HPP
#define BOOST_MATH_BERNOULLI_DETAIL_HPP

#include <boost/math/tools/atomic.hpp>
#include <boost/math/tools/toms748_solve.hpp>
#include <boost/math/tools/cxx03_warn.hpp>
#include <boost/math/tools/throw_exception.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <vector>
#include <type_traits>

#ifdef BOOST_HAS_THREADS
#include <mutex>
#endif

namespace boost{ namespace math{ namespace detail{
//
// Asymptotic expansion for B2n due to
// Luschny LogB3 formula (http://www.luschny.de/math/primes/bernincl.html)
//
template <class T, class Policy>
T b2n_asymptotic(int n)
{
   BOOST_MATH_STD_USING
   const T nx = static_cast<T>(n);
   const T nx2(nx * nx);

   const T approximate_log_of_bernoulli_bn = 
        ((boost::math::constants::half<T>() + nx) * log(nx))
        + ((boost::math::constants::half<T>() - nx) * log(boost::math::constants::pi<T>()))
        + (((T(3) / 2) - nx) * boost::math::constants::ln_two<T>())
        + ((nx * (T(2) - (nx2 * 7) * (1 + ((nx2 * 30) * ((nx2 * 12) - 1))))) / (((nx2 * nx2) * nx2) * 2520));
   return ((n / 2) & 1 ? 1 : -1) * (approximate_log_of_bernoulli_bn > tools::log_max_value<T>() 
      ? policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, nx, Policy())
      : static_cast<T>(exp(approximate_log_of_bernoulli_bn)));
}

template <class T, class Policy>
T t2n_asymptotic(int n)
{
   BOOST_MATH_STD_USING
   // Just get B2n and convert to a Tangent number:
   T t2n = fabs(b2n_asymptotic<T, Policy>(2 * n)) / (2 * n);
   T p2 = ldexp(T(1), n);
   if(tools::max_value<T>() / p2 < t2n)
      return policies::raise_overflow_error<T>("boost::math::tangent_t2n<%1%>(std::size_t)", 0, T(n), Policy());
   t2n *= p2;
   p2 -= 1;
   if(tools::max_value<T>() / p2 < t2n)
      return policies::raise_overflow_error<T>("boost::math::tangent_t2n<%1%>(std::size_t)", 0, Policy());
   t2n *= p2;
   return t2n;
}
//
// We need to know the approximate value of /n/ which will
// cause bernoulli_b2n<T>(n) to return infinity - this allows
// us to elude a great deal of runtime checking for values below
// n, and only perform the full overflow checks when we know that we're
// getting close to the point where our calculations will overflow.
// We use Luschny's LogB3 formula (http://www.luschny.de/math/primes/bernincl.html) 
// to find the limit, and since we're dealing with the log of the Bernoulli numbers
// we need only perform the calculation at double precision and not with T
// (which may be a multiprecision type).  The limit returned is within 1 of the true
// limit for all the types tested.  Note that although the code below is basically
// the same as b2n_asymptotic above, it has been recast as a continuous real-valued 
// function as this makes the root finding go smoother/faster.  It also omits the
// sign of the Bernoulli number.
//
struct max_bernoulli_root_functor
{
   max_bernoulli_root_functor(unsigned long long t) : target(static_cast<double>(t)) {}
   double operator()(double n)
   {
      BOOST_MATH_STD_USING

      // Luschny LogB3(n) formula.

      const double nx2(n * n);

      const double approximate_log_of_bernoulli_bn
         =   ((boost::math::constants::half<double>() + n) * log(n))
           + ((boost::math::constants::half<double>() - n) * log(boost::math::constants::pi<double>()))
           + (((double(3) / 2) - n) * boost::math::constants::ln_two<double>())
           + ((n * (2 - (nx2 * 7) * (1 + ((nx2 * 30) * ((nx2 * 12) - 1))))) / (((nx2 * nx2) * nx2) * 2520));

      return approximate_log_of_bernoulli_bn - target;
   }
private:
   double target;
};

template <class T, class Policy>
inline std::size_t find_bernoulli_overflow_limit(const std::false_type&)
{
   // Set a limit on how large the result can ever be:
   static const double max_result = static_cast<double>((std::numeric_limits<std::size_t>::max)() - 1000u);

   unsigned long long t = lltrunc(boost::math::tools::log_max_value<T>());
   max_bernoulli_root_functor fun(t);
   boost::math::tools::equal_floor tol;
   std::uintmax_t max_iter = boost::math::policies::get_max_root_iterations<Policy>();
   double result = boost::math::tools::toms748_solve(fun, sqrt(double(t)), double(t), tol, max_iter).first / 2;
   if (result > max_result)
      result = max_result;
   
   return static_cast<std::size_t>(result);
}

template <class T, class Policy>
inline std::size_t find_bernoulli_overflow_limit(const std::true_type&)
{
   return max_bernoulli_index<bernoulli_imp_variant<T>::value>::value;
}

template <class T, class Policy>
std::size_t b2n_overflow_limit()
{
   // This routine is called at program startup if it's called at all:
   // that guarantees safe initialization of the static variable.
   typedef std::integral_constant<bool, (bernoulli_imp_variant<T>::value >= 1) && (bernoulli_imp_variant<T>::value <= 3)> tag_type;
   static const std::size_t lim = find_bernoulli_overflow_limit<T, Policy>(tag_type());
   return lim;
}

//
// The tangent numbers grow larger much more rapidly than the Bernoulli numbers do....
// so to compute the Bernoulli numbers from the tangent numbers, we need to avoid spurious
// overflow in the calculation, we can do this by scaling all the tangent number by some scale factor:
//
template <class T, typename std::enable_if<std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::radix == 2), bool>::type = true>
inline T tangent_scale_factor()
{
   BOOST_MATH_STD_USING
   return ldexp(T(1), std::numeric_limits<T>::min_exponent + 5);
}

template <class T, typename std::enable_if<!std::numeric_limits<T>::is_specialized || !(std::numeric_limits<T>::radix == 2), bool>::type = true>
inline T tangent_scale_factor()
{
   return tools::min_value<T>() * 16;
}

//
// We need something to act as a cache for our calculated Bernoulli numbers.  In order to
// ensure both fast access and thread safety, we need a stable table which may be extended
// in size, but which never reallocates: that way values already calculated may be accessed
// concurrently with another thread extending the table with new values.
//
// Very very simple vector class that will never allocate more than once, we could use
// boost::container::static_vector here, but that allocates on the stack, which may well
// cause issues for the amount of memory we want in the extreme case...
//
template <class T>
struct fixed_vector : private std::allocator<T>
{
   typedef unsigned size_type;
   typedef T* iterator;
   typedef const T* const_iterator;
   fixed_vector() : m_used(0)
   { 
      std::size_t overflow_limit = 5 + b2n_overflow_limit<T, policies::policy<> >();
      m_capacity = static_cast<unsigned>((std::min)(overflow_limit, static_cast<std::size_t>(100000u)));
      m_data = this->allocate(m_capacity); 
   }
   ~fixed_vector()
   {
      typedef std::allocator<T> allocator_type;
      typedef std::allocator_traits<allocator_type> allocator_traits; 
      allocator_type& alloc = *this; 
      for(unsigned i = 0; i < m_used; ++i)
         allocator_traits::destroy(alloc, &m_data[i]);
      allocator_traits::deallocate(alloc, m_data, m_capacity);
   }
   T& operator[](unsigned n) { BOOST_MATH_ASSERT(n < m_used); return m_data[n]; }
   const T& operator[](unsigned n)const { BOOST_MATH_ASSERT(n < m_used); return m_data[n]; }
   unsigned size()const { return m_used; }
   unsigned size() { return m_used; }
   void resize(unsigned n, const T& val)
   {
      if(n > m_capacity)
      {
         BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Exhausted storage for Bernoulli numbers."));
      }
      for(unsigned i = m_used; i < n; ++i)
         new (m_data + i) T(val);
      m_used = n;
   }
   void resize(unsigned n) { resize(n, T()); }
   T* begin() { return m_data; }
   T* end() { return m_data + m_used; }
   T* begin()const { return m_data; }
   T* end()const { return m_data + m_used; }
   unsigned capacity()const { return m_capacity; }
   void clear() { m_used = 0; }
private:
   T* m_data;
   unsigned m_used, m_capacity;
};

template <class T, class Policy>
class bernoulli_numbers_cache
{
public:
   bernoulli_numbers_cache() : m_overflow_limit((std::numeric_limits<std::size_t>::max)())
      , m_counter(0)
      , m_current_precision(boost::math::tools::digits<T>())
   {}

   typedef fixed_vector<T> container_type;

   void tangent(std::size_t m)
   {
      static const std::size_t min_overflow_index = b2n_overflow_limit<T, Policy>() - 1;
      tn.resize(static_cast<typename container_type::size_type>(m), T(0U));

      BOOST_MATH_INSTRUMENT_VARIABLE(min_overflow_index);

      std::size_t prev_size = m_intermediates.size();
      m_intermediates.resize(m, T(0U));

      if(prev_size == 0)
      {
         m_intermediates[1] = tangent_scale_factor<T>() /*T(1U)*/;
         tn[0U] = T(0U);
         tn[1U] = tangent_scale_factor<T>()/* T(1U)*/;
         BOOST_MATH_INSTRUMENT_VARIABLE(tn[0]);
         BOOST_MATH_INSTRUMENT_VARIABLE(tn[1]);
      }

      for(std::size_t i = std::max<size_t>(2, prev_size); i < m; i++)
      {
         bool overflow_check = false;
         if(i >= min_overflow_index && (boost::math::tools::max_value<T>() / (i-1) < m_intermediates[1]) )
         {
            std::fill(tn.begin() + i, tn.end(), boost::math::tools::max_value<T>());
            break;
         }
         m_intermediates[1] = m_intermediates[1] * (i-1);
         for(std::size_t j = 2; j <= i; j++)
         {
            overflow_check =
                  (i >= min_overflow_index) && (
                  (boost::math::tools::max_value<T>() / (i - j) < m_intermediates[j])
                  || (boost::math::tools::max_value<T>() / (i - j + 2) < m_intermediates[j-1])
                  || (boost::math::tools::max_value<T>() - m_intermediates[j] * (i - j) < m_intermediates[j-1] * (i - j + 2))
                  || ((boost::math::isinf)(m_intermediates[j]))
                );

            if(overflow_check)
            {
               std::fill(tn.begin() + i, tn.end(), boost::math::tools::max_value<T>());
               break;
            }
            m_intermediates[j] = m_intermediates[j] * (i - j) + m_intermediates[j-1] * (i - j + 2);
         }
         if(overflow_check)
            break; // already filled the tn...
         tn[static_cast<typename container_type::size_type>(i)] = m_intermediates[i];
         BOOST_MATH_INSTRUMENT_VARIABLE(i);
         BOOST_MATH_INSTRUMENT_VARIABLE(tn[static_cast<typename container_type::size_type>(i)]);
      }
   }

   void tangent_numbers_series(const std::size_t m)
   {
      BOOST_MATH_STD_USING
      static const std::size_t min_overflow_index = b2n_overflow_limit<T, Policy>() - 1;

      typename container_type::size_type old_size = bn.size();

      tangent(m);
      bn.resize(static_cast<typename container_type::size_type>(m));

      if(!old_size)
      {
         bn[0] = 1;
         old_size = 1;
      }

      T power_two(ldexp(T(1), static_cast<int>(2 * old_size)));

      for(std::size_t i = old_size; i < m; i++)
      {
         T b(static_cast<T>(i * 2));
         //
         // Not only do we need to take care to avoid spurious over/under flow in
         // the calculation, but we also need to avoid overflow altogether in case
         // we're calculating with a type where "bad things" happen in that case:
         //
         b  = b / (power_two * tangent_scale_factor<T>());
         b /= (power_two - 1);
         bool overflow_check = (i >= min_overflow_index) && (tools::max_value<T>() / tn[static_cast<typename container_type::size_type>(i)] < b);
         if(overflow_check)
         {
            m_overflow_limit = i;
            while(i < m)
            {
               b = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : tools::max_value<T>();
               bn[static_cast<typename container_type::size_type>(i)] = ((i % 2U) ? b : T(-b));
               ++i;
            }
            break;
         }
         else
         {
            b *= tn[static_cast<typename container_type::size_type>(i)];
         }

         power_two = ldexp(power_two, 2);

         const bool b_neg = i % 2 == 0;

         bn[static_cast<typename container_type::size_type>(i)] = ((!b_neg) ? b : T(-b));
      }
   }

   template <class OutputIterator>
   OutputIterator copy_bernoulli_numbers(OutputIterator out, std::size_t start, std::size_t n, const Policy& pol)
   {
      //
      // There are basically 3 thread safety options:
      //
      // 1) There are no threads (BOOST_HAS_THREADS is not defined).
      // 2) There are threads, but we do not have a true atomic integer type, 
      //    in this case we just use a mutex to guard against race conditions.
      // 3) There are threads, and we have an atomic integer: in this case we can
      //    use the double-checked locking pattern to avoid thread synchronisation
      //    when accessing values already in the cache.
      //
      // First off handle the common case for overflow and/or asymptotic expansion:
      //
      if(start + n > bn.capacity())
      {
         if(start < bn.capacity())
         {
            out = copy_bernoulli_numbers(out, start, bn.capacity() - start, pol);
            n -= bn.capacity() - start;
            start = static_cast<std::size_t>(bn.capacity());
         }
         if(start < b2n_overflow_limit<T, Policy>() + 2u)
         {
            for(; n; ++start, --n)
            {
               *out = b2n_asymptotic<T, Policy>(static_cast<typename container_type::size_type>(start * 2U));
               ++out;
            }
         }
         for(; n; ++start, --n)
         {
            *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(start), pol);
            ++out;
         }
         return out;
      }

      #ifndef BOOST_HAS_THREADS
      //
      // Single threaded code, very simple:
      //
      if(m_current_precision < boost::math::tools::digits<T>())
      {
         bn.clear();
         tn.clear();
         m_intermediates.clear();
         m_current_precision = boost::math::tools::digits<T>();
      }
      if(start + n >= bn.size())
      {
         std::size_t new_size = (std::min)((std::max)((std::max)(std::size_t(start + n), std::size_t(bn.size() + 20)), std::size_t(50)), std::size_t(bn.capacity()));
         tangent_numbers_series(new_size);
      }

      for(std::size_t i = (std::max)(std::size_t(max_bernoulli_b2n<T>::value + 1), start); i < start + n; ++i)
      {
         *out = (i >= m_overflow_limit) ? policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol) : bn[i];
         ++out;
      }
      #else
      //
      // Double-checked locking pattern, lets us access cached already cached values
      // without locking:
      //
      // Get the counter and see if we need to calculate more constants:
      //
      if((static_cast<std::size_t>(m_counter.load(std::memory_order_consume)) < start + n)
         || (static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>()))
      {
         std::lock_guard<std::mutex> l(m_mutex);

         if((static_cast<std::size_t>(m_counter.load(std::memory_order_consume)) < start + n)
            || (static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>()))
         {
            if(static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>())
            {
               bn.clear();
               tn.clear();
               m_intermediates.clear();
               m_counter.store(0, std::memory_order_release);
               m_current_precision = boost::math::tools::digits<T>();
            }
            if(start + n >= bn.size())
            {
               std::size_t new_size = (std::min)((std::max)((std::max)(std::size_t(start + n), std::size_t(bn.size() + 20)), std::size_t(50)), std::size_t(bn.capacity()));
               tangent_numbers_series(new_size);
            }
            m_counter.store(static_cast<atomic_integer_type>(bn.size()), std::memory_order_release);
         }
      }

      for(std::size_t i = (std::max)(static_cast<std::size_t>(max_bernoulli_b2n<T>::value + 1), start); i < start + n; ++i)
      {
         *out = (i >= m_overflow_limit) ? policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol) : bn[static_cast<typename container_type::size_type>(i)];
         ++out;
      }

      #endif // BOOST_HAS_THREADS
      return out;
   }

   template <class OutputIterator>
   OutputIterator copy_tangent_numbers(OutputIterator out, std::size_t start, std::size_t n, const Policy& pol)
   {
      //
      // There are basically 3 thread safety options:
      //
      // 1) There are no threads (BOOST_HAS_THREADS is not defined).
      // 2) There are threads, but we do not have a true atomic integer type, 
      //    in this case we just use a mutex to guard against race conditions.
      // 3) There are threads, and we have an atomic integer: in this case we can
      //    use the double-checked locking pattern to avoid thread synchronisation
      //    when accessing values already in the cache.
      //
      //
      // First off handle the common case for overflow and/or asymptotic expansion:
      //
      if(start + n > bn.capacity())
      {
         if(start < bn.capacity())
         {
            out = copy_tangent_numbers(out, start, bn.capacity() - start, pol);
            n -= bn.capacity() - start;
            start = static_cast<std::size_t>(bn.capacity());
         }
         if(start < b2n_overflow_limit<T, Policy>() + 2u)
         {
            for(; n; ++start, --n)
            {
               *out = t2n_asymptotic<T, Policy>(static_cast<typename container_type::size_type>(start));
               ++out;
            }
         }
         for(; n; ++start, --n)
         {
            *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(start), pol);
            ++out;
         }
         return out;
      }

      #ifndef BOOST_HAS_THREADS
      //
      // Single threaded code, very simple:
      //
      if(m_current_precision < boost::math::tools::digits<T>())
      {
         bn.clear();
         tn.clear();
         m_intermediates.clear();
         m_current_precision = boost::math::tools::digits<T>();
      }
      if(start + n >= bn.size())
      {
         std::size_t new_size = (std::min)((std::max)((std::max)(start + n, std::size_t(bn.size() + 20)), std::size_t(50)), std::size_t(bn.capacity()));
         tangent_numbers_series(new_size);
      }

      for(std::size_t i = start; i < start + n; ++i)
      {
         if(i >= m_overflow_limit)
            *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol);
         else
         {
            if(tools::max_value<T>() * tangent_scale_factor<T>() < tn[static_cast<typename container_type::size_type>(i)])
               *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol);
            else
               *out = tn[static_cast<typename container_type::size_type>(i)] / tangent_scale_factor<T>();
         }
         ++out;
      }
      #else
      //
      // Double-checked locking pattern, lets us access cached already cached values
      // without locking:
      //
      // Get the counter and see if we need to calculate more constants:
      //
      if((static_cast<std::size_t>(m_counter.load(std::memory_order_consume)) < start + n)
         || (static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>()))
      {
         std::lock_guard<std::mutex> l(m_mutex);

         if((static_cast<std::size_t>(m_counter.load(std::memory_order_consume)) < start + n)
            || (static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>()))
         {
            if(static_cast<int>(m_current_precision.load(std::memory_order_consume)) < boost::math::tools::digits<T>())
            {
               bn.clear();
               tn.clear();
               m_intermediates.clear();
               m_counter.store(0, std::memory_order_release);
               m_current_precision = boost::math::tools::digits<T>();
            }
            if(start + n >= bn.size())
            {
               std::size_t new_size = (std::min)((std::max)((std::max)(start + n, std::size_t(bn.size() + 20)), std::size_t(50)), std::size_t(bn.capacity()));
               tangent_numbers_series(new_size);
            }
            m_counter.store(static_cast<atomic_integer_type>(bn.size()), std::memory_order_release);
         }
      }

      for(std::size_t i = start; i < start + n; ++i)
      {
         if(i >= m_overflow_limit)
            *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol);
         else
         {
            if(tools::max_value<T>() * tangent_scale_factor<T>() < tn[static_cast<typename container_type::size_type>(i)])
               *out = policies::raise_overflow_error<T>("boost::math::bernoulli_b2n<%1%>(std::size_t)", 0, T(i), pol);
            else
               *out = tn[static_cast<typename container_type::size_type>(i)] / tangent_scale_factor<T>();
         }
         ++out;
      }

      #endif // BOOST_HAS_THREADS
      return out;
   }

private:
   //
   // The caches for Bernoulli and tangent numbers, once allocated,
   // these must NEVER EVER reallocate as it breaks our thread
   // safety guarantees:
   //
   fixed_vector<T> bn, tn;
   std::vector<T> m_intermediates;
   // The value at which we know overflow has already occurred for the Bn:
   std::size_t m_overflow_limit;

   #ifdef BOOST_HAS_THREADS
   std::mutex m_mutex;
   atomic_counter_type m_counter, m_current_precision;
   #else
   int m_counter;
   int m_current_precision;
   #endif // BOOST_HAS_THREADS
};

template <class T, class Policy>
inline typename std::enable_if<(std::numeric_limits<T>::digits == 0) || (std::numeric_limits<T>::digits >= INT_MAX), bernoulli_numbers_cache<T, Policy>&>::type get_bernoulli_numbers_cache()
{
   //
   // When numeric_limits<>::digits is zero, the type has either not specialized numeric_limits at all
   // or it's precision can vary at runtime.  So make the cache thread_local so that each thread can
   // have it's own precision if required:
   //
   static 
#ifndef BOOST_MATH_NO_THREAD_LOCAL_WITH_NON_TRIVIAL_TYPES
      BOOST_MATH_THREAD_LOCAL
#endif
      bernoulli_numbers_cache<T, Policy> data;
   return data;
}
template <class T, class Policy>
inline typename std::enable_if<std::numeric_limits<T>::digits && (std::numeric_limits<T>::digits < INT_MAX), bernoulli_numbers_cache<T, Policy>&>::type get_bernoulli_numbers_cache()
{
   //
   // Note that we rely on C++11 thread-safe initialization here:
   //
   static bernoulli_numbers_cache<T, Policy> data;
   return data;
}

}}}

#endif // BOOST_MATH_BERNOULLI_DETAIL_HPP
