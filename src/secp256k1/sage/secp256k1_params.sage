"""Prime order of finite field underlying secp256k1 (2^256 - 2^32 - 977)"""
P = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F

"""Finite field underlying secp256k1"""
F = FiniteField(P)

"""Elliptic curve secp256k1: y^2 = x^3 + 7"""
C = EllipticCurve([F(0), F(7)])

"""Base point of secp256k1"""
G = C.lift_x(0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798)
if int(G[1]) & 1:
    # G.y is even
    G = -G

"""Prime order of secp256k1"""
N = C.order()

"""Finite field of scalars of secp256k1"""
Z = FiniteField(N)

""" Beta value of secp256k1 non-trivial endomorphism: lambda * (x, y) = (beta * x, y)"""
BETA = F(2)^((P-1)/3)

""" Lambda value of secp256k1 non-trivial endomorphism: lambda * (x, y) = (beta * x, y)"""
LAMBDA = Z(3)^((N-1)/3)

assert is_prime(P)
assert is_prime(N)

assert BETA != F(1)
assert BETA^3 == F(1)
assert BETA^2 + BETA + 1 == 0

assert LAMBDA != Z(1)
assert LAMBDA^3 == Z(1)
assert LAMBDA^2 + LAMBDA + 1 == 0

assert Integer(LAMBDA)*G == C(BETA*G[0], G[1])
