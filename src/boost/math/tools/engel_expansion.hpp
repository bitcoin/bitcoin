//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_ENGEL_EXPANSION_HPP
#define BOOST_MATH_TOOLS_ENGEL_EXPANSION_HPP

#include <cmath>
#include <cstdint>
#include <vector>
#include <ostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace boost::math::tools {

template<typename Real, typename Z = int64_t>
class engel_expansion {
public:
    engel_expansion(Real x) : x_{x}
    {
        using std::floor;
        using std::abs;
        using std::sqrt;
        using std::isfinite;
        if (!isfinite(x))
        {
            throw std::domain_error("Cannot convert non-finites into an Engel expansion.");
        }

        if(x==0)
        {
            throw std::domain_error("Zero does not have an Engel expansion.");
        }
        a_.reserve(64);
        // Let the error bound grow by 1 ULP/iteration.
        // I haven't done the error analysis to show that this is an expected rate of error growth,
        // but if you don't do this, you can easily get into an infinite loop.
        Real i = 1;
        Real computed = 0;
        Real term = 1;
        Real scale = std::numeric_limits<Real>::epsilon()*abs(x_)/2;
        Real u = x;
        while (abs(x_ - computed) > (i++)*scale)
        {
            Real recip = 1/u;
            Real ak = ceil(recip);
            a_.push_back(static_cast<Z>(ak));
            u = u*ak - 1;
            if (u==0)
            {
                break;
            }
            term /= ak;
            computed += term;
        }

        for (size_t j = 1; j < a_.size(); ++j)
        {
            // Sanity check: This should only happen when wraparound occurs:
            if (a_[j] < a_[j-1])
            {
                throw std::domain_error("The digits of an Engel expansion must form a non-decreasing sequence; consider increasing the wide of the integer type.");
            }
            // Watch out for saturating behavior:
            if (a_[j] == (std::numeric_limits<Z>::max)())
            {
                throw std::domain_error("The integer type Z does not have enough width to hold the terms of the Engel expansion; please widen the type.");
            }
        }
        a_.shrink_to_fit();
    }
    
    
    const std::vector<Z>& digits() const
    {
        return a_;
    }

    template<typename T, typename Z2>
    friend std::ostream& operator<<(std::ostream& out, engel_expansion<T, Z2>& eng);

private:
    Real x_;
    std::vector<Z> a_;
};


template<typename Real, typename Z2>
std::ostream& operator<<(std::ostream& out, engel_expansion<Real, Z2>& engel)
{
    constexpr const int p = std::numeric_limits<Real>::max_digits10;
    if constexpr (p == 2147483647)
    {
        out << std::setprecision(engel.x_.backend().precision());
    }
    else
    {
        out << std::setprecision(p);
    }

    out << "{";
    for (size_t i = 0; i < engel.a_.size() - 1; ++i)
    {
        out << engel.a_[i] << ", ";
    }
    out << engel.a_.back();
    out << "}";
    return out;
}


}
#endif
