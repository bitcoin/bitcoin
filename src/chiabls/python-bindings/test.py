# flake8: noqa: E501
from blspy import (PrivateKey, PublicKey,
                   Signature, AggregationInfo,
                   ExtendedPrivateKey, BLS)


def test1():
    seed = bytes([0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                  19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                  82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22])
    sk = PrivateKey.from_seed(seed)
    pk = sk.get_public_key()

    msg = bytes([100, 2, 254, 88, 90, 45, 23])

    sig = sk.sign(msg)

    sk_bytes = sk.serialize()
    pk_bytes = pk.serialize()
    sig_bytes = sig.serialize()

    sk = PrivateKey.from_bytes(sk_bytes)
    pk = PublicKey.from_bytes(pk_bytes)
    sig = Signature.from_bytes(sig_bytes)

    sig.set_aggregation_info(AggregationInfo.from_msg(pk, msg))
    ok = sig.verify()
    assert(ok)

    seed = bytes([1]) + seed[1:]
    sk1 = PrivateKey.from_seed(seed)
    seed = bytes([2]) + seed[1:]
    sk2 = PrivateKey.from_seed(seed)

    pk1 = sk1.get_public_key()
    sig1 = sk1.sign(msg)

    pk2 = sk2.get_public_key()
    sig2 = sk2.sign(msg)

    agg_sig = Signature.aggregate([sig1, sig2])
    agg_pubkey = PublicKey.aggregate([pk1, pk2])

    agg_sig.set_aggregation_info(AggregationInfo.from_msg(agg_pubkey, msg))
    assert(agg_sig.verify())

    seed = bytes([3]) + seed[1:]
    sk3 = PrivateKey.from_seed(seed)
    pk3 = sk3.get_public_key()
    msg2 = bytes([100, 2, 254, 88, 90, 45, 23])

    sig1 = sk1.sign(msg)
    sig2 = sk2.sign(msg)
    sig3 = sk3.sign(msg2)
    agg_sig_l = Signature.aggregate([sig1, sig2])
    agg_sig_final = Signature.aggregate([agg_sig_l, sig3])

    sig_bytes = agg_sig_final.serialize()

    agg_sig_final = Signature.from_bytes(sig_bytes)
    a1 = AggregationInfo.from_msg(pk1, msg)
    a2 = AggregationInfo.from_msg(pk2, msg)
    a3 = AggregationInfo.from_msg(pk3, msg2)
    a1a2 = AggregationInfo.merge_infos([a1, a2])
    a_final = AggregationInfo.merge_infos([a1a2, a3])
    print(a_final)
    agg_sig_final.set_aggregation_info(a_final)
    ok = agg_sig_final.verify()

    ok = agg_sig_l.verify()
    agg_sig_final = agg_sig_final.divide_by([agg_sig_l])

    ok = agg_sig_final.verify()

    agg_sk = PrivateKey.aggregate([sk1, sk2], [pk1, pk2])
    agg_sk.sign(msg)

    seed = bytes([1, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                  19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                  82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22])

    esk = ExtendedPrivateKey.from_seed(seed)
    epk = esk.get_extended_public_key()

    sk_child = esk.private_child(0).private_child(5)
    pk_child = epk.public_child(0).public_child(5)

    buffer1 = pk_child.serialize()
    buffer2 = sk_child.serialize()

    print(len(buffer1), buffer1)
    print(len(buffer2), buffer2)
    assert(sk_child.get_extended_public_key() == pk_child)


def test2():
    seed = bytes([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
    seed2 = bytes([1, 20, 102, 229, 1, 157])

    sk = PrivateKey.from_seed(seed)
    sk_cp = PrivateKey.from_seed(seed)
    sk2 = PrivateKey.from_seed(seed2)
    pk = sk.get_public_key()
    pk2 = sk2.get_public_key()
    assert(sk == sk_cp)
    assert(sk != sk2)
    assert(pk.get_fingerprint() == 0xddad59bb)

    sk2_ser = sk2.serialize()
    pk2_ser = pk2.serialize()
    pk2_copy = PublicKey.from_bytes(pk2_ser)
    assert(pk2 == pk2_copy)
    assert(pk != pk2)
    assert(len(pk2_ser) == 48)
    assert(len(sk2_ser) == 32)

    message = bytes("this is the message", "utf-8")
    sig = sk.sign(message)
    sig_ser = sig.serialize()
    sig_cp = Signature.from_bytes(sig_ser)
    a1 = AggregationInfo.from_msg(pk, message)
    sig_cp.set_aggregation_info(a1)
    a2 = sig_cp.get_aggregation_info()
    assert(a1 == a2)
    sig2 = sk2.sign(message)

    assert(len(sig_ser) == 96)
    assert(sig != sig2)
    assert(sig == sig_cp)

    sig_agg = Signature.aggregate([sig, sig2])

    result = sig_cp.verify()
    result2 = sig2.verify()
    result3 = sig_agg.verify()
    assert(result)
    assert(result2)
    assert(result3)
    sk2 = sk


def test_vectors():
    sk1 = PrivateKey.from_seed(bytes([1, 2, 3, 4, 5]))
    pk1 = sk1.get_public_key()
    sig1 = sk1.sign(bytes([7, 8, 9]))

    sk2 = PrivateKey.from_seed(bytes([1, 2, 3, 4, 5, 6]))
    pk2 = sk2.get_public_key()
    sig2 = sk2.sign(bytes([7, 8, 9]))
    assert(sk1.serialize() == bytes.fromhex("022fb42c08c12de3a6af053880199806532e79515f94e83461612101f9412f9e"))
    assert(pk1.get_fingerprint() == 0x26d53247)
    assert(pk2.get_fingerprint() == 0x289bb56e)
    assert(sig1.serialize() == bytes.fromhex("93eb2e1cb5efcfb31f2c08b235e8203a67265bc6a13d9f0ab77727293b74a357ff0459ac210dc851fcb8a60cb7d393a419915cfcf83908ddbeac32039aaa3e8fea82efcb3ba4f740f20c76df5e97109b57370ae32d9b70d256a98942e5806065"))
    assert(sig2.serialize() == bytes.fromhex("975b5daa64b915be19b5ac6d47bc1c2fc832d2fb8ca3e95c4805d8216f95cf2bdbb36cc23645f52040e381550727db420b523b57d494959e0e8c0c6060c46cf173872897f14d43b2ac2aec52fc7b46c02c5699ff7a10beba24d3ced4e89c821e"))

    agg_sig = Signature.aggregate([sig1, sig2])
    agg_pk = PublicKey.aggregate([pk1, pk2])
    agg_sk = PrivateKey.aggregate([sk1, sk2], [pk1, pk2])
    assert(agg_sig.serialize() == bytes.fromhex("0a638495c1403b25be391ed44c0ab013390026b5892c796a85ede46310ff7d0e0671f86ebe0e8f56bee80f28eb6d999c0a418c5fc52debac8fc338784cd32b76338d629dc2b4045a5833a357809795ef55ee3e9bee532edfc1d9c443bf5bc658"))
    assert(agg_sk.sign(bytes([7, 8, 9])).serialize() == agg_sig.serialize())


    assert(sig1.verify())
    assert(agg_sig.verify())

    agg_sig.set_aggregation_info(AggregationInfo.from_msg(agg_pk, bytes([7, 8, 9])))
    assert(agg_sig.verify())

    sig1.set_aggregation_info(sig2.get_aggregation_info())
    assert(not sig1.verify())

    sig3 = sk1.sign(bytes([1, 2, 3]))
    sig4 = sk1.sign(bytes([1, 2, 3, 4]))
    sig5 = sk2.sign(bytes([1, 2]))


    agg_sig2 = Signature.aggregate([sig3, sig4, sig5])
    assert(agg_sig2.verify())
    assert(agg_sig2.serialize() == bytes.fromhex("8b11daf73cd05f2fe27809b74a7b4c65b1bb79cc1066bdf839d96b97e073c1a635d2ec048e0801b4a208118fdbbb63a516bab8755cc8d850862eeaa099540cd83621ff9db97b4ada857ef54c50715486217bd2ecb4517e05ab49380c041e159b"))


def test_vectors2():
    m1 = bytes([1, 2, 3, 40])
    m2 = bytes([5, 6, 70, 201])
    m3 = bytes([9, 10, 11, 12, 13])
    m4 = bytes([15, 63, 244, 92, 0, 1])

    sk1 = PrivateKey.from_seed(bytes([1, 2, 3, 4, 5]))
    sk2 = PrivateKey.from_seed(bytes([1, 2, 3, 4, 5, 6]))

    sig1 = sk1.sign(m1)
    sig2 = sk2.sign(m2)
    sig3 = sk2.sign(m1)
    sig4 = sk1.sign(m3)
    sig5 = sk1.sign(m1)
    sig6 = sk1.sign(m4)

    sig_L = Signature.aggregate([sig1, sig2])
    sig_R = Signature.aggregate([sig3, sig4, sig5])
    assert(sig_L.verify())
    assert(sig_R.verify())

    sig_final = Signature.aggregate([sig_L, sig_R, sig6])
    assert(sig_final.serialize() == bytes.fromhex("07969958fbf82e65bd13ba0749990764cac81cf10d923af9fdd2723f1e3910c3fdb874a67f9d511bb7e4920f8c01232b12e2fb5e64a7c2d177a475dab5c3729ca1f580301ccdef809c57a8846890265d195b694fa414a2a3aa55c32837fddd80"))
    assert(sig_final.verify())
    quotient = sig_final.divide_by([sig2, sig5, sig6])
    assert(quotient.verify())
    assert(sig_final.verify())
    assert(quotient.serialize() == bytes.fromhex("8ebc8a73a2291e689ce51769ff87e517be6089fd0627b2ce3cd2f0ee1ce134b39c4da40928954175014e9bbe623d845d0bdba8bfd2a85af9507ddf145579480132b676f027381314d983a63842fcc7bf5c8c088461e3ebb04dcf86b431d6238f"))
    assert(quotient.divide_by([]) == quotient)
    try:
        quotient.divide_by([sig6])
        assert(False)  # Should fail due to not subset
    except:
        pass
    sig_final.divide_by([sig1]) # Should not throw
    try:
        sig_final.divide_by([sig_L]) # Should throw due to not unique
        assert(False)  # Should fail due to not unique
    except:
        pass

    # Divide by aggregate
    sig7 = sk2.sign(m3)
    sig8 = sk2.sign(m4)
    sig_R2 = Signature.aggregate([sig7, sig8])
    sig_final2 = Signature.aggregate([sig_final, sig_R2])
    quotient2 = sig_final2.divide_by([sig_R2])
    assert(quotient2.verify())
    assert(quotient2.serialize() == bytes.fromhex("06af6930bd06838f2e4b00b62911fb290245cce503ccf5bfc2901459897731dd08fc4c56dbde75a11677ccfbfa61ab8b14735fddc66a02b7aeebb54ab9a41488f89f641d83d4515c4dd20dfcf28cbbccb1472c327f0780be3a90c005c58a47d3"))


def test_vectors3():
    seed = bytes([1, 50, 6, 244, 24, 199, 1, 25])
    esk =  ExtendedPrivateKey.from_seed(seed)
    assert(esk.get_public_key().get_fingerprint() == 0xa4700b27)
    assert(esk.get_chain_code().serialize().hex() == "d8b12555b4cc5578951e4a7c80031e22019cc0dce168b3ed88115311b8feb1e3")
    esk77 = esk.private_child(77 + 2**31)
    assert(esk77.get_chain_code().serialize().hex() == "f2c8e4269bb3e54f8179a5c6976d92ca14c3260dd729981e9d15f53049fd698b")
    assert(esk77.get_public_key().get_fingerprint() == 0xa8063dcf)

    assert(esk.private_child(3)
              .private_child(17)
              .get_public_key()
              .get_fingerprint() == 0xff26a31f)

    assert(esk.get_extended_public_key()
              .public_child(3)
              .public_child(17)
              .get_public_key()
              .get_fingerprint() == 0xff26a31f)

test1()
test2()
test_vectors()
test_vectors2()
test_vectors3()

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
