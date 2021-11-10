// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2019 Tinko Bartels, Berlin, Germany.

// Contributed and/or modified by Tinko Bartels,
//   as part of Google Summer of Code 2019 program.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_TRIANGULATION_STRATEGIES_CARTESIAN_DETAIL_PRECISE_MATH_HPP
#define BOOST_GEOMETRY_EXTENSIONS_TRIANGULATION_STRATEGIES_CARTESIAN_DETAIL_PRECISE_MATH_HPP

#include<numeric>
#include<cmath>
#include<limits>
#include<array>

#include <boost/geometry/core/access.hpp>

// The following code is based on "Adaptive Precision Floating-Point Arithmetic
// and Fast Robust Geometric Predicates" by Richard Shewchuk,
// J. Discrete Comput Geom (1997) 18: 305. https://doi.org/10.1007/PL00009321

namespace boost { namespace geometry
{

namespace detail { namespace precise_math
{

// See Theorem 6, page 6
template
<
    typename RealNumber
>
inline std::array<RealNumber, 2> fast_two_sum(RealNumber const a,
                                              RealNumber const b)
{
    RealNumber x = a + b;
    RealNumber b_virtual = x - a;
    return {{x, b - b_virtual}};
}

// See Theorem 7, page 7 - 8
template
<
    typename RealNumber
>
inline std::array<RealNumber, 2> two_sum(RealNumber const a,
                                         RealNumber const b)
{
    RealNumber x = a + b;
    RealNumber b_virtual = x - a;
    RealNumber a_virtual = x - b_virtual;
    RealNumber b_roundoff = b - b_virtual;
    RealNumber a_roundoff = a - a_virtual;
    RealNumber y = a_roundoff + b_roundoff;
    return {{ x,  y }};
}

// See bottom of page 8
template
<
    typename RealNumber
>
inline RealNumber two_diff_tail(RealNumber const a,
                                RealNumber const b,
                                RealNumber const x)
{
    RealNumber b_virtual = a - x;
    RealNumber a_virtual = x + b_virtual;
    RealNumber b_roundoff = b_virtual - b;
    RealNumber a_roundoff = a - a_virtual;
    return a_roundoff + b_roundoff;
}

// see bottom of page 8
template
<
    typename RealNumber
>
inline std::array<RealNumber, 2> two_diff(RealNumber const a,
                                          RealNumber const b)
{
    RealNumber x = a - b;
    RealNumber y = two_diff_tail(a, b, x);
    return {{ x, y }};
}

// see theorem 18, page 19
template
<
    typename RealNumber
>
inline RealNumber two_product_tail(RealNumber const a,
                                   RealNumber const b,
                                   RealNumber const x)
{
    return std::fma(a, b, -x);
}

// see theorem 18, page 19
template
<
    typename RealNumber
>
inline std::array<RealNumber, 2> two_product(RealNumber const a,
                                             RealNumber const b)
{
    RealNumber x = a * b;
    RealNumber y = two_product_tail(a, b, x);
    return {{ x , y }};
}

// see theorem 12, figure 7, page 11 - 12,
// this is the 2 by 2 case for the corresponding diff-method
// note that this method takes input in descending order of magnitude and
// returns components in ascending order of magnitude
template
<
    typename RealNumber
>
inline std::array<RealNumber, 4> two_two_expansion_diff(
        std::array<RealNumber, 2> const a,
        std::array<RealNumber, 2> const b)
{
    std::array<RealNumber, 4> h;
    std::array<RealNumber, 2> Qh = two_diff(a[1], b[1]);
    h[0] = Qh[1];
    Qh = two_sum( a[0], Qh[0] );
    RealNumber _j = Qh[0];
    Qh = two_diff(Qh[1], b[0]);
    h[1] = Qh[1];
    Qh = two_sum( _j, Qh[0] );
    h[2] = Qh[1];
    h[3] = Qh[0];
    return h;
}

// see theorem 13, figure 8. This implementation uses zero elimination as
// suggested on page 17, second to last paragraph. Returns the number of
// non-zero components in the result and writes the result to h.
// the merger into a single sequence g is done implicitly
template
<
    typename RealNumber,
    std::size_t InSize1,
    std::size_t InSize2,
    std::size_t OutSize
>
inline int fast_expansion_sum_zeroelim(
        std::array<RealNumber, InSize1> const& e,
        std::array<RealNumber, InSize2> const& f,
        std::array<RealNumber, OutSize> & h,
        int m = InSize1,
        int n = InSize2)
{
    std::array<RealNumber, 2> Qh;
    int i_e = 0, i_f = 0, i_h = 0;
    if (std::abs(f[0]) > std::abs(e[0]))
    {
        Qh[0] = e[i_e++];
    }
    else
    {
        Qh[0] = f[i_f++];
    }
    i_h = 0;
    if ((i_e < m) && (i_f < n))
    {
        if (std::abs(f[i_f]) > std::abs(e[i_e]))
        {
            Qh = fast_two_sum(e[i_e++], Qh[0]);
        }
        else
        {
            Qh = fast_two_sum(f[i_f++], Qh[0]);
        }
        if (Qh[1] != 0.0)
        {
            h[i_h++] = Qh[1];
        }
        while ((i_e < m) && (i_f < n))
        {
            if (std::abs(f[i_f]) > std::abs(e[i_e]))
            {
                Qh = two_sum(Qh[0], e[i_e++]);
            }
            else
            {
                Qh = two_sum(Qh[0], f[i_f++]);
            }
            if (Qh[1] != 0.0)
            {
                h[i_h++] = Qh[1];
            }
        }
    }
    while (i_e < m)
    {
        Qh = two_sum(Qh[0], e[i_e++]);
        if (Qh[1] != 0.0)
        {
            h[i_h++] = Qh[1];
        }
    }
    while (i_f < n)
    {
        Qh = two_sum(Qh[0], f[i_f++]);
        if (Qh[1] != 0.0)
        {
            h[i_h++] = Qh[1];
        }
    }
    if ((Qh[0] != 0.0) || (i_h == 0))
    {
        h[i_h++] = Qh[0];
    }
    return i_h;
}

// see theorem 19, figure 13, page 20 - 21. This implementation uses zero
// elimination as suggested on page 17, second to last paragraph. Returns the
// number of non-zero components in the result and writes the result to h.
template
<
    typename RealNumber,
    std::size_t InSize
>
inline int scale_expansion_zeroelim(
        std::array<RealNumber, InSize> const& e,
        RealNumber const b,
        std::array<RealNumber, 2 * InSize> & h,
        int e_non_zeros = InSize)
{
    std::array<RealNumber, 2> Qh = two_product(e[0], b);
    int i_h = 0;
    if (Qh[1] != 0)
    {
        h[i_h++] = Qh[1];
    }
    for (int i_e = 1; i_e < e_non_zeros; i_e++)
    {
        std::array<RealNumber, 2> Tt = two_product(e[i_e], b);
        Qh = two_sum(Qh[0], Tt[1]);
        if (Qh[1] != 0)
        {
            h[i_h++] = Qh[1];
        }
        Qh = fast_two_sum(Tt[0], Qh[0]);
        if (Qh[1] != 0)
        {
            h[i_h++] = Qh[1];
        }
    }
    if ((Qh[0] != 0.0) || (i_h == 0))
    {
        h[i_h++] = Qh[0];
    }
    return i_h;
}

template<typename RealNumber>
struct vec2d
{
    RealNumber x;
    RealNumber y;
};

template
<
    typename RealNumber,
    std::size_t Robustness
>
inline RealNumber orient2dtail(vec2d<RealNumber> const& p1,
                               vec2d<RealNumber> const& p2,
                               vec2d<RealNumber> const& p3,
                               std::array<RealNumber, 2>& t1,
                               std::array<RealNumber, 2>& t2,
                               std::array<RealNumber, 2>& t3,
                               std::array<RealNumber, 2>& t4,
                               std::array<RealNumber, 2>& t5_01,
                               std::array<RealNumber, 2>& t6_01,
                               RealNumber const& magnitude
                               )
{
    t5_01[1] = two_product_tail(t1[0], t2[0], t5_01[0]);
    t6_01[1] = two_product_tail(t3[0], t4[0], t6_01[0]);
    std::array<RealNumber, 4> tA_03 = two_two_expansion_diff(t5_01, t6_01);
    RealNumber det = std::accumulate(tA_03.begin(), tA_03.end(), static_cast<RealNumber>(0));
    if(Robustness == 1) return det;
    // see p.39, mind the different definition of epsilon for error bound
    RealNumber B_relative_bound =
          (1 + 3 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon();
    RealNumber absolute_bound = B_relative_bound * magnitude;
    if (std::abs(det) >= absolute_bound)
    {
        return det; //B estimate
    }
    t1[1] = two_diff_tail(p1.x, p3.x, t1[0]);
    t2[1] = two_diff_tail(p2.y, p3.y, t2[0]);
    t3[1] = two_diff_tail(p1.y, p3.y, t3[0]);
    t4[1] = two_diff_tail(p2.x, p3.x, t4[0]);

    if ((t1[1] == 0) && (t3[1] == 0) && (t2[1] == 0) && (t4[1] == 0))
    {
        return det; //If all tails are zero, there is noething else to compute
    }
    RealNumber sub_bound =
          (1.5 + 2 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon();
    // see p.39, mind the different definition of epsilon for error bound
    RealNumber C_relative_bound =
          (2.25 + 8 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon()
        * std::numeric_limits<RealNumber>::epsilon();
    absolute_bound = C_relative_bound * magnitude + sub_bound * std::abs(det);
    det += (t1[0] * t2[1] + t2[0] * t1[1]) - (t3[0] * t4[1] + t4[0] * t3[1]);
    if (Robustness == 2 || std::abs(det) >= absolute_bound)
    {
        return det; //C estimate
    }
    std::array<RealNumber, 8> D_left;
    int D_left_nz;
    {
        std::array<RealNumber, 2> t5_23 = two_product(t1[1], t2[0]);
        std::array<RealNumber, 2> t6_23 = two_product(t3[1], t4[0]);
        std::array<RealNumber, 4> tA_47 = two_two_expansion_diff(t5_23, t6_23);
        D_left_nz = fast_expansion_sum_zeroelim(tA_03, tA_47, D_left);
    }
    std::array<RealNumber, 8> D_right;
    int D_right_nz;
    {
        std::array<RealNumber, 2> t5_45 = two_product(t1[0], t2[1]);
        std::array<RealNumber, 2> t6_45 = two_product(t3[0], t4[1]);
        std::array<RealNumber, 4> tA_8_11 = two_two_expansion_diff(t5_45, t6_45);
        std::array<RealNumber, 2> t5_67 = two_product(t1[1], t2[1]);
        std::array<RealNumber, 2> t6_67 = two_product(t3[1], t4[1]);
        std::array<RealNumber, 4> tA_12_15 = two_two_expansion_diff(t5_67, t6_67);
        D_right_nz = fast_expansion_sum_zeroelim(tA_8_11, tA_12_15, D_right);
    }
    std::array<RealNumber, 16> D;
    int D_nz = fast_expansion_sum_zeroelim(D_left, D_right, D, D_left_nz, D_right_nz);
    // only return component of highest magnitude because we mostly care about the sign.
    return(D[D_nz - 1]);
}

// see page 38, Figure 21 for the calculations, notation follows the notation in the figure.
template
<
    typename RealNumber,
    std::size_t Robustness = 3
>
inline RealNumber orient2d(vec2d<RealNumber> const& p1,
                           vec2d<RealNumber> const& p2,
                           vec2d<RealNumber> const& p3)
{
    if(Robustness == 0)
    {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p1.y - p3.y) * (p2.x - p3.x);
    }
    std::array<RealNumber, 2> t1, t2, t3, t4;
    t1[0] = p1.x - p3.x;
    t2[0] = p2.y - p3.y;
    t3[0] = p1.y - p3.y;
    t4[0] = p2.x - p3.x;
    std::array<RealNumber, 2> t5_01, t6_01;
    t5_01[0] = t1[0] * t2[0];
    t6_01[0] = t3[0] * t4[0];
    RealNumber det = t5_01[0] - t6_01[0];
    RealNumber const magnitude = std::abs(t5_01[0]) + std::abs(t6_01[0]);

    // see p.39, mind the different definition of epsilon for error bound
    RealNumber const A_relative_bound =
          (1.5 + 4 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon();
    RealNumber absolute_bound = A_relative_bound * magnitude;
    if ( std::abs(det) >= absolute_bound )
    {
        return det; //A estimate
    }

    if ( (t5_01[0] > 0 && t6_01[0] <= 0) || (t5_01[0] < 0 && t6_01[0] >= 0) )
    {
        //if diagonal and antidiagonal have different sign, the sign of det is
        //obvious
        return det;
    }
    return orient2dtail<RealNumber, Robustness>(p1, p2, p3, t1, t2, t3, t4, t5_01, t6_01, magnitude);
}

// This method adaptively computes increasingly precise approximations of the following
// determinant using Laplace expansion along the last column.
// det A =
//      | p1_x - p4_x    p1_y - p4_y     ( p1_x - p4_x ) ^ 2 + ( p1_y - p4_y ) ^ 2 |
//      | p2_x - p4_x    p2_y - p4_y     ( p2_x - p4_x ) ^ 2 + ( p1_y - p4_y ) ^ 2 |
//      | p3_x - p4_x    p3_y - p4_y     ( p3_x - p4_x ) ^ 2 + ( p3_y - p4_y ) ^ 2 |
// = a_13 * C_13 + a_23 * C_23 + a_33 * C_33
// where a_ij is the i-j-entry and C_ij is the i_j Cofactor

template
<
    typename RealNumber,
    std::size_t Robustness = 2
>
RealNumber incircle(std::array<RealNumber, 2> const& p1,
                    std::array<RealNumber, 2> const& p2,
                    std::array<RealNumber, 2> const& p3,
                    std::array<RealNumber, 2> const& p4)
{
    RealNumber A_11 = p1[0] - p4[0];
    RealNumber A_21 = p2[0] - p4[0];
    RealNumber A_31 = p3[0] - p4[0];
    RealNumber A_12 = p1[1] - p4[1];
    RealNumber A_22 = p2[1] - p4[1];
    RealNumber A_32 = p3[1] - p4[1];

    std::array<RealNumber, 2> A_21_x_A_32,
                              A_31_x_A_22,
                              A_31_x_A_12,
                              A_11_x_A_32,
                              A_11_x_A_22,
                              A_21_x_A_12;
    A_21_x_A_32[0] = A_21 * A_32;
    A_31_x_A_22[0] = A_31 * A_22;
    RealNumber A_13 = A_11 * A_11 + A_12 * A_12;

    A_31_x_A_12[0] = A_31 * A_12;
    A_11_x_A_32[0] = A_11 * A_32;
    RealNumber A_23 = A_21 * A_21 + A_22 * A_22;

    A_11_x_A_22[0] = A_11 * A_22;
    A_21_x_A_12[0] = A_21 * A_12;
    RealNumber A_33 = A_31 * A_31 + A_32 * A_32;

    RealNumber det = A_13 * (A_21_x_A_32[0] - A_31_x_A_22[0])
      + A_23 * (A_31_x_A_12[0] - A_11_x_A_32[0])
      + A_33 * (A_11_x_A_22[0] - A_21_x_A_12[0]);
    if(Robustness == 0) return det;

    RealNumber magnitude =
          (std::abs(A_21_x_A_32[0]) + std::abs(A_31_x_A_22[0])) * A_13
        + (std::abs(A_31_x_A_12[0]) + std::abs(A_11_x_A_32[0])) * A_23
        + (std::abs(A_11_x_A_22[0]) + std::abs(A_21_x_A_12[0])) * A_33;
    RealNumber A_relative_bound =
          (5 + 24 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon();
    RealNumber absolute_bound = A_relative_bound * magnitude;
    if (std::abs(det) > absolute_bound)
    {
        return det;
    }
    // (p2_x - p4_x) * (p3_y - p4_y)
    A_21_x_A_32[1] = two_product_tail(A_21, A_32, A_21_x_A_32[0]);
    // (p3_x - p4_x) * (p2_y - p4_y)
    A_31_x_A_22[1] = two_product_tail(A_31, A_22, A_31_x_A_22[0]);
    // (bx - dx) * (cy - dy) - (cx - dx) * (by - dy)
    std::array<RealNumber, 4> C_13 = two_two_expansion_diff(A_21_x_A_32, A_31_x_A_22);
    std::array<RealNumber, 8> C_13_x_A11;
    // ( (bx - dx) * (cy - dy) - (cx - dx) * (by - dy) ) * ( ax - dx )
    int C_13_x_A11_nz = scale_expansion_zeroelim(C_13, A_11, C_13_x_A11);
    std::array<RealNumber, 16> C_13_x_A11_sq;
    // ( (bx - dx) * (cy - dy) - (cx - dx) * (by - dy) ) * ( ax - dx ) * (ax - dx)
    int C_13_x_A11_sq_nz = scale_expansion_zeroelim(C_13_x_A11,
                                                    A_11,
                                                    C_13_x_A11_sq,
                                                    C_13_x_A11_nz);

    std::array<RealNumber, 8> C_13_x_A12;
    // ( (bx - dx) * (cy - dy) - (cx - dx) * (by - dy) ) * ( ay - dy )
    int C_13_x_A12_nz = scale_expansion_zeroelim(C_13, A_12, C_13_x_A12);

    std::array<RealNumber, 16> C_13_x_A12_sq;
    // ( (bx - dx) * (cy - dy) - (cx - dx) * (by - dy) ) * ( ay - dy ) * ( ay - dy )
    int C_13_x_A12_sq_nz = scale_expansion_zeroelim(C_13_x_A12, A_12,
                                                    C_13_x_A12_sq,
                                                    C_13_x_A12_nz);

    std::array<RealNumber, 32> A_13_x_C13;
    //   ( (bx - dx) * (cy - dy) - (cx - dx) * (by - dy) )
    // * ( ( ay - dy ) * ( ay - dy ) + ( ax - dx ) * (ax - dx) )
    int A_13_x_C13_nz = fast_expansion_sum_zeroelim(C_13_x_A11_sq,
                                                    C_13_x_A12_sq,
                                                    A_13_x_C13,
                                                    C_13_x_A11_sq_nz,
                                                    C_13_x_A12_sq_nz);

    // (cx - dx) * (ay - dy)
    A_31_x_A_12[1] = two_product_tail(A_31, A_12, A_31_x_A_12[0]);
    // (ax - dx) * (cy - dy)
    A_11_x_A_32[1] = two_product_tail(A_11, A_32, A_11_x_A_32[0]);
    // (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy)
    std::array<RealNumber, 4> C_23 = two_two_expansion_diff(A_31_x_A_12,
                                                            A_11_x_A_32);
    std::array<RealNumber, 8> C_23_x_A_21;
    // ( (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy) ) * ( bx - dx )
    int C_23_x_A_21_nz = scale_expansion_zeroelim(C_23, A_21, C_23_x_A_21);
    std::array<RealNumber, 16> C_23_x_A_21_sq;
    // ( (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy) ) * ( bx - dx ) * ( bx - dx )
    int C_23_x_A_21_sq_nz = scale_expansion_zeroelim(C_23_x_A_21, A_21,
                                                     C_23_x_A_21_sq,
                                                     C_23_x_A_21_nz);
    std::array<RealNumber, 8>  C_23_x_A_22;
    // ( (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy) ) * ( by - dy )
    int C_23_x_A_22_nz = scale_expansion_zeroelim(C_23, A_22, C_23_x_A_22);
    std::array<RealNumber, 16> C_23_x_A_22_sq;
    // ( (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy) ) * ( by - dy ) * ( by - dy )
    int C_23_x_A_22_sq_nz = scale_expansion_zeroelim(C_23_x_A_22, A_22,
                                                     C_23_x_A_22_sq,
                                                     C_23_x_A_22_nz);
    std::array<RealNumber, 32> A_23_x_C_23;
    //   ( (cx - dx) * (ay - dy) - (ax - dx) * (cy - dy) )
    // * ( ( bx - dx ) * ( bx - dx ) + ( by - dy ) * ( by - dy ) )
    int A_23_x_C_23_nz = fast_expansion_sum_zeroelim(C_23_x_A_21_sq,
                                                     C_23_x_A_22_sq,
                                                     A_23_x_C_23,
                                                     C_23_x_A_21_sq_nz,
                                                     C_23_x_A_22_sq_nz);

    // (ax - dx) * (by - dy)
    A_11_x_A_22[1] = two_product_tail(A_11, A_22, A_11_x_A_22[0]);
    // (bx - dx) * (ay - dy)
    A_21_x_A_12[1] = two_product_tail(A_21, A_12, A_21_x_A_12[0]);
    // (ax - dx) * (by - dy) - (bx - dx) * (ay - dy)
    std::array<RealNumber, 4> C_33 = two_two_expansion_diff(A_11_x_A_22,
                                                            A_21_x_A_12);
    std::array<RealNumber, 8>  C_33_x_A31;
    // ( (ax - dx) * (by - dy) - (bx - dx) * (ay - dy) ) * ( cx - dx )
    int C_33_x_A31_nz = scale_expansion_zeroelim(C_33, A_31, C_33_x_A31);
    std::array<RealNumber, 16> C_33_x_A31_sq;
    // ( (ax - dx) * (by - dy) - (bx - dx) * (ay - dy) ) * ( cx - dx ) * ( cx - dx )
    int C_33_x_A31_sq_nz = scale_expansion_zeroelim(C_33_x_A31, A_31,
                                                    C_33_x_A31_sq,
                                                    C_33_x_A31_nz);
    std::array<RealNumber, 8>  C_33_x_A_32;
    // ( (ax - dx) * (by - dy) - (bx - dx) * (ay - dy) ) * ( cy - dy )
    int C_33_x_A_32_nz = scale_expansion_zeroelim(C_33, A_32, C_33_x_A_32);
    std::array<RealNumber, 16> C_33_x_A_32_sq;
    // ( (ax - dx) * (by - dy) - (bx - dx) * (ay - dy) ) * ( cy - dy ) * ( cy - dy )
    int C_33_x_A_32_sq_nz = scale_expansion_zeroelim(C_33_x_A_32, A_32,
                                                     C_33_x_A_32_sq,
                                                     C_33_x_A_32_nz);

    std::array<RealNumber, 32> A_33_x_C_33;
    int A_33_x_C_33_nz = fast_expansion_sum_zeroelim(C_33_x_A31_sq,
                                                     C_33_x_A_32_sq,
                                                     A_33_x_C_33,
                                                     C_33_x_A31_sq_nz,
                                                     C_33_x_A_32_sq_nz);
    std::array<RealNumber, 64> A_13_x_C13_p_A_13_x_C13;
    int A_13_x_C13_p_A_13_x_C13_nz = fast_expansion_sum_zeroelim(
            A_13_x_C13, A_23_x_C_23,
            A_13_x_C13_p_A_13_x_C13,
            A_13_x_C13_nz,
            A_23_x_C_23_nz);
    std::array<RealNumber, 96> det_expansion;
    int det_expansion_nz = fast_expansion_sum_zeroelim(
            A_13_x_C13_p_A_13_x_C13,
            A_33_x_C_33,
            det_expansion,
            A_13_x_C13_p_A_13_x_C13_nz,
            A_33_x_C_33_nz);

    det = std::accumulate(det_expansion.begin(),
                          det_expansion.begin() + det_expansion_nz,
                          static_cast<RealNumber>(0));
    if(Robustness == 1) return det;
    RealNumber B_relative_bound =
          (2 + 12 * std::numeric_limits<RealNumber>::epsilon())
        * std::numeric_limits<RealNumber>::epsilon();
    absolute_bound = B_relative_bound * magnitude;
    if (std::abs(det) >= absolute_bound)
    {
        return det;
    }
    RealNumber A_11tail = two_diff_tail(p1[0], p4[0], A_11);
    RealNumber A_12tail = two_diff_tail(p1[1], p4[1], A_12);
    RealNumber A_21tail = two_diff_tail(p2[0], p4[0], A_21);
    RealNumber A_22tail = two_diff_tail(p2[1], p4[1], A_22);
    RealNumber A_31tail = two_diff_tail(p3[0], p4[0], A_31);
    RealNumber A_32tail = two_diff_tail(p3[1], p4[1], A_32);
    if ((A_11tail == 0) && (A_21tail == 0) && (A_31tail == 0)
        && (A_12tail == 0) && (A_22tail == 0) && (A_32tail == 0))
    {
        return det;
    }
    //  RealNumber sub_bound =  (1.5 + 2.0 * std::numeric_limits<RealNumber>::epsilon())
    //    * std::numeric_limits<RealNumber>::epsilon();
    //  RealNumber C_relative_bound = (11.0 + 72.0 * std::numeric_limits<RealNumber>::epsilon())
    //    * std::numeric_limits<RealNumber>::epsilon()
    //    * std::numeric_limits<RealNumber>::epsilon();
    //absolute_bound = C_relative_bound * magnitude + sub_bound * std::abs(det);
    det += ((A_11 * A_11 + A_12 * A_12) * ((A_21 * A_32tail + A_32 * A_21tail)
        - (A_22 * A_31tail + A_31 * A_22tail))
    + 2 * (A_11 * A_11tail + A_12 * A_12tail) * (A_21 * A_32 - A_22 * A_31))
    + ((A_21 * A_21 + A_22 * A_22) * ((A_31 * A_12tail + A_12 * A_31tail)
        - (A_32 * A_11tail + A_11 * A_32tail))
    + 2 * (A_21 * A_21tail + A_22 * A_22tail) * (A_31 * A_12 - A_32 * A_11))
    + ((A_31 * A_31 + A_32 * A_32) * ((A_11 * A_22tail + A_22 * A_11tail)
        - (A_12 * A_21tail + A_21 * A_12tail))
    + 2 * (A_31 * A_31tail + A_32 * A_32tail) * (A_11 * A_22 - A_12 * A_21));
    //if (std::abs(det) >= absolute_bound)
    //{
    return det;
    //}
}

}} // namespace detail::precise_math

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_TRIANGULATION_STRATEGIES_CARTESIAN_DETAIL_PRECISE_MATH_HPP
