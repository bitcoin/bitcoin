#!/usr/bin/env python3
from contextlib import contextmanager
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.key import ECKey
from test_framework.segwit_addr import bech32_encode, convertbits, Encoding
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_not_equal,
    assert_raises_rpc_error,
)

XPRV = "tprv8ZgxMBicQKsPevADjDCWsa6DfhkVXicu8NQUzfibwX2MexVwW4tCec5mXdCW8kJwkzBRRmAay1KZya4WsehVvjTGVW6JLqiqd8DdZ4xSg52"
XPUB = "tpubD6NzVbkrYhZ4YNXVQbNhMK1WqguFsUXceaVJKbmno2aZ3B6QfbMeraaYvnBSGpV3vxLyTTK9DYT1yoEck4XUScMzXoQ2U2oSmE2JyMedq3H"

P2TR_EXT  = f"{XPRV}/86h/1h/0h/0/*"
P2TR_INT  = f"{XPRV}/86h/1h/0h/1/*"
P2PKH_EXT = f"{XPRV}/44h/1h/0h/0/*"
P2SH_EXT  = f"{XPRV}/49h/1h/0h/0/*"
WPKH_INT  = f"{XPRV}/84h/1h/0h/1/*"

# Per-test subtrees for non-standard paths; each test owns one numeric namespace so
# wallets that share XPRV cannot accidentally see each other's UTXOs.
PREFER_ELIGIBLE_PKH    = f"{XPRV}/10/0/*"   # test_prefer_eligible_inputs — eligible recv
PREFER_ELIGIBLE_WSH    = f"{XPRV}/10/1/*"   # test_prefer_eligible_inputs — non-eligible
NO_ELIGIBLE_EXT        = f"{XPRV}/11/0/*"   # test_no_eligible_inputs — recv
NO_ELIGIBLE_INT        = f"{XPRV}/11/1/*"   # test_no_eligible_inputs — change
MIXED_WSH              = f"{XPRV}/12/0/*"   # test_mixed_input_types — non-eligible WSH


def make_sp_addr(scan_key, spend_key, hrp="sprt"):
    """Encode an SP address from two ECKey objects (scan and spend)."""
    payload = scan_key.get_pubkey().get_bytes() + spend_key.get_pubkey().get_bytes()
    return bech32_encode(Encoding.BECH32M, hrp, [0] + convertbits(list(payload), 8, 5))


def desc(template):
    return {"desc": descsum_create(template), "timestamp": "now", "active": True}

def internal_desc(template):
    d = desc(template)
    d["internal"] = True
    return d

class SilentPaymentsSendingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txindex=1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_funded_wallet(
        self,
        node,
        name,
        *,
        amount=25,
        blank=False,
        descriptors=None,
        passphrase=None,
        disable_private_keys=False,
        address_type=None,
    ):
        kwargs = {"wallet_name": name}
        if blank:
            kwargs["blank"] = True
        if passphrase:
            kwargs["passphrase"] = passphrase
        if disable_private_keys:
            kwargs["disable_private_keys"] = True
        node.createwallet(**kwargs)
        wallet = node.get_wallet_rpc(name)
        if descriptors:
            wallet.importdescriptors(descriptors)
        recv_addr = (
            wallet.getnewaddress(address_type=address_type)
            if address_type
            else wallet.getnewaddress()
        )
        self.funder.sendtoaddress(recv_addr, amount)
        self.generate(self.nodes[0], 1)
        return wallet

    def do_send(self, wallet, node, recipients, *, expected_error=None, expected_error_fund=None, mine_block=True):
        results = {}
        addr0 = recipients[0]["address"]
        amt0 = recipients[0]["amount"]
        outputs = [{r["address"]: r["amount"]} for r in recipients]
        sendmany_dict = {r["address"]: r["amount"] for r in recipients}

        def _collect(txid):
            tx = node.getrawtransaction(txid, True)
            return {"txid": txid, "vouts": tx["vout"]}

        single = len(recipients) == 1

        if expected_error is not None:
            code, msg = expected_error
            # send/walletcreatefundedpsbt go through different code path,
            # so they may raise a different error code/message
            fund_code, fund_msg = expected_error_fund if expected_error_fund is not None else expected_error
            if single:
                assert_raises_rpc_error(code, msg, wallet.sendtoaddress, addr0, amt0)
            assert_raises_rpc_error(code, msg, wallet.sendmany, "", sendmany_dict)
            assert_raises_rpc_error(fund_code, fund_msg, wallet.send, outputs=outputs)
            assert_raises_rpc_error(fund_code, fund_msg, wallet.walletcreatefundedpsbt, [], outputs)
            return {}

        if single:
            txid = wallet.sendtoaddress(addr0, amt0)
            assert txid in node.getrawmempool()
            assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
            results["sendtoaddress"] = _collect(txid)

        txid = wallet.sendmany("", sendmany_dict)
        assert txid in node.getrawmempool()
        assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
        results["sendmany"] = _collect(txid)

        txid = wallet.send(outputs=outputs)["txid"]
        assert txid in node.getrawmempool()
        assert_equal(wallet.gettransaction(txid)["confirmations"], 0)
        results["send"] = _collect(txid)

        funded_psbt = wallet.walletcreatefundedpsbt([], outputs)["psbt"]
        processed = wallet.walletprocesspsbt(funded_psbt)
        assert processed["complete"], "PSBT signing incomplete — wallet may be locked"
        finalized = node.finalizepsbt(processed["psbt"])
        assert finalized["complete"]
        txid = node.sendrawtransaction(finalized["hex"])
        assert txid in node.getrawmempool()
        results["psbt"] = _collect(txid)

        txid = wallet.sendall(recipients=[r["address"] for r in recipients])["txid"]
        assert txid in node.getrawmempool()
        results["sendall"] = _collect(txid)

        if mine_block:
            self.generate(node, 1)
            for rpc_result in results.values():
                assert_equal(wallet.gettransaction(rpc_result["txid"])["confirmations"], 1)

        return results

    def assert_sp_outputs(self, txid, *recipients):
        """Assert each (scan_key, spend_key, expected_n) recipient has exactly expected_n SP outputs in txid."""
        tx_hex = self.nodes[0].getrawtransaction(txid)
        for scan_key, spend_key, expected_n in recipients:
            found = self.nodes[0].scantxforsilentpayments(
                tx_hex,
                scan_key.get_bytes().hex(),
                spend_key.get_pubkey().get_bytes().hex(),
            )
            assert_equal(len(found), expected_n)

    def test_watch_only_wallet(self):
        self.log.info("Testing watch-only wallet cannot send silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "watch_only",
            blank=True,
            disable_private_keys=True,
            descriptors=[{
                "desc": descsum_create(f"wpkh({XPUB}/0/*)"),
                "timestamp": "now",
                "active": True,
                "keypool": True,
                "range": [0, 100],
                "watchonly": True,
            }],
        )
        self.do_send(
            wallet,
            self.nodes[0],
            [{"address": self.sp_addr_1, "amount": 5}],
            expected_error=(-4, "Private keys are disabled for this wallet"),
            expected_error_fund=(-4, "Silent payments require access to private keys to build transactions."),
        )

    def test_encrypted_wallet_locked(self):
        self.log.info("Testing locked encrypted wallet cannot send silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "encrypted_locked",
            passphrase="passphrase",
        )
        self.do_send(
            wallet,
            self.nodes[0],
            [{"address": self.sp_addr_1, "amount": 5}],
            expected_error=(-13, "Please enter the wallet passphrase"),
        )

    def test_encrypted_wallet_unlocked(self):
        self.log.info("Testing unlocked encrypted wallet can send silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "encrypted_unlocked",
            passphrase="passphrase",
        )
        wallet.walletpassphrase("passphrase", 120)
        results = self.do_send(wallet, self.nodes[0], [{"address": self.sp_addr_1, "amount": 5}])
        for r in results.values():
            self.assert_sp_outputs(r["txid"], (self.scan_key_1, self.spend_key_1, 1))

    def test_p2tr_inputs(self):
        self.log.info("Testing P2TR inputs are eligible for silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "p2tr_wallet",
            blank=True,
            address_type="bech32m",
            descriptors=[
                desc(f"tr({P2TR_EXT})"),
                internal_desc(f"tr({P2TR_INT})"),
            ],
        )
        results = self.do_send(wallet, self.nodes[0], [{"address": self.sp_addr_1, "amount": 5}])
        for r in results.values():
            self.assert_sp_outputs(r["txid"], (self.scan_key_1, self.spend_key_1, 1))

    def test_p2pkh_inputs(self):
        self.log.info("Testing P2PKH inputs are eligible for silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "p2pkh_wallet",
            blank=True,
            address_type="legacy",
            descriptors=[
                desc(f"pkh({P2PKH_EXT})"),
                # P2WPKH for change — wallet default change type is bech32
                internal_desc(f"wpkh({WPKH_INT})"),
            ],
        )
        results = self.do_send(wallet, self.nodes[0], [{"address": self.sp_addr_1, "amount": 5}])
        for r in results.values():
            self.assert_sp_outputs(r["txid"], (self.scan_key_1, self.spend_key_1, 1))

    def test_p2sh_p2wpkh_inputs(self):
        self.log.info("Testing P2SH-P2WPKH inputs are eligible for silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "p2sh_p2wpkh_wallet",
            blank=True,
            address_type="p2sh-segwit",
            descriptors=[
                desc(f"sh(wpkh({P2SH_EXT}))"),
                # P2WPKH for change — wallet default change type is bech32
                internal_desc(f"wpkh({WPKH_INT})"),
            ],
        )
        results = self.do_send(wallet, self.nodes[0], [{"address": self.sp_addr_1, "amount": 5}])
        for r in results.values():
            self.assert_sp_outputs(r["txid"], (self.scan_key_1, self.spend_key_1, 1))

    def test_prefer_eligible_inputs(self):
        self.log.info("Testing wallet selects eligible inputs over non-eligible inputs")
        self.nodes[0].createwallet("prefer_eligible", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("prefer_eligible")
        eligible_desc = descsum_create(f"pkh({PREFER_ELIGIBLE_PKH})")
        non_eligible_desc = descsum_create(f"wsh(multi(1,{PREFER_ELIGIBLE_WSH}))")
        wallet.importdescriptors([
            desc(f"pkh({PREFER_ELIGIBLE_PKH})"),
            desc(f"wsh(multi(1,{PREFER_ELIGIBLE_WSH}))"),
            # P2WPKH for change — wallet default change type is bech32
            internal_desc(f"wpkh({WPKH_INT})"),
        ])

        def fund(descriptor, amount):
            addr = self.nodes[0].deriveaddresses(descriptor, [0, 0])[0]
            self.funder.sendtoaddress(addr, amount)
            self.generate(self.nodes[0], 1)

        def verify_inputs(txid):
            """Assert the transaction has both an eligible and an ineligible input."""
            tx = self.nodes[0].getrawtransaction(txid, True)
            input_types = set()
            for vin in tx["vin"]:
                prev = self.nodes[0].getrawtransaction(vin["txid"], True)
                input_types.add(prev["vout"][vin["vout"]]["scriptPubKey"]["type"])
            assert_equal(len(input_types), 2)
            assert "pubkeyhash" in input_types, f"{txid}: expected P2PKH input"
            assert "witness_v0_scripthash" in input_types, f"{txid}: expected WSH input"

        @contextmanager
        def funded_wallet():
            fund(eligible_desc, 10)
            fund(non_eligible_desc, 10)
            fund(non_eligible_desc, 3)
            yield
            if wallet.listunspent(minconf=1):
                wallet.sendall(recipients=[self.funder.getnewaddress()])
                self.generate(self.nodes[0], 1)

        with funded_wallet():
            txid = wallet.sendtoaddress(self.sp_addr_1, 12)
            verify_inputs(txid)
            self.generate(self.nodes[0], 1)
            self.assert_sp_outputs(txid, (self.scan_key_1, self.spend_key_1, 1))

        with funded_wallet():
            txid = wallet.sendmany("", {self.sp_addr_1: 12})
            verify_inputs(txid)
            self.generate(self.nodes[0], 1)
            self.assert_sp_outputs(txid, (self.scan_key_1, self.spend_key_1, 1))

        with funded_wallet():
            txid = wallet.send(outputs=[{self.sp_addr_1: 12}])["txid"]
            verify_inputs(txid)
            self.generate(self.nodes[0], 1)
            self.assert_sp_outputs(txid, (self.scan_key_1, self.spend_key_1, 1))

        with funded_wallet():
            funded_psbt = wallet.walletcreatefundedpsbt([], [{self.sp_addr_1: 12}])["psbt"]
            processed = wallet.walletprocesspsbt(funded_psbt)
            assert processed["complete"], "PSBT signing incomplete"
            finalized = self.nodes[0].finalizepsbt(processed["psbt"])
            assert finalized["complete"]
            txid = self.nodes[0].sendrawtransaction(finalized["hex"])
            verify_inputs(txid)
            self.generate(self.nodes[0], 1)
            self.assert_sp_outputs(txid, (self.scan_key_1, self.spend_key_1, 1))

    def test_no_eligible_inputs(self):
        self.log.info("Testing wallet with only P2WSH inputs cannot send silent payments")
        wallet = self.create_funded_wallet(
            self.nodes[0],
            "wsh_only_wallet",
            blank=True,
            descriptors=[
                desc(f"wsh(multi(1,{NO_ELIGIBLE_EXT}))"),
                internal_desc(f"wsh(multi(1,{NO_ELIGIBLE_INT}))"),
            ],
        )
        self.do_send(
            wallet,
            self.nodes[0],
            [{"address": self.sp_addr_1, "amount": 5}],
            expected_error=(-6, "No silent payment eligible inputs were found."),
            expected_error_fund=(-4, "No silent payment eligible inputs were found."),
        )

    def test_address_reuse(self):
        self.log.info("Testing that two sends to the same SP address produce distinct outputs")
        self.nodes[0].createwallet(wallet_name="address_reuse_sender")
        sender = self.nodes[0].get_wallet_rpc("address_reuse_sender")
        sender_address = sender.getnewaddress()

        self.funder.send(outputs=[{sender_address: 5}])
        self.funder.send(outputs=[{sender_address: 7}])
        self.generate(self.nodes[0], 8, sync_fun=self.no_op)

        txid1 = sender.sendtoaddress(address=self.sp_addr_2, amount=4)
        txid2 = sender.sendtoaddress(address=self.sp_addr_2, amount=3)

        output1 = ""
        output2 = ""
        for output in sender.getrawtransaction(txid1, True)["vout"]:
            if output["value"] == 4.0:
                output1 = output["scriptPubKey"]["address"]
        for output in sender.getrawtransaction(txid2, True)["vout"]:
            if output["value"] == 3.0:
                output2 = output["scriptPubKey"]["address"]

        assert_not_equal(output1, output2, error_message="Two sends to same SP address must produce distinct outputs")
        self.assert_sp_outputs(txid1, (self.scan_key_2, self.spend_key_2, 1))
        self.assert_sp_outputs(txid2, (self.scan_key_2, self.spend_key_2, 1))

    def test_multiple_sp_recipients(self):
        self.log.info("Testing a single tx with multiple SP recipient addresses")
        wallet = self.create_funded_wallet(self.nodes[0], "multi_sp_wallet")
        results = self.do_send(
            wallet,
            self.nodes[0],
            [
                {"address": self.sp_addr_1, "amount": 3},
                {"address": self.sp_addr_2, "amount": 4},
            ],
        )
        for result in results.values():
            self.assert_sp_outputs(result["txid"],
                (self.scan_key_1, self.spend_key_1, 1),
                (self.scan_key_2, self.spend_key_2, 1))

    def test_mixed_sp_and_regular(self):
        self.log.info("Testing a single tx with both SP and regular outputs")
        wallet = self.create_funded_wallet(self.nodes[0], "mixed_output_wallet")
        regular_wallet = self.create_funded_wallet(self.nodes[0], "regular_recv_wallet", amount=1)
        regular_addr = regular_wallet.getnewaddress()

        results = self.do_send(
            wallet,
            self.nodes[0],
            [
                {"address": self.sp_addr_1, "amount": 2},
                {"address": regular_addr, "amount": 3},
            ],
        )
        for result in results.values():
            regular_vouts = [
                v for v in result["vouts"]
                if v["scriptPubKey"].get("address") == regular_addr
            ]
            assert_equal(len(regular_vouts), 1)
            self.assert_sp_outputs(result["txid"], (self.scan_key_1, self.spend_key_1, 1))

    def test_rbf(self):
        self.log.info("Testing RBF for silent payment transactions")
        node = self.nodes[0]

        def bump_and_confirm(wallet, send_fn):
            regular_addr = self.funder.getnewaddress()

            # Plain bumpfee
            txid = send_fn(wallet)
            assert txid in node.getrawmempool()
            new_txid = wallet.bumpfee(txid)["txid"]
            assert new_txid in node.getrawmempool()
            assert txid not in node.getrawmempool()
            self.generate(node, 1)
            assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)
            self.assert_sp_outputs(new_txid, (self.scan_key_1, self.spend_key_1, 1))

            # bumpfee with outputs: add a new SP output and a regular output
            txid = send_fn(wallet)
            assert txid in node.getrawmempool()
            new_txid = wallet.bumpfee(txid, outputs=[{self.sp_addr_2: 1}, {regular_addr: 0.5}])["txid"]
            assert new_txid in node.getrawmempool()
            assert txid not in node.getrawmempool()
            self.generate(node, 1)
            assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)
            # bumpfee with outputs= replaces all outputs; sp_addr_2 + change are the only SP output
            self.assert_sp_outputs(new_txid, (self.scan_key_2, self.spend_key_2, 1))

            # psbtbumpfee with outputs: replace existing outputs with sp_addr_2
            txid = send_fn(wallet)
            assert txid in node.getrawmempool()
            bumped_psbt = wallet.psbtbumpfee(txid, outputs=[{self.sp_addr_2: 1}])["psbt"]
            processed = wallet.walletprocesspsbt(bumped_psbt)
            assert processed["complete"]
            finalized = node.finalizepsbt(processed["psbt"])
            assert finalized["complete"]
            new_txid = node.sendrawtransaction(finalized["hex"])
            assert new_txid in node.getrawmempool()
            assert txid not in node.getrawmempool()
            self.generate(node, 1)
            assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)
            # psbtbumpfee with outputs= replaces all outputs; sp_addr_2 + change are the only SP output
            self.assert_sp_outputs(new_txid, (self.scan_key_2, self.spend_key_2, 1))

            # bumpfee with outputs: replace existing outputs with a regular output (no SP output)
            txid = send_fn(wallet)
            assert txid in node.getrawmempool()
            new_txid = wallet.bumpfee(txid, outputs=[{regular_addr: 0.5}])["txid"]
            assert new_txid in node.getrawmempool()
            assert txid not in node.getrawmempool()
            self.generate(node, 1)
            assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)

        def send_psbt(w):
            psbt = w.walletcreatefundedpsbt([], [{self.sp_addr_1: 5}], 0)["psbt"]
            return node.sendrawtransaction(node.finalizepsbt(w.walletprocesspsbt(psbt)["psbt"])["hex"])

        send_methods = [
            ("rbf_sendtoaddress", lambda w: w.sendtoaddress(self.sp_addr_1, 5)),
            ("rbf_sendmany",      lambda w: w.sendmany("", {self.sp_addr_1: 5})),
            ("rbf_send",          lambda w: w.send(outputs=[{self.sp_addr_1: 5}])["txid"]),
            ("rbf_psbt",          send_psbt),
        ]
        for name, send_fn in send_methods:
            wallet = self.create_funded_wallet(node, name)
            bump_and_confirm(wallet, send_fn)

        # sendall: use maxconf=1 to restrict sendall to one UTXO, leaving the
        # original (2+ confirmations) available as additional input for the fee bump
        wallet = self.create_funded_wallet(node, "rbf_sendall", amount=25)
        extra_addr = wallet.getnewaddress()
        self.funder.sendtoaddress(extra_addr, 5)
        self.generate(node, 1)  # rbf_sendall funding UTXO now has 2 confs; extra UTXO has 1 conf

        txid = wallet.sendall(recipients=[self.sp_addr_1], options={"maxconf": 1})["txid"]
        assert txid in node.getrawmempool()
        original_input_count = len(node.getrawtransaction(txid, True)["vin"])
        result = wallet.bumpfee(txid, fee_rate=500)
        new_txid = result["txid"]
        assert new_txid in node.getrawmempool()
        assert txid not in node.getrawmempool()
        assert_greater_than(len(node.getrawtransaction(new_txid, True)["vin"]), original_input_count)
        self.generate(node, 1)
        assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)
        self.assert_sp_outputs(new_txid, (self.scan_key_1, self.spend_key_1, 1))

        # Test wallet unload/reload: verify m_sprecipients is persisted and RBF still works
        wallet = self.create_funded_wallet(node, "rbf_reload")
        txid = wallet.sendtoaddress(self.sp_addr_1, 5)
        assert txid in node.getrawmempool()
        node.unloadwallet("rbf_reload")
        node.loadwallet("rbf_reload")
        wallet = node.get_wallet_rpc("rbf_reload")
        new_txid = wallet.bumpfee(txid)["txid"]
        assert new_txid in node.getrawmempool()
        assert txid not in node.getrawmempool()
        self.generate(node, 1)
        assert_equal(wallet.gettransaction(new_txid)["confirmations"], 1)
        self.assert_sp_outputs(new_txid, (self.scan_key_1, self.spend_key_1, 1))

    def test_reorg(self):
        self.log.info("Testing wallet state is correctly updated after a reorg of SP transactions")
        wallet = self.create_funded_wallet(self.nodes[0], "reorg_wallet")

        results = self.do_send(
            wallet,
            self.nodes[0],
            [{"address": self.sp_addr_1, "amount": 5}],
            mine_block=False,
        )
        txids = [r["txid"] for r in results.values()]

        mempool = self.nodes[0].getrawmempool()
        for txid in txids:
            assert txid in mempool
            assert_equal(wallet.gettransaction(txid)["confirmations"], 0)

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

        self.generate(self.nodes[0], 5, sync_fun=self.no_op)
        self.nodes[0].setmocktime(0)

        for txid in txids:
            assert_greater_than_or_equal(wallet.gettransaction(txid)["confirmations"], 1)

    def run_test(self):
        self.nodes[0].createwallet("funder")
        self.funder = self.nodes[0].get_wallet_rpc("funder")
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 15, self.funder.getnewaddress())

        self.scan_key_1, self.spend_key_1 = ECKey(), ECKey()
        self.scan_key_1.generate()
        self.spend_key_1.generate()
        self.sp_addr_1 = make_sp_addr(self.scan_key_1, self.spend_key_1)

        self.scan_key_2, self.spend_key_2 = ECKey(), ECKey()
        self.scan_key_2.generate()
        self.spend_key_2.generate()
        self.sp_addr_2 = make_sp_addr(self.scan_key_2, self.spend_key_2)

        self.test_watch_only_wallet()
        self.test_encrypted_wallet_locked()
        self.test_encrypted_wallet_unlocked()

        self.test_p2tr_inputs()
        self.test_p2pkh_inputs()
        self.test_p2sh_p2wpkh_inputs()
        self.test_prefer_eligible_inputs()
        self.test_no_eligible_inputs()

        self.test_address_reuse()

        self.test_multiple_sp_recipients()
        self.test_mixed_sp_and_regular()

        self.test_rbf()
        self.test_reorg()


if __name__ == "__main__":
    SilentPaymentsSendingTest(__file__).main()
