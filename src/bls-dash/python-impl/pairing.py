from collections import namedtuple
from typing import List

import bls12381
from ec import AffinePoint, JacobianPoint, untwist
from fields import Fq, Fq12

# Struct for elliptic curve parameters
EC = namedtuple("EC", "q a b gx gy g2x g2y n h x k sqrt_n3 sqrt_n3m1o2")

# use secp256k1 as default
default_ec = EC(*bls12381.parameters())
default_ec_twist = EC(*bls12381.parameters())


def int_to_bits(i: int) -> List[int]:
    if i < 1:
        return [0]
    bits = []
    while i != 0:
        bits.append(i % 2)
        i = i // 2
    return list(reversed(bits))


def double_line_eval(R: AffinePoint, P: AffinePoint, ec=default_ec):
    """
    Creates an equation for a line tangent to R,
    and evaluates this at the point P. f(x) = y - sv - v.
    f(P).
    """
    R12 = untwist(R)

    slope = (Fq(ec.q, 3) * (R12.x ** 2) + ec.a) / (Fq(ec.q, 2) * R12.y)
    v = R12.y - slope * R12.x

    return P.y - P.x * slope - v


def add_line_eval(R: AffinePoint, Q: AffinePoint, P: AffinePoint, ec=default_ec) -> Fq:
    """
    Creates an equation for a line between R and Q,
    and evaluates this at the point P. f(x) = y - sv - v.
    f(P).
    """
    R12 = untwist(R)
    Q12 = untwist(Q)

    # This is the case of a vertical line, where the denominator
    # will be 0.
    if R12 == Q12.negate():
        return P.x - R12.x

    slope = (Q12.y - R12.y) / (Q12.x - R12.x)
    v = (Q12.y * R12.x - R12.y * Q12.x) / (R12.x - Q12.x)

    return P.y - P.x * slope - v


def miller_loop(T: int, P: AffinePoint, Q: AffinePoint, ec=default_ec) -> Fq12:
    """
    Performs a double and add algorithm for the ate pairing. This algorithm
    is taken from Craig Costello's "Pairing for Beginners".
    """
    T_bits = int_to_bits(T)
    R = Q
    f = Fq12.one(ec.q)  # f is an element of Fq12
    for i in range(1, len(T_bits)):
        # Compute sloped line lrr
        lrr = double_line_eval(R, P, ec)
        f = f * f * lrr

        R = Fq(ec.q, 2) * R
        if T_bits[i] == 1:
            # Compute sloped line lrq
            lrq = add_line_eval(R, Q, P, ec)
            f = f * lrq

            R = R + Q
    return f


def final_exponentiation(element: Fq12, ec=default_ec) -> Fq12:
    """
    Performs a final exponentiation to map the result of the miller
    loop to a unique element of Fq12.
    """
    if ec.k == 12:
        ans = element ** ((pow(ec.q, 4) - pow(ec.q, 2) + 1) // ec.n)
        ans = ans.qi_power(2) * ans
        ans = ans.qi_power(6) / ans
        return ans
    else:
        return element ** ((pow(ec.q, ec.k) - 1) // ec.n)


def ate_pairing(P: JacobianPoint, Q: JacobianPoint, ec=default_ec) -> Fq12:
    """
    Performs one ate pairing.
    """
    t = default_ec.x + 1
    T = abs(t - 1)
    element = miller_loop(T, P.to_affine(), Q.to_affine(), ec)
    return final_exponentiation(element, ec)


def ate_pairing_multi(
    Ps: List[JacobianPoint], Qs: List[JacobianPoint], ec=default_ec
) -> Fq12:
    """
    Computes multiple pairings at once. This is more efficient,
    since we can multiply all the results of the miller loops,
    and perform just one final exponentiation.
    """
    t = default_ec.x + 1
    T = abs(t - 1)
    prod = Fq12.one(ec.q)
    for i in range(len(Qs)):
        prod *= miller_loop(T, Ps[i].to_affine(), Qs[i].to_affine(), ec)
    return final_exponentiation(prod, ec)


"""
Copyright 2020 Chia Network Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
