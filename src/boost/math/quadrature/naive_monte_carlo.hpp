/*
 * Copyright Nick Thompson, 2018
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef BOOST_MATH_QUADRATURE_NAIVE_MONTE_CARLO_HPP
#define BOOST_MATH_QUADRATURE_NAIVE_MONTE_CARLO_HPP
#include <sstream>
#include <algorithm>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <future>
#include <thread>
#include <initializer_list>
#include <utility>
#include <random>
#include <chrono>
#include <map>
#include <type_traits>
#include <boost/math/policies/error_handling.hpp>

namespace boost { namespace math { namespace quadrature {

namespace detail {
  enum class limit_classification {FINITE,
                                   LOWER_BOUND_INFINITE,
                                   UPPER_BOUND_INFINITE,
                                   DOUBLE_INFINITE};
}

template<class Real, class F, class RandomNumberGenerator = std::mt19937_64, class Policy = boost::math::policies::policy<>,
         typename std::enable_if<std::is_trivially_copyable<Real>::value, bool>::type = true>
class naive_monte_carlo
{
public:
    naive_monte_carlo(const F& integrand,
                      std::vector<std::pair<Real, Real>> const & bounds,
                      Real error_goal,
                      bool singular = true,
                      uint64_t threads = std::thread::hardware_concurrency(),
                      uint64_t seed = 0) noexcept : m_num_threads{threads}, m_seed{seed}
    {
        using std::numeric_limits;
        using std::sqrt;
        uint64_t n = bounds.size();
        m_lbs.resize(n);
        m_dxs.resize(n);
        m_limit_types.resize(n);
        m_volume = 1;
        static const char* function = "boost::math::quadrature::naive_monte_carlo<%1%>";
        for (uint64_t i = 0; i < n; ++i)
        {
            if (bounds[i].second <= bounds[i].first)
            {
                boost::math::policies::raise_domain_error(function, "The upper bound is <= the lower bound.\n", bounds[i].second, Policy());
                return;
            }
            if (bounds[i].first == -numeric_limits<Real>::infinity())
            {
                if (bounds[i].second == numeric_limits<Real>::infinity())
                {
                    m_limit_types[i] = detail::limit_classification::DOUBLE_INFINITE;
                }
                else
                {
                    m_limit_types[i] = detail::limit_classification::LOWER_BOUND_INFINITE;
                    // Ok ok this is bad to use the second bound as the lower limit and then reflect.
                    m_lbs[i] = bounds[i].second;
                    m_dxs[i] = numeric_limits<Real>::quiet_NaN();
                }
            }
            else if (bounds[i].second == numeric_limits<Real>::infinity())
            {
                m_limit_types[i] = detail::limit_classification::UPPER_BOUND_INFINITE;
                if (singular)
                {
                    // I've found that it's easier to sample on a closed set and perturb the boundary
                    // than to try to sample very close to the boundary.
                    m_lbs[i] = std::nextafter(bounds[i].first, (std::numeric_limits<Real>::max)());
                }
                else
                {
                    m_lbs[i] = bounds[i].first;
                }
                m_dxs[i] = numeric_limits<Real>::quiet_NaN();
            }
            else
            {
                m_limit_types[i] = detail::limit_classification::FINITE;
                if (singular)
                {
                    if (bounds[i].first == 0)
                    {
                        m_lbs[i] = std::numeric_limits<Real>::epsilon();
                    }
                    else
                    {
                        m_lbs[i] = std::nextafter(bounds[i].first, (std::numeric_limits<Real>::max)());
                    }

                    m_dxs[i] = std::nextafter(bounds[i].second, std::numeric_limits<Real>::lowest()) - m_lbs[i];
                }
                else
                {
                    m_lbs[i] = bounds[i].first;
                    m_dxs[i] = bounds[i].second - bounds[i].first;
                }
                m_volume *= m_dxs[i];
            }
        }

        m_integrand = [this, &integrand](std::vector<Real> & x)->Real
        {
            Real coeff = m_volume;
            for (uint64_t i = 0; i < x.size(); ++i)
            {
                // Variable transformation are listed at:
                // https://en.wikipedia.org/wiki/Numerical_integration
                // However, we've made some changes to these so that we can evaluate on a compact domain.
                if (m_limit_types[i] == detail::limit_classification::FINITE)
                {
                    x[i] = m_lbs[i] + x[i]*m_dxs[i];
                }
                else if (m_limit_types[i] == detail::limit_classification::UPPER_BOUND_INFINITE)
                {
                    Real t = x[i];
                    Real z = 1/(1 + numeric_limits<Real>::epsilon() - t);
                    coeff *= (z*z)*(1 + numeric_limits<Real>::epsilon());
                    x[i] = m_lbs[i] + t*z;
                }
                else if (m_limit_types[i] == detail::limit_classification::LOWER_BOUND_INFINITE)
                {
                    Real t = x[i];
                    Real z = 1/(t+sqrt((numeric_limits<Real>::min)()));
                    coeff *= (z*z);
                    x[i] = m_lbs[i] + (t-1)*z;
                }
                else
                {
                    Real t1 = 1/(1+numeric_limits<Real>::epsilon() - x[i]);
                    Real t2 = 1/(x[i]+numeric_limits<Real>::epsilon());
                    x[i] = (2*x[i]-1)*t1*t2/4;
                    coeff *= (t1*t1+t2*t2)/4;
                }
            }
            return coeff*integrand(x);
        };

        // If we don't do a single function call in the constructor,
        // we can't do a restart.
        std::vector<Real> x(m_lbs.size());

        // If the seed is zero, that tells us to choose a random seed for the user:
        if (seed == 0)
        {
            std::random_device rd;
            seed = rd();
        }

        RandomNumberGenerator gen(seed);
        Real inv_denom = 1/static_cast<Real>(((gen.max)()-(gen.min)()));

        m_num_threads = (std::max)(m_num_threads, (uint64_t) 1);
        m_thread_calls.reset(new std::atomic<uint64_t>[threads]);
        m_thread_Ss.reset(new std::atomic<Real>[threads]);
        m_thread_averages.reset(new std::atomic<Real>[threads]);

        Real avg = 0;
        for (uint64_t i = 0; i < m_num_threads; ++i)
        {
            for (uint64_t j = 0; j < m_lbs.size(); ++j)
            {
                x[j] = (gen()-(gen.min)())*inv_denom;
            }
            Real y = m_integrand(x);
            m_thread_averages[i] = y; // relaxed store
            m_thread_calls[i] = 1;
            m_thread_Ss[i] = 0;
            avg += y;
        }
        avg /= m_num_threads;
        m_avg = avg; // relaxed store

        m_error_goal = error_goal; // relaxed store
        m_start = std::chrono::system_clock::now();
        m_done = false; // relaxed store
        m_total_calls = m_num_threads;  // relaxed store
        m_variance = (numeric_limits<Real>::max)();
    }

    std::future<Real> integrate()
    {
        // Set done to false in case we wish to restart:
        m_done.store(false); // relaxed store, no worker threads yet
        m_start = std::chrono::system_clock::now();
        return std::async(std::launch::async,
                          &naive_monte_carlo::m_integrate, this);
    }

    void cancel()
    {
        // If seed = 0 (meaning have the routine pick the seed), this leaves the seed the same.
        // If seed != 0, then the seed is changed, so a restart doesn't do the exact same thing.
        m_seed = m_seed*m_seed;
        m_done = true; // relaxed store, worker threads will get the message eventually
        // Make sure the error goal is infinite, because otherwise we'll loop when we do the final error goal check:
        m_error_goal = (std::numeric_limits<Real>::max)();
    }

    Real variance() const
    {
        return m_variance.load();
    }

    Real current_error_estimate() const
    {
        using std::sqrt;
        //
        // There is a bug here: m_variance and m_total_calls get updated asynchronously
        // and may be out of synch when we compute the error estimate, not sure if it matters though...
        //
        return sqrt(m_variance.load()/m_total_calls.load());
    }

    std::chrono::duration<Real> estimated_time_to_completion() const
    {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<Real> elapsed_seconds = now - m_start;
        Real r = this->current_error_estimate()/m_error_goal.load(); // relaxed load
        if (r*r <= 1) {
            return 0*elapsed_seconds;
        }
        return (r*r - 1)*elapsed_seconds;
    }

    void update_target_error(Real new_target_error)
    {
        m_error_goal = new_target_error;  // relaxed store
    }

    Real progress() const
    {
        Real r = m_error_goal.load()/this->current_error_estimate();  // relaxed load
        if (r*r >= 1)
        {
            return 1;
        }
        return r*r;
    }

    Real current_estimate() const
    {
        return m_avg.load();
    }

    uint64_t calls() const
    {
        return m_total_calls.load();  // relaxed load
    }

private:

   Real m_integrate()
   {
      uint64_t seed;
      // If the user tells us to pick a seed, pick a seed:
      if (m_seed == 0)
      {
         std::random_device rd;
         seed = rd();
      }
      else // use the seed we are given:
      {
         seed = m_seed;
      }
      RandomNumberGenerator gen(seed);
      int max_repeat_tries = 5;
      do{

         if (max_repeat_tries < 5)
         {
            m_done = false;

#ifdef BOOST_NAIVE_MONTE_CARLO_DEBUG_FAILURES
            std::cout << "Failed to achieve required tolerance first time through..\n";
            std::cout << "  variance =    " << m_variance << std::endl;
            std::cout << "  average =     " << m_avg << std::endl;
            std::cout << "  total calls = " << m_total_calls << std::endl;

            for (std::size_t i = 0; i < m_num_threads; ++i)
               std::cout << "  thread_calls[" << i << "] = " << m_thread_calls[i] << std::endl;
            for (std::size_t i = 0; i < m_num_threads; ++i)
               std::cout << "  thread_averages[" << i << "] = " << m_thread_averages[i] << std::endl;
            for (std::size_t i = 0; i < m_num_threads; ++i)
               std::cout << "  thread_Ss[" << i << "] = " << m_thread_Ss[i] << std::endl;
#endif
         }

         std::vector<std::thread> threads(m_num_threads);
         for (uint64_t i = 0; i < threads.size(); ++i)
         {
            threads[i] = std::thread(&naive_monte_carlo::m_thread_monte, this, i, gen());
         }
         do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            uint64_t total_calls = 0;
            for (uint64_t i = 0; i < m_num_threads; ++i)
            {
               uint64_t t_calls = m_thread_calls[i].load(std::memory_order_consume);
               total_calls += t_calls;
            }
            Real variance = 0;
            Real avg = 0;
            for (uint64_t i = 0; i < m_num_threads; ++i)
            {
               uint64_t t_calls = m_thread_calls[i].load(std::memory_order_consume);
               // Will this overflow? Not hard to remove . . .
               avg += m_thread_averages[i].load(std::memory_order_relaxed)*((Real)t_calls / (Real)total_calls);
               variance += m_thread_Ss[i].load(std::memory_order_relaxed);
            }
            m_avg.store(avg, std::memory_order_release);
            m_variance.store(variance / (total_calls - 1), std::memory_order_release);
            m_total_calls = total_calls; // relaxed store, it's just for user feedback
            // Allow cancellation:
            if (m_done) // relaxed load
            {
               break;
            }
         } while (m_total_calls < 2048 || this->current_error_estimate() > m_error_goal.load(std::memory_order_consume));
         // Error bound met; signal the threads:
         m_done = true; // relaxed store, threads will get the message in the end
         std::for_each(threads.begin(), threads.end(),
            std::mem_fn(&std::thread::join));
         if (m_exception)
         {
            std::rethrow_exception(m_exception);
         }
         // Incorporate their work into the final estimate:
         uint64_t total_calls = 0;
         for (uint64_t i = 0; i < m_num_threads; ++i)
         {
            uint64_t t_calls = m_thread_calls[i].load(std::memory_order_consume);
            total_calls += t_calls;
         }
         Real variance = 0;
         Real avg = 0;

         for (uint64_t i = 0; i < m_num_threads; ++i)
         {
            uint64_t t_calls = m_thread_calls[i].load(std::memory_order_consume);
            // Averages weighted by the number of calls the thread made:
            avg += m_thread_averages[i].load(std::memory_order_relaxed)*((Real)t_calls / (Real)total_calls);
            variance += m_thread_Ss[i].load(std::memory_order_relaxed);
         }
         m_avg.store(avg, std::memory_order_release);
         m_variance.store(variance / (total_calls - 1), std::memory_order_release);
         m_total_calls = total_calls; // relaxed store, this is just user feedback

         // Sometimes, the master will observe the variance at a very "good" (or bad?) moment,
         // Then the threads proceed to find the variance is much greater by the time they hear the message to stop.
         // This *WOULD* make sure that the final error estimate is within the error bounds.
      }
      while ((--max_repeat_tries >= 0) && (this->current_error_estimate() > m_error_goal));

      return m_avg.load(std::memory_order_consume);
    }

    void m_thread_monte(uint64_t thread_index, uint64_t seed)
    {
        using std::numeric_limits;
        try
        {
            std::vector<Real> x(m_lbs.size());
            RandomNumberGenerator gen(seed);
            Real inv_denom = (Real) 1/(Real)( (gen.max)() - (gen.min)()  );
            Real M1 = m_thread_averages[thread_index].load(std::memory_order_consume);
            Real S = m_thread_Ss[thread_index].load(std::memory_order_consume);
            // Kahan summation is required or the value of the integrand will go on a random walk during long computations.
            // See the implementation discussion.
            // The idea is that the unstabilized additions have error sigma(f)/sqrt(N) + epsilon*N, which diverges faster than it converges!
            // Kahan summation turns this to sigma(f)/sqrt(N) + epsilon^2*N, and the random walk occurs on a timescale of 10^14 years (on current hardware)
            Real compensator = 0;
            uint64_t k = m_thread_calls[thread_index].load(std::memory_order_consume);
            while (!m_done) // relaxed load
            {
                int j = 0;
                // If we don't have a certain number of calls before an update, we can easily terminate prematurely
                // because the variance estimate is way too low. This magic number is a reasonable compromise, as 1/sqrt(2048) = 0.02,
                // so it should recover 2 digits if the integrand isn't poorly behaved, and if it is, it should discover that before premature termination.
                // Of course if the user has 64 threads, then this number is probably excessive.
                int magic_calls_before_update = 2048;
                while (j++ < magic_calls_before_update)
                {
                    for (uint64_t i = 0; i < m_lbs.size(); ++i)
                    {
                        x[i] = (gen() - (gen.min)())*inv_denom;
                    }
                    Real f = m_integrand(x);
                    using std::isfinite;
                    if (!isfinite(f))
                    {
                        // The call to m_integrand transform x, so this error message states the correct node.
                        std::stringstream os;
                        os << "Your integrand was evaluated at {";
                        for (uint64_t i = 0; i < x.size() -1; ++i)
                        {
                             os << x[i] << ", ";
                        }
                        os << x[x.size() -1] << "}, and returned " << f << std::endl;
                        static const char* function = "boost::math::quadrature::naive_monte_carlo<%1%>";
                        boost::math::policies::raise_domain_error(function, os.str().c_str(), /*this is a dummy arg to make it compile*/ 7.2, Policy());
                    }
                    ++k;
                    Real term = (f - M1)/k;
                    Real y1 = term - compensator;
                    Real M2 = M1 + y1;
                    compensator = (M2 - M1) - y1;
                    S += (f - M1)*(f - M2);
                    M1 = M2;
                }
                m_thread_averages[thread_index].store(M1, std::memory_order_release);
                m_thread_Ss[thread_index].store(S, std::memory_order_release);
                m_thread_calls[thread_index].store(k, std::memory_order_release);
            }
        }
        catch (...)
        {
            // Signal the other threads that the computation is ruined:
            m_done = true; // relaxed store
            std::lock_guard<std::mutex> lock(m_exception_mutex); // Scoped lock to prevent race writing to m_exception
            m_exception = std::current_exception();
        }
    }

    std::function<Real(std::vector<Real> &)> m_integrand;
    uint64_t m_num_threads;
    std::atomic<uint64_t> m_seed;
    std::atomic<Real> m_error_goal;
    std::atomic<bool> m_done;
    std::vector<Real> m_lbs;
    std::vector<Real> m_dxs;
    std::vector<detail::limit_classification> m_limit_types;
    Real m_volume;
    std::atomic<uint64_t> m_total_calls;
    // I wanted these to be vectors rather than maps,
    // but you can't resize a vector of atomics.
    std::unique_ptr<std::atomic<uint64_t>[]> m_thread_calls;
    std::atomic<Real> m_variance;
    std::unique_ptr<std::atomic<Real>[]> m_thread_Ss;
    std::atomic<Real> m_avg;
    std::unique_ptr<std::atomic<Real>[]> m_thread_averages;
    std::chrono::time_point<std::chrono::system_clock> m_start;
    std::exception_ptr m_exception;
    std::mutex m_exception_mutex;
};

}}}
#endif
