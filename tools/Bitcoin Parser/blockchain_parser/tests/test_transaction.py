# Copyright (C) 2015-2016 The bitcoin-blockchain-parser developers
#
# This file is part of bitcoin-blockchain-parser.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of bitcoin-blockchain-parser, including this file, may be copied,
# modified, propagated, or distributed except according to the terms contained
# in the LICENSE file.

import os
import unittest
from binascii import a2b_hex, b2a_hex
from blockchain_parser.transaction import Transaction

dir_path = os.path.dirname(os.path.realpath(__file__))


class TestTransaction(unittest.TestCase):
    def test_rbf(self):
        data = a2b_hex("01000000019222bbb054bb9f94571dfe769af5866835f2a97e8839"
                       "59fa757de4064bed8bca01000000035101b1000000000100000000"
                       "00000000016a01000000")
        tx = Transaction(data)
        self.assertTrue(tx.uses_replace_by_fee())

        coinbase = a2b_hex("01000000010000000000000000000000000000000000000000"
                           "000000000000000000000000ffffffff4203c8e405fabe6d6d"
                           "98b0e98e3809941f1fd8cafe7c8236e27b8d1a776b1835aa54"
                           "8bb84fe5b5f3d7010000000000000002650300aaa757eb0000"
                           "002f736c7573682f0000000001baa98396000000001976a914"
                           "7c154ed1dc59609e3d26abb2df2ea3d587cd8c4188ac000000"
                           "00")
        tx = Transaction(coinbase)
        self.assertTrue(tx.is_coinbase())
        self.assertFalse(tx.uses_replace_by_fee())

    def test_bip69(self):
        noncompliant = "bip69_false.txt"
        with open(os.path.join(dir_path, noncompliant)) as f:
            data = a2b_hex(f.read().strip())

        tx = Transaction(data)
        self.assertFalse(tx.uses_bip69())

        compliant = "bip69_true.txt"
        with open(os.path.join(dir_path, compliant)) as f:
            data = a2b_hex(f.read().strip())

        tx = Transaction(data)
        self.assertTrue(tx.uses_bip69())

    def test_segwit(self):
        example_tx = "segwit.txt"
        with open(os.path.join(dir_path, example_tx)) as f:
            data = a2b_hex(f.read().strip())

        tx = Transaction(data)
        self.assertTrue(tx.is_segwit)
        id = "22116f1d76ab425ddc6d10d184331e70e080dd6275d7aa90237ceb648dc38224"
        self.assertTrue(tx.txid == id)
        h = "1eac09f372a8c13bb7dea6bd66ee71a6bcc469b57b35c1e394ad7eb7c107c507"
        self.assertTrue(tx.hash == h)

        segwit_input = tx.inputs[0]
        self.assertTrue(len(segwit_input.witnesses) == 4)
        self.assertTrue(len(segwit_input.witnesses[0]) == 0)

        wit_1 = "3045022100bc2ba8808127f8a74beed6dfa1b9fe54675c55aab85a61d7" \
                "a74c15b993e67e5f02204dada4e15f0b4e659dae7bf0d0f648010d1f2b" \
                "665f587a35eb6f22e44194952301"

        wit_2 = "3045022100f4c7ec7c2064fe2cc4389733ac0a57d8080a62180a004b02" \
                "a19b89267113a17f022004ee9fdb081359c549ee42ffb58279363563ea" \
                "f0191cd8b2b0ceebf62146b50b01"

        wit_3 = "5221022b003d276bce58bef509bdcd9cf7e156f0eae18e1175815282e6" \
                "5e7da788bb5b21035c58f2f60ecf38c9c8b9d1316b662627ec672f5fd9" \
                "12b1a2cc28d0b9b00575fd2103c96d495bfdd5ba4145e3e046fee45e84" \
                "a8a48ad05bd8dbb395c011a32cf9f88053ae"

        parsed_1 = b2a_hex(segwit_input.witnesses[1]).decode("utf-8")
        parsed_2 = b2a_hex(segwit_input.witnesses[2]).decode("utf-8")
        parsed_3 = b2a_hex(segwit_input.witnesses[3]).decode("utf-8")

        self.assertEqual(parsed_1, wit_1)
        self.assertEqual(parsed_2, wit_2)
        self.assertEqual(parsed_3, wit_3)

    def test_large(self):
        data = a2b_hex(
            "010000000b04a4abee8360f7a660bcf1c298496d927b6ad1f25dd12485"
            "e6a572e275cbd43f000000008b483045022100c6d59290df934bac08bc"
            "4124b06154ee21394d37ec958d145129a885dbd63eab0220036251120e"
            "4ceacc8e9ff13585049945c7dbdb85921a6e077a80c7e410931f4a0141"
            "04f3ce8948c592690cb36a7e6e31534927569f41e45c77fbdb95fdb0e2"
            "35717a295b2a281c04619139c453dd9139e951086325b4021bd28da6fc"
            "3da86943cd04cfffffffff21fd91e6dd64838eadce9bc0d687af571779"
            "c3af4b53bada845485466decf005000000008b48304502205d309b632a"
            "816f1f2479b8ca284301fce3fc8f979306722bba4cf28c8fa7ed760221"
            "00a3313ee59bbf0e61e29302b2ea597a993a73e93be841f2f7dd3f6e96"
            "23572793014104ca3b25750307a483c0cff8efb1413c98b4e18765485d"
            "94eac147f21f507169997a72aeb50f0b01ed6b468c9554143125f63c0b"
            "2b229a597e695718b86d711607ffffffff4dd6c46e3e68fc05243b6f89"
            "4e2f3e4d2782d1c8a3d17b9a2c5e78912bbaf433010000008b48304502"
            "2100f3f1d6c0e76a00e89a5278fa14eae8de93454174199f72c98aef54"
            "dabb2074d10220172dc0d6958efa8dc18c621f2d56fd0484ecb04e3d65"
            "e2284268916c1c324ade014104daeab6af397baf06fcc91b6c04a28ffb"
            "2cd80312e954a2a55f7c864094608032ef657e8296dfc2f86ecf28fe32"
            "925c0f648d3ef16b96934a34b29dd871ff057affffffff52aabc7fdee0"
            "588d63fff3fc00e20b00a87cffa6a37aad7168741b1f0e77bdff000000"
            "008b4830450220274a5a9577f0786fb17d373b6d1995a6cff752320bf4"
            "e3d5ee41b15fdd86ef2a022100fd1d0c0f43d05e479430c3c9bd5481f5"
            "be929a7f2b82d14292ef192653c3b94c014104988828ea7144ff00def9"
            "988e88aad98b1f22d01000fc4b52d1003c9b32de0534cff71c0a045098"
            "bb28f699bcc9db13533c0998eb2133085d37cac65b5bd74c1fffffffff"
            "5aab5581c575a56a803ef5d8698dc34014261d196ada1da3300731380d"
            "7e672d000000008b48304502201615a116a2e057f3f8f2a63340d0387b"
            "820a6f9d1067758f4f6cdec76c04951a022100a1ada9e03a0d9603743d"
            "41347ffd992642544d6dfdc51121b7bf622e12cb1547014104830b3b13"
            "9b0a8707f348840b0fa358164869fbc0c423a2785cb0a18ff73517f887"
            "8cefd643e86e682eb93d07216f76993c9e980592de07c507c3a2659eff"
            "f6aaffffffff74d7cd370a334aabee3b8328b67a63f1768583405e73eb"
            "d291dd86189983a61c000000008a4730440220432ff0f90a3f5372a8dc"
            "4b3d2e7165e110efa16b40784ea4d5ab866b736712ed02201a759b630d"
            "562f4dbc4a7d2fca406ad54e235564346bc360dec26d6e72bb60fa0141"
            "04ca0bb72303672885ad953279309694c71574d50d2c6e1f5f1e7ab1b0"
            "bf3df67e5439b3f6e2501dda539812e414991ebaa547631374ce2bda9d"
            "d7211099886827ffffffff8941eb7fe688ff48357981c4756906e331c0"
            "7f332429be5b2cfc7a945644f80f000000008a47304402204e074678b5"
            "25927a8f459d1dc93ad738ce02c0ea77c50a749647032eda662eee0220"
            "3c1a98e9b116dc2fe9e7325a5696ad8e3ad26d2d10503e6e3209f4f5f4"
            "0f6e34014104f9baa86b46b04f2a2dc4270903bcca9444026b7f184d87"
            "9f089462e332f260f6de8e05f08285a4a3c716220d9ec0f7e9fde354c7"
            "79f32f00dd4d4cd8d4a1ac28ffffffff948be2e03f9537a35319e8cee2"
            "b7b9ed2769ca59734e5d4b42951ef7ba3bb4a8000000008b4830450221"
            "00fcf1deaaa047e8f1b8b8ac783760ea5a0af15adbb9eed1c1748942a5"
            "7510e27402204c278b12c4991480dc714af66395f5d8f42e411423bf27"
            "b024f62267c45a0d37014104cfd128c3f322b59c21d452d5c4c7b4d1a3"
            "237a2d31c6f4033b32db78c19cb342ffc151acc272a02a13ba985a523b"
            "655689d8118f502b8a7fb8df5fc7bd8458ceffffffffb2cae04cf238b4"
            "d47c614da5ce9948f089a02f1ff3e22cda6b9f9196bda8721501000000"
            "8b483045022017a74373ef19756e275f2d987925a8bd22cfaf1c93eede"
            "7db63102dc73b7cfd0022100fd2f040d7d3ccd1a2188923f8c7560253a"
            "a2a910d8e9cd06a8a05fc69c12f6ce014104daeab6af397baf06fcc91b"
            "6c04a28ffb2cd80312e954a2a55f7c864094608032ef657e8296dfc2f8"
            "6ecf28fe32925c0f648d3ef16b96934a34b29dd871ff057affffffffbe"
            "75c9f1faf79fb885018e3353ada4a7fb5c989bc26f031c5aee69d1eaba"
            "d957000000008a4730440220360d35b0a98dd086d242f8976aa704b808"
            "541ad78ef5cba45e4eb73c1efbaaa7022025a3d981b9e0836a2493698b"
            "802bf7f0eae72de0252c7eac059ad14b1b27001b014104d969f0d45619"
            "6f4c16945bc71fc460615002d1554fc8d774288aa8fc5c9eca1622a3dc"
            "b1e6a8314c94844e516c0543316b2e75d465c8fc2f0d0d20334e53f7db"
            "ffffffffbcbad95cac8bbe08538f7b12c240e96866358dfa819de6e940"
            "bb60ce7fae5c3d000000008b483045022100d916990888d0d26246b8d6"
            "cb944b1e8ebd6a4cdb2965ff7e4430883eef02c4e102205693b96aa1d6"
            "e9335f3cb2986b7f3a03357959e989570d03a56ccdb18a8d7e6d014104"
            "0840a410ed7afdc3e3fe3923ae388413bbd8a9089062403e0adce62212"
            "bccfe1f80400530121cfdba5b52f3e16397f1959b14a09089778afd5ff"
            "41f8a856980fffffffff02005039278c0400001976a914f1c87a5e8ff7"
            "d14e74b858089bf771c94b1b6db488ac00203d88792d00001976a914dc"
            "df2f9892bfa1cb086530354eab3ba078a2f09088ac00000000")

        tx = Transaction(data)
        self.assertTrue(
            tx.hash == "29a3efd3ef04f9153d47a990bd7b048a4b2d213daa"
                       "a5fb8ed670fb85f13bdbcf")
        self.assertTrue(tx.size == len(tx.hex))

    def test_incomplete(self):
        data = a2b_hex(
            "010000000b04a4abee8360f7a660bcf1c298496d927b6ad1f25dd12485"
            "e6a572e275cbd43f000000008b483045022100c6d59290df934bac08bc"
            "4124b06154ee21394d37ec958d145129a885dbd63eab0220036251120e"
            "4ceacc8e9ff13585049945c7dbdb85921a6e077a80c7e410931f4a0141"
            "04f3ce8948c592690cb36a7e6e31534927569f41e45c77fbdb95fdb0e2"
            "35717a295b2a281c04619139c453dd9139e951086325b4021bd28da6fc"
            "3da86943cd04cfffffffff21fd91e6dd64838eadce9bc0d687af571779"
            "c3af4b53bada845485466decf005000000008b48304502205d309b632a"
            "816f1f2479b8ca284301fce3fc8f979306722bba4cf28c8fa7ed760221"
            "00a3313ee59bbf0e61e29302b2ea597a993a73e93be841f2f7dd3f6e96"
            "23572793014104ca3b25750307a483c0cff8efb1413c98b4e18765485d"
            "94eac147f21f507169997a72aeb50f0b01ed6b468c9554143125f63c0b"
            "2b229a597e695718b86d711607ffffffff4dd6c46e3e68fc05243b6f89"
            "4e2f3e4d2782d1c8a3d17b9a2c5e78912bbaf433010000008b48304502"
            "2100f3f1d6c0e76a00e89a5278fa14eae8de93454174199f72c98aef54"
            "dabb2074d10220172dc0d6958efa8dc18c621f2d56fd0484ecb04e3d65"
            "e2284268916c1c324ade014104daeab6af397baf06fcc91b6c04a28ffb"
            "2cd80312e954a2a55f7c864094608032ef657e8296dfc2f86ecf28fe32"
            "925c0f648d3ef16b96934a34b29dd871ff057affffffff52aabc7fdee0"
            "588d63fff3fc00e20b00a87cffa6a37aad7168741b1f0e77bdff000000"
            "008b4830450220274a5a9577f0786fb17d373b6d1995a6cff752320bf4"
            "e3d5ee41b15fdd86ef2a022100fd1d0c0f43d05e479430c3c9bd5481f5"
            "be929a7f2b82d14292ef192653c3b94c014104988828ea7144ff00def9"
            "988e88aad98b1f22d01000fc4b52d1003c9b32de0534cff71c0a045098"
            "bb28f699bcc9db13533c0998eb2133085d37cac65b5bd74c1fffffffff"
            "5aab5581c575a56a803ef5d8698dc34014261d196ada1da3300731380d"
            "7e672d000000008b48304502201615a116a2e057f3f8f2a63340d0387b"
            "820a6f9d1067758f4f6cdec76c04951a022100a1ada9e03a0d9603743d"
            "41347ffd992642544d6dfdc51121b7bf622e12cb1547014104830b3b13"
            "9b0a8707f348840b0fa358164869fbc0c423a2785cb0a18ff73517f887"
            "8cefd643e86e682eb93d07216f76993c9e980592de07c507c3a2659eff"
            "f6aaffffffff74d7cd370a334aabee3b8328b67a63f1768583405e73eb"
            "d291dd86189983a61c000000008a4730440220432ff0f90a3f5372a8dc"
            "4b3d2e7165e110efa16b40784ea4d5ab866b736712ed02201a759b630d"
            "562f4dbc4a7d2fca406ad54e235564346bc360dec26d6e72bb60fa0141"
            "04ca0bb72303672885ad953279309694c71574d50d2c6e1f5f1e7ab1b0"
            "bf3df67e5439b3f6e2501dda539812e414991ebaa547631374ce2bda9d"
            "d7211099886827ffffffff8941eb7fe688ff48357981c4756906e331c0"
            "7f332429be5b2cfc7a945644f80f000000008a47304402204e074678b5"
            "25927a8f459d1dc93ad738ce02c0ea77c50a749647032eda662eee0220"
            "3c1a98e9b116dc2fe9e7325a5696ad8e3ad26d2d10503e6e3209f4f5f4"
            "0f6e34014104f9baa86b46b04f2a2dc4270903bcca9444026b7f184d87"
            "9f089462e332f260f6de8e05f08285a4a3c716220d9ec0f7e9fde354c7"
            "79f32f00dd4d4cd8d4a1ac28ffffffff948be2e03f9537a35319e8cee2"
            "b7b9ed2769ca59734e5d4b42951ef7ba3bb4a8000000008b4830450221"
            "00fcf1deaaa047e8f1b8b8ac783760ea5a0af15adbb9eed1c1748942a5"
            "7510e27402204c278b12c4991480dc714af66395f5d8f42e411423bf27"
            "b024f62267c45a0d37014104cfd128c3f322b59c21d452d5c4c7b4d1a3"
            "237a2d31c6f4033b32db78c19cb342ffc151acc272a02a13ba985a523b"
            "655689d8118f502b8a7fb8df5fc7bd8458ceffffffffb2cae04cf238b4"
            "d47c614da5ce9948f089a02f1ff3e22cda6b9f9196bda8721501000000"
            "8b483045022017a74373ef19756e275f2d987925a8bd22cfaf1c93eede"
            "7db63102dc73b7cfd0022100fd2f040d7d3ccd1a2188923f8c7560253a"
            "a2a910d8e9cd06a8a05fc69c12f6ce014104daeab6af397baf06fcc91b"
            "6c04a28ffb2cd80312e954a2a55f7c864094608032ef657e8296dfc2f8"
            "6ecf28fe32925c0f648d3ef16b96934a34b29dd871ff057affffffffbe"
            "75c9f1faf79fb885018e3353ada4a7fb5c989bc26f031c5aee69d1eaba"
            "d957000000008a4730440220360d35b0a98dd086d242f8976aa704b808"
            "541ad78ef5cba45e4eb73c1efbaaa7022025a3d981b9e0836a2493698b"
            "802bf7f0eae72de0252c7eac059ad14b1b27001b014104d969f0d45619"
            "6f4c16945bc71fc460615002d1554fc8d774288aa8fc5c9eca1622a3dc"
            "b1e6a8314c94844e516c0543316b2e75d465c8fc2f0d0d20334e53f7db"
            "ffffffffbcbad95cac8bbe08538f7b12c240e96866358dfa819de6e940"
            "bb60ce7fae5c3d000000008b483045022100d916990888d0d26246b8d6"
            "cb944b1e8ebd6a4cdb2965ff7e4430883eef02c4e102205693b96aa1d6"
            "e9335f3cb2986b7f3a03357959e989570d03a56ccdb18a8d7e6d014104"
            "0840a410ed7afdc3e3fe3923ae388413bbd8a9089062403e0adce62212"
            "bccfe1f80400530121cfdba5b52f3e16397f1959b14a09089778afd5ff"
            "41f8a856980fffffffff02005039278c0400001976a914f1c87a5e8ff7"
            "d14e74b858089bf771c94b1b6db488ac00203d88792d00001976a914dc"
            "df2f9892bfa1cb086530354eab3ba078a2f0")

        self.assertRaises(Exception, Transaction, data)
