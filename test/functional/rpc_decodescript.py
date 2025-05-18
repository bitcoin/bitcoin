#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test decoding scripts via decodescript RPC command."""

import json
import os

from test_framework.messages import (
    sha256,
    tx_from_hex,
)
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import (
    assert_equal,
)


class DecodeScriptTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def decodescript_script_sig(self):
        signature = '304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        push_signature = '48' + signature
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key

        # below are test cases for all of the standard transaction types

        self.log.info("- P2PK")
        # the scriptSig of a public key scriptPubKey simply pushes a signature onto the stack
        rpc_result = self.nodes[0].decodescript(push_signature)
        assert_equal(signature, rpc_result['asm'])

        self.log.info("- P2PKH")
        rpc_result = self.nodes[0].decodescript(push_signature + push_public_key)
        assert_equal(signature + ' ' + public_key, rpc_result['asm'])

        self.log.info("- multisig")
        # this also tests the leading portion of a P2SH multisig scriptSig
        # OP_0 <A sig> <B sig>
        rpc_result = self.nodes[0].decodescript('00' + push_signature + push_signature)
        assert_equal('0 ' + signature + ' ' + signature, rpc_result['asm'])

        self.log.info("- P2SH")
        # an empty P2SH redeemScript is valid and makes for a very simple test case.
        # thus, such a spending scriptSig would just need to pass the outer redeemScript
        # hash test and leave true on the top of the stack.
        rpc_result = self.nodes[0].decodescript('5100')
        assert_equal('1 0', rpc_result['asm'])

        # null data scriptSig - no such thing because null data scripts cannot be spent.
        # thus, no test case for that standard transaction type is here.

    def decodescript_script_pub_key(self):
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key
        public_key_hash = '5dd1d3a048119c27b28293056724d9522f26d945'
        push_public_key_hash = '14' + public_key_hash
        uncompressed_public_key = '04b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb25e01fc8fde47c96c98a4f3a8123e33a38a50cf9025cc8c4494a518f991792bb7'
        push_uncompressed_public_key = '41' + uncompressed_public_key
        p2wsh_p2pk_script_hash = 'd8590cf8ea0674cf3d49fd7ca249b85ef7485dea62c138468bddeb20cd6519f7'

        # below are test cases for all of the standard transaction types

        self.log.info("- P2PK")
        # <pubkey> OP_CHECKSIG
        rpc_result = self.nodes[0].decodescript(push_public_key + 'ac')
        assert_equal(public_key + ' OP_CHECKSIG', rpc_result['asm'])
        assert_equal('pubkey', rpc_result['type'])
        # P2PK is translated to P2WPKH
        assert_equal('0 ' + public_key_hash, rpc_result['segwit']['asm'])

        self.log.info("- P2PKH")
        # OP_DUP OP_HASH160 <PubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
        rpc_result = self.nodes[0].decodescript('76a9' + push_public_key_hash + '88ac')
        assert_equal('pubkeyhash', rpc_result['type'])
        assert_equal('OP_DUP OP_HASH160 ' + public_key_hash + ' OP_EQUALVERIFY OP_CHECKSIG', rpc_result['asm'])
        # P2PKH is translated to P2WPKH
        assert_equal('witness_v0_keyhash', rpc_result['segwit']['type'])
        assert_equal('0 ' + public_key_hash, rpc_result['segwit']['asm'])

        self.log.info("- multisig")
        # <m> <A pubkey> <B pubkey> <C pubkey> <n> OP_CHECKMULTISIG
        # just imagine that the pub keys used below are different.
        # for our purposes here it does not matter that they are the same even though it is unrealistic.
        multisig_script = '52' + push_public_key + push_public_key + push_public_key + '53ae'
        rpc_result = self.nodes[0].decodescript(multisig_script)
        assert_equal('multisig', rpc_result['type'])
        assert_equal('2 ' + public_key + ' ' + public_key + ' ' + public_key +  ' 3 OP_CHECKMULTISIG', rpc_result['asm'])
        # multisig in P2WSH
        multisig_script_hash = sha256(bytes.fromhex(multisig_script)).hex()
        assert_equal('witness_v0_scripthash', rpc_result['segwit']['type'])
        assert_equal('0 ' + multisig_script_hash, rpc_result['segwit']['asm'])

        self.log.info ("- P2SH")
        # OP_HASH160 <Hash160(redeemScript)> OP_EQUAL.
        # push_public_key_hash here should actually be the hash of a redeem script.
        # but this works the same for purposes of this test.
        rpc_result = self.nodes[0].decodescript('a9' + push_public_key_hash + '87')
        assert_equal('scripthash', rpc_result['type'])
        assert_equal('OP_HASH160 ' + public_key_hash + ' OP_EQUAL', rpc_result['asm'])
        # P2SH does not work in segwit secripts. decodescript should not return a result for it.
        assert 'segwit' not in rpc_result

        self.log.info("- null data")
        # use a signature look-alike here to make sure that we do not decode random data as a signature.
        # this matters if/when signature sighash decoding comes along.
        # would want to make sure that no such decoding takes place in this case.
        signature_imposter = '48304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        # OP_RETURN <data>
        rpc_result = self.nodes[0].decodescript('6a' + signature_imposter)
        assert_equal('nulldata', rpc_result['type'])
        assert_equal('OP_RETURN ' + signature_imposter[2:], rpc_result['asm'])

        self.log.info("- CLTV redeem script")
        # redeem scripts are in-effect scriptPubKey scripts, so adding a test here.
        # OP_NOP2 is also known as OP_CHECKLOCKTIMEVERIFY.
        # just imagine that the pub keys used below are different.
        # for our purposes here it does not matter that they are the same even though it is unrealistic.
        #
        # OP_IF
        #   <receiver-pubkey> OP_CHECKSIGVERIFY
        # OP_ELSE
        #   <lock-until> OP_CHECKLOCKTIMEVERIFY OP_DROP
        # OP_ENDIF
        # <sender-pubkey> OP_CHECKSIG
        #
        # lock until block 500,000
        cltv_script = '63' + push_public_key + 'ad670320a107b17568' + push_public_key + 'ac'
        rpc_result = self.nodes[0].decodescript(cltv_script)
        assert_equal('nonstandard', rpc_result['type'])
        assert_equal('OP_IF ' + public_key + ' OP_CHECKSIGVERIFY OP_ELSE 500000 OP_CHECKLOCKTIMEVERIFY OP_DROP OP_ENDIF ' + public_key + ' OP_CHECKSIG', rpc_result['asm'])
        # CLTV script in P2WSH
        cltv_script_hash = sha256(bytes.fromhex(cltv_script)).hex()
        assert_equal('0 ' + cltv_script_hash, rpc_result['segwit']['asm'])

        self.log.info("- P2PK with uncompressed pubkey")
        # <pubkey> OP_CHECKSIG
        rpc_result = self.nodes[0].decodescript(push_uncompressed_public_key + 'ac')
        assert_equal('pubkey', rpc_result['type'])
        assert_equal(uncompressed_public_key + ' OP_CHECKSIG', rpc_result['asm'])
        # uncompressed pubkeys are invalid for checksigs in segwit scripts.
        # decodescript should not return a P2WPKH equivalent.
        assert 'segwit' not in rpc_result

        self.log.info("- multisig with uncompressed pubkey")
        # <m> <A pubkey> <B pubkey> <n> OP_CHECKMULTISIG
        # just imagine that the pub keys used below are different.
        # the purpose of this test is to check that a segwit script is not returned for bare multisig scripts
        # with an uncompressed pubkey in them.
        rpc_result = self.nodes[0].decodescript('52' + push_public_key + push_uncompressed_public_key +'52ae')
        assert_equal('multisig', rpc_result['type'])
        assert_equal('2 ' + public_key + ' ' + uncompressed_public_key + ' 2 OP_CHECKMULTISIG', rpc_result['asm'])
        # uncompressed pubkeys are invalid for checksigs in segwit scripts.
        # decodescript should not return a P2WPKH equivalent.
        assert 'segwit' not in rpc_result

        self.log.info("- P2WPKH")
        # 0 <PubKeyHash>
        rpc_result = self.nodes[0].decodescript('00' + push_public_key_hash)
        assert_equal('witness_v0_keyhash', rpc_result['type'])
        assert_equal('0 ' + public_key_hash, rpc_result['asm'])
        # segwit scripts do not work nested into each other.
        # a nested segwit script should not be returned in the results.
        assert 'segwit' not in rpc_result

        self.log.info("- P2WSH")
        # 0 <ScriptHash>
        # even though this hash is of a P2PK script which is better used as bare P2WPKH, it should not matter
        # for the purpose of this test.
        rpc_result = self.nodes[0].decodescript('0020' + p2wsh_p2pk_script_hash)
        assert_equal('witness_v0_scripthash', rpc_result['type'])
        assert_equal('0 ' + p2wsh_p2pk_script_hash, rpc_result['asm'])
        # segwit scripts do not work nested into each other.
        # a nested segwit script should not be returned in the results.
        assert 'segwit' not in rpc_result

        self.log.info("- P2TR")
        # 1 <x-only pubkey>
        xonly_public_key = '01'*32  # first ever P2TR output on mainnet
        rpc_result = self.nodes[0].decodescript('5120' + xonly_public_key)
        assert_equal('witness_v1_taproot', rpc_result['type'])
        assert_equal('1 ' + xonly_public_key, rpc_result['asm'])
        assert 'segwit' not in rpc_result

        self.log.info("- P2A (anchor)")
        # 1 <4e73>
        witprog_hex = '4e73'
        rpc_result = self.nodes[0].decodescript('5102' + witprog_hex)
        assert_equal('anchor', rpc_result['type'])
        # in the disassembly, the witness program is shown as single decimal due to its small size
        witprog_as_decimal = int.from_bytes(bytes.fromhex(witprog_hex), 'little')
        assert_equal(f'1 {witprog_as_decimal}', rpc_result['asm'])
        assert_equal('bcrt1pfeesnyr2tx', rpc_result['address'])

    def decoderawtransaction_asm_sighashtype(self):
        """Test decoding scripts via RPC command "decoderawtransaction".

        This test is in with the "decodescript" tests because they are testing the same "asm" script decodes.
        """

        self.log.info("- various mainnet txs")
        # this test case uses a mainnet transaction that has a P2SH input and both P2PKH and P2SH outputs.
        tx = '0100000001696a20784a2c70143f634e95227dbdfdf0ecd51647052e70854512235f5986ca010000008a47304402207174775824bec6c2700023309a168231ec80b82c6069282f5133e6f11cbb04460220570edc55c7c5da2ca687ebd0372d3546ebc3f810516a002350cac72dfe192dfb014104d3f898e6487787910a690410b7a917ef198905c27fb9d3b0a42da12aceae0544fc7088d239d9a48f2828a15a09e84043001f27cc80d162cb95404e1210161536ffffffff0100e1f505000000001976a914eb6c6e0cdb2d256a32d97b8df1fc75d1920d9bca88ac00000000'
        rpc_result = self.nodes[0].decoderawtransaction(tx)
        assert_equal('304402207174775824bec6c2700023309a168231ec80b82c6069282f5133e6f11cbb04460220570edc55c7c5da2ca687ebd0372d3546ebc3f810516a002350cac72dfe192dfb[ALL] 04d3f898e6487787910a690410b7a917ef198905c27fb9d3b0a42da12aceae0544fc7088d239d9a48f2828a15a09e84043001f27cc80d162cb95404e1210161536', rpc_result['vin'][0]['scriptSig']['asm'])

        # this test case uses a mainnet transaction that has a P2SH input and both P2PKH and P2SH outputs.
        # it's from James D'Angelo's awesome introductory videos about multisig: https://www.youtube.com/watch?v=zIbUSaZBJgU and https://www.youtube.com/watch?v=OSA1pwlaypc
        # verify that we have not altered scriptPubKey decoding.
        tx = '01000000018d1f5635abd06e2c7e2ddf58dc85b3de111e4ad6e0ab51bb0dcf5e84126d927300000000fdfe0000483045022100ae3b4e589dfc9d48cb82d41008dc5fa6a86f94d5c54f9935531924602730ab8002202f88cf464414c4ed9fa11b773c5ee944f66e9b05cc1e51d97abc22ce098937ea01483045022100b44883be035600e9328a01b66c7d8439b74db64187e76b99a68f7893b701d5380220225bf286493e4c4adcf928c40f785422572eb232f84a0b83b0dea823c3a19c75014c695221020743d44be989540d27b1b4bbbcfd17721c337cb6bc9af20eb8a32520b393532f2102c0120a1dda9e51a938d39ddd9fe0ebc45ea97e1d27a7cbd671d5431416d3dd87210213820eb3d5f509d7438c9eeecb4157b2f595105e7cd564b3cdbb9ead3da41eed53aeffffffff02611e0000000000001976a914dc863734a218bfe83ef770ee9d41a27f824a6e5688acee2a02000000000017a9142a5edea39971049a540474c6a99edf0aa4074c588700000000'
        rpc_result = self.nodes[0].decoderawtransaction(tx)
        assert_equal('8e3730608c3b0bb5df54f09076e196bc292a8e39a78e73b44b6ba08c78f5cbb0', rpc_result['txid'])
        assert_equal('0 3045022100ae3b4e589dfc9d48cb82d41008dc5fa6a86f94d5c54f9935531924602730ab8002202f88cf464414c4ed9fa11b773c5ee944f66e9b05cc1e51d97abc22ce098937ea[ALL] 3045022100b44883be035600e9328a01b66c7d8439b74db64187e76b99a68f7893b701d5380220225bf286493e4c4adcf928c40f785422572eb232f84a0b83b0dea823c3a19c75[ALL] 5221020743d44be989540d27b1b4bbbcfd17721c337cb6bc9af20eb8a32520b393532f2102c0120a1dda9e51a938d39ddd9fe0ebc45ea97e1d27a7cbd671d5431416d3dd87210213820eb3d5f509d7438c9eeecb4157b2f595105e7cd564b3cdbb9ead3da41eed53ae', rpc_result['vin'][0]['scriptSig']['asm'])
        assert_equal('OP_DUP OP_HASH160 dc863734a218bfe83ef770ee9d41a27f824a6e56 OP_EQUALVERIFY OP_CHECKSIG', rpc_result['vout'][0]['scriptPubKey']['asm'])
        assert_equal('OP_HASH160 2a5edea39971049a540474c6a99edf0aa4074c58 OP_EQUAL', rpc_result['vout'][1]['scriptPubKey']['asm'])
        txSave = tx_from_hex(tx)

        self.log.info("- tx not passing DER signature checks")
        # make sure that a specifically crafted op_return value will not pass all the IsDERSignature checks and then get decoded as a sighash type
        tx = '01000000015ded05872fdbda629c7d3d02b194763ce3b9b1535ea884e3c8e765d42e316724020000006b48304502204c10d4064885c42638cbff3585915b322de33762598321145ba033fc796971e2022100bb153ad3baa8b757e30a2175bd32852d2e1cb9080f84d7e32fcdfd667934ef1b012103163c0ff73511ea1743fb5b98384a2ff09dd06949488028fd819f4d83f56264efffffffff0200000000000000000b6a0930060201000201000180380100000000001976a9141cabd296e753837c086da7a45a6c2fe0d49d7b7b88ac00000000'
        rpc_result = self.nodes[0].decoderawtransaction(tx)
        assert_equal('OP_RETURN 300602010002010001', rpc_result['vout'][0]['scriptPubKey']['asm'])

        self.log.info("- tx passing DER signature checks")
        # verify that we have not altered scriptPubKey processing even of a specially crafted P2PKH pubkeyhash and P2SH redeem script hash that is made to pass the der signature checks
        tx = '01000000018d1f5635abd06e2c7e2ddf58dc85b3de111e4ad6e0ab51bb0dcf5e84126d927300000000fdfe0000483045022100ae3b4e589dfc9d48cb82d41008dc5fa6a86f94d5c54f9935531924602730ab8002202f88cf464414c4ed9fa11b773c5ee944f66e9b05cc1e51d97abc22ce098937ea01483045022100b44883be035600e9328a01b66c7d8439b74db64187e76b99a68f7893b701d5380220225bf286493e4c4adcf928c40f785422572eb232f84a0b83b0dea823c3a19c75014c695221020743d44be989540d27b1b4bbbcfd17721c337cb6bc9af20eb8a32520b393532f2102c0120a1dda9e51a938d39ddd9fe0ebc45ea97e1d27a7cbd671d5431416d3dd87210213820eb3d5f509d7438c9eeecb4157b2f595105e7cd564b3cdbb9ead3da41eed53aeffffffff02611e0000000000001976a914301102070101010101010102060101010101010188acee2a02000000000017a91430110207010101010101010206010101010101018700000000'
        rpc_result = self.nodes[0].decoderawtransaction(tx)
        assert_equal('OP_DUP OP_HASH160 3011020701010101010101020601010101010101 OP_EQUALVERIFY OP_CHECKSIG', rpc_result['vout'][0]['scriptPubKey']['asm'])
        assert_equal('OP_HASH160 3011020701010101010101020601010101010101 OP_EQUAL', rpc_result['vout'][1]['scriptPubKey']['asm'])

        # some more full transaction tests of varying specific scriptSigs. used instead of
        # tests in decodescript_script_sig because the decodescript RPC is specifically
        # for working on scriptPubKeys (argh!).
        push_signature = txSave.vin[0].scriptSig.hex()[2:(0x48*2+4)]
        signature = push_signature[2:]
        der_signature = signature[:-2]
        signature_sighash_decoded = der_signature + '[ALL]'
        signature_2 = der_signature + '82'
        push_signature_2 = '48' + signature_2
        signature_2_sighash_decoded = der_signature + '[NONE|ANYONECANPAY]'

        self.log.info("- P2PK scriptSig")
        txSave.vin[0].scriptSig = bytes.fromhex(push_signature)
        rpc_result = self.nodes[0].decoderawtransaction(txSave.serialize().hex())
        assert_equal(signature_sighash_decoded, rpc_result['vin'][0]['scriptSig']['asm'])

        # make sure that the sighash decodes come out correctly for a more complex / lesser used case.
        txSave.vin[0].scriptSig = bytes.fromhex(push_signature_2)
        rpc_result = self.nodes[0].decoderawtransaction(txSave.serialize().hex())
        assert_equal(signature_2_sighash_decoded, rpc_result['vin'][0]['scriptSig']['asm'])

        self.log.info("- multisig scriptSig")
        txSave.vin[0].scriptSig = bytes.fromhex('00' + push_signature + push_signature_2)
        rpc_result = self.nodes[0].decoderawtransaction(txSave.serialize().hex())
        assert_equal('0 ' + signature_sighash_decoded + ' ' + signature_2_sighash_decoded, rpc_result['vin'][0]['scriptSig']['asm'])

        self.log.info("- scriptSig that contains more than push operations")
        # in fact, it contains an OP_RETURN with data specially crafted to cause improper decode if the code does not catch it.
        txSave.vin[0].scriptSig = bytes.fromhex('6a143011020701010101010101020601010101010101')
        rpc_result = self.nodes[0].decoderawtransaction(txSave.serialize().hex())
        assert_equal('OP_RETURN 3011020701010101010101020601010101010101', rpc_result['vin'][0]['scriptSig']['asm'])

    def decodescript_datadriven_tests(self):
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_decodescript.json'), encoding='utf-8') as f:
            dd_tests = json.load(f)

        for script, result in dd_tests:
            rpc_result = self.nodes[0].decodescript(script)
            assert_equal(result, rpc_result)

    def decodescript_miniscript(self):
        """Check that a Miniscript is decoded when possible under P2WSH context."""
        # Sourced from https://github.com/tortoisecoin/tortoisecoin/pull/27037#issuecomment-1416151907.
        # Miniscript-compatible offered HTLC
        res = self.nodes[0].decodescript("82012088a914ffffffffffffffffffffffffffffffffffffffff88210250929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0ad51b2")
        assert res["segwit"]["desc"] == "wsh(and_v(and_v(v:hash160(ffffffffffffffffffffffffffffffffffffffff),v:pk(0250929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0)),older(1)))#gm8xz4fl"
        # Miniscript-incompatible offered HTLC
        res = self.nodes[0].decodescript("82012088a914ffffffffffffffffffffffffffffffffffffffff882102ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffacb2")
        assert res["segwit"]["desc"] == "addr(bcrt1q73qyfypp47hvgnkjqnav0j3k2lq3v76wg22dk8tmwuz5sfgv66xsvxg6uu)#9p3q328s"
        # Miniscript-compatible multisig bigger than 520 byte P2SH limit.
        res = self.nodes[0].decodescript("5b21020e0338c96a8870479f2396c373cc7696ba124e8635d41b0ea581112b678172612102675333a4e4b8fb51d9d4e22fa5a8eaced3fdac8a8cbf9be8c030f75712e6af992102896807d54bc55c24981f24a453c60ad3e8993d693732288068a23df3d9f50d4821029e51a5ef5db3137051de8323b001749932f2ff0d34c82e96a2c2461de96ae56c2102a4e1a9638d46923272c266631d94d36bdb03a64ee0e14c7518e49d2f29bc401021031c41fdbcebe17bec8d49816e00ca1b5ac34766b91c9f2ac37d39c63e5e008afb2103079e252e85abffd3c401a69b087e590a9b86f33f574f08129ccbd3521ecf516b2103111cf405b627e22135b3b3733a4a34aa5723fb0f58379a16d32861bf576b0ec2210318f331b3e5d38156da6633b31929c5b220349859cc9ca3d33fb4e68aa08401742103230dae6b4ac93480aeab26d000841298e3b8f6157028e47b0897c1e025165de121035abff4281ff00660f99ab27bb53e6b33689c2cd8dcd364bc3c90ca5aea0d71a62103bd45cddfacf2083b14310ae4a84e25de61e451637346325222747b157446614c2103cc297026b06c71cbfa52089149157b5ff23de027ac5ab781800a578192d175462103d3bde5d63bdb3a6379b461be64dad45eabff42f758543a9645afd42f6d4248282103ed1e8d5109c9ed66f7941bc53cc71137baa76d50d274bda8d5e8ffbd6e61fe9a5fae736402c00fb269522103aab896d53a8e7d6433137bbba940f9c521e085dd07e60994579b64a6d992cf79210291b7d0b1b692f8f524516ed950872e5da10fb1b808b5a526dedc6fed1cf29807210386aa9372fbab374593466bc5451dc59954e90787f08060964d95c87ef34ca5bb53ae68")
        assert_equal(res["segwit"]["desc"], "wsh(or_d(multi(11,020e0338c96a8870479f2396c373cc7696ba124e8635d41b0ea581112b67817261,02675333a4e4b8fb51d9d4e22fa5a8eaced3fdac8a8cbf9be8c030f75712e6af99,02896807d54bc55c24981f24a453c60ad3e8993d693732288068a23df3d9f50d48,029e51a5ef5db3137051de8323b001749932f2ff0d34c82e96a2c2461de96ae56c,02a4e1a9638d46923272c266631d94d36bdb03a64ee0e14c7518e49d2f29bc4010,031c41fdbcebe17bec8d49816e00ca1b5ac34766b91c9f2ac37d39c63e5e008afb,03079e252e85abffd3c401a69b087e590a9b86f33f574f08129ccbd3521ecf516b,03111cf405b627e22135b3b3733a4a34aa5723fb0f58379a16d32861bf576b0ec2,0318f331b3e5d38156da6633b31929c5b220349859cc9ca3d33fb4e68aa0840174,03230dae6b4ac93480aeab26d000841298e3b8f6157028e47b0897c1e025165de1,035abff4281ff00660f99ab27bb53e6b33689c2cd8dcd364bc3c90ca5aea0d71a6,03bd45cddfacf2083b14310ae4a84e25de61e451637346325222747b157446614c,03cc297026b06c71cbfa52089149157b5ff23de027ac5ab781800a578192d17546,03d3bde5d63bdb3a6379b461be64dad45eabff42f758543a9645afd42f6d424828,03ed1e8d5109c9ed66f7941bc53cc71137baa76d50d274bda8d5e8ffbd6e61fe9a),and_v(v:older(4032),multi(2,03aab896d53a8e7d6433137bbba940f9c521e085dd07e60994579b64a6d992cf79,0291b7d0b1b692f8f524516ed950872e5da10fb1b808b5a526dedc6fed1cf29807,0386aa9372fbab374593466bc5451dc59954e90787f08060964d95c87ef34ca5bb))))#7jwwklk4")

    def run_test(self):
        self.log.info("Test decoding of standard input scripts [scriptSig]")
        self.decodescript_script_sig()
        self.log.info("Test decoding of standard output scripts [scriptPubKey]")
        self.decodescript_script_pub_key()
        self.log.info("Test 'asm' script decoding of transactions")
        self.decoderawtransaction_asm_sighashtype()
        self.log.info("Data-driven tests")
        self.decodescript_datadriven_tests()
        self.log.info("Miniscript descriptor decoding")
        self.decodescript_miniscript()

if __name__ == '__main__':
    DecodeScriptTest(__file__).main()
