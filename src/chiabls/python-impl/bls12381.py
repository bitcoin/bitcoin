# flake8: noqa
from fields import Fq, Fq2

# BLS parameter used to generate the other parameters
# Spec is found here: https://github.com/zkcrypto/pairing/tree/master/src/bls12_381
x = -0xd201000000010000

# 381 bit prime
# Also see fields:bls12381_q
q = 0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab

# a,b and a2, b2, define the elliptic curve and twisted curve.
# y^2 = x^3 + 4
# y^2 = x^3 + 4(u + 1)
a = Fq(q, 0)
b = Fq(q, 4)
a_twist = Fq2(q, 0, 0)
b_twist = Fq2(q, 4, 4)

# The generators for g1 and g2
gx = Fq(q, 0x17F1D3A73197D7942695638C4FA9AC0FC3688C4F9774B905A14E3A3F171BAC586C55E83FF97A1AEFFB3AF00ADB22C6BB)
gy = Fq(q, 0x08B3F481E3AAA0F1A09E30ED741D8AE4FCF5E095D5D00AF600DB18CB2C04B3EDD03CC744A2888AE40CAA232946C5E7E1)

g2x = Fq2(q, 352701069587466618187139116011060144890029952792775240219908644239793785735715026873347600343865175952761926303160, 3059144344244213709971259814753781636986470325476647558659373206291635324768958432433509563104347017837885763365758)
g2y = Fq2(q, 1985150602287291935568054521177171638300868978215655730859378665066344726373823718423869104263333984641494340347905, 927553665492332455747201965776037880757740193453592970025027978793976877002675564980949289727957565575433344219582)

# The order of all three groups (g1, g2, and gt). Note, the elliptic curve E_twist
# actually has more valid points than this. This is relevant when hashing onto the
# curve, where we use a point that is not in g2, and map it into g2.
n = 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001

# Cofactor used to generate r torsion points
h = 0x396C8C005555E1568C00AAAB0000AAAB
h_twist = 0x5d543a95414e7f1091d50792876a202cd91de4547085abaa68a205b2e5a7ddfa628f1cb4d9e82ef21537e293a6691ae1616ec6e786f0c70cf1c38e31c7238e5

# Embedding degree
k = 12

# sqrt(-3) mod q
sqrt_n3 = 1586958781458431025242759403266842894121773480562120986020912974854563298150952611241517463240701

# (sqrt(-3) - 1) / 2 mod q
sqrt_n3m1o2 = 793479390729215512621379701633421447060886740281060493010456487427281649075476305620758731620350

# This is the normal elliptic curve. G1 points are on here.
def parameters():
    return (q, a, b, gx, gy, g2x, g2y, n, h, x, k, sqrt_n3, sqrt_n3m1o2)

# This is the sextic twist used to send elements of G2 from
# coordinates in Fq12 to coordinates in Fq2. It's isomorphic
# to the above elliptic curve. See Page 63 of Costello.
def parameters_twist():
    return (q, a_twist, b_twist, gx, gy, g2x, g2y, n, h_twist, x, k, sqrt_n3, sqrt_n3m1o2)


"""
Copyright 2018 Chia Network Inc

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
