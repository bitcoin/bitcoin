//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_COHEN_ACCELERATION_HPP
#define BOOST_MATH_TOOLS_COHEN_ACCELERATION_HPP
#include <limits>
#include <cmath>
#include <cstdint>

namespace boost::math::tools {

// Algorithm 1 of https://people.mpim-bonn.mpg.de/zagier/files/exp-math-9/fulltext.pdf
// Convergence Acceleration of Alternating Series: Henri Cohen, Fernando Rodriguez Villegas, and Don Zagier    
template<class G>
auto cohen_acceleration(G& generator, std::int64_t n = -1)
{
    using Real = decltype(generator());
    // This test doesn't pass for float128, sad!
    //static_assert(std::is_floating_point_v<Real>, "Real must be a floating point type.");
    using std::log;
    using std::pow;
    using std::ceil;
    using std::sqrt;
    Real n_ = n;
    if (n < 0)
    {
        // relative error grows as 2*5.828^-n; take 5.828^-n < eps/4 => -nln(5.828) < ln(eps/4) => n > ln(4/eps)/ln(5.828).
        // Is there a way to do it rapidly with std::log2? (Yes, of course; but for primitive types it's computed at compile-time anyway.)
        n_ = ceil(log(4/std::numeric_limits<Real>::epsilon())*0.5672963285532555);
        n = static_cast<std::int64_t>(n_);
    }
    // d can get huge and overflow if you pick n too large:
    Real d = pow(3 + sqrt(Real(8)), n);
    d = (d + 1/d)/2;
    Real b = -1;
    Real c = -d;
    Real s = 0;
    for (Real k = 0; k < n_; ++k) {
        c = b - c;
        s += c*generator();
        b = (k+n_)*(k-n_)*b/((k+Real(1)/Real(2))*(k+1));
    }

    return s/d;
}

}
#endif
