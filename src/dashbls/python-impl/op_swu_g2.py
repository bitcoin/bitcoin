#!/usr/bin/python
#
# pure Python implementation of optimized simplified SWU map to BLS12-381 G2
# https://github.com/algorand/bls_sigs_ref
#
# This software is (C) 2019 Algorand, Inc.
#
# Licensed under the MIT license (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://opensource.org/licenses/MIT

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from bls12381 import h_eff, q
from ec import JacobianPoint, default_ec_twist, eval_iso
from fields import Fq, Fq2, roots_of_unity
from hash_to_field import Hp2


def sgn0(x: Fq2) -> int:
    # https://tools.ietf.org/html/draft-irtf-cfrg-hash-to-curve-07#section-4.1

    sign_0: int = x[0].value % 2
    zero_0: bool = x[0] == 0
    sign_1: int = x[1].value % 2
    return sign_0 or (zero_0 and sign_1)


# distinguished non-square in Fp2 for SWU map
xi_2 = Fq2(q, -2, -1)

# 3-isogenous curve parameters
Ell2p_a = Fq2(q, 0, 240)
Ell2p_b = Fq2(q, 1012, 1012)


# eta values, used for computing sqrt(g(X1(t)))
# For details on how to compute, see ../sage-impl/opt_sswu_g2.sage
ev1 = 0x699BE3B8C6870965E5BF892AD5D2CC7B0E85A117402DFD83B7F4A947E02D978498255A2AAEC0AC627B5AFBDF1BF1C90
ev2 = 0x8157CD83046453F5DD0972B6E3949E4288020B5B8A9CC99CA07E27089A2CE2436D965026ADAD3EF7BABA37F2183E9B5
ev3 = 0xAB1C2FFDD6C253CA155231EB3E71BA044FD562F6F72BC5BAD5EC46A0B7A3B0247CF08CE6C6317F40EDBC653A72DEE17
ev4 = 0xAA404866706722864480885D68AD0CCAC1967C7544B447873CC37E0181271E006DF72162A3D3E0287BF597FBF7F8FC1
etas = (Fq2(q, ev1, ev2), Fq2(q, q - ev2, ev1), Fq2(q, ev3, ev4), Fq2(q, q - ev4, ev3))
del ev1, ev2, ev3, ev4


#
# Simplified SWU map, optimized and adapted to Ell2'
#
# This function maps an element of Fp^2 to the curve Ell2', 3-isogenous to Ell2.
def osswu2_help(t):
    assert isinstance(t, Fq2)

    # first, compute X0(t), detecting and handling exceptional case
    num_den_common = xi_2 ** 2 * t ** 4 + xi_2 * t ** 2
    x0_num = Ell2p_b * (num_den_common + Fq(q, 1))
    x0_den = -Ell2p_a * num_den_common
    x0_den = Ell2p_a * xi_2 if x0_den == 0 else x0_den

    # compute num and den of g(X0(t))
    gx0_den = pow(x0_den, 3)
    gx0_num = Ell2p_b * gx0_den
    gx0_num += Ell2p_a * x0_num * pow(x0_den, 2)
    gx0_num += pow(x0_num, 3)

    # try taking sqrt of g(X0(t))
    # this uses the trick for combining division and sqrt from Section 5 of
    # Bernstein, Duif, Lange, Schwabe, and Yang, "High-speed high-security signatures."
    # J Crypt Eng 2(2):77--89, Sept. 2012. http://ed25519.cr.yp.to/ed25519-20110926.pdf
    tmp1 = pow(gx0_den, 7)  # v^7
    tmp2 = gx0_num * tmp1  # u v^7
    tmp1 = tmp1 * tmp2 * gx0_den  # u v^15
    sqrt_candidate = tmp2 * pow(tmp1, (q ** 2 - 9) // 16)

    # check if g(X0(t)) is square and return the sqrt if so
    for root in roots_of_unity:
        y0 = sqrt_candidate * root
        if y0 ** 2 * gx0_den == gx0_num:
            # found sqrt(g(X0(t))). force sign of y to equal sign of t
            if sgn0(y0) != sgn0(t):
                y0 = -y0
            assert sgn0(y0) == sgn0(t)
            return JacobianPoint(
                x0_num * x0_den, y0 * pow(x0_den, 3), x0_den, False, default_ec_twist
            )

    # if we've gotten here, then g(X0(t)) is not square. convert srqt_candidate to sqrt(g(X1(t)))
    (x1_num, x1_den) = (xi_2 * t ** 2 * x0_num, x0_den)
    (gx1_num, gx1_den) = (xi_2 ** 3 * t ** 6 * gx0_num, gx0_den)
    sqrt_candidate *= t ** 3
    for eta in etas:
        y1 = eta * sqrt_candidate
        if y1 ** 2 * gx1_den == gx1_num:
            # found sqrt(g(X1(t))). force sign of y to equal sign of t
            if sgn0(y1) != sgn0(t):
                y1 = -y1
            assert sgn0(y1) == sgn0(t)
            return JacobianPoint(
                x1_num * x1_den, y1 * pow(x1_den, 3), x1_den, False, default_ec_twist
            )

    # if we got here, something is wrong
    raise RuntimeError("osswu2_help failed for unknown reasons")


#
# 3-Isogeny from Ell2' to Ell2
#
# coefficients for the 3-isogeny map from Ell2' to Ell2
xnum = (
    Fq2(
        q,
        0x5C759507E8E333EBB5B7A9A47D7ED8532C52D39FD3A042A88B58423C50AE15D5C2638E343D9C71C6238AAAAAAAA97D6,
        0x5C759507E8E333EBB5B7A9A47D7ED8532C52D39FD3A042A88B58423C50AE15D5C2638E343D9C71C6238AAAAAAAA97D6,
    ),
    Fq2(
        q,
        0x0,
        0x11560BF17BAA99BC32126FCED787C88F984F87ADF7AE0C7F9A208C6B4F20A4181472AAA9CB8D555526A9FFFFFFFFC71A,
    ),
    Fq2(
        q,
        0x11560BF17BAA99BC32126FCED787C88F984F87ADF7AE0C7F9A208C6B4F20A4181472AAA9CB8D555526A9FFFFFFFFC71E,
        0x8AB05F8BDD54CDE190937E76BC3E447CC27C3D6FBD7063FCD104635A790520C0A395554E5C6AAAA9354FFFFFFFFE38D,
    ),
    Fq2(
        q,
        0x171D6541FA38CCFAED6DEA691F5FB614CB14B4E7F4E810AA22D6108F142B85757098E38D0F671C7188E2AAAAAAAA5ED1,
        0x0,
    ),
)
xden = (
    Fq2(
        q,
        0x0,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAA63,
    ),
    Fq2(
        q,
        0xC,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAA9F,
    ),
    Fq2(q, 0x1, 0x0),
)
ynum = (
    Fq2(
        q,
        0x1530477C7AB4113B59A4C18B076D11930F7DA5D4A07F649BF54439D87D27E500FC8C25EBF8C92F6812CFC71C71C6D706,
        0x1530477C7AB4113B59A4C18B076D11930F7DA5D4A07F649BF54439D87D27E500FC8C25EBF8C92F6812CFC71C71C6D706,
    ),
    Fq2(
        q,
        0x0,
        0x5C759507E8E333EBB5B7A9A47D7ED8532C52D39FD3A042A88B58423C50AE15D5C2638E343D9C71C6238AAAAAAAA97BE,
    ),
    Fq2(
        q,
        0x11560BF17BAA99BC32126FCED787C88F984F87ADF7AE0C7F9A208C6B4F20A4181472AAA9CB8D555526A9FFFFFFFFC71C,
        0x8AB05F8BDD54CDE190937E76BC3E447CC27C3D6FBD7063FCD104635A790520C0A395554E5C6AAAA9354FFFFFFFFE38F,
    ),
    Fq2(
        q,
        0x124C9AD43B6CF79BFBF7043DE3811AD0761B0F37A1E26286B0E977C69AA274524E79097A56DC4BD9E1B371C71C718B10,
        0x0,
    ),
)
yden = (
    Fq2(
        q,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFA8FB,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFA8FB,
    ),
    Fq2(
        q,
        0x0,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFA9D3,
    ),
    Fq2(
        q,
        0x12,
        0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAA99,
    ),
    Fq2(q, 0x1, 0x0),
)


# compute 3-isogeny map from Ell2' to Ell2
def iso3(P):
    return eval_iso(P, (xnum, xden, ynum, yden), default_ec_twist)


#
# map from Fq2 element(s) to point in G2 subgroup of Ell2
#
def opt_swu2_map(t: Fq2, t2: Fq2 = None) -> JacobianPoint:
    Pp = iso3(osswu2_help(t))
    if t2 is not None:
        Pp2 = iso3(osswu2_help(t2))
        Pp = Pp + Pp2
    return Pp * h_eff


#
# map from bytes() to point in G2 subgroup of Ell2
#
def g2_map(alpha: bytes, dst=None):
    return opt_swu2_map(*(Fq2(q, *hh) for hh in Hp2(alpha, 2, dst)))
