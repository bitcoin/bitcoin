// Boost.Geometry

// Copyright (c) 2016-2019 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_FORMULAS_INVERSE_DIFFERENTIAL_QUANTITIES_HPP
#define BOOST_GEOMETRY_FORMULAS_INVERSE_DIFFERENTIAL_QUANTITIES_HPP

#include <boost/geometry/core/assert.hpp>

#include <boost/geometry/util/condition.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace formula
{

/*!
\brief The solution of a part of the inverse problem - differential quantities.
\author See
- Charles F.F Karney, Algorithms for geodesics, 2011
https://arxiv.org/pdf/1109.4448.pdf
*/
template <
    typename CT,
    bool EnableReducedLength,
    bool EnableGeodesicScale,
    unsigned int Order = 2,
    bool ApproxF = true
>
class differential_quantities
{
public:
    static inline void apply(CT const& lon1, CT const& lat1,
                             CT const& lon2, CT const& lat2,
                             CT const& azimuth, CT const& reverse_azimuth,
                             CT const& b, CT const& f,
                             CT & reduced_length, CT & geodesic_scale)
    {
        CT const dlon = lon2 - lon1;
        CT const sin_lat1 = sin(lat1);
        CT const cos_lat1 = cos(lat1);
        CT const sin_lat2 = sin(lat2);
        CT const cos_lat2 = cos(lat2);

        apply(dlon, sin_lat1, cos_lat1, sin_lat2, cos_lat2,
              azimuth, reverse_azimuth,
              b, f,
              reduced_length, geodesic_scale);
    }

    static inline void apply(CT const& dlon,
                             CT const& sin_lat1, CT const& cos_lat1,
                             CT const& sin_lat2, CT const& cos_lat2,
                             CT const& azimuth, CT const& reverse_azimuth,
                             CT const& b, CT const& f,
                             CT & reduced_length, CT & geodesic_scale)
    {
        CT const c0 = 0;
        CT const c1 = 1;
        CT const one_minus_f = c1 - f;

        CT sin_bet1 = one_minus_f * sin_lat1;
        CT sin_bet2 = one_minus_f * sin_lat2;
            
        // equator
        if (math::equals(sin_bet1, c0) && math::equals(sin_bet2, c0))
        {
            CT const sig_12 = dlon / one_minus_f;
            if (BOOST_GEOMETRY_CONDITION(EnableReducedLength))
            {
                BOOST_GEOMETRY_ASSERT((-math::pi<CT>() <= azimuth && azimuth <= math::pi<CT>()));

                int azi_sign = math::sign(azimuth) >= 0 ? 1 : -1; // for antipodal
                CT m12 = azi_sign * sin(sig_12) * b;
                reduced_length = m12;
            }
                
            if (BOOST_GEOMETRY_CONDITION(EnableGeodesicScale))
            {
                CT M12 = cos(sig_12);
                geodesic_scale = M12;
            }
        }
        else
        {
            CT const c2 = 2;
            CT const e2 = f * (c2 - f);
            CT const ep2 = e2 / math::sqr(one_minus_f);

            CT const sin_alp1 = sin(azimuth);
            CT const cos_alp1 = cos(azimuth);
            //CT const sin_alp2 = sin(reverse_azimuth);
            CT const cos_alp2 = cos(reverse_azimuth);

            CT cos_bet1 = cos_lat1;
            CT cos_bet2 = cos_lat2;

            normalize(sin_bet1, cos_bet1);
            normalize(sin_bet2, cos_bet2);

            CT sin_sig1 = sin_bet1;
            CT cos_sig1 = cos_alp1 * cos_bet1;
            CT sin_sig2 = sin_bet2;
            CT cos_sig2 = cos_alp2 * cos_bet2;

            normalize(sin_sig1, cos_sig1);
            normalize(sin_sig2, cos_sig2);

            CT const sin_alp0 = sin_alp1 * cos_bet1;
            CT const cos_alp0_sqr = c1 - math::sqr(sin_alp0);

            CT const J12 = BOOST_GEOMETRY_CONDITION(ApproxF) ?
                           J12_f(sin_sig1, cos_sig1, sin_sig2, cos_sig2, cos_alp0_sqr, f) :
                           J12_ep_sqr(sin_sig1, cos_sig1, sin_sig2, cos_sig2, cos_alp0_sqr, ep2) ;

            CT const dn1 = math::sqrt(c1 + ep2 * math::sqr(sin_bet1));
            CT const dn2 = math::sqrt(c1 + ep2 * math::sqr(sin_bet2));

            if (BOOST_GEOMETRY_CONDITION(EnableReducedLength))
            {
                CT const m12_b = dn2 * (cos_sig1 * sin_sig2)
                               - dn1 * (sin_sig1 * cos_sig2)
                               - cos_sig1 * cos_sig2 * J12;
                CT const m12 = m12_b * b;

                reduced_length = m12;
            }

            if (BOOST_GEOMETRY_CONDITION(EnableGeodesicScale))
            {
                CT const cos_sig12 = cos_sig1 * cos_sig2 + sin_sig1 * sin_sig2;
                CT const t = ep2 * (cos_bet1 - cos_bet2) * (cos_bet1 + cos_bet2) / (dn1 + dn2);
                CT const M12 = cos_sig12 + (t * sin_sig2 - cos_sig2 * J12) * sin_sig1 / dn1;

                geodesic_scale = M12;
            }
        }
    }

private:
    /*! Approximation of J12, expanded into taylor series in f
        Maxima script:
        ep2: f * (2-f) / ((1-f)^2);
        k2: ca02 * ep2;
        assume(f < 1);
        assume(sig > 0);
        I1(sig):= integrate(sqrt(1 + k2 * sin(s)^2), s, 0, sig);
        I2(sig):= integrate(1/sqrt(1 + k2 * sin(s)^2), s, 0, sig);
        J(sig):= I1(sig) - I2(sig);
        S: taylor(J(sig), f, 0, 3);
        S1: factor( 2*integrate(sin(s)^2,s,0,sig)*ca02*f );
        S2: factor( ((integrate(-6*ca02^2*sin(s)^4+6*ca02*sin(s)^2,s,0,sig)+integrate(-2*ca02^2*sin(s)^4+6*ca02*sin(s)^2,s,0,sig))*f^2)/4 );
        S3: factor( ((integrate(30*ca02^3*sin(s)^6-54*ca02^2*sin(s)^4+24*ca02*sin(s)^2,s,0,sig)+integrate(6*ca02^3*sin(s)^6-18*ca02^2*sin(s)^4+24*ca02*sin(s)^2,s,0,sig))*f^3)/12 );
    */
    static inline CT J12_f(CT const& sin_sig1, CT const& cos_sig1,
                           CT const& sin_sig2, CT const& cos_sig2,
                           CT const& cos_alp0_sqr, CT const& f)
    {
        if (Order == 0)
        {
            return 0;
        }

        CT const c2 = 2;

        CT const sig_12 = atan2(cos_sig1 * sin_sig2 - sin_sig1 * cos_sig2,
                                cos_sig1 * cos_sig2 + sin_sig1 * sin_sig2);
        CT const sin_2sig1 = c2 * cos_sig1 * sin_sig1; // sin(2sig1)
        CT const sin_2sig2 = c2 * cos_sig2 * sin_sig2; // sin(2sig2)
        CT const sin_2sig_12 = sin_2sig2 - sin_2sig1;
        CT const L1 = sig_12 - sin_2sig_12 / c2;

        if (Order == 1)
        {
            return cos_alp0_sqr * f * L1;
        }
        
        CT const sin_4sig1 = c2 * sin_2sig1 * (math::sqr(cos_sig1) - math::sqr(sin_sig1)); // sin(4sig1)
        CT const sin_4sig2 = c2 * sin_2sig2 * (math::sqr(cos_sig2) - math::sqr(sin_sig2)); // sin(4sig2)
        CT const sin_4sig_12 = sin_4sig2 - sin_4sig1;
        
        CT const c8 = 8;
        CT const c12 = 12;
        CT const c16 = 16;
        CT const c24 = 24;

        CT const L2 = -( cos_alp0_sqr * sin_4sig_12
                         + (-c8 * cos_alp0_sqr + c12) * sin_2sig_12
                         + (c12 * cos_alp0_sqr - c24) * sig_12)
                       / c16;

        if (Order == 2)
        {
            return cos_alp0_sqr * f * (L1 + f * L2);
        }

        CT const c4 = 4;
        CT const c9 = 9;
        CT const c48 = 48;
        CT const c60 = 60;
        CT const c64 = 64;
        CT const c96 = 96;
        CT const c128 = 128;
        CT const c144 = 144;

        CT const cos_alp0_quad = math::sqr(cos_alp0_sqr);
        CT const sin3_2sig1 = math::sqr(sin_2sig1) * sin_2sig1;
        CT const sin3_2sig2 = math::sqr(sin_2sig2) * sin_2sig2;
        CT const sin3_2sig_12 = sin3_2sig2 - sin3_2sig1;

        CT const A = (c9 * cos_alp0_quad - c12 * cos_alp0_sqr) * sin_4sig_12;
        CT const B = c4 * cos_alp0_quad * sin3_2sig_12;
        CT const C = (-c48 * cos_alp0_quad + c96 * cos_alp0_sqr - c64) * sin_2sig_12;
        CT const D = (c60 * cos_alp0_quad - c144 * cos_alp0_sqr + c128) * sig_12;

        CT const L3 = (A + B + C + D) / c64;

        // Order 3 and higher
        return cos_alp0_sqr * f * (L1 + f * (L2 + f * L3));
    }

    /*! Approximation of J12, expanded into taylor series in e'^2
        Maxima script:
        k2: ca02 * ep2;
        assume(sig > 0);
        I1(sig):= integrate(sqrt(1 + k2 * sin(s)^2), s, 0, sig);
        I2(sig):= integrate(1/sqrt(1 + k2 * sin(s)^2), s, 0, sig);
        J(sig):= I1(sig) - I2(sig);
        S: taylor(J(sig), ep2, 0, 3);
        S1: factor( integrate(sin(s)^2,s,0,sig)*ca02*ep2 );
        S2: factor( (integrate(sin(s)^4,s,0,sig)*ca02^2*ep2^2)/2 );
        S3: factor( (3*integrate(sin(s)^6,s,0,sig)*ca02^3*ep2^3)/8 );
    */
    static inline CT J12_ep_sqr(CT const& sin_sig1, CT const& cos_sig1,
                                CT const& sin_sig2, CT const& cos_sig2,
                                CT const& cos_alp0_sqr, CT const& ep_sqr)
    {
        if (Order == 0)
        {
            return 0;
        }

        CT const c2 = 2;
        CT const c4 = 4;

        CT const c2a0ep2 = cos_alp0_sqr * ep_sqr;

        CT const sig_12 = atan2(cos_sig1 * sin_sig2 - sin_sig1 * cos_sig2,
                                cos_sig1 * cos_sig2 + sin_sig1 * sin_sig2); // sig2 - sig1
        CT const sin_2sig1 = c2 * cos_sig1 * sin_sig1; // sin(2sig1)
        CT const sin_2sig2 = c2 * cos_sig2 * sin_sig2; // sin(2sig2)
        CT const sin_2sig_12 = sin_2sig2 - sin_2sig1;

        CT const L1 = (c2 * sig_12 - sin_2sig_12) / c4;

        if (Order == 1)
        {
            return c2a0ep2 * L1;
        }

        CT const c8 = 8;
        CT const c64 = 64;
        
        CT const sin_4sig1 = c2 * sin_2sig1 * (math::sqr(cos_sig1) - math::sqr(sin_sig1)); // sin(4sig1)
        CT const sin_4sig2 = c2 * sin_2sig2 * (math::sqr(cos_sig2) - math::sqr(sin_sig2)); // sin(4sig2)
        CT const sin_4sig_12 = sin_4sig2 - sin_4sig1;
        
        CT const L2 = (sin_4sig_12 - c8 * sin_2sig_12 + 12 * sig_12) / c64;

        if (Order == 2)
        {
            return c2a0ep2 * (L1 + c2a0ep2 * L2);
        }

        CT const sin3_2sig1 = math::sqr(sin_2sig1) * sin_2sig1;
        CT const sin3_2sig2 = math::sqr(sin_2sig2) * sin_2sig2;
        CT const sin3_2sig_12 = sin3_2sig2 - sin3_2sig1;

        CT const c9 = 9;
        CT const c48 = 48;
        CT const c60 = 60;
        CT const c512 = 512;

        CT const L3 = (c9 * sin_4sig_12 + c4 * sin3_2sig_12 - c48 * sin_2sig_12 + c60 * sig_12) / c512;

        // Order 3 and higher
        return c2a0ep2 * (L1 + c2a0ep2 * (L2 + c2a0ep2 * L3));
    }

    static inline void normalize(CT & x, CT & y)
    {
        CT const len = math::sqrt(math::sqr(x) + math::sqr(y));
        x /= len;
        y /= len;
    }
};

}}} // namespace boost::geometry::formula


#endif // BOOST_GEOMETRY_FORMULAS_INVERSE_DIFFERENTIAL_QUANTITIES_HPP
