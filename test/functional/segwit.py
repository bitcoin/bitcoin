#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the SegWit changeover logic."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import sha256, CTransaction, CTxIn, COutPoint, CTxOut, COIN, ToHex, FromHex
from test_framework.address import script_to_p2sh, key_to_p2pkh
from test_framework.script import CScript, OP_HASH160, OP_CHECKSIG, OP_0, hash160, OP_EQUAL, OP_DUP, OP_EQUALVERIFY, OP_1, OP_2, OP_CHECKMULTISIG, OP_TRUE
from io import BytesIO

NODE_0 = 0
NODE_1 = 1
NODE_2 = 2
WIT_V0 = 0
WIT_V1 = 1

# Create a scriptPubKey corresponding to either a P2WPKH output for the
# given pubkey, or a P2WSH output of a 1-of-1 multisig for the given
# pubkey. Returns the hex encoding of the scriptPubKey.
def witness_script(use_p2wsh, pubkey):
    if (use_p2wsh == False):
        # P2WPKH instead
        pubkeyhash = hash160(hex_str_to_bytes(pubkey))
        pkscript = CScript([OP_0, pubkeyhash])
    else:
        # 1-of-1 multisig
        witness_program = CScript([OP_1, hex_str_to_bytes(pubkey), OP_1, OP_CHECKMULTISIG])
        scripthash = sha256(witness_program)
        pkscript = CScript([OP_0, scripthash])
    return bytes_to_hex_str(pkscript)

# Return a transaction (in hex) that spends the given utxo to a segwit output,
# optionally wrapping the segwit output using P2SH.
def create_witnessprogram(use_p2wsh, utxo, pubkey, encode_p2sh, amount):
    pkscript = hex_str_to_bytes(witness_script(use_p2wsh, pubkey))
    if (encode_p2sh):
        p2sh_hash = hash160(pkscript)
        pkscript = CScript([OP_HASH160, p2sh_hash, OP_EQUAL])
    tx = CTransaction()
    tx.vin.append(CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]), b""))
    tx.vout.append(CTxOut(int(amount*COIN), pkscript))
    return ToHex(tx)

# Create a transaction spending a given utxo to a segwit output corresponding
# to the given pubkey: use_p2wsh determines whether to use P2WPKH or P2WSH;
# encode_p2sh determines whether to wrap in P2SH.
# sign=True will have the given node sign the transaction.
# insert_redeem_script will be added to the scriptSig, if given.
def send_to_witness(use_p2wsh, node, utxo, pubkey, encode_p2sh, amount, sign=True, insert_redeem_script=""):
    tx_to_witness = create_witnessprogram(use_p2wsh, utxo, pubkey, encode_p2sh, amount)
    if (sign):
        signed = node.signrawtransaction(tx_to_witness)
        assert("errors" not in signed or len(["errors"]) == 0)
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx = FromHex(CTransaction(), tx_to_witness)
            tx.vin[0].scriptSig += CScript([hex_str_to_bytes(insert_redeem_script)])
            tx_to_witness = ToHex(tx)

    return node.sendrawtransaction(tx_to_witness)

def getutxo(txid):
    utxo = {}
    utxo["vout"] = 0
    utxo["txid"] = txid
    return utxo

def find_unspent(node, min_value):
    for utxo in node.listunspent():
        if utxo['amount'] >= min_value:
            return utxo

class SegWitTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-walletprematurewitness", "-rpcserialversion=0"],
                           ["-blockversion=4", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness", "-rpcserialversion=1"],
                           ["-blockversion=536870915", "-promiscuousmempoolflags=517", "-prematurewitness", "-walletprematurewitness"]]

    def setup_network(self):
        super().setup_network()
        connect_nodes(self.nodes[0], 2)
        self.sync_all()

    def success_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 2)
        sync_blocks(self.nodes)

    def skip_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        block = node.generate(1)
        assert_equal(len(node.getblock(block[0])["tx"]), 1)
        sync_blocks(self.nodes)

    def fail_accept(self, node, error_msg, txid, sign, redeem_script=""):
        assert_raises_jsonrpc(-26, error_msg, send_to_witness, 1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)

    def fail_mine(self, node, txid, sign, redeem_script=""):
        send_to_witness(1, node, getutxo(txid), self.pubkey[0], False, Decimal("49.998"), sign, redeem_script)
        assert_raises_jsonrpc(-1, "CreateNewBlock: TestBlockValidity failed", node.generate, 1)
        sync_blocks(self.nodes)

    def run_test(self):
        self.nodes[0].generate(161) #block 161

        self.log.info("Verify sigops are counted in GBT with pre-BIP141 rules before the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(tmpl['sizelimit'] == 1000000)
        assert('weightlimit' not in tmpl)
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit']})
        assert(tmpl['sizelimit'] == 1000000)
        assert('weightlimit' not in tmpl)
        assert(tmpl['sigoplimit'] == 20000)
        assert(tmpl['transactions'][0]['hash'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 2)
        self.nodes[0].generate(1) #block 162

        balance_presetup = self.nodes[0].getbalance()
        self.pubkey = []
        p2sh_ids = [] # p2sh_ids[NODE][VER] is an array of txids that spend to a witness version VER pkscript to an address for NODE embedded in p2sh
        wit_ids = [] # wit_ids[NODE][VER] is an array of txids that spend to a witness version VER pkscript to an address for NODE via bare witness
        for i in range(3):
            newaddress = self.nodes[i].getnewaddress()
            self.pubkey.append(self.nodes[i].validateaddress(newaddress)["pubkey"])
            multiaddress = self.nodes[i].addmultisigaddress(1, [self.pubkey[-1]])
            self.nodes[i].addwitnessaddress(newaddress)
            self.nodes[i].addwitnessaddress(multiaddress)
            p2sh_ids.append([])
            wit_ids.append([])
            for v in range(2):
                p2sh_ids[i].append([])
                wit_ids[i].append([])

        for i in range(5):
            for n in range(3):
                for v in range(2):
                    wit_ids[n][v].append(send_to_witness(v, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[n], False, Decimal("49.999")))
                    p2sh_ids[n][v].append(send_to_witness(v, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[n], True, Decimal("49.999")))

        self.nodes[0].generate(1) #block 163
        sync_blocks(self.nodes)

        # Make sure all nodes recognize the transactions as theirs
        assert_equal(self.nodes[0].getbalance(), balance_presetup - 60*50 + 20*Decimal("49.999") + 50)
        assert_equal(self.nodes[1].getbalance(), 20*Decimal("49.999"))
        assert_equal(self.nodes[2].getbalance(), 20*Decimal("49.999"))

        self.nodes[0].generate(260) #block 423
        sync_blocks(self.nodes)

        self.log.info("Verify default node can't accept any witness format txs before fork")
        # unsigned, no scriptsig
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", wit_ids[NODE_0][WIT_V0][0], False)
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", wit_ids[NODE_0][WIT_V1][0], False)
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", p2sh_ids[NODE_0][WIT_V0][0], False)
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", p2sh_ids[NODE_0][WIT_V1][0], False)
        # unsigned with redeem script
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", p2sh_ids[NODE_0][WIT_V0][0], False, witness_script(False, self.pubkey[0]))
        self.fail_accept(self.nodes[0], "mandatory-script-verify-flag", p2sh_ids[NODE_0][WIT_V1][0], False, witness_script(True, self.pubkey[0]))
        # signed
        self.fail_accept(self.nodes[0], "no-witness-yet", wit_ids[NODE_0][WIT_V0][0], True)
        self.fail_accept(self.nodes[0], "no-witness-yet", wit_ids[NODE_0][WIT_V1][0], True)
        self.fail_accept(self.nodes[0], "no-witness-yet", p2sh_ids[NODE_0][WIT_V0][0], True)
        self.fail_accept(self.nodes[0], "no-witness-yet", p2sh_ids[NODE_0][WIT_V1][0], True)

        self.log.info("Verify witness txs are skipped for mining before the fork")
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][0], True) #block 424
        self.skip_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][0], True) #block 425
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][0], True) #block 426
        self.skip_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][0], True) #block 427

        # TODO: An old node would see these txs without witnesses and be able to mine them

        self.log.info("Verify unsigned bare witness txs in versionbits-setting blocks are valid before the fork")
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][1], False) #block 428
        self.success_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][1], False) #block 429

        self.log.info("Verify unsigned p2sh witness txs without a redeem script are invalid")
        self.fail_accept(self.nodes[2], "mandatory-script-verify-flag", p2sh_ids[NODE_2][WIT_V0][1], False)
        self.fail_accept(self.nodes[2], "mandatory-script-verify-flag", p2sh_ids[NODE_2][WIT_V1][1], False)

        self.log.info("Verify unsigned p2sh witness txs with a redeem script in versionbits-settings blocks are valid before the fork")
        self.success_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][1], False, witness_script(False, self.pubkey[2])) #block 430
        self.success_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][1], False, witness_script(True, self.pubkey[2])) #block 431

        self.log.info("Verify previous witness txs skipped for mining can now be mined")
        assert_equal(len(self.nodes[2].getrawmempool()), 4)
        block = self.nodes[2].generate(1) #block 432 (first block with new rules; 432 = 144 * 3)
        sync_blocks(self.nodes)
        assert_equal(len(self.nodes[2].getrawmempool()), 0)
        segwit_tx_list = self.nodes[2].getblock(block[0])["tx"]
        assert_equal(len(segwit_tx_list), 5)

        self.log.info("Verify block and transaction serialization rpcs return differing serializations depending on rpc serialization flag")
        assert(self.nodes[2].getblock(block[0], False) !=  self.nodes[0].getblock(block[0], False))
        assert(self.nodes[1].getblock(block[0], False) ==  self.nodes[2].getblock(block[0], False))
        for i in range(len(segwit_tx_list)):
            tx = FromHex(CTransaction(), self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[2].getrawtransaction(segwit_tx_list[i]) != self.nodes[0].getrawtransaction(segwit_tx_list[i]))
            assert(self.nodes[1].getrawtransaction(segwit_tx_list[i], 0) == self.nodes[2].getrawtransaction(segwit_tx_list[i]))
            assert(self.nodes[0].getrawtransaction(segwit_tx_list[i]) != self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[1].getrawtransaction(segwit_tx_list[i]) == self.nodes[2].gettransaction(segwit_tx_list[i])["hex"])
            assert(self.nodes[0].getrawtransaction(segwit_tx_list[i]) == bytes_to_hex_str(tx.serialize_without_witness()))

        self.log.info("Verify witness txs without witness data are invalid after the fork")
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V0][2], False)
        self.fail_mine(self.nodes[2], wit_ids[NODE_2][WIT_V1][2], False)
        self.fail_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V0][2], False, witness_script(False, self.pubkey[2]))
        self.fail_mine(self.nodes[2], p2sh_ids[NODE_2][WIT_V1][2], False, witness_script(True, self.pubkey[2]))

        self.log.info("Verify default node can now use witness txs")
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V0][0], True) #block 432
        self.success_mine(self.nodes[0], wit_ids[NODE_0][WIT_V1][0], True) #block 433
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][WIT_V0][0], True) #block 434
        self.success_mine(self.nodes[0], p2sh_ids[NODE_0][WIT_V1][0], True) #block 435

        self.log.info("Verify sigops are counted in GBT with BIP141 rules after the fork")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        tmpl = self.nodes[0].getblocktemplate({'rules':['segwit']})
        assert(tmpl['sizelimit'] >= 3999577)  # actual maximum size is lower due to minimum mandatory non-witness data
        assert(tmpl['weightlimit'] == 4000000)
        assert(tmpl['sigoplimit'] == 80000)
        assert(tmpl['transactions'][0]['txid'] == txid)
        assert(tmpl['transactions'][0]['sigops'] == 8)

        self.nodes[0].generate(1) # Mine a block to clear the gbt cache

        self.log.info("Non-segwit miners are able to use GBT response after activation.")
        # Create a 3-tx chain: tx1 (non-segwit input, paying to a segwit output) ->
        #                      tx2 (segwit input, paying to a non-segwit output) ->
        #                      tx3 (non-segwit input, paying to a non-segwit output).
        # tx1 is allowed to appear in the block, but no others.
        txid1 = send_to_witness(1, self.nodes[0], find_unspent(self.nodes[0], 50), self.pubkey[0], False, Decimal("49.996"))
        hex_tx = self.nodes[0].gettransaction(txid)['hex']
        tx = FromHex(CTransaction(), hex_tx)
        assert(tx.wit.is_null()) # This should not be a segwit input
        assert(txid1 in self.nodes[0].getrawmempool())

        # Now create tx2, which will spend from txid1.
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(txid1, 16), 0), b''))
        tx.vout.append(CTxOut(int(49.99*COIN), CScript([OP_TRUE])))
        tx2_hex = self.nodes[0].signrawtransaction(ToHex(tx))['hex']
        txid2 = self.nodes[0].sendrawtransaction(tx2_hex)
        tx = FromHex(CTransaction(), tx2_hex)
        assert(not tx.wit.is_null())

        # Now create tx3, which will spend from txid2
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(txid2, 16), 0), b""))
        tx.vout.append(CTxOut(int(49.95*COIN), CScript([OP_TRUE]))) # Huge fee
        tx.calc_sha256()
        txid3 = self.nodes[0].sendrawtransaction(ToHex(tx))
        assert(tx.wit.is_null())
        assert(txid3 in self.nodes[0].getrawmempool())

        # Now try calling getblocktemplate() without segwit support.
        template = self.nodes[0].getblocktemplate()

        # Check that tx1 is the only transaction of the 3 in the template.
        template_txids = [ t['txid'] for t in template['transactions'] ]
        assert(txid2 not in template_txids and txid3 not in template_txids)
        assert(txid1 in template_txids)

        # Check that running with segwit support results in all 3 being included.
        template = self.nodes[0].getblocktemplate({"rules": ["segwit"]})
        template_txids = [ t['txid'] for t in template['transactions'] ]
        assert(txid1 in template_txids)
        assert(txid2 in template_txids)
        assert(txid3 in template_txids)

        # Mine a block to clear the gbt cache again.
        self.nodes[0].generate(1)

        self.log.info("Verify behaviour of importaddress, addwitnessaddress and listunspent")

        # Some public keys to be used later
        pubkeys = [
            "0363D44AABD0F1699138239DF2F042C3282C0671CC7A76826A55C8203D90E39242", # cPiM8Ub4heR9NBYmgVzJQiUH1if44GSBGiqaeJySuL2BKxubvgwb
            "02D3E626B3E616FC8662B489C123349FECBFC611E778E5BE739B257EAE4721E5BF", # cPpAdHaD6VoYbW78kveN2bsvb45Q7G5PhaPApVUGwvF8VQ9brD97
            "04A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538A62F5BD8EC85C2477F39650BD391EA6250207065B2A81DA8B009FC891E898F0E", # 91zqCU5B9sdWxzMt1ca3VzbtVm2YM6Hi5Rxn4UDtxEaN9C9nzXV
            "02A47F2CBCEFFA7B9BCDA184E7D5668D3DA6F9079AD41E422FA5FD7B2D458F2538", # cPQFjcVRpAUBG8BA9hzr2yEzHwKoMgLkJZBBtK9vJnvGJgMjzTbd
            "036722F784214129FEB9E8129D626324F3F6716555B603FFE8300BBCB882151228", # cQGtcm34xiLjB1v7bkRa4V3aAc9tS2UTuBZ1UnZGeSeNy627fN66
            "0266A8396EE936BF6D99D17920DB21C6C7B1AB14C639D5CD72B300297E416FD2EC", # cTW5mR5M45vHxXkeChZdtSPozrFwFgmEvTNnanCW6wrqwaCZ1X7K
            "0450A38BD7F0AC212FEBA77354A9B036A32E0F7C81FC4E0C5ADCA7C549C4505D2522458C2D9AE3CEFD684E039194B72C8A10F9CB9D4764AB26FCC2718D421D3B84", # 92h2XPssjBpsJN5CqSP7v9a7cf2kgDunBC6PDFwJHMACM1rrVBJ
        ]

        # Import a compressed key and an uncompressed key, generate some multisig addresses
        self.nodes[0].importprivkey("92e6XLo5jVAVwrQKPNTs93oQco8f8sDNBcpv73Dsrs397fQtFQn")
        uncompressed_spendable_address = ["mvozP4UwyGD2mGZU4D2eMvMLPB9WkMmMQu"]
        self.nodes[0].importprivkey("cNC8eQ5dg3mFAVePDX4ddmPYpPbw41r9bm2jd1nLJT77e6RrzTRR")
        compressed_spendable_address = ["mmWQubrDomqpgSYekvsU7HWEVjLFHAakLe"]
        assert ((self.nodes[0].validateaddress(uncompressed_spendable_address[0])['iscompressed'] == False))
        assert ((self.nodes[0].validateaddress(compressed_spendable_address[0])['iscompressed'] == True))

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

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], compressed_solvable_address[1]]))
        unknown_address = ["mtKKyoHabkk6e4ppT7NaM7THqPUt7AzPrT", "2NDP3jLWAFT8NDAiUa9qiE6oBt2awmMq7Dx"]

        # Test multisig_without_privkey
        # We have 2 public keys without private keys, use addmultisigaddress to add to wallet.
        # Money sent to P2SH of multisig of this should only be seen after importaddress with the BASE58 P2SH address.

        multisig_without_privkey_address = self.nodes[0].addmultisigaddress(2, [pubkeys[3], pubkeys[4]])
        script = CScript([OP_2, hex_str_to_bytes(pubkeys[3]), hex_str_to_bytes(pubkeys[4]), OP_2, OP_CHECKMULTISIG])
        solvable_after_importaddress.append(CScript([OP_HASH160, hash160(script), OP_EQUAL]))

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with compressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are spendable after direct importaddress
                spendable_after_importaddress.extend([p2wpkh, p2sh_p2wpkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in uncompressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # bare and p2sh multisig with uncompressed keys should always be spendable
                spendable_anytime.extend([bare, p2sh])
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with uncompressed keys should always be spendable
                spendable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK and P2SH_P2PKH are spendable after direct importaddress
                spendable_after_importaddress.extend([p2sh_p2pk, p2sh_p2pkh])
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # Multisig without private is not seen after addmultisigaddress, but seen after importaddress
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                solvable_after_importaddress.extend([bare, p2sh, p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # normal P2PKH and P2PK with compressed keys should always be seen
                solvable_anytime.extend([p2pkh, p2pk])
                # P2SH_P2PK, P2SH_P2PKH, and witness with compressed keys are seen after direct importaddress
                solvable_after_importaddress.extend([p2wpkh, p2sh_p2wpkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        for i in uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
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
                # witness with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh])

        op1 = CScript([OP_1])
        op0 = CScript([OP_0])
        # 2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe is the P2SH(P2PKH) version of mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V
        unsolvable_address = ["mjoE3sSrb8ByYEvgnC3Aox86u1CHnfJA4V", "2N7MGY19ti4KDMSzRfPAssP6Pxyuxoi6jLe", script_to_p2sh(op1), script_to_p2sh(op0)]
        unsolvable_address_key = hex_str_to_bytes("02341AEC7587A51CDE5279E0630A531AEA2615A9F80B17E8D9376327BAEAA59E3D")
        unsolvablep2pkh = CScript([OP_DUP, OP_HASH160, hash160(unsolvable_address_key), OP_EQUALVERIFY, OP_CHECKSIG])
        unsolvablep2wshp2pkh = CScript([OP_0, sha256(unsolvablep2pkh)])
        p2shop0 = CScript([OP_HASH160, hash160(op0), OP_EQUAL])
        p2wshop1 = CScript([OP_0, sha256(op1)])
        unsolvable_after_importaddress.append(unsolvablep2pkh)
        unsolvable_after_importaddress.append(unsolvablep2wshp2pkh)
        unsolvable_after_importaddress.append(op1) # OP_1 will be imported as script
        unsolvable_after_importaddress.append(p2wshop1)
        unseen_anytime.append(op0) # OP_0 will be imported as P2SH address with no script provided
        unsolvable_after_importaddress.append(p2shop0)

        spendable_txid = []
        solvable_txid = []
        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime, 1))
        self.mine_and_test_listunspent(spendable_after_importaddress + solvable_after_importaddress + unseen_anytime + unsolvable_after_importaddress, 0)

        importlist = []
        for i in compressed_spendable_address + uncompressed_spendable_address + compressed_solvable_address + uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                bare = hex_str_to_bytes(v['hex'])
                importlist.append(bytes_to_hex_str(bare))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(bare)])))
            else:
                pubkey = hex_str_to_bytes(v['pubkey'])
                p2pk = CScript([pubkey, OP_CHECKSIG])
                p2pkh = CScript([OP_DUP, OP_HASH160, hash160(pubkey), OP_EQUALVERIFY, OP_CHECKSIG])
                importlist.append(bytes_to_hex_str(p2pk))
                importlist.append(bytes_to_hex_str(p2pkh))
                importlist.append(bytes_to_hex_str(CScript([OP_0, hash160(pubkey)])))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pk)])))
                importlist.append(bytes_to_hex_str(CScript([OP_0, sha256(p2pkh)])))

        importlist.append(bytes_to_hex_str(unsolvablep2pkh))
        importlist.append(bytes_to_hex_str(unsolvablep2wshp2pkh))
        importlist.append(bytes_to_hex_str(op1))
        importlist.append(bytes_to_hex_str(p2wshop1))

        for i in importlist:
            # import all generated addresses. The wallet already has the private keys for some of these, so catch JSON RPC
            # exceptions and continue.
            try:
                self.nodes[0].importaddress(i,"",False,True)
            except JSONRPCException as exp:
                assert_equal(exp.error["message"], "The wallet already contains the private key for this address or script")
                assert_equal(exp.error["code"], -4)

        self.nodes[0].importaddress(script_to_p2sh(op0)) # import OP_0 as address only
        self.nodes[0].importaddress(multisig_without_privkey_address) # Test multisig_without_privkey

        spendable_txid.append(self.mine_and_test_listunspent(spendable_anytime + spendable_after_importaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_anytime + solvable_after_importaddress, 1))
        self.mine_and_test_listunspent(unsolvable_after_importaddress, 1)
        self.mine_and_test_listunspent(unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used
        # note that no witness address should be returned by unsolvable addresses
        for i in uncompressed_spendable_address + uncompressed_solvable_address + unknown_address + unsolvable_address:
            assert_raises_jsonrpc(-4, "Public key or redeemscript not known to wallet, or the key is uncompressed", self.nodes[0].addwitnessaddress, i)

        # addwitnessaddress should return a witness addresses even if keys are not in the wallet
        self.nodes[0].addwitnessaddress(multisig_without_privkey_address)

        for i in compressed_spendable_address + compressed_solvable_address:
            witaddress = self.nodes[0].addwitnessaddress(i)
            # addwitnessaddress should return the same address if it is a known P2SH-witness address
            assert_equal(witaddress, self.nodes[0].addwitnessaddress(witaddress))

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

        spendable_after_addwitnessaddress = []      # These outputs should be seen after importaddress
        solvable_after_addwitnessaddress=[]         # These outputs should be seen after importaddress but not spendable
        unseen_anytime = []                         # These outputs should never be seen

        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [uncompressed_spendable_address[0], uncompressed_spendable_address[0]]))
        compressed_spendable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_spendable_address[0]]))
        uncompressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_solvable_address[0], uncompressed_solvable_address[0]]))
        compressed_solvable_address.append(self.nodes[0].addmultisigaddress(2, [compressed_spendable_address[0], compressed_solvable_address[0]]))

        premature_witaddress = []

        for i in compressed_spendable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # P2WSH and P2SH(P2WSH) multisig with compressed keys are spendable after addwitnessaddress
                spendable_after_addwitnessaddress.extend([p2wsh, p2sh_p2wsh])
                premature_witaddress.append(script_to_p2sh(p2wsh))
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2WPKH, P2SH_P2WPKH are spendable after addwitnessaddress
                spendable_after_addwitnessaddress.extend([p2wpkh, p2sh_p2wpkh])
                premature_witaddress.append(script_to_p2sh(p2wpkh))

        for i in uncompressed_spendable_address + uncompressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                # P2WSH and P2SH(P2WSH) multisig with uncompressed keys are never seen
                unseen_anytime.extend([p2wsh, p2sh_p2wsh])
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2WPKH, P2SH_P2WPKH with uncompressed keys are never seen
                unseen_anytime.extend([p2wpkh, p2sh_p2wpkh])

        for i in compressed_solvable_address:
            v = self.nodes[0].validateaddress(i)
            if (v['isscript']):
                # P2WSH multisig without private key are seen after addwitnessaddress
                [bare, p2sh, p2wsh, p2sh_p2wsh] = self.p2sh_address_to_script(v)
                solvable_after_addwitnessaddress.extend([p2wsh, p2sh_p2wsh])
                premature_witaddress.append(script_to_p2sh(p2wsh))
            else:
                [p2wpkh, p2sh_p2wpkh, p2pk, p2pkh, p2sh_p2pk, p2sh_p2pkh, p2wsh_p2pk, p2wsh_p2pkh, p2sh_p2wsh_p2pk, p2sh_p2wsh_p2pkh] = self.p2pkh_address_to_script(v)
                # P2SH_P2PK, P2SH_P2PKH with compressed keys are seen after addwitnessaddress
                solvable_after_addwitnessaddress.extend([p2wpkh, p2sh_p2wpkh])
                premature_witaddress.append(script_to_p2sh(p2wpkh))

        self.mine_and_test_listunspent(spendable_after_addwitnessaddress + solvable_after_addwitnessaddress + unseen_anytime, 0)

        # addwitnessaddress should refuse to return a witness address if an uncompressed key is used
        # note that a multisig address returned by addmultisigaddress is not solvable until it is added with importaddress
        # premature_witaddress are not accepted until the script is added with addwitnessaddress first
        for i in uncompressed_spendable_address + uncompressed_solvable_address + premature_witaddress:
            # This will raise an exception
            assert_raises_jsonrpc(-4, "Public key or redeemscript not known to wallet, or the key is uncompressed", self.nodes[0].addwitnessaddress, i)

        # after importaddress it should pass addwitnessaddress
        v = self.nodes[0].validateaddress(compressed_solvable_address[1])
        self.nodes[0].importaddress(v['hex'],"",False,True)
        for i in compressed_spendable_address + compressed_solvable_address + premature_witaddress:
            witaddress = self.nodes[0].addwitnessaddress(i)
            assert_equal(witaddress, self.nodes[0].addwitnessaddress(witaddress))

        spendable_txid.append(self.mine_and_test_listunspent(spendable_after_addwitnessaddress, 2))
        solvable_txid.append(self.mine_and_test_listunspent(solvable_after_addwitnessaddress, 1))
        self.mine_and_test_listunspent(unseen_anytime, 0)

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

    def mine_and_test_listunspent(self, script_list, ismine):
        utxo = find_unspent(self.nodes[0], 50)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int('0x'+utxo['txid'],0), utxo['vout'])))
        for i in script_list:
            tx.vout.append(CTxOut(10000000, i))
        tx.rehash()
        signresults = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        txid = self.nodes[0].sendrawtransaction(signresults, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        watchcount = 0
        spendcount = 0
        for i in self.nodes[0].listunspent():
            if (i['txid'] == txid):
                watchcount += 1
                if (i['spendable'] == True):
                    spendcount += 1
        if (ismine == 2):
            assert_equal(spendcount, len(script_list))
        elif (ismine == 1):
            assert_equal(watchcount, len(script_list))
            assert_equal(spendcount, 0)
        else:
            assert_equal(watchcount, 0)
        return txid

    def p2sh_address_to_script(self,v):
        bare = CScript(hex_str_to_bytes(v['hex']))
        p2sh = CScript(hex_str_to_bytes(v['scriptPubKey']))
        p2wsh = CScript([OP_0, sha256(bare)])
        p2sh_p2wsh = CScript([OP_HASH160, hash160(p2wsh), OP_EQUAL])
        return([bare, p2sh, p2wsh, p2sh_p2wsh])

    def p2pkh_address_to_script(self,v):
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

    def create_and_mine_tx_from_txids(self, txids, success = True):
        tx = CTransaction()
        for i in txids:
            txtmp = CTransaction()
            txraw = self.nodes[0].getrawtransaction(i)
            f = BytesIO(hex_str_to_bytes(txraw))
            txtmp.deserialize(f)
            for j in range(len(txtmp.vout)):
                tx.vin.append(CTxIn(COutPoint(int('0x'+i,0), j)))
        tx.vout.append(CTxOut(0, CScript()))
        tx.rehash()
        signresults = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        self.nodes[0].sendrawtransaction(signresults, True)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)


if __name__ == '__main__':
    SegWitTest().main()
