//  (C) Copyright Nick Thompson 2020.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_AGM_HPP
#define BOOST_MATH_TOOLS_AGM_HPP
#include <limits>
#include <cmath>

namespace boost { namespace math { namespace tools {

template<typename Real>
Real agm(Real a, Real g)
{
    using std::sqrt;
    
    if (a < g)
    {
        // Mathematica, mpfr, and mpmath are all symmetric functions:
        return agm(g, a);
    }
    // Use: M(rx, ry) = rM(x,y)
    if (a <= 0 || g <= 0) {
        if (a < 0 || g < 0) {
            return std::numeric_limits<Real>::quiet_NaN();
        }
        return Real(0);
    }

    // The number of correct digits doubles on each iteration.
    // Divide by 512 for some leeway:
    const Real scale = sqrt(std::numeric_limits<Real>::epsilon())/512;
    while (a-g > scale*g)
    {
        Real anp1 = (a + g)/2;
        g = sqrt(a*g);
        a = anp1;
    }

    // Final cleanup iteration recovers down to ~2ULPs:
    return (a + g)/2;
}


}}}
#endif
