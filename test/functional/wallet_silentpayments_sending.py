#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test sending to silent payments (BIP352) addresses."""

from decimal import Decimal
from test_framework.bip352 import make_sp_addr, scan
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.extendedkey import ExtendedPrivateKey
from test_framework.key import H_POINT
from test_framework.messages import tx_from_hex
from test_framework.script import CScript, OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_not_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.wallet_util import generate_keypair

_master_key = ExtendedPrivateKey.generate()
XPRV = _master_key.to_string()
XPUB = _master_key.pubkey().to_string()

P2TR_EXT  = f"{XPRV}/86h/1h/0h/0/*"
P2TR_INT  = f"{XPRV}/86h/1h/0h/1/*"
P2PKH_EXT = f"{XPRV}/44h/1h/0h/0/*"
P2SH_EXT  = f"{XPRV}/49h/1h/0h/0/*"
WPKH_INT  = f"{XPRV}/84h/1h/0h/1/*"

# Per-test subtrees for non-standard paths; each test owns one numeric namespace so
# wallets that share XPRV cannot accidentally see each other's UTXOs.
NO_ELIGIBLE_EXT        = f"{XPRV}/11/0/*"   # test_no_eligible_inputs — recv
NO_ELIGIBLE_INT        = f"{XPRV}/11/1/*"   # test_no_eligible_inputs — change
TR_SCRIPT_INTERNAL     = f"{XPRV}/13/0/*"   # test_taproot_script_path_inputs — internal key
TR_SCRIPT_LEAF         = f"{XPRV}/13/1/*"   # test_taproot_script_path_inputs — leaf key
TR_NUMS_LEAF           = f"{XPRV}/14/0/*"   # test_taproot_script_path_inputs — NUMS leaf key
UNCOMPRESSED_INT       = f"{XPRV}/15/0/*"   # test_uncompressed_p2pkh_inputs — change
UNCOMPRESSED_WPKH      = f"{XPRV}/15/1/*"   # test_uncompressed_p2pkh_inputs — eligible P2WPKH
PRESET_ELIGIBLE_EXT    = f"{XPRV}/17/0/*"   # test_preset_eligible_input — non-eligible recv
PRESET_ELIGIBLE_INT    = f"{XPRV}/17/1/*"   # test_preset_eligible_input — non-eligible change
PRESET_ELIGIBLE_WPKH   = f"{XPRV}/17/2/*"   # test_preset_eligible_input — eligible P2WPKH
PRESET_INELIGIBLE_WSH  = f"{XPRV}/18/0/*"   # test_preset_ineligible_input — non-eligible
MULTIPLE_SPKMS_EXT     = f"{XPRV}/19/0/*"   # test_inputs_with_multiple_spkms — recv, also imported as combo()
MULTIPLE_SPKMS_INT     = f"{XPRV}/19/1/*"   # test_inputs_with_multiple_spkms — change


def desc(template):
    return {"desc": descsum_create(template), "timestamp": "now", "active": True}

def internal_desc(template):
    return {**desc(template), "internal": True}

def import_descriptors(wallet, requests):
    for result in wallet.importdescriptors(requests):
        assert result["success"], result


class SPRecipient:
    """A silent payments recipient, with a new spend key and optionally an existing scan key."""
    def __init__(self, scan_key=None):
        self.scan_key = scan_key if scan_key is not None else generate_keypair()[0]
        self.spend_key = generate_keypair()[0]
        self.address = make_sp_addr(self.scan_key, self.spend_key)


class SilentPaymentsSendingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txindex=1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_funded_wallet(
        self,
        name,
        *,
        amount=25,
        blank=False,
        descriptors=None,
        passphrase=None,
        disable_private_keys=False,
        address_type=None,
    ):
        self.nodes[0].createwallet(wallet_name=name, blank=blank, passphrase=passphrase, disable_private_keys=disable_private_keys)
        wallet = self.nodes[0].get_wallet_rpc(name)
        if descriptors:
            import_descriptors(wallet, descriptors)
        self.funder.sendtoaddress(wallet.getnewaddress(address_type=address_type), amount)
        self.generate(self.nodes[0], 1)
        return wallet

    def do_send(self, wallet, recipients, *, expect=(), expected_error=None, expected_error_fund=None, mine_block=True):
        """Send to recipients ({address: amount}) with each send RPC and check each transaction
        has one output for each SP recipient in expect. Return the txids by RPC name."""
        addr0, amt0 = next(iter(recipients.items()))
        outputs = [{addr: amt} for addr, amt in recipients.items()]
        single = len(recipients) == 1

        if expected_error is not None:
            code, msg = expected_error
            # send goes through a different code path,
            # so it may raise a different error code/message
            fund_code, fund_msg = expected_error_fund if expected_error_fund is not None else expected_error
            if single:
                assert_raises_rpc_error(code, msg, wallet.sendtoaddress, addr0, amt0)
            assert_raises_rpc_error(code, msg, wallet.sendmany, "", recipients)
            assert_raises_rpc_error(fund_code, fund_msg, wallet.send, outputs=outputs)
            return {}

        sends = {
            "sendmany": lambda: wallet.sendmany("", recipients),
            "send": lambda: wallet.send(outputs=outputs)["txid"],
            # sendall spends all the coins, so it goes last
            "sendall": lambda: wallet.sendall(recipients=list(recipients))["txid"],
        }
        if single:
            sends = {"sendtoaddress": lambda: wallet.sendtoaddress(addr0, amt0), **sends}

        txids = {}
        for rpc, send in sends.items():
            txid = send()
            assert txid in self.nodes[0].getrawmempool()
            assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
            self.assert_sp_outputs(txid, *expect)
            txids[rpc] = txid

        if mine_block:
            self.mine_and_check_confirmed(wallet, *txids.values())

        return txids

    def scan_tx(self, tx_hex, recipient):
        """Return the outputs of tx_hex paying to recipient, looking up its prevouts on node 0."""
        node = self.nodes[0]
        tx = tx_from_hex(tx_hex)
        inputs = []
        for i, txin in enumerate(tx.vin):
            txid, n = f"{txin.prevout.hash:064x}", txin.prevout.n
            prevout = node.gettxout(txid, n, include_mempool=False) or node.getrawtransaction(txid, True)["vout"][n]
            witness = tx.wit.vtxinwit[i].scriptWitness.stack if i < len(tx.wit.vtxinwit) else []
            inputs.append((txin.prevout, CScript(txin.scriptSig), witness, bytes.fromhex(prevout["scriptPubKey"]["hex"])))
        return scan(inputs, [txout.scriptPubKey for txout in tx.vout], recipient.scan_key.get_bytes(), recipient.spend_key.get_pubkey().get_bytes())

    def assert_sp_outputs(self, txid, *recipients, n=1):
        """Assert each SP recipient has exactly n outputs in txid. Return the outputs found."""
        tx_hex = self.nodes[0].getrawtransaction(txid)
        found = []
        for r in recipients:
            outputs = self.scan_tx(tx_hex, r)
            assert_equal(len(outputs), n)
            found += outputs
        return found

    def bumpfee(self, wallet, txid, **options):
        """Bump txid and assert the replacement took its place in the mempool. Return the new txid."""
        assert txid in self.nodes[0].getrawmempool()
        new_txid = wallet.bumpfee(txid, **options)["txid"]
        mempool = self.nodes[0].getrawmempool()
        assert new_txid in mempool
        assert txid not in mempool
        return new_txid

    def mine_and_check_confirmed(self, wallet, *txids):
        self.generate(self.nodes[0], 1)
        for txid in txids:
            assert_equal(wallet.gettransaction(txid)["confirmations"], 1)

    def fund_descriptor(self, wallet, template, amount):
        """Import a non-active descriptor into wallet and fund its first address. Return the funded outpoint."""
        descriptor = descsum_create(template)
        request = {"desc": descriptor, "timestamp": "now"}
        if "*" in template:
            request["range"] = [0, 0]
        import_descriptors(wallet, [request])
        addr = self.nodes[0].deriveaddresses(descriptor, [0, 0] if "*" in template else None)[0]
        txid = self.funder.sendtoaddress(addr, amount)
        self.generate(self.nodes[0], 1)
        return {"txid": txid, "vout": find_vout_for_address(self.nodes[0], txid, addr)}

    def test_taproot_script_path_inputs(self):
        self.log.info("Testing explicit taproot inputs that have a script path")
        node = self.nodes[0]
        node.createwallet("taproot_script_path", blank=True)
        wallet = node.get_wallet_rpc("taproot_script_path")

        # The internal key is ours, so the input is used for the shared secret
        tr_input = self.fund_descriptor(wallet, f"tr({TR_SCRIPT_INTERNAL},pk({TR_SCRIPT_LEAF}))", 2)
        txid = wallet.sendall(recipients=[self.sp_1.address], options={"inputs": [tr_input]})["txid"]
        self.assert_sp_outputs(txid, self.sp_1)

        # The internal key is the NUMS point, so BIP352 leaves the input out of the shared secret
        nums_input = self.fund_descriptor(wallet, f"tr({H_POINT},pk({TR_NUMS_LEAF}))", 2)
        assert_raises_rpc_error(-4, "No silent payment eligible inputs were found.",
            wallet.sendall, recipients=[self.sp_1.address], options={"inputs": [nums_input]})

    def test_uncompressed_p2pkh_inputs(self):
        self.log.info("Testing P2PKH inputs with an uncompressed key are not used for the shared secret")
        node = self.nodes[0]
        node.createwallet("uncompressed_p2pkh", blank=True)
        wallet = node.get_wallet_rpc("uncompressed_p2pkh")
        import_descriptors(wallet, [internal_desc(f"wpkh({UNCOMPRESSED_INT})")])

        uncompressed_wif, _ = generate_keypair(compressed=False, wif=True)
        self.fund_descriptor(wallet, f"pkh({uncompressed_wif})", 3)
        self.fund_descriptor(wallet, f"wpkh({UNCOMPRESSED_WPKH})", 3)

        # Paying 5 needs both inputs, and only the P2WPKH one is used for the shared secret
        txid = wallet.sendtoaddress(self.sp_1.address, 5)
        assert_equal(len(node.getrawtransaction(txid, True)["vin"]), 2)
        self.assert_sp_outputs(txid, self.sp_1)
        self.generate(node, 1)

    def test_watch_only_wallet(self):
        self.log.info("Testing watch-only wallet cannot send silent payments")
        wallet = self.create_funded_wallet(
            "watch_only",
            blank=True,
            disable_private_keys=True,
            descriptors=[{**desc(f"wpkh({XPUB}/0/*)"), "range": [0, 100]}],
        )
        self.do_send(
            wallet,
            {self.sp_1.address: 5},
            expected_error=(-4, "Private keys are disabled for this wallet"),
            expected_error_fund=(-4, "Silent payments require access to private keys to build transactions."),
        )

    def test_encrypted_wallet_locked(self):
        self.log.info("Testing locked encrypted wallet cannot send silent payments")
        wallet = self.create_funded_wallet("encrypted_locked", passphrase="passphrase")
        self.do_send(wallet, {self.sp_1.address: 5}, expected_error=(-13, "Please enter the wallet passphrase"))

    def test_encrypted_wallet_unlocked(self):
        self.log.info("Testing unlocked encrypted wallet can send silent payments")
        wallet = self.create_funded_wallet("encrypted_unlocked", passphrase="passphrase")
        wallet.walletpassphrase("passphrase", 120)
        self.do_send(wallet, {self.sp_1.address: 5}, expect=[self.sp_1])

    def test_inputs_with_multiple_spkms(self):
        self.log.info("Testing inputs whose script is managed by more than one ScriptPubKeyMan")
        wallet = self.create_funded_wallet(
            "multiple_spkms",
            blank=True,
            descriptors=[
                desc(f"wpkh({MULTIPLE_SPKMS_EXT})"),
                internal_desc(f"wpkh({MULTIPLE_SPKMS_INT})"),
            ],
        )
        # combo() also produces the P2WPKH scripts of the active receive descriptor, including the funded one
        import_descriptors(wallet, [{"desc": descsum_create(f"combo({MULTIPLE_SPKMS_EXT})"), "timestamp": "now", "range": [0, 10]}])
        self.do_send(wallet, {self.sp_1.address: 5}, expect=[self.sp_1])

    def test_eligible_input_types(self):
        # P2WPKH for change — wallet default change type is bech32
        wpkh_change = f"wpkh({WPKH_INT})"
        for input_type, address_type, recv, change in [
            ("P2TR", "bech32m", f"tr({P2TR_EXT})", f"tr({P2TR_INT})"),
            ("P2PKH", "legacy", f"pkh({P2PKH_EXT})", wpkh_change),
            ("P2SH-P2WPKH", "p2sh-segwit", f"sh(wpkh({P2SH_EXT}))", wpkh_change),
        ]:
            self.log.info(f"Testing {input_type} inputs are eligible for silent payments")
            wallet = self.create_funded_wallet(
                f"eligible_{address_type}",
                blank=True,
                address_type=address_type,
                descriptors=[desc(recv), internal_desc(change)],
            )
            self.do_send(wallet, {self.sp_1.address: 5}, expect=[self.sp_1])

    def test_no_eligible_inputs(self):
        self.log.info("Testing wallet with only P2WSH inputs cannot send silent payments")
        wallet = self.create_funded_wallet(
            "wsh_only_wallet",
            blank=True,
            descriptors=[
                desc(f"wsh(multi(1,{NO_ELIGIBLE_EXT}))"),
                internal_desc(f"wsh(multi(1,{NO_ELIGIBLE_INT}))"),
            ],
        )
        self.do_send(
            wallet,
            {self.sp_1.address: 5},
            expected_error=(-6, "No silent payment eligible inputs were found."),
            expected_error_fund=(-4, "No silent payment eligible inputs were found."),
        )

        self.log.info("- a silent payments change address needs an eligible input only if there is change")
        regular_addr = self.funder.getnewaddress()
        assert_raises_rpc_error(-4, "No silent payment eligible inputs were found.",
                                wallet.send, outputs=[{regular_addr: 1}], options={"change_address": self.sp_1.address})
        txid = wallet.send(outputs=[{regular_addr: wallet.getbalance()}],
                           options={"change_address": self.sp_1.address, "subtract_fee_from_outputs": [0]})["txid"]
        assert_equal(len(self.nodes[0].getrawtransaction(txid, True)["vout"]), 1)
        self.generate(self.nodes[0], 1)

    def test_preset_eligible_input(self):
        self.log.info("Testing an eligible preset input allows the other inputs to be ineligible")
        wallet = self.create_funded_wallet(
            "preset_eligible_wallet",
            blank=True,
            descriptors=[
                desc(f"wsh(multi(1,{PRESET_ELIGIBLE_EXT}))"),
                internal_desc(f"wsh(multi(1,{PRESET_ELIGIBLE_INT}))"),
            ],
        )
        eligible_input = self.fund_descriptor(wallet, f"wpkh({PRESET_ELIGIBLE_WPKH})", 2)

        # Paying 5 needs the funded P2WSH coin too, which the wallet adds to the preset input
        txid = wallet.send(outputs=[{self.sp_1.address: 5}], options={"inputs": [eligible_input], "add_inputs": True})["txid"]
        assert_equal(len(self.nodes[0].getrawtransaction(txid, True)["vin"]), 2)
        self.assert_sp_outputs(txid, self.sp_1)
        self.generate(self.nodes[0], 1)

    def test_preset_ineligible_input(self):
        self.log.info("Testing ineligible preset inputs alone cannot fund a silent payments transaction")
        wallet = self.create_funded_wallet("preset_ineligible_wallet")
        ineligible_input = self.fund_descriptor(wallet, f"wsh(multi(1,{PRESET_INELIGIBLE_WSH}))", 10)

        assert_raises_rpc_error(-4, "No silent payment eligible inputs were found.",
                                wallet.send, outputs=[{self.sp_1.address: 5}], options={"inputs": [ineligible_input], "add_inputs": False})

    def test_must_add_to_wallet(self):
        self.log.info("Testing silent payments transactions must be added to the wallet when they are created")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("must_add_to_wallet")

        for options, error in [
            ({"psbt": True}, "psbt=true is not supported for silent payments transactions"),
            ({"add_to_wallet": False}, "add_to_wallet=false is not supported for silent payments transactions"),
        ]:
            assert_raises_rpc_error(-8, error, wallet.send, outputs=[{self.sp_1.address: 1}], options=options)
            assert_raises_rpc_error(-8, error, wallet.sendall, recipients=[self.sp_1.address], options=options)
        # Transactions without silent payments recipients are not affected
        wallet.send(outputs=[{self.funder.getnewaddress(): 1}], options={"add_to_wallet": False})

        # A transaction the wallet cannot fully sign would be returned as a PSBT
        _, external_pubkey = generate_keypair()
        descriptor = descsum_create(f"wsh(pk({external_pubkey.hex()}))")
        addr = node.deriveaddresses(descriptor)[0]
        external_txid = self.funder.sendtoaddress(addr, 2)
        self.generate(node, 1)
        vout = find_vout_for_address(node, external_txid, addr)
        assert_raises_rpc_error(-4, "Silent payments transactions must be fully signed by the wallet",
            wallet.send, outputs=[{self.sp_1.address: 3}],
            options={"inputs": [{"txid": external_txid, "vout": vout}], "solving_data": {"descriptors": [descriptor]}, "add_inputs": True})

        txid = wallet.sendtoaddress(self.sp_1.address, 1)
        assert_raises_rpc_error(-8, "psbtbumpfee does not support silent payments transactions, use bumpfee instead", wallet.psbtbumpfee, txid)
        new_txid = self.bumpfee(wallet, txid)
        self.mine_and_check_confirmed(wallet, new_txid)

    def test_address_reuse(self):
        self.log.info("Testing that two sends to the same SP address produce distinct outputs")
        sender = self.create_funded_wallet("address_reuse_sender", amount=5)
        self.funder.sendtoaddress(sender.getnewaddress(), 7)
        self.generate(self.nodes[0], 1)

        def output_address(txid, amount):
            return next(o["scriptPubKey"]["address"] for o in sender.getrawtransaction(txid, True)["vout"] if o["value"] == amount)

        txid1 = sender.sendtoaddress(self.sp_2.address, 4)
        txid2 = sender.sendtoaddress(self.sp_2.address, 3)
        assert_not_equal(output_address(txid1, 4), output_address(txid2, 3), error_message="Two sends to same SP address must produce distinct outputs")
        self.assert_sp_outputs(txid1, self.sp_2)
        self.assert_sp_outputs(txid2, self.sp_2)

    def test_multiple_sp_recipients(self):
        self.log.info("Testing a single tx with multiple SP recipient addresses")
        wallet = self.create_funded_wallet("multi_sp_wallet")
        self.do_send(wallet, {self.sp_1.address: 3, self.sp_2.address: 4}, expect=[self.sp_1, self.sp_2])

    def test_mixed_sp_and_regular(self):
        self.log.info("Testing a single tx with both SP and regular outputs")
        wallet = self.create_funded_wallet("mixed_output_wallet")
        regular_wallet = self.create_funded_wallet("regular_recv_wallet", amount=1)
        regular_addr = regular_wallet.getnewaddress()

        txids = self.do_send(wallet, {self.sp_1.address: 2, regular_addr: 3}, expect=[self.sp_1])
        for txid in txids.values():
            regular_vouts = [
                v for v in self.nodes[0].getrawtransaction(txid, True)["vout"]
                if v["scriptPubKey"].get("address") == regular_addr
            ]
            assert_equal(len(regular_vouts), 1)

    def test_rbf(self):
        self.log.info("Testing RBF for silent payment transactions")
        node = self.nodes[0]

        def bump_and_confirm(wallet, send_fn):
            regular_addr = self.funder.getnewaddress()

            # Plain bumpfee
            new_txid = self.bumpfee(wallet, send_fn(wallet))
            self.mine_and_check_confirmed(wallet, new_txid)
            self.assert_sp_outputs(new_txid, self.sp_1)

            # bumpfee with outputs: add a new SP output and a regular output
            new_txid = self.bumpfee(wallet, send_fn(wallet), outputs=[{self.sp_2.address: 1}, {regular_addr: 0.5}])
            self.mine_and_check_confirmed(wallet, new_txid)
            # bumpfee with outputs= replaces all outputs; sp_2 + change are the only SP output
            self.assert_sp_outputs(new_txid, self.sp_2)

            # bumpfee with outputs: replace existing outputs with a regular output (no SP output)
            new_txid = self.bumpfee(wallet, send_fn(wallet), outputs=[{regular_addr: 0.5}])
            self.mine_and_check_confirmed(wallet, new_txid)

        send_methods = [
            ("rbf_sendtoaddress", lambda w: w.sendtoaddress(self.sp_1.address, 5)),
            ("rbf_sendmany",      lambda w: w.sendmany("", {self.sp_1.address: 5})),
            ("rbf_send",          lambda w: w.send(outputs=[{self.sp_1.address: 5}])["txid"]),
        ]
        for name, send_fn in send_methods:
            wallet = self.create_funded_wallet(name)
            bump_and_confirm(wallet, send_fn)

        # sendall: use maxconf=1 to restrict sendall to one UTXO, leaving the
        # original (2+ confirmations) available as additional input for the fee bump
        wallet = self.create_funded_wallet("rbf_sendall")
        extra_addr = wallet.getnewaddress()
        self.funder.sendtoaddress(extra_addr, 5)
        self.generate(node, 1)  # rbf_sendall funding UTXO now has 2 confs; extra UTXO has 1 conf

        txid = wallet.sendall(recipients=[self.sp_1.address], options={"maxconf": 1})["txid"]
        original_input_count = len(node.getrawtransaction(txid, True)["vin"])
        new_txid = self.bumpfee(wallet, txid, fee_rate=500)
        assert_greater_than(len(node.getrawtransaction(new_txid, True)["vin"]), original_input_count)
        self.mine_and_check_confirmed(wallet, new_txid)
        self.assert_sp_outputs(new_txid, self.sp_1)

        # Test wallet unload/reload: verify m_sprecipients is persisted and RBF still works
        wallet = self.create_funded_wallet("rbf_reload")
        txid = wallet.sendtoaddress(self.sp_1.address, 5)
        node.unloadwallet("rbf_reload")
        node.loadwallet("rbf_reload")
        wallet = node.get_wallet_rpc("rbf_reload")
        new_txid = self.bumpfee(wallet, txid)
        self.mine_and_check_confirmed(wallet, new_txid)
        self.assert_sp_outputs(new_txid, self.sp_1)

        self.log.info("- a regular transaction can be bumped to a silent payments output")
        wallet = self.create_funded_wallet("rbf_regular_to_sp")
        txid = wallet.sendtoaddress(self.funder.getnewaddress(), 1)
        # A PSBT may be signed elsewhere, so psbtbumpfee cannot create silent payments outputs
        assert_raises_rpc_error(-5, "Silent payments are not supported by this RPC",
            wallet.psbtbumpfee, txid, outputs=[{self.sp_1.address: 1}])
        new_txid = self.bumpfee(wallet, txid, outputs=[{self.sp_1.address: 1}])
        self.assert_sp_outputs(new_txid, self.sp_1)
        # The replacement is now a silent payments transaction
        assert_raises_rpc_error(-8, "psbtbumpfee does not support silent payments transactions, use bumpfee instead", wallet.psbtbumpfee, new_txid)
        # Its silent payments recipient is persisted, so it can be bumped again
        newer_txid = self.bumpfee(wallet, new_txid)
        self.mine_and_check_confirmed(wallet, newer_txid)
        self.assert_sp_outputs(newer_txid, self.sp_1)

    def test_external_inputs(self):
        self.log.info("Testing external inputs that may be eligible cannot fund a silent payments transaction")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("external_inputs_sender")
        node.createwallet("external_inputs_owner")
        owner = node.get_wallet_rpc("external_inputs_owner")
        for address_type in ["bech32m", "p2sh-segwit"]:
            addr = owner.getnewaddress(address_type=address_type)
            txid = self.funder.sendtoaddress(addr, 5)
            self.generate(node, 1)
            vout = find_vout_for_address(node, txid, addr)
            assert_raises_rpc_error(-4, f"External input COutPoint({txid[:10]}, {vout}) cannot be used in a silent payments transaction",
                wallet.send, outputs=[{self.sp_1.address: 1}],
                options={"inputs": [{"txid": txid, "vout": vout}],
                         "solving_data": {"descriptors": [owner.getaddressinfo(addr)["desc"]]},
                         "add_inputs": True})

    def test_rbf_sp_change(self):
        self.log.info("Testing bumpfee with a silent payments output as original_change_index")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("rbf_sp_change")
        txid = wallet.sendtoaddress(self.sp_1.address, 5)
        sp_index = next(o["n"] for o in node.getrawtransaction(txid, True)["vout"] if o["value"] == 5)

        # The fee is deducted from the silent payments output, which is recomputed for the new inputs
        new_txid = self.bumpfee(wallet, txid, original_change_index=sp_index)
        found = self.assert_sp_outputs(new_txid, self.sp_1)
        sp_vout = next(o for o in node.getrawtransaction(new_txid, True)["vout"] if o["scriptPubKey"]["hex"] == "5120" + found[0]["output"])
        assert_greater_than(5, sp_vout["value"])

        # The silent payments change destination is persisted, so the output is found again on the next bump
        newer_txid = self.bumpfee(wallet, new_txid)
        self.mine_and_check_confirmed(wallet, newer_txid)
        self.assert_sp_outputs(newer_txid, self.sp_1)

    def test_rbf_sp_change_dropped(self):
        self.log.info("Testing bumpfee drops a silent payments change output without affecting a recipient with the same scan key")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("rbf_sp_change_dropped")
        # sp_1 and sp_3 share a scan key, so their outputs are numbered together
        txid = wallet.sendmany("", {self.sp_1.address: 0.00001, self.sp_3.address: 1}, fee_rate=1)
        sp_index = next(o["n"] for o in node.getrawtransaction(txid, True)["vout"] if o["value"] == Decimal("0.00001"))

        # Deducting the higher fee from the sp_1 output leaves less than dust, so it is dropped
        new_txid = self.bumpfee(wallet, txid, original_change_index=sp_index, fee_rate=6)
        assert_equal(len(node.getrawtransaction(new_txid, True)["vout"]), 2)
        self.assert_sp_outputs(new_txid, self.sp_1, n=0)
        self.assert_sp_outputs(new_txid, self.sp_3)

        # The remaining output is recomputed again when the replacement is bumped
        newer_txid = self.bumpfee(wallet, new_txid)
        self.mine_and_check_confirmed(wallet, newer_txid)
        self.assert_sp_outputs(newer_txid, self.sp_1, n=0)
        self.assert_sp_outputs(newer_txid, self.sp_3)

    def test_sp_change_address(self):
        self.log.info("Testing a silent payments change address")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("sp_change_address")
        # A second coin, for the bump below
        self.funder.sendtoaddress(wallet.getnewaddress(), 5)
        self.generate(node, 1)
        regular_addr = self.funder.getnewaddress()

        # The change alone makes it a silent payments transaction
        txid = wallet.send(outputs=[{regular_addr: 1}], options={"change_address": self.sp_1.address})["txid"]
        self.assert_sp_outputs(txid, self.sp_1)
        num_inputs = len(node.getrawtransaction(txid, True)["vin"])

        # The change does not return to the wallet, so bumping adds an input, which changes the
        # silent payments output. The change destination is persisted, so the output is recomputed.
        new_txid = self.bumpfee(wallet, txid)
        assert_greater_than(len(node.getrawtransaction(new_txid, True)["vin"]), num_inputs)
        self.assert_sp_outputs(new_txid, self.sp_1)
        self.mine_and_check_confirmed(wallet, new_txid)

        self.log.info("- the transaction must be added to the wallet")
        for options, error in [
            ({"psbt": True}, "psbt=true is not supported for silent payments transactions, which must be added to the wallet"),
            ({"add_to_wallet": False}, "add_to_wallet=false is not supported for silent payments transactions, which must be added to the wallet"),
        ]:
            assert_raises_rpc_error(-8, error, wallet.send, outputs=[{regular_addr: 1}], options={"change_address": self.sp_1.address, **options})
        raw = node.createrawtransaction([], [{regular_addr: 1}])
        assert_raises_rpc_error(-8, "fundrawtransaction does not support silent payments change addresses, use send instead",
            wallet.fundrawtransaction, raw, {"change_address": self.sp_1.address})
        assert_raises_rpc_error(-8, "walletcreatefundedpsbt does not support silent payments change addresses, use send instead",
            wallet.walletcreatefundedpsbt, [], [{regular_addr: 1}], 0, {"change_address": self.sp_1.address})

    def test_regular_change(self):
        self.log.info("Testing a silent payments transaction with regular change")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("regular_change")

        self.log.info("- change to a newly generated address")
        txid = wallet.send(outputs=[{self.sp_1.address: 1}])["txid"]
        self.assert_sp_outputs(txid, self.sp_1)
        change_vouts = [
            v for v in node.getrawtransaction(txid, True)["vout"]
            if "address" in v["scriptPubKey"] and wallet.getaddressinfo(v["scriptPubKey"]["address"])["ischange"]
        ]
        assert_equal(len(change_vouts), 1)

        self.log.info("- change to a custom address")
        change_addr = self.funder.getnewaddress()
        txid = wallet.send(outputs=[{self.sp_1.address: 1}], options={"change_address": change_addr})["txid"]
        self.assert_sp_outputs(txid, self.sp_1)
        change_vouts = [v for v in node.getrawtransaction(txid, True)["vout"] if v["scriptPubKey"].get("address") == change_addr]
        assert_equal(len(change_vouts), 1)
        self.generate(node, 1)

    def test_rbf_malleated_variant(self):
        self.log.info("Testing bumpfee of a malleated variant of a silent payments tx")
        node = self.nodes[0]
        wallet = self.create_funded_wallet("malleated_bump", address_type="legacy")
        p2pkh_utxo = wallet.listunspent()[0]
        # Spare coin for the bump to add
        spare_addr = wallet.getnewaddress(address_type="bech32")
        spare_txid = self.funder.sendtoaddress(spare_addr, 5)
        self.generate(node, 1)
        spare_utxo = (spare_txid, find_vout_for_address(node, spare_txid, spare_addr))

        # No change, so the bump has to add an input
        txid = wallet.sendall(recipients=[self.sp_1.address], inputs=[{"txid": p2pkh_utxo["txid"], "vout": p2pkh_utxo["vout"]}])["txid"]
        decoded = node.getrawtransaction(txid, True)
        assert_equal([(i["txid"], i["vout"]) for i in decoded["vin"]], [(p2pkh_utxo["txid"], p2pkh_utxo["vout"])])
        assert_equal(len(decoded["vout"]), 1)

        # Third-party malleation: prepend OP_TRUE to the P2PKH scriptSig. Non-standard, but valid
        tx = tx_from_hex(node.getrawtransaction(txid))
        tx.vin[0].scriptSig = CScript(bytes(CScript([OP_TRUE])) + bytes(tx.vin[0].scriptSig))
        malleated_txid = tx.txid_hex
        assert_not_equal(malleated_txid, txid)
        block = self.generateblock(node, self.funder.getnewaddress(), [tx.serialize().hex()])["hash"]
        self.assert_sp_outputs(malleated_txid, self.sp_1)
        assert_equal(wallet.gettransaction(txid)["confirmations"], -1)

        # The variant is non-standard, so it is not re-accepted to the mempool after the reorg
        node.invalidateblock(block)
        node.syncwithvalidationinterfacequeue()
        assert malleated_txid not in node.getrawmempool()
        assert_equal(wallet.gettransaction(malleated_txid)["confirmations"], 0)

        # The variant's SP recipients survive a reload
        wallet.unloadwallet()
        node.loadwallet("malleated_bump")

        # The bump adds an input and recomputes the SP output for the new inputs
        new_txid = wallet.bumpfee(malleated_txid)["txid"]
        bump_inputs = {(i["txid"], i["vout"]) for i in node.getrawtransaction(new_txid, True)["vin"]}
        assert_equal(bump_inputs, {(p2pkh_utxo["txid"], p2pkh_utxo["vout"]), spare_utxo})
        self.assert_sp_outputs(new_txid, self.sp_1)
        self.mine_and_check_confirmed(wallet, new_txid)

    def test_reorg(self):
        self.log.info("Testing wallet state is correctly updated after a reorg of SP transactions")
        wallet = self.create_funded_wallet("reorg_wallet")
        txids = list(self.do_send(wallet, {self.sp_1.address: 5}, mine_block=False).values())

        self.generate(self.nodes[0], 3)
        for txid in txids:
            assert_equal(wallet.gettransaction(txid)["confirmations"], 3)

        for _ in range(3):
            self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        self.nodes[0].syncwithvalidationinterfacequeue()

        for txid in txids:
            assert_greater_than_or_equal(0, wallet.gettransaction(txid)["confirmations"])

        # Advance mock time so re-mined blocks have different timestamps from the
        # now-invalid blocks. In fast regtest, same parent + same txs + same timestamp
        # produces identical block hashes, rejected as "duplicate-invalid".
        tip_time = self.nodes[0].getblockchaininfo()["time"]
        self.nodes[0].setmocktime(tip_time + 600)

        self.generate(self.nodes[0], 5)
        self.nodes[0].setmocktime(0)

        for txid in txids:
            assert_greater_than_or_equal(wallet.gettransaction(txid)["confirmations"], 1)

    def run_test(self):
        self.nodes[0].createwallet("funder")
        self.funder = self.nodes[0].get_wallet_rpc("funder")
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 15, self.funder.getnewaddress())

        self.sp_1 = SPRecipient()
        self.sp_2 = SPRecipient()
        # Shares its scan key with sp_1
        self.sp_3 = SPRecipient(scan_key=self.sp_1.scan_key)

        self.test_watch_only_wallet()
        self.test_encrypted_wallet_locked()
        self.test_encrypted_wallet_unlocked()

        self.test_eligible_input_types()
        self.test_no_eligible_inputs()
        self.test_preset_eligible_input()
        self.test_preset_ineligible_input()
        self.test_taproot_script_path_inputs()
        self.test_uncompressed_p2pkh_inputs()
        self.test_inputs_with_multiple_spkms()

        self.test_address_reuse()

        self.test_multiple_sp_recipients()
        self.test_mixed_sp_and_regular()

        self.test_external_inputs()
        self.test_must_add_to_wallet()

        self.test_rbf()
        self.test_rbf_sp_change()
        self.test_rbf_malleated_variant()
        self.test_rbf_sp_change_dropped()
        self.test_sp_change_address()
        self.test_regular_change()
        self.test_reorg()


if __name__ == "__main__":
    SilentPaymentsSendingTest(__file__).main()
