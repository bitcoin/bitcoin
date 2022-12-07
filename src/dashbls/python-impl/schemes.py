from typing import List

from ec import G1Generator, JacobianPoint, default_ec
from fields import Fq12
from hd_keys import (derive_child_g1_unhardened, derive_child_sk,
                     derive_child_sk_unhardened, key_gen)
from op_swu_g2 import g2_map
from pairing import ate_pairing_multi
from private_key import PrivateKey

basic_scheme_dst = b"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_"
aug_scheme_dst = b"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_AUG_"
pop_scheme_dst = b"BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_"
pop_scheme_pop_dst = b"BLS_POP_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_"


def core_sign_mpl(sk: PrivateKey, message: bytes, dst: bytes) -> JacobianPoint:
    return sk.value * g2_map(message, dst)


def core_verify_mpl(
    pk: JacobianPoint, message: bytes, signature: JacobianPoint, dst: bytes
) -> bool:
    try:
        signature.check_valid()
        pk.check_valid()
    except AssertionError:
        return False
    q = g2_map(message, dst)
    one = Fq12.one(default_ec.q)
    pairing_result = ate_pairing_multi([pk, G1Generator().negate()], [q, signature])
    return pairing_result == one


def core_aggregate_mpl(signatures: List[JacobianPoint]) -> JacobianPoint:
    if len(signatures) < 1:
        raise ValueError("Must aggregate at least 1 signature")
    aggregate = signatures[0]
    aggregate.check_valid()
    for signature in signatures[1:]:
        signature.check_valid()
        aggregate += signature
    return aggregate


def core_aggregate_verify(
    pks: List[JacobianPoint], ms: List[bytes], signature: JacobianPoint, dst: bytes
) -> bool:
    if len(pks) != len(ms) or len(pks) < 1:
        return False
    try:
        signature.check_valid()
        qs = [signature]
        ps = [G1Generator().negate()]
        for i in range(len(pks)):
            pks[i].check_valid()
            qs.append(g2_map(ms[i], dst))
            ps.append(pks[i])
        return Fq12.one(default_ec.q) == ate_pairing_multi(ps, qs)

    except AssertionError:
        return False


class BasicSchemeMPL:
    @staticmethod
    def key_gen(seed: bytes) -> PrivateKey:
        return key_gen(seed)

    @staticmethod
    def sign(sk: PrivateKey, message: bytes) -> JacobianPoint:
        return core_sign_mpl(sk, message, basic_scheme_dst)

    @staticmethod
    def verify(pk: JacobianPoint, message: bytes, signature: JacobianPoint) -> bool:
        return core_verify_mpl(pk, message, signature, basic_scheme_dst)

    @staticmethod
    def aggregate(signatures: List[JacobianPoint]) -> JacobianPoint:
        return core_aggregate_mpl(signatures)

    @staticmethod
    def aggregate_verify(
        pks: List[JacobianPoint], ms: List[bytes], signature: JacobianPoint
    ) -> bool:
        if len(pks) != len(ms) or len(pks) < 1:
            return False
        if len(set(ms)) != len(ms):
            # Disallow repeated messages
            return False
        return core_aggregate_verify(pks, ms, signature, basic_scheme_dst)

    @staticmethod
    def derive_child_sk(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk(sk, index)

    @staticmethod
    def derive_child_sk_unhardened(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk_unhardened(sk, index)

    @staticmethod
    def derive_child_pk_unhardened(pk: JacobianPoint, index: int) -> JacobianPoint:
        return derive_child_g1_unhardened(pk, index)


class AugSchemeMPL:
    @staticmethod
    def key_gen(seed: bytes) -> PrivateKey:
        return key_gen(seed)

    @staticmethod
    def sign(sk: PrivateKey, message: bytes) -> JacobianPoint:
        pk = sk.get_g1()
        return core_sign_mpl(sk, bytes(pk) + message, aug_scheme_dst)

    @staticmethod
    def verify(pk: JacobianPoint, message: bytes, signature: JacobianPoint) -> bool:
        return core_verify_mpl(pk, bytes(pk) + message, signature, aug_scheme_dst)

    @staticmethod
    def aggregate(signatures: List[JacobianPoint]) -> JacobianPoint:
        return core_aggregate_mpl(signatures)

    @staticmethod
    def aggregate_verify(
        pks: List[JacobianPoint], ms: List[bytes], signature: JacobianPoint
    ) -> bool:
        if len(pks) != len(ms) or len(pks) < 1:
            return False
        m_primes = [bytes(pks[i]) + ms[i] for i in range(len(pks))]
        return core_aggregate_verify(pks, m_primes, signature, aug_scheme_dst)

    @staticmethod
    def derive_child_sk(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk(sk, index)

    @staticmethod
    def derive_child_sk_unhardened(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk_unhardened(sk, index)

    @staticmethod
    def derive_child_pk_unhardened(pk: JacobianPoint, index: int) -> JacobianPoint:
        return derive_child_g1_unhardened(pk, index)


class PopSchemeMPL:
    @staticmethod
    def key_gen(seed: bytes) -> PrivateKey:
        return key_gen(seed)

    @staticmethod
    def sign(sk: PrivateKey, message: bytes) -> JacobianPoint:
        return core_sign_mpl(sk, message, pop_scheme_dst)

    @staticmethod
    def verify(pk: JacobianPoint, message: bytes, signature: JacobianPoint) -> bool:
        return core_verify_mpl(pk, message, signature, pop_scheme_dst)

    @staticmethod
    def aggregate(signatures: List[JacobianPoint]) -> JacobianPoint:
        return core_aggregate_mpl(signatures)

    @staticmethod
    def aggregate_verify(
        pks: List[JacobianPoint], ms: List[bytes], signature: JacobianPoint
    ) -> bool:
        if len(pks) != len(ms) or len(pks) < 1:
            return False
        return core_aggregate_verify(pks, ms, signature, pop_scheme_dst)

    @staticmethod
    def pop_prove(sk: PrivateKey) -> JacobianPoint:
        pk: JacobianPoint = sk.get_g1()
        return sk.value * g2_map(bytes(pk), pop_scheme_pop_dst)

    @staticmethod
    def pop_verify(pk: JacobianPoint, proof: JacobianPoint) -> bool:
        try:
            proof.check_valid()
            pk.check_valid()
            q = g2_map(bytes(pk), pop_scheme_pop_dst)
            one = Fq12.one(default_ec.q)
            pairing_result = ate_pairing_multi([pk, G1Generator().negate()], [q, proof])
            return pairing_result == one
        except AssertionError:
            return False

    @staticmethod
    def fast_aggregate_verify(
        pks: List[JacobianPoint], message: bytes, signature: JacobianPoint
    ) -> bool:
        if len(pks) < 1:
            return False
        aggregate: JacobianPoint = pks[0]
        for pk in pks[1:]:
            aggregate += pk
        return core_verify_mpl(aggregate, message, signature, pop_scheme_dst)

    @staticmethod
    def derive_child_sk(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk(sk, index)

    @staticmethod
    def derive_child_sk_unhardened(sk: PrivateKey, index: int) -> PrivateKey:
        return derive_child_sk_unhardened(sk, index)

    @staticmethod
    def derive_child_pk_unhardened(pk: JacobianPoint, index: int) -> JacobianPoint:
        return derive_child_g1_unhardened(pk, index)
