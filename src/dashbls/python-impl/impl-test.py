import hashlib
from copy import deepcopy
from secrets import token_bytes

from ec import (G1FromBytes, G1Generator, G1Infinity, G2FromBytes, G2Generator,
                G2Infinity, JacobianPoint, default_ec, default_ec_twist,
                sign_Fq2, twist, untwist, y_for_x)
from fields import Fq, Fq2, Fq6, Fq12
from hash_to_field import expand_message_xmd
from hkdf import expand, extract
from op_swu_g2 import g2_map
from pairing import ate_pairing
from private_key import PrivateKey
from schemes import AugSchemeMPL, BasicSchemeMPL, PopSchemeMPL

G1Element = JacobianPoint
G2Element = JacobianPoint


def test_hkdf():
    def test_one_case(
        ikm_hex, salt_hex, info_hex, prk_expected_hex, okm_expected_hex, L
    ):
        prk = extract(bytes.fromhex(salt_hex), bytes.fromhex(ikm_hex))
        okm = expand(L, prk, bytes.fromhex(info_hex))
        assert len(bytes.fromhex(prk_expected_hex)) == 32
        assert L == len(bytes.fromhex(okm_expected_hex))
        assert prk == bytes.fromhex(prk_expected_hex)
        assert okm == bytes.fromhex(okm_expected_hex)

    test_case_1 = (
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
        "000102030405060708090a0b0c",
        "f0f1f2f3f4f5f6f7f8f9",
        "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
        42,
    )
    test_case_2 = (
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f"
        "303132333435363738393a3b3c3d3e3f"
        "404142434445464748494a4b4c4d4e4f",
        "606162636465666768696a6b6c6d6e6f"
        "707172737475767778797a7b7c7d7e7f"
        "808182838485868788898a8b8c8d8e8f"
        "909192939495969798999a9b9c9d9e9f"
        "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
        "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
        "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
        "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
        "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244",
        "b11e398dc80327a1c8e7f78c596a4934"
        "4f012eda2d4efad8a050cc4c19afa97c"
        "59045a99cac7827271cb41c65e590e09"
        "da3275600c2f09b8367793a9aca3db71"
        "cc30c58179ec3e87c14c01d5c1f3434f"
        "1d87",
        82,
    )
    test_case_3 = (
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
        "",
        "",
        "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04",
        "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8",
        42,
    )
    test_case_4 = (
        "8704f9ac024139fe62511375cf9bc534c0507dcf00c41603ac935cd5943ce0b4b88599390de14e743ca2f56a73a04eae13aa3f3b969b39d8701e0d69a6f8d42f",
        "53d8e19b",
        "",
        "eb01c9cd916653df76ffa61b6ab8a74e254ebfd9bfc43e624cc12a72b0373dee",
        "8faabea85fc0c64e7ca86217cdc6dcdc88551c3244d56719e630a3521063082c46455c2fd5483811f9520a748f0099c1dfcfa52c54e1c22b5cdf70efb0f3c676",
        64,
    )

    test_one_case(*test_case_1)
    test_one_case(*test_case_2)
    test_one_case(*test_case_3)
    test_one_case(*test_case_4)


def test_eip2333():
    def test_one_case(seed_hex, master_sk_hex, child_sk_hex, child_index):
        master = BasicSchemeMPL.key_gen(bytes.fromhex(seed_hex))
        child = BasicSchemeMPL.derive_child_sk(master, child_index)

        assert len(bytes(master)) == 32
        assert len(bytes(child)) == 32
        assert bytes(master) == bytes.fromhex(master_sk_hex)
        assert bytes(child) == bytes.fromhex(child_sk_hex)

    test_case_1 = (
        "3141592653589793238462643383279502884197169399375105820974944592",
        "4ff5e145590ed7b71e577bb04032396d1619ff41cb4e350053ed2dce8d1efd1c",
        "5c62dcf9654481292aafa3348f1d1b0017bbfb44d6881d26d2b17836b38f204d",
        3141592653,
    )
    test_case_2 = (
        "0099FF991111002299DD7744EE3355BBDD8844115566CC55663355668888CC00",
        "1ebd704b86732c3f05f30563dee6189838e73998ebc9c209ccff422adee10c4b",
        "1b98db8b24296038eae3f64c25d693a269ef1e4d7ae0f691c572a46cf3c0913c",
        4294967295,
    )
    test_case_3 = (
        "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
        "614d21b10c0e4996ac0608e0e7452d5720d95d20fe03c59a3321000a42432e1a",
        "08de7136e4afc56ae3ec03b20517d9c1232705a747f588fd17832f36ae337526",
        42,
    )
    test_case_intermediate = (
        "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04",
        "0befcabff4a664461cc8f190cdd51c05621eb2837c71a1362df5b465a674ecfb",
        "1a1de3346883401f1e3b2281be5774080edb8e5ebe6f776b0f7af9fea942553a",
        0,
    )
    test_one_case(*test_case_1)
    test_one_case(*test_case_2)
    test_one_case(*test_case_3)
    test_one_case(*test_case_intermediate)


def test_fields():
    a = Fq(17, 30)
    b = Fq(17, -18)
    c = Fq2(17, a, b)
    d = Fq2(17, a + a, Fq(17, -5))
    e = c * d
    f = e * d
    assert f != e
    e_sq = e * e
    e_sqrt = e_sq.modsqrt()
    assert pow(e_sqrt, 2) == e_sq

    a2 = Fq(
        172487123095712930573140951348,
        3012492130751239573498573249085723940848571098237509182375,
    )
    b2 = Fq(172487123095712930573140951348, 3432984572394572309458723045723849)
    c2 = Fq2(172487123095712930573140951348, a2, b2)
    assert b2 != c2

    g = Fq6(17, c, d, d * d * c)
    h = Fq6(17, a + a * c, c * b * a, b * b * d * Fq(17, 21))
    i = Fq12(17, g, h)
    assert ~(~i) == i
    assert (~(i.root)) * i.root == Fq6.one(17)
    x = Fq12(17, Fq6.zero(17), i.root)
    assert (~x) * x == Fq12.one(17)

    j = Fq6(17, a + a * c, Fq2.zero(17), Fq2.zero(17))
    j2 = Fq6(17, a + a * c, Fq2.zero(17), Fq2.one(17))
    assert j == (a + a * c)
    assert j2 != (a + a * c)
    assert j != j2

    # Test frob_coeffs
    one = Fq(default_ec.q, 1)
    two = one + one
    a = Fq2(default_ec.q, two, two)
    b = Fq6(default_ec.q, a, a, a)
    c = Fq12(default_ec.q, b, b)
    for base in (a, b, c):
        for expo in range(1, base.extension):
            assert base.qi_power(expo) == pow(base, pow(default_ec.q, expo))


def test_ec():
    q = default_ec.q
    g = G1Generator()

    assert g.is_on_curve()
    assert 2 * g == g + g
    assert (3 * g).is_on_curve()
    assert 3 * g == g + g + g

    g2 = G2Generator()
    assert g2.x * (Fq(q, 2) * g2.y) == Fq(q, 2) * (g2.x * g2.y)
    assert g2.is_on_curve()
    s = g2 + g2
    assert untwist(twist(s.to_affine())) == s.to_affine()
    assert untwist(5 * twist(s.to_affine())) == (5 * s).to_affine()
    assert 5 * twist(s.to_affine()) == twist((5 * s).to_affine())
    assert s.is_on_curve()
    assert g2.is_on_curve()
    assert g2 + g2 == 2 * g2
    assert g2 * 5 == (g2 * 2) + (2 * g2) + g2
    y = y_for_x(g2.x, default_ec_twist, Fq2)
    assert y == g2.y or -y == g2.y

    g_j = G1Generator()
    g2_j = G2Generator()
    g2_j2 = G2Generator() * 2
    assert g.to_affine().to_jacobian() == g
    assert (g_j * 2).to_affine() == g.to_affine() * 2
    assert (g2_j + g2_j2).to_affine() == g2.to_affine() * 3


def test_edge_case_sign_Fq2():
    q = default_ec.q
    a = Fq(q, 62323)
    test_case_1 = Fq2(q, a, Fq(q, 0))
    test_case_2 = Fq2(q, -a, Fq(q, 0))
    assert sign_Fq2(test_case_1) != sign_Fq2(test_case_2)

    test_case_3 = Fq2(q, Fq(q, 0), a)
    test_case_4 = Fq2(q, Fq(q, 0), -a)

    assert sign_Fq2(test_case_3) != sign_Fq2(test_case_4)


def test_xmd():
    msg = token_bytes(48)
    dst = token_bytes(16)
    ress = {}
    for length in range(16, 8192):
        result = expand_message_xmd(msg, dst, length, hashlib.sha512)
        assert length == len(result)
        key = result[:16]
        ress[key] = ress.get(key, 0) + 1
    assert all(x == 1 for x in ress.values())


def test_swu():
    dst_1 = b"QUUX-V01-CS02-with-BLS12381G2_XMD:SHA-256_SSWU_RO_"
    msg_1 = b"abcdef0123456789"
    res = g2_map(msg_1, dst_1).to_affine()
    assert (
        res.x[0].value
        == 0x121982811D2491FDE9BA7ED31EF9CA474F0E1501297F68C298E9F4C0028ADD35AEA8BB83D53C08CFC007C1E005723CD0
    )
    assert (
        res.x[1].value
        == 0x190D119345B94FBD15497BCBA94ECF7DB2CBFD1E1FE7DA034D26CBBA169FB3968288B3FAFB265F9EBD380512A71C3F2C
    )
    assert (
        res.y[0].value
        == 0x05571A0F8D3C08D094576981F4A3B8EDA0A8E771FCDCC8ECCEAF1356A6ACF17574518ACB506E435B639353C2E14827C8
    )
    assert (
        res.y[1].value
        == 0x0BB5E7572275C567462D91807DE765611490205A941A5A6AF3B1691BFE596C31225D3AABDF15FAFF860CB4EF17C7C3BE
    )


def test_elements():
    i1 = int.from_bytes(bytes([1, 2]), byteorder="big")
    i2 = int.from_bytes(bytes([3, 1, 4, 1, 5, 9]), byteorder="big")
    b1 = i1
    b2 = i2
    g1 = G1Generator()
    g2 = G2Generator()
    u1 = G1Infinity()
    u2 = G2Infinity()

    x1 = g1 * b1
    x2 = g1 * b2
    y1 = g2 * b1
    y2 = g2 * b2

    # G1
    assert x1 != x2
    assert x1 * b1 == b1 * x1
    assert x1 * b1 != x1 * b2

    left = x1 + u1
    right = x1

    assert left == right
    assert x1 + x2 == x2 + x1
    assert x1 + x1.negate() == u1
    assert x1 == G1FromBytes(bytes(x1))
    copy = deepcopy(x1)
    assert x1 == copy
    x1 += x2
    assert x1 != copy

    # G2
    assert y1 != y2
    assert y1 * b1 == b1 * y1
    assert y1 * b1 != y1 * b2
    assert y1 + u2 == y1
    assert y1 + y2 == y2 + y1
    assert y1 + y1.negate() == u2
    assert y1 == G2FromBytes(bytes(y1))
    copy = deepcopy(y1)
    assert y1 == copy
    y1 += y2
    assert y1 != copy

    # pairing operation
    pair = ate_pairing(x1, y1)
    assert pair != ate_pairing(x1, y2)
    assert pair != ate_pairing(x2, y1)
    copy = deepcopy(pair)
    assert pair == copy
    pair = None
    assert pair != copy

    sk = 728934712938472938472398074
    pk = sk * g1
    Hm = y2 * 12371928312 + y2 * 12903812903891023

    sig = Hm * sk

    assert ate_pairing(g1, sig) == ate_pairing(pk, Hm)


def test_chia_vectors_1():
    seed1: bytes = bytes([0x00] * 32)
    seed2: bytes = bytes([0x01] * 32)
    msg1: bytes = bytes([7, 8, 9])
    msg2: bytes = bytes([10, 11, 12])
    sk1 = BasicSchemeMPL.key_gen(seed1)
    sk2 = BasicSchemeMPL.key_gen(seed2)
    assert (
        bytes(sk1).hex()
        == "4a353be3dac091a0a7e640620372f5e1e2e4401717c1e79cac6ffba8f6905604"
    )
    assert (
        bytes(sk1.get_g1()).hex()
        == "85695fcbc06cc4c4c9451f4dce21cbf8de3e5a13bf48f44cdbb18e2038ba7b8bb1632d7911ef1e2e08749bddbf165352"
    )

    sig1 = BasicSchemeMPL.sign(sk1, msg1)
    sig2 = BasicSchemeMPL.sign(sk2, msg2)

    assert (
        bytes(sig1).hex()
        == "b8faa6d6a3881c9fdbad803b170d70ca5cbf1e6ba5a586262df368c75acd1d1ffa3ab6ee21c71f844494659878f5eb230c958dd576b08b8564aad2ee0992e85a1e565f299cd53a285de729937f70dc176a1f01432129bb2b94d3d5031f8065a1"
    )
    assert bytes(sig2).hex() == (
        "a9c4d3e689b82c7ec7e838dac2380cb014f9a08f6cd6ba044c263746e39a8f7a60ffee4afb7"
        "8f146c2e421360784d58f0029491e3bd8ab84f0011d258471ba4e87059de295d9aba845c044e"
        "e83f6cf2411efd379ef38bf4cf41d5f3c0ae1205d"
    )

    agg_sig_1 = BasicSchemeMPL.aggregate([sig1, sig2])

    assert bytes(agg_sig_1).hex() == (
        "aee003c8cdaf3531b6b0ca354031b0819f7586b5846796615aee8108fec75ef838d181f9d24"
        "4a94d195d7b0231d4afcf06f27f0cc4d3c72162545c240de7d5034a7ef3a2a03c0159de982fb"
        "c2e7790aeb455e27beae91d64e077c70b5506dea3"
    )

    assert BasicSchemeMPL.aggregate_verify(
        [sk1.get_g1(), sk2.get_g1()], [msg1, msg2], agg_sig_1
    )

    msg3: bytes = bytes([1, 2, 3])
    msg4: bytes = bytes([1, 2, 3, 4])
    msg5: bytes = bytes([1, 2])

    sig3 = BasicSchemeMPL.sign(sk1, msg3)
    sig4 = BasicSchemeMPL.sign(sk1, msg4)
    sig5 = BasicSchemeMPL.sign(sk2, msg5)

    agg_sig_2 = BasicSchemeMPL.aggregate([sig3, sig4, sig5])
    assert BasicSchemeMPL.aggregate_verify(
        [sk1.get_g1(), sk1.get_g1(), sk2.get_g1()], [msg3, msg4, msg5], agg_sig_2
    )

    assert bytes(agg_sig_2).hex() == (
        "a0b1378d518bea4d1100adbc7bdbc4ff64f2c219ed6395cd36fe5d2aa44a4b8e710b607afd9"
        "65e505a5ac3283291b75413d09478ab4b5cfbafbeea366de2d0c0bcf61deddaa521f6020460f"
        "d547ab37659ae207968b545727beba0a3c5572b9c"
    )


def test_chia_vectors_2():
    msg1 = bytes([1, 2, 3, 40])
    msg2 = bytes([5, 6, 70, 201])
    msg3 = bytes([9, 10, 11, 12, 13])
    msg4 = bytes([15, 63, 244, 92, 0, 1])

    seed1 = bytes([0x02] * 32)
    seed2 = bytes([0x03] * 32)

    sk1 = AugSchemeMPL.key_gen(seed1)
    sk2 = AugSchemeMPL.key_gen(seed2)

    pk1 = sk1.get_g1()
    pk2 = sk2.get_g1()

    sig1 = AugSchemeMPL.sign(sk1, msg1)
    sig2 = AugSchemeMPL.sign(sk2, msg2)
    sig3 = AugSchemeMPL.sign(sk2, msg1)
    sig4 = AugSchemeMPL.sign(sk1, msg3)
    sig5 = AugSchemeMPL.sign(sk1, msg1)
    sig6 = AugSchemeMPL.sign(sk1, msg4)

    agg_sig_l = AugSchemeMPL.aggregate([sig1, sig2])
    agg_sig_r = AugSchemeMPL.aggregate([sig3, sig4, sig5])
    agg_sig = AugSchemeMPL.aggregate([agg_sig_l, agg_sig_r, sig6])

    assert AugSchemeMPL.aggregate_verify(
        [pk1, pk2, pk2, pk1, pk1, pk1], [msg1, msg2, msg1, msg3, msg1, msg4], agg_sig
    )

    assert bytes(agg_sig).hex() == (
        "a1d5360dcb418d33b29b90b912b4accde535cf0e52caf467a005dc632d9f7af44b6c4e9acd4"
        "6eac218b28cdb07a3e3bc087df1cd1e3213aa4e11322a3ff3847bbba0b2fd19ddc25ca964871"
        "997b9bceeab37a4c2565876da19382ea32a962200"
    )


def test_chia_vectors_3():
    seed1: bytes = bytes([0x04] * 32)
    sk1 = PopSchemeMPL.key_gen(seed1)
    proof = PopSchemeMPL.pop_prove(sk1)
    assert (
        bytes(proof).hex()
        == "84f709159435f0dc73b3e8bf6c78d85282d19231555a8ee3b6e2573aaf66872d9203fefa1ef"
        "700e34e7c3f3fb28210100558c6871c53f1ef6055b9f06b0d1abe22ad584ad3b957f3018a8f5"
        "8227c6c716b1e15791459850f2289168fa0cf9115"
    )


def test_pyecc_vectors():
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
    print(bytes(sig1Basic).hex())
    assert bytes(sig2Basic) == ref_sig2Basic
    assert bytes(sigABasic) == ref_sigABasic
    assert bytes(sig1Aug) == ref_sig1Aug
    assert bytes(sig2Aug) == ref_sig2Aug
    assert bytes(sigAAug) == ref_sigAAug
    assert bytes(sig1Pop) == ref_sig1Pop
    assert bytes(sig2Pop) == ref_sig2Pop
    assert bytes(sigAPop) == ref_sigAPop


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
        bytes_ = bytes.fromhex(s)
        try:
            g1 = G1FromBytes(bytes_)
            assert g1 is not None
            assert False, "Failed to disallow creation of G1 element."
        except Exception:
            pass

    for s in invalid_inputs_2:
        bytes_ = bytes.fromhex(s)
        try:
            g2 = G2FromBytes(bytes_)
            assert g2 is not None
            assert False, "Failed to disallow creation of G2 element."
        except Exception:
            pass


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
    assert sk is not None
    pk = G1FromBytes(pk_bytes)
    assert pk is not None
    signature: G2Element = G2FromBytes(signature_bytes)

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
    assert grandchild is not None

    master_pk: G1Element = master_sk.get_g1()
    child_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(master_sk, 22)
    grandchild_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(child_u, 0)

    child_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(master_pk, 22)
    grandchild_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(child_u_pk, 0)

    ok = grandchild_u_pk == grandchild_u.get_g1()
    assert ok


test_hkdf()
test_eip2333()
test_fields()
test_ec()
test_xmd()
test_swu()
test_edge_case_sign_Fq2()
test_elements()
test_chia_vectors_1()
test_chia_vectors_2()
test_chia_vectors_3()
test_pyecc_vectors()
test_vectors_invalid()
test_readme()
