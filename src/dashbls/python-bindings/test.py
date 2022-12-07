# flake8: noqa: E501
import binascii
import time
from copy import deepcopy

from blspy import (
    AugSchemeMPL,
    BasicSchemeMPL,
    G1Element,
    G2Element,
    PopSchemeMPL,
    PrivateKey,
    Util,
)


def test_schemes():
    # fmt: off
    seed = bytes([
        0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6,
        220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22
    ])
    # fmt: on
    msg = bytes([100, 2, 254, 88, 90, 45, 23])
    msg2 = bytes([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
    sk = BasicSchemeMPL.key_gen(seed)
    pk = sk.get_g1()

    assert sk == PrivateKey.from_bytes(bytes(sk))
    assert pk == G1Element.from_bytes(bytes(pk))

    for Scheme in (BasicSchemeMPL, AugSchemeMPL, PopSchemeMPL):
        sig = Scheme.sign(sk, msg)
        assert sig == G2Element.from_bytes(bytes(sig))
        assert Scheme.verify(pk, msg, sig)

    seed = bytes([1]) + seed[1:]
    sk1 = BasicSchemeMPL.key_gen(seed)
    pk1 = sk1.get_g1()
    seed = bytes([2]) + seed[1:]
    sk2 = BasicSchemeMPL.key_gen(seed)
    pk2 = sk2.get_g1()

    for Scheme in (BasicSchemeMPL, AugSchemeMPL, PopSchemeMPL):
        # Aggregate same message
        agg_pk = pk1 + pk2
        if Scheme is AugSchemeMPL:
            sig1 = Scheme.sign(sk1, msg, agg_pk)
            sig2 = Scheme.sign(sk2, msg, agg_pk)
        else:
            sig1 = Scheme.sign(sk1, msg)
            sig2 = Scheme.sign(sk2, msg)
        agg_sig = Scheme.aggregate([sig1, sig2])

        assert Scheme.verify(agg_pk, msg, agg_sig)

        # Aggregate different message
        sig1 = Scheme.sign(sk1, msg)
        sig2 = Scheme.sign(sk2, msg2)
        agg_sig = Scheme.aggregate([sig1, sig2])
        assert Scheme.aggregate_verify([pk1, pk2], [msg, msg2], agg_sig)

        # Manual pairing calculation and verification
        if Scheme is AugSchemeMPL:
            # AugSchemeMPL requires prepending the public key to message
            aug_msg1 = bytes(pk1) + msg
            aug_msg2 = bytes(pk2) + msg2
        else:
            aug_msg1 = msg
            aug_msg2 = msg2
        pair1 = pk1.pair(Scheme.g2_from_message(aug_msg1))
        pair2 = pk2.pair(Scheme.g2_from_message(aug_msg2))
        pair = pair1 * pair2
        agg_sig_pair = G1Element.generator().pair(agg_sig)
        assert pair == agg_sig_pair

        # HD keys
        child = Scheme.derive_child_sk(sk1, 123)
        childU = Scheme.derive_child_sk_unhardened(sk1, 123)
        childUPk = Scheme.derive_child_pk_unhardened(pk1, 123)

        sig_child = Scheme.sign(child, msg)
        assert Scheme.verify(child.get_g1(), msg, sig_child)

        sigU_child = Scheme.sign(childU, msg)
        assert Scheme.verify(childUPk, msg, sigU_child)


def test_vectors_invalid():
    # Invalid inputs from https://github.com/algorand/bls_sigs_ref/blob/master/python-impl/serdesZ.py
    invalid_inputs_1 = [
        # infinity points: too short
        "c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        # infinity points: not all zeros
        "c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000",
        # bad tags
        "3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
        "7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
        "fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
        # wrong length for compresed point
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa",
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaaaa",
        # invalid x-coord
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
        # invalid elm of Fp --- equal to p (must be strictly less)
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
    ]
    invalid_inputs_2 = [
        # infinity points: too short
        "c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        # infinity points: not all zeros
        "c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "c00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000",
        # bad tags
        "3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        # wrong length for compressed point
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        # invalid x-coord
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaa7",
        # invalid elm of Fp --- equal to p (must be strictly less)
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
    ]

    for s in invalid_inputs_1:
        bytes_ = binascii.unhexlify(s)
        try:
            g1 = G1Element(bytes_)
            assert False, "Failed to disallow creation of G1 element."
        except Exception as e:
            pass

    for s in invalid_inputs_2:
        bytes_ = binascii.unhexlify(s)
        try:
            g2 = G2Element(bytes_)
            assert False, "Failed to disallow creation of G2 element."
        except Exception as e:
            pass


def test_vectors_valid():
    # The following code was used to generate these vectors
    """
    from py_ecc.bls import (
        G2Basic,
        G2MessageAugmentation as G2MA,
        G2ProofOfPossession as G2Pop,
    )

    secret1 = bytes([1] * 32)
    secret2 = bytes([x * 314159 % 256 for x in range(32)])
    sk1 = int.from_bytes(secret1, 'big')
    sk2 = int.from_bytes(secret2, 'big')
    msg = bytes([3, 1, 4, 1, 5, 9])
    pk1 = G2Basic.SkToPk(sk1)
    pk2 = G2Basic.SkToPk(sk2)

    for Scheme in (G2Basic, G2MA, G2Pop):
        sig1 = Scheme.Sign(sk1, msg)
        sig2 = Scheme.Sign(sk2, msg)
        sig_agg = Scheme.Aggregate([sig1, sig2])
        print(sig1)
        print(sig2)
        print(sig_agg)
    """

    ref_sig1Basic = b"\x96\xba4\xfa\xc3<\x7f\x12\x9d`*\x0b\xc8\xa3\xd4?\x9a\xbc\x01N\xce\xaa\xb75\x91F\xb4\xb1P\xe5{\x80\x86Es\x8f5g\x1e\x9e\x10\xe0\xd8b\xa3\x0c\xabp\x07N\xb5\x83\x1d\x13\xe6\xa5\xb1b\xd0\x1e\xeb\xe6\x87\xd0\x16J\xdb\xd0\xa8d7\n|\"*'h\xd7pM\xa2T\xf1\xbf\x18#f[\xc26\x1f\x9d\xd8\xc0\x0e\x99"
    ref_sig2Basic = b'\xa4\x02y\t2\x13\x0fvj\xf1\x1b\xa7\x16Sf\x83\xd8\xc4\xcf\xa5\x19G\xe4\xf9\x08\x1f\xed\xd6\x92\xd6\xdc\x0c\xac[\x90K\xee^\xa6\xe2Ui\xe3m{\xe4\xcaY\x06\x9a\x96\xe3K\x7fp\x07X\xb7\x16\xf9IJ\xaaY\xa9nt\xd1J;U*\x9ak\xc1)\xe7\x17\x19[\x9d`\x06\xfdm\\\xefGh\xc0"\xe0\xf71j\xbf'
    ref_sigABasic = b"\x98|\xfd;\xcdb(\x02\x87\x02t\x83\xf2\x9cU$^\xd81\xf5\x1d\xd6\xbd\x99\x9ao\xf1\xa1\xf1\xf1\xf0\xb6Gw\x8b\x01g5\x9cqPUX\xa7n\x15\x8ef\x18\x1e\xe5\x12Y\x05\xa6B$k\x01\xe7\xfa^\xe5=h\xa4\xfe\x9b\xfb)\xa8\xe2f\x01\xf0\xb9\xadW}\xdd\x18\x87js1|!n\xa6\x1fC\x04\x14\xecQ\xc5"
    ref_sig1Aug = b'\x81\x80\xf0,\xcbr\xe9"\xb1R\xfc\xed\xbe\x0e\x1d\x19R\x105Opp6X\xe8\xe0\x8c\xbe\xbf\x11\xd4\x97\x0e\xabj\xc3\xcc\xf7\x15\xf3\xfb\x87m\xf9\xa9yz\xbd\x0c\x1a\xf6\x1a\xae\xad\xc9,,\xfe\\\nV\xc1F\xcc\x8c?qQ\xa0s\xcf_\x16\xdf8$g$\xc4\xae\xd7?\xf3\x0e\xf5\xda\xa6\xaa\xca\xed\x1a&\xec\xaa3k'
    ref_sig2Aug = b'\x99\x11\x1e\xea\xfbA-\xa6\x1eL7\xd3\xe8\x06\xc6\xfdj\xc9\xf3\x87\x0eT\xda\x92"\xbaNIH"\xc5\xb7eg1\xfazdY4\xd0KU\x9e\x92a\xb8b\x01\xbb\xeeW\x05RP\xa4Y\xa2\xda\x10\xe5\x1f\x9c\x1aiA)\x7f\xfc]\x97\nUr6\xd0\xbd\xeb|\xf8\xff\x18\x80\x0b\x08c8q\xa0\xf0\xa7\xeaB\xf4t\x80'
    ref_sigAAug = b"\x8c]\x03\xf9\xda\xe7~\x19\xa5\x94Z\x06\xa2\x14\x83n\xdb\x8e\x03\xb8QR]\x84\xb9\xded@\xe6\x8f\xc0\xcas\x03\xee\xed9\r\x86<\x9bU\xa8\xcfmY\x14\n\x01\xb5\x88G\x88\x1e\xb5\xafgsMD\xb2UVF\xc6al9\xab\x88\xd2S)\x9a\xcc\x1e\xb1\xb1\x9d\xdb\x9b\xfc\xbev\xe2\x8a\xdd\xf6q\xd1\x16\xc0R\xbb\x18G"
    ref_sig1Pop = b"\x95P\xfbN\x7f~\x8c\xc4\xa9\x0b\xe8V\n\xb5\xa7\x98\xb0\xb20\x00\xb6\xa5J!\x17R\x02\x10\xf9\x86\xf3\xf2\x81\xb3v\xf2Y\xc0\xb7\x80b\xd1\xeb1\x92\xb3\xd9\xbb\x04\x9fY\xec\xc1\xb0:pI\xebf^\r\xf3d\x94\xaeL\xb5\xf1\x13l\xca\xee\xfc\x99X\xcb0\xc33==C\xf0qH\xc3\x86)\x9a{\x1b\xfc\r\xc5\xcf|"
    ref_sig2Pop = b"\xa6\x906\xbc\x11\xae^\xfc\xbfa\x80\xaf\xe3\x9a\xdd\xde~'s\x1e\xc4\x02W\xbf\xdc<7\xf1{\x8d\xf6\x83\x06\xa3N\xbd\x10\xe9\xe3*5%7P\xdf\\\x87\xc2\x14/\x82\x07\xe8\xd5eG\x12\xb4\xe5T\xf5\x85\xfbhF\xff8\x04\xe4)\xa9\xf8\xa1\xb4\xc5ku\xd0\x86\x9e\xd6u\x80\xd7\x89\x87\x0b\xab\xe2\xc7\xc8\xa9\xd5\x1e{*"
    ref_sigAPop = b"\xa4\xeat+\xcd\xc1U>\x9c\xa4\xe5`\xbe~^ln\xfajd\xdd\xdf\x9c\xa3\xbb(T#=\x85\xa6\xaa\xc1\xb7n\xc7\xd1\x03\xdbN3\x14\x8b\x82\xaf\x99#\xdb\x05\x93Jn\xce\x9aq\x01\xcd\x8a\x9dG\xce'\x97\x80V\xb0\xf5\x90\x00!\x81\x8cEi\x8a\xfd\xd6\xcf\x8ako\x7f\xee\x1f\x0bCqoU\xe4\x13\xd4\xb8z`9"

    secret1 = bytes([1] * 32)
    secret2 = bytes([x * 314159 % 256 for x in range(32)])
    sk1 = PrivateKey.from_bytes(secret1)
    sk2 = PrivateKey.from_bytes(secret2)

    msg = bytes([3, 1, 4, 1, 5, 9])
    sig1Basic = BasicSchemeMPL.sign(sk1, msg)
    sig2Basic = BasicSchemeMPL.sign(sk2, msg)
    sigABasic = BasicSchemeMPL.aggregate([sig1Basic, sig2Basic])
    sig1Aug = AugSchemeMPL.sign(sk1, msg)
    sig2Aug = AugSchemeMPL.sign(sk2, msg)
    sigAAug = AugSchemeMPL.aggregate([sig1Aug, sig2Aug])
    sig1Pop = PopSchemeMPL.sign(sk1, msg)
    sig2Pop = PopSchemeMPL.sign(sk2, msg)
    sigAPop = PopSchemeMPL.aggregate([sig1Pop, sig2Pop])

    assert bytes(sig1Basic) == ref_sig1Basic
    assert bytes(sig2Basic) == ref_sig2Basic
    assert bytes(sigABasic) == ref_sigABasic
    assert bytes(sig1Aug) == ref_sig1Aug
    assert bytes(sig2Aug) == ref_sig2Aug
    assert bytes(sigAAug) == ref_sigAAug
    assert bytes(sig1Pop) == ref_sig1Pop
    assert bytes(sig2Pop) == ref_sig2Pop
    assert bytes(sigAPop) == ref_sigAPop


def test_readme():
    seed: bytes = bytes(
        [
            0,
            50,
            6,
            244,
            24,
            199,
            1,
            25,
            52,
            88,
            192,
            19,
            18,
            12,
            89,
            6,
            220,
            18,
            102,
            58,
            209,
            82,
            12,
            62,
            89,
            110,
            182,
            9,
            44,
            20,
            254,
            22,
        ]
    )
    sk: PrivateKey = AugSchemeMPL.key_gen(seed)
    pk: G1Element = sk.get_g1()

    message: bytes = bytes([1, 2, 3, 4, 5])
    signature: G2Element = AugSchemeMPL.sign(sk, message)

    ok: bool = AugSchemeMPL.verify(pk, message, signature)
    assert ok

    sk_bytes: bytes = bytes(sk)  # 32 bytes
    pk_bytes: bytes = bytes(pk)  # 48 bytes
    signature_bytes: bytes = bytes(signature)  # 96 bytes

    print(sk_bytes.hex(), pk_bytes.hex(), signature_bytes.hex())

    sk = PrivateKey.from_bytes(sk_bytes)
    pk = G1Element.from_bytes(pk_bytes)
    signature = G2Element.from_bytes(signature_bytes)

    seed = bytes([1]) + seed[1:]
    sk1: PrivateKey = AugSchemeMPL.key_gen(seed)
    seed = bytes([2]) + seed[1:]
    sk2: PrivateKey = AugSchemeMPL.key_gen(seed)
    message2: bytes = bytes([1, 2, 3, 4, 5, 6, 7])

    pk1: G1Element = sk1.get_g1()
    sig1: G2Element = AugSchemeMPL.sign(sk1, message)

    pk2: G1Element = sk2.get_g1()
    sig2: G2Element = AugSchemeMPL.sign(sk2, message2)

    agg_sig: G2Element = AugSchemeMPL.aggregate([sig1, sig2])

    ok = AugSchemeMPL.aggregate_verify([pk1, pk2], [message, message2], agg_sig)
    assert ok

    seed = bytes([3]) + seed[1:]
    sk3: PrivateKey = AugSchemeMPL.key_gen(seed)
    pk3: G1Element = sk3.get_g1()
    message3: bytes = bytes([100, 2, 254, 88, 90, 45, 23])
    sig3: G2Element = AugSchemeMPL.sign(sk3, message3)

    agg_sig_final: G2Element = AugSchemeMPL.aggregate([agg_sig, sig3])
    ok = AugSchemeMPL.aggregate_verify(
        [pk1, pk2, pk3], [message, message2, message3], agg_sig_final
    )
    assert ok

    pop_sig1: G2Element = PopSchemeMPL.sign(sk1, message)
    pop_sig2: G2Element = PopSchemeMPL.sign(sk2, message)
    pop_sig3: G2Element = PopSchemeMPL.sign(sk3, message)
    pop1: G2Element = PopSchemeMPL.pop_prove(sk1)
    pop2: G2Element = PopSchemeMPL.pop_prove(sk2)
    pop3: G2Element = PopSchemeMPL.pop_prove(sk3)

    ok = PopSchemeMPL.pop_verify(pk1, pop1)
    assert ok
    ok = PopSchemeMPL.pop_verify(pk2, pop2)
    assert ok
    ok = PopSchemeMPL.pop_verify(pk3, pop3)
    assert ok

    pop_sig_agg: G2Element = PopSchemeMPL.aggregate([pop_sig1, pop_sig2, pop_sig3])

    ok = PopSchemeMPL.fast_aggregate_verify([pk1, pk2, pk3], message, pop_sig_agg)
    assert ok

    pop_agg_pk: G1Element = pk1 + pk2 + pk3
    ok = PopSchemeMPL.verify(pop_agg_pk, message, pop_sig_agg)
    assert ok

    pop_agg_sk: PrivateKey = PrivateKey.aggregate([sk1, sk2, sk3])
    ok = PopSchemeMPL.sign(pop_agg_sk, message) == pop_sig_agg
    assert ok

    master_sk: PrivateKey = AugSchemeMPL.key_gen(seed)
    child: PrivateKey = AugSchemeMPL.derive_child_sk(master_sk, 152)
    grandchild: PrivateKey = AugSchemeMPL.derive_child_sk(child, 952)

    master_pk: G1Element = master_sk.get_g1()
    child_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(master_sk, 22)
    grandchild_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(child_u, 0)

    child_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(master_pk, 22)
    grandchild_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(child_u_pk, 0)

    ok = grandchild_u_pk == grandchild_u.get_g1()
    assert ok


def test_aggregate_verify_zero_items():
    assert AugSchemeMPL.aggregate_verify([], [], G2Element())


def test_invalid_points():
    sk1 = BasicSchemeMPL.key_gen(b"1" *32)
    good_point = sk1.get_g1()
    good_point_bytes = bytes(good_point)
    start = time.time()
    for i in range(2000):
        gp1 = G1Element.from_bytes(good_point_bytes)
    print(f"from_bytes avg: {(time.time() - start) }")

    start = time.time()
    for i in range(2000):
        gp2 = G1Element.from_bytes_unchecked(good_point_bytes)
    print(f"from_bytes_unchecked avg: {(time.time() - start) }")
    assert gp1 == gp2

    bad_point_hex: str = "8d5d0fb73b9c92df4eab4216e48c3e358578b4cc30f82c268bd6fef3bd34b558628daf1afef798d4c3b0fcd8b28c8973";
    try:
        G1Element.from_bytes(bytes.fromhex(bad_point_hex))
        assert False
    except ValueError:
        pass

    p: G1Element = G1Element.from_bytes_unchecked(bytes.fromhex(bad_point_hex))

    bad_g2_point_hex = "8f2886c94eaeac335c8414cbf14c16681b225380cfee3293becc4531d5b415984b4ea4050d9ecda11fbc21c60627e9d212dfcb17d2b5ae399aa3fbcb099e05baa496b852ad976fb633cc6766b02fca4da549dc063908463b2906ad64e8b310ad"

    try:
        G2Element.from_bytes(bytes.fromhex(bad_g2_point_hex))
        assert False
    except ValueError:
        pass



test_schemes()
test_vectors_invalid()
test_vectors_valid()
test_readme()
test_aggregate_verify_zero_items()
test_invalid_points()

print("\nAll tests passed.")

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
