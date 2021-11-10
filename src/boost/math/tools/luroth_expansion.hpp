//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_LUROTH_EXPANSION_HPP
#define BOOST_MATH_TOOLS_LUROTH_EXPANSION_HPP

#include <vector>
#include <ostream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace boost::math::tools {

template<typename Real, typename Z = int64_t>
class luroth_expansion {
public:
    luroth_expansion(Real x) : x_{x}
    {
        using std::floor;
        using std::abs;
        using std::sqrt;
        using std::isfinite;
        if (!isfinite(x))
        {
            throw std::domain_error("Cannot convert non-finites into a Luroth representation.");
        }
        d_.reserve(50);
        Real dn1 = floor(x);
        d_.push_back(static_cast<Z>(dn1));
        if (dn1 == x)
        {
           d_.shrink_to_fit();
           return;
        }
        // This attempts to follow the notation of:
        // "Khinchine's constant for Luroth Representation", by Sophia Kalpazidou.
        x = x - dn1;
        Real computed = dn1;
        Real prod = 1;
        // Let the error bound grow by 1 ULP/iteration.
        // I haven't done the error analysis to show that this is an expected rate of error growth,
        // but if you don't do this, you can easily get into an infinite loop.
        Real i = 1;
        Real scale = std::numeric_limits<Real>::epsilon()*abs(x_)/2;
        while (abs(x_ - computed) > (i++)*scale)
        {
           Real recip = 1/x;
           Real dn = floor(recip);
           // x = n + 1/k => lur(x) = ((n; k - 1))
           // Note that this is a bit different than Kalpazidou (examine the half-open interval of definition carefully).
           // One way to examine this definition is better for rationals (it never happens for irrationals)
           // is to consider i + 1/3. If you follow Kalpazidou, then you get ((i, 3, 0)); a zero digit!
           // That's bad since it destroys uniqueness and also breaks the computation of the geometric mean.
           if (recip == dn) {
              d_.push_back(static_cast<Z>(dn - 1));
              break;
           }
           d_.push_back(static_cast<Z>(dn));
           Real tmp = 1/(dn+1);
           computed += prod*tmp;
           prod *= tmp/dn;
           x = dn*(dn+1)*(x - tmp);
        }

        for (size_t i = 1; i < d_.size(); ++i)
        {
            // Sanity check:
            if (d_[i] <= 0)
            {
                throw std::domain_error("Found a digit <= 0; this is an error.");
            }
        }
        d_.shrink_to_fit();
    }
    
    
    const std::vector<Z>& digits() const {
      return d_;
    }

    // Under the assumption of 'randomness', this mean converges to 2.2001610580.
    // See Finch, Mathematical Constants, section 1.8.1.
    Real digit_geometric_mean() const {
        if (d_.size() == 1) {
            return std::numeric_limits<Real>::quiet_NaN();
        }
        using std::log;
        using std::exp;
        Real g = 0;
        for (size_t i = 1; i < d_.size(); ++i) {
            g += log(static_cast<Real>(d_[i]));
        }
        return exp(g/(d_.size() - 1));
    }
    
    template<typename T, typename Z2>
    friend std::ostream& operator<<(std::ostream& out, luroth_expansion<T, Z2>& scf);

private:
    const Real x_;
    std::vector<Z> d_;
};


template<typename Real, typename Z2>
std::ostream& operator<<(std::ostream& out, luroth_expansion<Real, Z2>& luroth)
{
   constexpr const int p = std::numeric_limits<Real>::max_digits10;
   if constexpr (p == 2147483647)
   {
      out << std::setprecision(luroth.x_.backend().precision());
   }
   else
   {
      out << std::setprecision(p);
   }

   out << "((" << luroth.d_.front();
   if (luroth.d_.size() > 1)
   {
      out << "; ";
      for (size_t i = 1; i < luroth.d_.size() -1; ++i)
      {
         out << luroth.d_[i] << ", ";
      }
      out << luroth.d_.back();
   }
   out << "))";
   return out;
}


}
#endif
