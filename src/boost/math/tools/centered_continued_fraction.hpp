//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_CENTERED_CONTINUED_FRACTION_HPP
#define BOOST_MATH_TOOLS_CENTERED_CONTINUED_FRACTION_HPP

#include <cmath>
#include <cstdint>
#include <vector>
#include <ostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <array>
#include <type_traits>
#include <boost/math/tools/is_standalone.hpp>

#ifndef BOOST_MATH_STANDALONE
#include <boost/core/demangle.hpp>
#endif

namespace boost::math::tools {

template<typename Real, typename Z = int64_t>
class centered_continued_fraction {
public:
    centered_continued_fraction(Real x) : x_{x} {
        static_assert(std::is_integral_v<Z> && std::is_signed_v<Z>,
                      "Centered continued fractions require signed integer types.");
        using std::round;
        using std::abs;
        using std::sqrt;
        using std::isfinite;
        if (!isfinite(x))
        {
            throw std::domain_error("Cannot convert non-finites into continued fractions.");  
        }
        b_.reserve(50);
        Real bj = round(x);
        b_.push_back(static_cast<Z>(bj));
        if (bj == x)
        {
            b_.shrink_to_fit();
            return;
        }
        x = 1/(x-bj);
        Real f = bj;
        if (bj == 0)
        {
            f = 16*(std::numeric_limits<Real>::min)();
        }
        Real C = f;
        Real D = 0;
        int i = 0;
        while (abs(f - x_) >= (1 + i++)*std::numeric_limits<Real>::epsilon()*abs(x_))
        {
            bj = round(x);
            b_.push_back(static_cast<Z>(bj));
            x = 1/(x-bj);
            D += bj;
            if (D == 0) {
                D = 16*(std::numeric_limits<Real>::min)();
            }
            C = bj + 1/C;
            if (C==0)
            {
                C = 16*(std::numeric_limits<Real>::min)();
            }
            D = 1/D;
            f *= (C*D);
        }
        // Deal with non-uniqueness of continued fractions: [a0; a1, ..., an, 1] = a0; a1, ..., an + 1].
        if (b_.size() > 2 && b_.back() == 1)
        {
            b_[b_.size() - 2] += 1;
            b_.resize(b_.size() - 1);
        }
        b_.shrink_to_fit();

        for (size_t i = 1; i < b_.size(); ++i)
        {
            if (b_[i] == 0) {
                std::ostringstream oss;
                oss << "Found a zero partial denominator: b[" << i << "] = " << b_[i] << "."
                    #ifndef BOOST_MATH_STANDALONE
                    << " This means the integer type '" << boost::core::demangle(typeid(Z).name())
                    #else
                    << " This means the integer type '" << typeid(Z).name()
                    #endif
                    << "' has overflowed and you need to use a wider type,"
                    << " or there is a bug.";
                throw std::overflow_error(oss.str());
            }
        }
    }

    Real khinchin_geometric_mean() const {
        if (b_.size() == 1)
        { 
            return std::numeric_limits<Real>::quiet_NaN();
        }
        using std::log;
        using std::exp;
        using std::abs;
        const std::array<Real, 7> logs{std::numeric_limits<Real>::quiet_NaN(), Real(0), log(static_cast<Real>(2)), log(static_cast<Real>(3)), log(static_cast<Real>(4)), log(static_cast<Real>(5)), log(static_cast<Real>(6))};
        Real log_prod = 0;
        for (size_t i = 1; i < b_.size(); ++i)
        {
            if (abs(b_[i]) < static_cast<Z>(logs.size()))
            {
                log_prod += logs[abs(b_[i])];
            }
            else
            {
                log_prod += log(static_cast<Real>(abs(b_[i])));
            }
        }
        log_prod /= (b_.size()-1);
        return exp(log_prod);
    }

    const std::vector<Z>& partial_denominators() const {
        return b_;
    }
    
    template<typename T, typename Z2>
    friend std::ostream& operator<<(std::ostream& out, centered_continued_fraction<T, Z2>& ccf);

private:
    const Real x_;
    std::vector<Z> b_;
};


template<typename Real, typename Z2>
std::ostream& operator<<(std::ostream& out, centered_continued_fraction<Real, Z2>& scf) {
    constexpr const int p = std::numeric_limits<Real>::max_digits10;
    if constexpr (p == 2147483647)
    {
        out << std::setprecision(scf.x_.backend().precision());
    }
    else
    {
        out << std::setprecision(p);
    }
   
    out << "[" << scf.b_.front();
    if (scf.b_.size() > 1)
    {
        out << "; ";
        for (size_t i = 1; i < scf.b_.size() -1; ++i)
        {
            out << scf.b_[i] << ", ";
        }
        out << scf.b_.back();
    }
    out << "]";
    return out;
}


}
#endif
