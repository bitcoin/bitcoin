#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the SegWit changeover logic."""

from decimal import Decimal

from test_framework.address import (
    key_to_p2pkh,
    program_to_witness,
    script_to_p2sh,
    script_to_p2sh_p2wsh,
    script_to_p2wsh,
)
from test_framework.blocktools import (
    send_to_witness,
    witness_script,
)
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    sha256,
    tx_from_hex,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_1,
    OP_2,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DROP,
    OP_DUP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    OP_TRUE,
    hash160,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_is_hex_string,
    assert_raises_rpc_error,
    hex_str_to_bytes,
    try_rpc,
)

NODE_0 = 0
NODE_2 = 2
P2WPKH = 0
P2WSH = 1

def getutxo(txid):
    utxo = {}
    utxo["vout"] = 0
    utxo["txid"] = txid
    return utxo

def find_spendable_utxo(node, min_value):
    for utxo in node.listunspent(query_options={'minimumAmount': min_value}):
        if utxo['spendable']:
            return utxo

    raise AssertionError("Unspent output equal or higher than %s not found" % min_value)

txs_mined = {} # txindex from txid to blockhash

class SegWitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        # This test tests SegWit both pre and post-activation, so use the normal BIP9 activation.
        self.extra_args = [
            [
                "-acceptnonstdtxn=1",
                "-rpcserialversion=0",
                "-segwitheight=432",
                "-addresstype=legacy",
            ],
            [
                "-acceptnonstdtxn=1",
                "-rpcserialversion=1",
                "-segwitheight=432",
                "-addresstype=legacy",
            ],
            [
                "-acceptnonstdtxn=1",
                "-segwitheight=432",
                "-addresstype=legacy",
            ],
        ]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        super().setup_network()
        self.connect_nodes(0, 2)
        self.sync_all()

    def success_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 2)
        self.sync_blocks()

    def skip_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 1)
        self.sync_blocks()

    def fail_accept(self, node, error_msg, txid, sign, redeem_script=""):
        assert_raises_rpc_error(-26, error_msg, send_to_witness, use_p2wsh=1, node=node, utxo=getutxo(txid), pubkey=self.pubkey[0], encode_p2sh=False, amount=Decimal("49.998"), sign=sign, insert_redeem_script=redeem_script)

    def run_test(self):
        self.nodes[0].generate(161)  # block 161

        self.log.info("Verify sigops are counted in GBT with pre-BIP141 rules before the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        assert tmpl['sizelimit'] == 1000000
        assert 'weightlimit' not in tmpl
        assert tmpl['sigoplimit'] == 20000
        assert tmpl['transactions'][0]['hash'] == txid
        assert tmpl['transactions'][0]['sigops'] == 2
        assert '!segwit' not in tmpl['rules']
        self.nodes[0].generate(1)  # block 162

        balance_presetup = self.nodes[0].getbalance()
        self.pubkey = []
        p2sh_ids = [] # p2sh_ids[NODE][TYPE] is an array of txids that spend to P2WPKH (TYPE=0) or P2WSH (TYPE=1) scripts to an address for NODE embedded in p2sh
        wit_ids = [] # wit_ids[NODE][TYPE] is an array of txids that spend to P2WPKH (TYPE=0) or P2WSH (TYPE=1) scripts to an address for NODE via bare witness
        for i in range(3):
            newaddress = self.nodes[i].getnewaddress()
            self.pubkey.append(self.nodes[i].getaddressinfo(newaddress)["pubkey"])
            multiscript = CScript([OP_1, hex_str_to_bytes(self.pubkey[-1]), OP_1, OP_CHECKMULTISIG])
            p2sh_ms_addr = self.nodes[i].addmultisigaddress(1, [self.pubkey[-1]], '', 'p2sh-segwit')['address']
            bip173_ms_addr = self.nodes[i].addmultisigaddress(1, [self.pubkey[-1]], '', 'bech32')['address']
            assert_equal(p2sh_ms_addr, script_to_p2sh_p2wsh(multiscript))
            assert_equal(bip173_ms_addr, script_to_p2wsh(multiscript))
            p2sh_ids.append([])
            wit_ids.append([])
            for _ in range(2):
                p2sh_ids[i].append([])
                wit_ids[i].append([])

        for _ in range(5):
            for n in range(3):
                for v in range(2):
                    wit_ids[n][v].append(send_to_witness(v, self.nodes[0], find_spendable_utxo(self.nodes[0], 50), self.pubkey[n], False, Decimal("49.999")))
                    p2sh_ids[n][v].append(send_to_witness(v, self.nodes[0], find_spendable_utxo(self.nodes[0], 50), self.pubkey[n], True, Decimal("49.999")))

        self.nodes[0].generate(1)  # block 163
        self.sync_blocks()

        # Make sure all nodes recognize the transactions as theirs
        assert_equal(self.nodes[0].getbalance(), balance_presetup - 60 * 50 + 20 * Decimal("49.999") + 50)
        assert_equal(self.nodes[1].getbalance(), 20 * Decimal("49.999"))
        assert_equal(self.nodes[2].getbalance(), 20 * Decimal("49.999"))

        self.nodes[0].generate(260)  # block 423
        self.sync_blocks()

        self.log.info("Verify witness txs are skipped for mining before the fork")
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][P2WPKH][0], True)  # block 424
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][P2WSH][0], True)  # block 425
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][P2WPKH][0], True)  # block 426
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][P2WSH][0], True)  # block 427

        self.log.info("Verify unsigned p2sh witness txs without a redeem script are invalid")
        self.fail_accept(self.nodes[2], "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)", p2sh_ids[NODE_2][P2WPKH][1], sign=False)
        self.fail_accept(self.nodes[2], "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)", p2sh_ids[NODE_2][P2WSH][1], sign=False)

        self.nodes[2].generate(4)  # blocks 428-431

        self.log.info("Verify previous witness txs skipped for mining can now be mined")
        assert_equal(len(self.nodes[2].getrawmempool()), 4)
        blockhash = self.nodes[2].generate(1)[0]  # block 432 (first block with new rules; 432 = 144 * 3)
        self.sync_blocks()
        assert_equal(len(self.nodes[2].getrawmempool()), 0)
        segwit_tx_list = self.nodes[2].getblock(blockhash)["tx"]
        assert_equal(len(segwit_tx_list), 5)

        self.log.info("Verify default node can't accept txs with missing witness")
        # unsigned, no scriptsig
        self.fail_accept(self.nodes[0], "non-mandatory-script-verify-flag (Witness program hash mismatch)", wit_ids[NODE_0][P2WPKH][0], sign=False)
        self.fail_accept(self.nodes[0], "non-mandatory-script-verify-flag (Witness program was passed an empty witness)", wit_ids[NODE_0][P2WSH][0], sign=False)
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)", p2sh_ids[NODE_0][P2WPKH][0], sign=False)
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)", p2sh_ids[NODE_0][P2WSH][0], sign=False)
        # unsigned with redeem script
        self.fail_accept(self.nodes[0], "non-mandatory-script-verify-flag (Witness program hash mismatch)", p2sh_ids[NODE_0][P2WPKH][0], sign=False, redeem_script=witness_script(False, self.pubkey[0]))
        self.fail_accept(self.nodes[0], "non-mandatory-script-verify-flag (Witness program was passed an empty witness)", p2sh_ids[NODE_0][P2WSH][0], sign=False, redeem_script=witness_script(True, self.pubkey[0]))

        self.log.info("Verify block and transaction serialization rpcs return differing serializations depending on rpc serialization flag")
        assert self.nodes[2].getblock(blockhash, False) != self.nodes[0].getblock(blockhash, False)
        assert self.nodes[1].getblock(blockhash, False) == self.nodes[2].getblock(blockhash, False)

        for tx_id in segwit_tx_list:
            tx = tx_from_hex(self.nodes[2].gettransaction(tx_id)["hex"])
            assert self.nodes[2].getrawtransaction(tx_id, False, blockhash) != self.nodes[0].getrawtransaction(tx_id, False, blockhash)
            assert self.nodes[1].getrawtransaction(tx_id, False, blockhash) == self.nodes[2].getrawtransaction(tx_id, False, blockhash)
            assert self.nodes[0].getrawtransaction(tx_id, False, blockhash) != self.nodes[2].gettransaction(tx_id)["hex"]
            assert self.nodes[1].getrawtransaction(tx_id, False, blockhash) == self.nodes[2].gettransaction(tx_id)["hex"]
            assert self.nodes[0].getrawtransaction(tx_id, False, blockhash) == tx.serialize_without_witness().hex()

        # Coinbase contains the witness commitment nonce, check that RPC shows us
        coinbase_txid = self.nodes[2].getblock(blockhash)['tx'][0]
        coinbase_tx = self.nodes[2].gettransaction(txid=coinbase_txid, verbose=True)
        witnesses = coinbase_tx["decoded"]["vin"][0]["txinwitness"]
        assert_equal(len(witnesses), 1)
        assert_is_hex_string(witnesses[0])
        assert_equal(witnesses[0], '00'*32)

        self.log.info("Verify witness txs without witness data are invalid after the fork")
        self.fail_accept(self.nodes[2], 'non-mandatory-script-verify-flag (Witness program hash mismatch)', wit_ids[NODE_2][P2WPKH][2], sign=False)
        self.fail_accept(self.nodes[2], 'non-mandatory-script-verify-flag (Witness program was passed an empty witness)', wit_ids[NODE_2][P2WSH][2], sign=False)
        self.fail_accept(self.nodes[2], 'non-mandatory-script-verify-flag (Witness program hash mismatch)', p2sh_ids[NODE_2][P2WPKH][2], sign=False, redeem_script=witness_script(False, self.pubkey[2]))
        self.fail_accept(self.nodes[2], 'non-mandatory-script-verify-flag (Witness program was passed an empty witness)', p2sh_ids[NODE_2][P2WSH][2], sign=False, redeem_script=witness_script(True, self.pubkey[2]))

        self.log.info("Verify default node can now use witness txs")
        self.success_mine(self.nodes[0], wit_ids[NODE_0][P2WPKH][0], True)  # block 432
        self.success_mine(self.nodes[0], wit_ids[NODE_0][P2WSH][0], True)  # block 433
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][P2WPKH][0], True)  # block 434
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][P2WSH][0], True)  # block 435

        self.log.info("Verify sigops are counted in GBT with BIP141 rules after the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({'rules': ['segwit']})
        assert tmpl['sizelimit'] >= 3999577  # actual maximum size is lower due to minimum mandatory non-witness data
        assert tmpl['weightlimit'] == 4000000
        assert tmpl['sigoplimit'] == 80000
        assert tmpl['transactions'][0]['txid'] == txid
        assert tmpl['transactions'][0]['sigops'] == 8
        assert '!segwit' in tmpl['rules']

        self.nodes[0].generate(1)  # Mine a block to clear the gbt cache

        self.log.info("Non-segwit miners are able to use GBT response after activation.")
        # Create a 3-tx chain: tx1 (non-segwit input, paying to a segwit output) ->
        #                      tx2 (segwit input, paying to a non-segwit output) ->
        #                      tx3 (non-segwit input, paying to a non-segwit output).
        # tx1 is allowed to appear in the block, but no others.
        txid1 = send_to_witness(1, self.nodes[0], find_spendable_utxo(self.nodes[0], 50), self.pubkey[0], False, Decimal("49.996"))
        hex_tx = self.nodes[0].gettransaction(txid)['hex']
        tx = tx_from_hex(hex_tx)
        assert tx.wit.is_null()  # This should not be a segwit input
        assert txid1 in self.nodes[0].getrawmempool()

        tx1_hex = self.nodes[0].gettransaction(txid1)['hex']
        tx1 = tx_from_hex(tx1_hex)

        # Check that wtxid is properly reported in mempool entry (txid1)
        assert_equal(int(self.nodes[0].getmempoolentry(txid1)["wtxid"], 16), tx1.calc_sha256(True))

        # Check that weight and vsize are properly reported in mempool entry (txid1)
        assert_equal(self.nodes[0].getmempoolentry(txid1)["vsize"], (self.nodes[0].getmempoolentry(txid1)["weight"] + 3) // 4)
        assert_equal(self.nodes[0].getmempoolentry(txid1)["weight"], len(tx1.serialize_without_witness())*3 + len(tx1.serialize_with_witness()))

        # Now create tx2, which will spend from txid1.
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(txid1, 16), 0), b''))
        tx.vout.append(CTxOut(int(49.99 * COIN), CScript([OP_TRUE, OP_DROP] * 15 + [OP_TRUE])))
        tx2_hex = self.nodes[0].signrawtransactionwithwallet(tx.serialize().hex())['hex']
        txid2 = self.nodes[0].sendrawtransaction(tx2_hex)
        tx = tx_from_hex(tx2_hex)
        assert not tx.wit.is_null()

        # Check that wtxid is properly reported in mempool entry (txid2)
        assert_equal(int(self.nodes[0].getmempoolentry(txid2)["wtxid"], 16), tx.calc_sha256(True))

        # Check that weight and vsize are properly reported in mempool entry (txid2)
        assert_equal(self.nodes[0].getmempoolentry(txid2)["vsize"], (self.nodes[0].getmempoolentry(txid2)["weight"] + 3) // 4)
        assert_equal(self.nodes[0].getmempoolentry(txid2)["weight"], len(tx.serialize_without_witness())*3 + len(tx.serialize_with_witness()))

        # Now create tx3, which will spend from txid2
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(txid2, 16), 0), b""))
        tx.vout.append(CTxOut(int(49.95 * COIN), CScript([OP_TRUE, OP_DROP] * 15 + [OP_TRUE])))  # Huge fee
        tx.calc_sha256()
        txid3 = self.nodes[0].sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
        assert tx.wit.is_null()
        assert txid3 in self.nodes[0].getrawmempool()

        # Check that getblocktemplate includes all transactions.
        template = self.nodes[0].getblocktemplate({"rules": ["segwit"]})
        template_txids = [t['txid'] for t in template['transactions']]
        assert txid1 in template_txids
        assert txid2 in template_txids
        assert txid3 in template_txids

        # Check that wtxid is properly reported in mempool entry (txid3)
        assert_equal(int(self.nodes[0].getmempoolentry(txid3)["wtxid"], 16), tx.calc_sha256(True))

        # Check that weight and vsize are properly reported in mempool entry (txid3)
        assert_equal(self.nodes[0].getmempoolentry(txid3)["vsize"], (self.nodes[0].getmempoolentry(txid3)["weight"] + 3) // 4)
        assert_equal(self.nodes[0].getmempoolentry(txid3)["weight"], len(tx.serialize_without_witness())*3 + len(tx.serialize_with_witness()))

        # Mine a block to clear the gbt cache again.
        self.nodes[0].generate(1)

        self.log.info("Verify behaviour of importaddress and listunspent")

        # Some public keys to be used later
        pubkeys = [
            "0363D44AABD0F1699138239DF2F042C3282C0671CC7A76826A55C8203D90E39242",  # cPiM8Ub4heR9NBYmgVzJQiUH1if44GSBGiqaeJySuL2BKxubvgwb
            "02D3E626B3E616FC8662B489C123349FECBFC611E778E5BE739B257EAE4721E5BF",  # cPpAdHaD6VoYbW78kveN2bsvb45Q7G5PhaPApVUGwvF8VQ9brD97
            "04A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538A62F5BD8EC85C2477F39650BD391EA6250207065B2A81DA8B009FC891E898F0E",  # 91zqCU5B9sdWxzMt1ca3VzbtVm2YM6Hi5Rxn4UDtxEaN9C9nzXV
            "02A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538",  # cPQFjcVRpAUBG8BA9hzr2yEzHwKoMgLkJZBBtK9vJnvGJgMjzTbd
            "036722F784214129FEB9E8129D626324F3F6716555B603FFE8300BBCB882151228",  # cQGtcm34xiLjB1v7bkRa4V3aAc9tS2UTuBZ1UnZGeSeNy627fN66
            "0266A8396EE936BF6D99D17920DB21C6C7B1AB14C639D5CD72B300297E416FD2EC",  # cTW5mR5M45vHxXkeChZdtSPozrFwFgmEvTNnanCW6wrqwaCZ1X7K
            "0450A38BD7F0AC212FEBA77354A9B036A32E0F7C81FC4E0C5ADCA7C549C4505D2522458C2D9AE3CEFD684E039194B72C8A10F9CB9D4764AB26FCC2718D421D3B84",  # 92h2XPssjBpsJN5CqSP7v9a7cf2kgDunBC6PDFwJHMACM1rrVBJ
        ]

        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("92e6XLo5jVAVwrQKPNTs93oQco8f8sDNBcpv73Dsrs397fQtFQn")
        uncompressed_spendable_address = ["mvozP4UwyGD2mGZU4D2eMvMLPB9WkMmMQu"]
        self.nodes[0].importprivkey("cNC8eQ5dg3mFAVePDX4ddmPYpPbw41r9bm2jd1nLJT77e6RrzTRR")
        compressed_spendable_address = ["mmWQubrDomqpgSYekvsU7HWEVjLFHAakLe"]
        assert not self.nodes[0].getaddressinfo(uncompressed_spendable_address[0])['iscompressed']
        assert self.nodes[0].getaddressinfo(compressed_spendable_address[0])['iscompressed']

        self.nodes[0].importpubkey(pubkeys[0])
        compressed_solvable_address = [key_to_p2pkh(pubkeys[0])]
        self.nodes[0].importpubkey(pubkeys[1])
        compressed_solvable_address.append(key_to_p2pkh(pubkeys[1]))
        self.nodes[0].importpubkey(pubkeys[2])
        uncompressed_solvable_address = [key_to_p2pkh(pubkeys[2])]

        spendable_anytime = []                      # These outputs should be seen anytime after importprivkey and addmultisigaddress
        spendable_after_importaddress = []          # These outputs should be seen after importaddress
        solvable_after_importaddress = []           # These outputs should be seen after importaddress but not spendable
        unsolvable_after_importaddress = []         # These outputs should be unsolvable after importaddress
        solvable_anytime = []                       # These outputs should be solvable after importpubkey
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]])['address'])
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]])['address'])
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]])['address'])
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], uncompressed_solvable_address[0]])['address'])
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]])['address'])
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], compressed_solvable_address[1]])['address'])

        # Test multisig_without_privkey
        # We have 2 public keys without private keys, use addmultisigaddress to add to wallet.
        # Money sent to P2SH of multisig of this should only be seen after importaddress with the BASE58 P2SH address.

        multisig_without_privkey_address = self.nodes[0].addmultisigaddress(2, [pubkeys[3], pubkeys[4]])['address']
        script = CScript([OP_2, hex_str_to_bytes(pubkeys[3]), hex_str_to_bytes(pubkeys[4]), OP_2, OP_CHECKMULTISIG])
        solvable_after_importaddress.append(CScript([OP_HASH160, hash160(script), OP_EQUAL]))

        for i in compressed_spendable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # p2sh multisig with compressed keys should always be spendable
                spendable_anytime.extend([p2sh])
                # bare multisig can be watched and signed, but is not treated as ours
                solvable_after_importaddress.extend([bare])
                # P2WSH and P2SH(P2WSH) multisig with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])
                # P2WPKH and P2SH_P2WPKH with compressed keys should always be spendable
                spendable_anytime.extend([p2wpkh, p2sh_p2wpkh])

        for i in uncompressed_spendable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # p2sh multisig with uncompressed keys should always be spendable
                spendable_anytime.extend([p2sh])
                # bare multisig can be watched and signed, but is not treated as ours
                solvable_after_importaddress.extend([bare])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK and P2SH_P2PKH are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # Witness output types with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                # Multisig without private is not seen after addmultisigaddress, but seen after importaddress
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                solvable_after_importaddress.extend([bare, p2sh, p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH, P2PK, P2WPKH and P2SH_P2WPKH with compressed keys should always be seen
                solvable_anytime.extend([p2pkh, p2pk, p2wpkh, p2sh_p2wpkh])
                # P2SH_P2PK, P2SH_P2PKH with compressed keys are seen after direct importaddress
                solvable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in uncompressed_solvable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # Base uncompressed multisig without private is not seen after addmultisigaddress, but seen after importaddress
                solvable_after_importaddress.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be seen
                solvable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH with uncompressed keys are seen after direct importaddress
                solvable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # Witness output types with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        op1 = CScript([OP_1])
        op0 = CScript([OP_0])
        # 2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe is the P2SH(P2PKH) version of mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V
        unsolvable_address_key = hex_str_to_bytes("02341AEC7587A51CDE5279E0630A531AEA2615A9F80B17E8D9376327BAEAA59E3D")
        unsolvablep2pkh = CScript([OP_DUP, OP_HASH160, hash160(unsolvable_address_key), OP_EQUALVERIFY, OP_CHECKSIG])
        unsolvablep2wshp2pkh = CScript([OP_0, sha256(unsolvablep2pkh)])
        p2shop0 = CScript([OP_HASH160, hash160(op0), OP_EQUAL])
        p2wshop1 = CScript([OP_0, sha256(op1)])
        unsolvable_after_importaddress.append(unsolvablep2pkh)
        unsolvable_after_importaddress.append(unsolvablep2wshp2pkh)
        unsolvable_after_importaddress.append(op1)  # OP_1 will be imported as script
        unsolvable_after_importaddress.append(p2wshop1)
        unseen_anytime.append(op0)  # OP_0 will be imported as P2SH address with no script provided
        unsolvable_after_importaddress.append(p2shop0)

        spendable_txid = []
        solvable_txid = []
        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime, 1))
        self.mine_and_test_listunspent(spendable_after_importaddress + solvable_after_importaddress + unseen_anytime + unsolvable_after_importaddress, 0)

        importlist = []
        for i in compressed_spendable_address + uncompressed_spendable_address + compressed_solvable_address + uncompressed_solvable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                bare = hex_str_to_bytes(v['hex'])
                importlist.append(bare.hex())
                importlist.append(CScript([OP_0, sha256(bare)]).hex())
            else:
                pubkey = hex_str_to_bytes(v['pubkey'])
                p2pk = CScript([pubkey, OP_CHECKSIG])
                p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
                importlist.append(p2pk.hex())
                importlist.append(p2pkh.hex())
                importlist.append(CScript([OP_0, hash160(pubkey)]).hex())
                importlist.append(CScript([OP_0, sha256(p2pk)]).hex())
                importlist.append(CScript([OP_0, sha256(p2pkh)]).hex())

        importlist.append(unsolvablep2pkh.hex())
        importlist.append(unsolvablep2wshp2pkh.hex())
        importlist.append(op1.hex())
        importlist.append(p2wshop1.hex())

        for i in importlist:
            # import all generated addresses. The wallet already has the private keys for some of these, so catch JSON RPC
            # exceptions and continue.
            try_rpc(-4, "The wallet already contains the private key for this address or script", self.nodes[0].importaddress, i, "", False, True)

        self.nodes[0].importaddress(script_to_p2sh(op0))  # import OP_0 as address only
        self.nodes[0].importaddress(multisig_without_privkey_address)  # Test multisig_without_privkey

        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime + solvable_after_importaddress, 1))
        self.mine_and_test_listunspent(unsolvable_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime + solvable_after_importaddress, 1))
        self.mine_and_test_listunspent(unsolvable_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # Repeat some tests. This time we don't add witness scripts with importaddress
        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("927pw6RW8ZekycnXqBQ2JS5nPyo1yRfGNN8oq74HeddWSpafDJH")
        uncompressed_spendable_address = ["mguN2vNSCEUh6rJaXoAVwY3YZwZvEmf5xi"]
        self.nodes[0].importprivkey("cMcrXaaUC48ZKpcyydfFo8PxHAjpsYLhdsp6nmtB3E2ER9UUHWnw")
        compressed_spendable_address = ["n1UNmpmbVUJ9ytXYXiurmGPQ3TRrXqPWKL"]

        self.nodes[0].importpubkey(pubkeys[5])
        compressed_solvable_address = [key_to_p2pkh(pubkeys[5])]
        self.nodes[0].importpubkey(pubkeys[6])
        uncompressed_solvable_address = [key_to_p2pkh(pubkeys[6])]

        unseen_anytime = []                         # These outputs should never be seen
        solvable_anytime = []                       # These outputs should be solvable after importpubkey
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]])['address'])
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]])['address'])
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]])['address'])
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], uncompressed_solvable_address[0]])['address'])
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]])['address'])

        premature_witaddress = []

        for i in compressed_spendable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                premature_witaddress.append(script_to_p2sh(p2wsh))
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2WPKH, P2SH_P2WPKH are always spendable
                spendable_anytime.extend([p2wpkh, p2sh_p2wpkh])

        for i in uncompressed_spendable_address + uncompressed_solvable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2WPKH, P2SH_P2WPKH with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].getaddressinfo(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                premature_witaddress.append(script_to_p2sh(p2wsh))
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2SH_P2PK, P2SH_P2PKH with compressed keys are always solvable
                solvable_anytime.extend([p2wpkh, p2sh_p2wpkh])

        self.mine_and_test_listunspent(spendable_anytime, 2)
        self.mine_and_test_listunspent(solvable_anytime, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # Check that createrawtransaction/decoderawtransaction with non-v0 Bech32 works
        v1_addr = program_to_witness(1, [3, 5])
        v1_tx = self.nodes[0].createrawtransaction([getutxo(spendable_txid[0])], {v1_addr: 1})
        v1_decoded = self.nodes[1].decoderawtransaction(v1_tx)
        assert_equal(v1_decoded['vout'][0]['scriptPubKey']['address'], v1_addr)
        assert_equal(v1_decoded['vout'][0]['scriptPubKey']['hex'], "51020305")

        # Check that spendable outputs are really spendable
        self.create_and_mine_tx_from_txids(spendable_txid)

        # import all the private keys so solvable addresses become spendable
        self.nodes[0].importprivkey("cPiM8Ub4heR9NBYmgVzJQiUH1if44GSBGiqaeJySuL2BKxubvgwb")
        self.nodes[0].importprivkey("cPpAdHaD6VoYbW78kveN2bsvb45Q7G5PhaPApVUGwvF8VQ9brD97")
        self.nodes[0].importprivkey("91zqCU5B9sdWxzMt1ca3VzbtVm2YM6Hi5Rxn4UDtxEaN9C9nzXV")
        self.nodes[0].importprivkey("cPQFjcVRpAUBG8BA9hzr2yEzHwKoMgLkJZBBtK9vJnvGJgMjzTbd")
        self.nodes[0].importprivkey("cQGtcm34xiLjB1v7bkRa4V3aAc9tS2UTuBZ1UnZGeSeNy627fN66")
        self.nodes[0].importprivkey("cTW5mR5M45vHxXkeChZdtSPozrFwFgmEvTNnanCW6wrqwaCZ1X7K")
        self.create_and_mine_tx_from_txids(solvable_txid)

        # Test that importing native P2WPKH/P2WSH scripts works
        for use_p2wsh in [False, True]:
            if use_p2wsh:
                scriptPubKey = "00203a59f3f56b713fdcf5d1a57357f02c44342cbf306ffe0c4741046837bf90561a"
                transaction = "01000000000100e1f505000000002200203a59f3f56b713fdcf5d1a57357f02c44342cbf306ffe0c4741046837bf90561a00000000"
            else:
                scriptPubKey = "a9142f8c469c2f0084c48e11f998ffbe7efa7549f26d87"
                transaction = "01000000000100e1f5050000000017a9142f8c469c2f0084c48e11f998ffbe7efa7549f26d8700000000"

            self.nodes[1].importaddress(scriptPubKey, "", False)
            rawtxfund = self.nodes[1].fundrawtransaction(transaction)['hex']
            rawtxfund = self.nodes[1].signrawtransactionwithwallet(rawtxfund)["hex"]
            txid = self.nodes[1].sendrawtransaction(rawtxfund)

            assert_equal(self.nodes[1].gettransaction(txid, True)["txid"], txid)
            assert_equal(self.nodes[1].listtransactions("*", 1, 0, True)[0]["txid"], txid)

            # Assert it is properly saved
            self.restart_node(1)
            assert_equal(self.nodes[1].gettransaction(txid, True)["txid"], txid)
            assert_equal(self.nodes[1].listtransactions("*", 1, 0, True)[0]["txid"], txid)

    def mine_and_test_listunspent(self, script_list, ismine):
        utxo = find_spendable_utxo(self.nodes[0], 50)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int('0x' + utxo['txid'], 0), utxo['vout'])))
        for i in script_list:
            tx.vout.append(CTxOut(10000000, i))
        tx.rehash()
        signresults = self.nodes[0].signrawtransactionwithwallet(tx.serialize_without_witness().hex())['hex']
        txid = self.nodes[0].sendrawtransaction(hexstring=signresults, maxfeerate=0)
        txs_mined[txid] = self.nodes[0].generate(1)[0]
        self.sync_blocks()
        watchcount = 0
        spendcount = 0
        for i in self.nodes[0].listunspent():
            if (i['txid'] == txid):
                watchcount += 1
                if i['spendable']:
                    spendcount += 1
        if (ismine == 2):
            assert_equal(spendcount, len(script_list))
        elif (ismine == 1):
            assert_equal(watchcount, len(script_list))
            assert_equal(spendcount, 0)
        else:
            assert_equal(watchcount, 0)
        return txid

    def p2sh_address_to_script(self, v):
        bare = CScript(hex_str_to_bytes(v['hex']))
        p2sh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        p2wsh = CScript([OP_0, sha256(bare)])
        p2sh_p2wsh = CScript([OP_HASH160, hash160(p2wsh), OP_EQUAL])
        return([bare, p2sh, p2wsh, p2sh_p2wsh])

    def p2pkh_address_to_script(self, v):
        pubkey = hex_str_to_bytes(v['pubkey'])
        p2wpkh = CScript([OP_0, hash160(pubkey)])
        p2sh_p2wpkh = CScript([OP_HASH160, hash160(p2wpkh), OP_EQUAL])
        p2pk = CScript([pubkey, OP_CHECKSIG])
        p2pkh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        p2sh_p2pk = CScript([OP_HASH160, hash160(p2pk), OP_EQUAL])
        p2sh_p2pkh = CScript([OP_HASH160, hash160(p2pkh), OP_EQUAL])
        p2wsh_p2pk = CScript([OP_0, sha256(p2pk)])
        p2wsh_p2pkh = CScript([OP_0, sha256(p2pkh)])
        p2sh_p2wsh_p2pk = CScript([OP_HASH160, hash160(p2wsh_p2pk), OP_EQUAL])
        p2sh_p2wsh_p2pkh = CScript([OP_HASH160, hash160(p2wsh_p2pkh), OP_EQUAL])
        return [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh]

    def create_and_mine_tx_from_txids(self, txids, success=True):
        tx = CTransaction()
        for i in txids:
            txraw = self.nodes[0].getrawtransaction(i, 0, txs_mined[i])
            txtmp = tx_from_hex(txraw)
            for j in range(len(txtmp.vout)):
                tx.vin.append(CTxIn(COutPoint(int('0x' + i, 0), j)))
        tx.vout.append(CTxOut(0, CScript()))
        tx.rehash()
        signresults = self.nodes[0].signrawtransactionwithwallet(tx.serialize_without_witness().hex())['hex']
        self.nodes[0].sendrawtransaction(hexstring=signresults, maxfeerate=0)
        self.nodes[0].generate(1)
        self.sync_blocks()


if __name__ == '__main__':
    SegWitTest().main()
