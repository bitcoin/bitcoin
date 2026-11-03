#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test decentralized masternode shares (shared collateral, reward split, dissolution)."""

import base64
from copy import deepcopy
import struct
from decimal import Decimal

from test_framework.descriptors import descsum_create
from test_framework.address import key_to_p2pkh
from test_framework.crypto import secp256k1
from test_framework.key import ECKey, ORDER, rfc6979_nonce
from test_framework.messages import (
    COIN,
    CBlock,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    from_hex,
    hash256,
    ser_string,
    tx_from_hex,
)
from test_framework.script import CScript
from test_framework.test_framework import DashTestFramework, p2p_port
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    get_bip9_details,
    softfork_active,
)
from test_framework.wallet_util import bytes_to_wif, get_generate_key

# Keep the earliest activation height above the ~119 blocks the framework setup mines, so
# run_test starts with v24 locked in but not yet active and can exercise pre-activation rules
V24_MIN_ACTIVATION_HEIGHT = 250
# Above the ~1300 blocks the regular shared masternode checks mine, which run before evo_shares
# activates
EVO_SHARES_MIN_ACTIVATION_HEIGHT = 2000
COLLATERAL = 1000 * COIN
EVO_COLLATERAL = 4000 * COIN
SHARED_COLLATERAL_SCRIPT = "04445348437551"
# Must comfortably exceed the blocks mined between the first registration and its early-period
# dissolution checks (~16 worst case), so the masternode is still inside the early period there
EARLY_PERIOD_BLOCKS = 30
EARLY_PENALTY = 50 * COIN
DISSOLVE_FEE = 100000
MAX_DISSOLVE_FEE = 1000000
TRANSACTION_PROVIDER_DISSOLVE = 10
TRANSACTION_PROVIDER_UPDATE_SHARE = 11
TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR = 12


def sign_compact(key, digest):
    """A recoverable compact signature by `key` over `digest`, as CHashSigner::SignHash produces"""
    z = int.from_bytes(digest, "big")
    k = int.from_bytes(rfc6979_nonce(key.get_bytes() + digest), "big")
    R = k * secp256k1.G
    r = int(R.x) % ORDER
    s = pow(k, -1, ORDER) * (z + int.from_bytes(key.get_bytes(), "big") * r) % ORDER
    recid = (0 if R.y.is_even() else 1) | (2 if int(R.x) >= ORDER else 0)
    if s > ORDER // 2:
        s = ORDER - s
        recid ^= 1
    return bytes([27 + 4 + recid]) + r.to_bytes(32, "big") + s.to_bytes(32, "big")


def prodis_sign_hash(tx, sig_count):
    """CProDisTx::MakeSignHash: the payload prefix is its 2-byte version, proTxHash and actorIndex"""
    payload = tx.vExtraPayload
    data = ser_string(b"DashSharedMNDissolve") + payload[:2]
    data += struct.pack("<hHI", tx.nVersion, tx.nType, tx.nLockTime)
    data += b"".join(txin.prevout.serialize() for txin in tx.vin)
    data += b"".join(struct.pack("<I", txin.nSequence) for txin in tx.vin)
    data += b"".join(out.serialize() for out in tx.vout)
    return hash256(data + payload[2:36] + bytes([sig_count]))


class MasternodeSharesTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        # evo_shares is activated by miners here; as shipped, masternodes do not sign it
        self.set_dash_test_params(2, 0, extra_args=[[
            f"-vbparams=v24:{self.mocktime}:999999999999:{V24_MIN_ACTIVATION_HEIGHT}:10:8:6:5:0",
            f"-vbparams=evo_shares:{self.mocktime}:999999999999:{EVO_SHARES_MIN_ACTIVATION_HEIGHT}:10:8:6:5:0",
        ]] * 2)

    def activate(self, name):
        while not softfork_active(self.nodes[0], name):
            self.bump_mocktime(50)
            self.generate(self.nodes[0], 50, sync_fun=self.no_op)

    def build_funding_tx(self, node, collateral=COLLATERAL):
        """Returns hex of a transaction with enough inputs to fund the collateral plus fee
        and a change output, but without the collateral output itself (shared_register_prepare
        appends it)."""
        dummy = node.getnewaddress()
        raw = node.createrawtransaction([], {dummy: collateral // COIN})
        funded = node.fundrawtransaction(raw, {"feeRate": 0.00010000})["hex"]
        tx = tx_from_hex(funded)
        vout = [out for out in tx.vout if out.nValue != collateral]
        assert_equal(len(vout), len(tx.vout) - 1)
        tx.vout = vout
        return tx.serialize().hex()

    def register_shared(self, node, shares, port_offset, early_period_blocks=EARLY_PERIOD_BLOCKS,
                        early_penalty=EARLY_PENALTY):
        operator_key = node.bls("generate")["public"]
        voting_address = node.getnewaddress()
        funding_hex = self.build_funding_tx(node)
        prepared = node.protx(
            "shared_register_prepare", funding_hex, shares, f"127.0.0.1:{p2p_port(port_offset)}",
            operator_key, voting_address, 0, early_period_blocks, early_penalty)
        assert_equal(len(prepared["consentHash"]), 64)

        signed_terms = node.protx("shared_sign", prepared["tx"])
        assert_equal(signed_terms["type"], "registration")
        assert_equal(signed_terms["terms"]["earlyPenalty"], early_penalty)
        assert_equal(len(signed_terms["terms"]["shares"]), len(shares))
        assert_equal("warning" in signed_terms, early_penalty == 0)
        sigs = signed_terms["signatures"]
        assert_equal(sorted(s["shareIndex"] for s in sigs), list(range(len(shares))))
        combined = node.protx("shared_combine", prepared["tx"], sigs)

        signed = node.signrawtransactionwithwallet(combined)
        assert_equal(signed["complete"], True)
        protx_hash = node.sendrawtransaction(signed["hex"])
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        return protx_hash, prepared["collateralIndex"]

    def owner_gbt_payees(self, node):
        return [p for p in node.getblocktemplate()["masternode"] if p["script"] != "6a"]

    def build_lifecycle_tx(self, n_type, payload):
        """Returns hex of a minimally well-formed special transaction of the given type. The dummy
        input never resolves, but block connection runs special-transaction validation before input
        lookups, so the reject reason isolates the special-tx rule under test."""
        tx = CTransaction()
        tx.nVersion = 3
        tx.nType = n_type
        tx.vExtraPayload = payload
        tx.vin.append(CTxIn(COutPoint(1, 0)))
        tx.vout.append(CTxOut(1, CScript(b"\x51")))
        return tx.serialize().hex()

    @staticmethod
    def high_s(signature_b64):
        """Return the high-S malleation of a base64 compact signature: s' = N - s with the recovery id
        parity flipped. The plain recoverable verify accepts it (same key), the canonical check must not."""
        sig = base64.b64decode(signature_b64)
        header = sig[0] - 27
        flipped = 27 + (header & 4) + ((header & 3) ^ 1)
        s_neg = (ORDER - int.from_bytes(sig[33:], "big")).to_bytes(32, "big")
        return base64.b64encode(bytes([flipped]) + sig[1:33] + s_neg).decode()

    def assert_rejected_transaction(self, node, tx, reason, *, mempool_reason=None):
        """Bypass wallet construction and mempool policy with an independently submitted block."""
        tip = node.getbestblockhash()
        mempool = set(node.getrawmempool())
        result, = node.testmempoolaccept([tx.serialize().hex()])
        assert_equal(result["allowed"], False)
        assert_equal(result["reject-reason"], mempool_reason or reason)
        address = node.get_wallet_rpc(self.default_wallet_name).getnewaddress()
        block = from_hex(CBlock(), node.generateblock(address, [], False, invalid_call=False)["hex"])
        block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), reason)
        assert_equal(node.getbestblockhash(), tip)
        assert_equal(set(node.getrawmempool()), mempool)

    def test_invalid_dissolutions(self, node, miner, wallets, owner_keys, protx_hash, shares):
        self.log.info("Every actor's dissolution protects all refund destinations and principal amounts")
        state = miner.protx("info", protx_hash)["state"]
        penalty = 10 * COIN
        assert_greater_than(state["registeredHeight"] + 100, node.getblockcount() + 1)
        attacker_script = CScript(bytes.fromhex(miner.getaddressinfo(miner.getnewaddress())["scriptPubKey"]))
        for actor, wallet in enumerate(wallets):
            self.log.info("Checking invalid dissolutions by participant %d", actor)
            good = tx_from_hex(wallet.protx("shared_dissolve", protx_hash, actor, DISSOLVE_FEE, False))
            assert_equal(node.testmempoolaccept([good.serialize().hex()])[0]["allowed"], True)
            non_actors = [share for i, share in enumerate(shares) if i != actor]
            total = sum(share["amount"] for share in non_actors)
            bonuses = [penalty * share["amount"] // total for share in non_actors[:-1]]
            bonuses.append(penalty - sum(bonuses))
            assert_equal([out.nValue for out in good.vout[:-1]],
                         [share["amount"] + bonus for share, bonus in zip(non_actors, bonuses)])
            assert_equal(good.vout[-1].nValue, shares[actor]["amount"] - penalty - DISSOLVE_FEE)

            for index, share in enumerate(non_actors):
                bad = deepcopy(good)
                stolen = bad.vout[index].nValue - share["amount"] + 1
                bad.vout[index].nValue -= stolen
                bad.vout[-1].nValue += stolen
                self.assert_rejected_transaction(node, bad, "bad-prodis-penalty-floor")
            for index in range(len(shares)):
                bad = deepcopy(good)
                bad.vout[index].scriptPubKey = attacker_script
                self.assert_rejected_transaction(node, bad, "bad-prodis-payee")

            bad = deepcopy(good)
            bad.vout[-1].nValue -= MAX_DISSOLVE_FEE - DISSOLVE_FEE + 1
            self.assert_rejected_transaction(node, bad, "bad-prodis-fee")
            bad = deepcopy(good)
            bad.vout.pop()
            self.assert_rejected_transaction(node, bad, "bad-prodis-fee")
            bad = deepcopy(good)
            bad.vout[0].nValue += 1
            bad.vout[-1].nValue -= 1
            self.assert_rejected_transaction(node, bad, "bad-prodis-bonus")

            # All individual floors still hold, but their sum omits the rounding remainder.
            bad = deepcopy(good)
            remainder = bonuses[-1] - penalty * non_actors[-1]["amount"] // total
            assert_greater_than(remainder, 0)
            bad.vout[-2].nValue -= remainder
            bad.vout[-1].nValue += remainder
            self.assert_rejected_transaction(node, bad, "bad-prodis-penalty-sum")

        self.log.info("Even fully signed unanimous transactions cannot override the refund covenant")
        prepared = miner.protx("shared_dissolve_prepare", protx_hash, 7, DISSOLVE_FEE)

        def sign_unanimous(tx):
            raw = tx.serialize().hex()
            sigs = [wallet.protx("shared_sign", raw)["signatures"][0] for wallet in wallets]
            return tx_from_hex(miner.protx("shared_combine", raw, sigs))

        def sign_outside_wallet(tx):
            signed = deepcopy(tx)
            digest = prodis_sign_hash(signed, len(owner_keys))
            signed.vExtraPayload = (signed.vExtraPayload[:36] + bytes([len(owner_keys)]) +
                                    b"".join(sign_compact(key, digest) for key in owner_keys))
            return signed

        def assert_sign_refused(tx, signers=wallets):
            for wallet in signers:
                assert_raises_rpc_error(-8, "does not return every share's principal", wallet.protx,
                                        "shared_sign", tx.serialize().hex())

        unsigned = tx_from_hex(prepared["tx"])
        unanimous = sign_unanimous(unsigned)
        assert_equal(node.testmempoolaccept([unanimous.serialize().hex()])[0]["allowed"], True)
        assert_equal(sign_outside_wallet(unsigned).vExtraPayload, unanimous.vExtraPayload)

        self.log.info("shared_sign signs only a dissolution that returns every share's principal")
        for actor, fee in ((0, DISSOLVE_FEE), (7, MAX_DISSOLVE_FEE)):
            edge = miner.protx("shared_dissolve_prepare", protx_hash, actor, fee)["tx"]
            assert_equal(len(wallets[actor].protx("shared_sign", edge)["signatures"]), 1)
        # Consensus accepts a redistribution every share owner signs, but no wallet signs one, neither
        # the actor's that gives its principal away nor the one receiving it
        redistribution = deepcopy(unsigned)
        redistribution.vout[0].nValue += redistribution.vout[-1].nValue
        redistribution.vout.pop()
        assert_sign_refused(redistribution)
        redistributed = sign_outside_wallet(redistribution)
        assert_equal(node.testmempoolaccept([redistributed.serialize().hex()])[0]["allowed"], True)
        # A zero fee, and a fee above the consensus ceiling
        for fee_change in (DISSOLVE_FEE, DISSOLVE_FEE - MAX_DISSOLVE_FEE - 1):
            bad = deepcopy(unsigned)
            bad.vout[-1].nValue += fee_change
            assert_sign_refused(bad, [wallets[0], wallets[7]])
        # These outputs satisfy both modes even during the early period. Dropping
        # signatures must fail authorization, rather than only the penalty checks.
        penalty_unsigned = tx_from_hex(wallets[7].protx("shared_dissolve", protx_hash, 7, DISSOLVE_FEE, False))
        penalty_unsigned.vExtraPayload = penalty_unsigned.vExtraPayload[:36] + b"\x00"
        assert_sign_refused(penalty_unsigned, [wallets[0], wallets[7]])
        penalty_unanimous = sign_outside_wallet(penalty_unsigned)
        assert_equal(node.testmempoolaccept([penalty_unanimous.serialize().hex()])[0]["allowed"], True)
        bad = deepcopy(penalty_unanimous)
        bad.vExtraPayload = bad.vExtraPayload[:36] + b"\x01" + bad.vExtraPayload[-65:]
        self.assert_rejected_transaction(node, bad, "bad-prodis-sig")
        for index in range(len(shares)):
            bad = deepcopy(unsigned)
            bad.vout[index].scriptPubKey = attacker_script
            assert_sign_refused(bad, [wallets[index]])
            self.assert_rejected_transaction(node, sign_outside_wallet(bad), "bad-prodis-payee")
        for index in range(len(shares) - 1):
            bad = deepcopy(unsigned)
            bad.vout[index].nValue -= 1
            bad.vout[-1].nValue += 1
            assert_sign_refused(bad, [wallets[index]])
            self.assert_rejected_transaction(node, sign_outside_wallet(bad), "bad-prodis-penalty-floor")

        self.log.info("Dissolution shape, signer ordering, canonical signatures and signed fields are enforced")
        bad = deepcopy(unanimous)
        bad.vout[0], bad.vout[1] = bad.vout[1], bad.vout[0]
        self.assert_rejected_transaction(node, bad, "bad-prodis-payee")
        bad = deepcopy(unanimous)
        bad.vout[-1].nValue -= 10000
        bad.vout.append(CTxOut(10000, attacker_script))
        self.assert_rejected_transaction(node, bad, "bad-prodis-payee-count")
        bad = deepcopy(unanimous)
        bad.vout[-1].nValue = 0
        self.assert_rejected_transaction(node, bad, "bad-prodis-actor-output-zero", mempool_reason="dust")
        bad = deepcopy(unanimous)
        bad.vin[0].scriptSig = CScript(b"\x51")
        self.assert_rejected_transaction(node, bad, "bad-prodis-input")
        bad = deepcopy(unanimous)
        bad.vin[0].prevout.n += 1
        self.assert_rejected_transaction(node, bad, "bad-prodis-input", mempool_reason="missing-inputs")
        bad = deepcopy(unanimous)
        bad.vin.append(CTxIn(COutPoint(1, 0)))
        self.assert_rejected_transaction(node, bad, "bad-prodis-input", mempool_reason="missing-inputs")

        # ProDisTx has a 36-byte version/hash/actor prefix, then a uint8 count and
        # fixed-width compact signatures. Mutate the wire payload, bypassing RPC guards.
        prefix = unanimous.vExtraPayload[:36]
        signatures = [unanimous.vExtraPayload[37 + i * 65:37 + (i + 1) * 65] for i in range(len(shares))]
        for sigs, reason in (([], "bad-prodis-sig-count"),
                             (signatures[:2], "bad-prodis-sig-count"),
                             ([signatures[-1]], "bad-prodis-penalty-floor"),
                             ([signatures[1], signatures[0]] + signatures[2:], "bad-prodis-sig"),
                             ([signatures[0]] * len(shares), "bad-prodis-sig")):
            bad = deepcopy(unanimous)
            bad.vExtraPayload = prefix + bytes([len(sigs)]) + b"".join(sigs)
            self.assert_rejected_transaction(node, bad, reason)
        for sig in (bytes([signatures[0][0] + 8]) + signatures[0][1:],
                    bytes([27 + ((signatures[0][0] - 27) ^ 1)]) + signatures[0][1:33] +
                    (ORDER - int.from_bytes(signatures[0][33:], "big")).to_bytes(32, "big")):
            bad = deepcopy(unanimous)
            bad.vExtraPayload = prefix + bytes([len(shares)]) + sig + b"".join(signatures[1:])
            self.assert_rejected_transaction(node, bad, "bad-prodis-sig")
        for field in ("locktime", "sequence", "fee"):
            bad = deepcopy(unanimous)
            if field == "locktime":
                bad.nLockTime = 1
            elif field == "sequence":
                bad.vin[0].nSequence -= 1
            else:
                bad.vout[-1].nValue -= 1
            self.assert_rejected_transaction(node, bad, "bad-prodis-sig")
        bad = deepcopy(unanimous)
        bad.vExtraPayload = prefix[:34] + struct.pack("<H", len(shares)) + unanimous.vExtraPayload[36:]
        self.assert_rejected_transaction(node, bad, "bad-prodis-actor")
        bad = deepcopy(unanimous)
        bad.vExtraPayload = bad.vExtraPayload[:-1]
        self.assert_rejected_transaction(node, bad, "bad-protx-payload")
        assert_equal(miner.protx("info", protx_hash)["state"], state)
        collateral = unanimous.vin[0].prevout
        assert node.gettxout(f"{collateral.hash:064x}", collateral.n) is not None

    def test_pending_registrar_update(self, node, protx_hash):
        self.log.info("An operator rotation preserves pending share-owner-authorized updates")
        pending_fee, mined_fee, share_fee = [node.getnewaddress() for _ in range(3)]
        funding_txid = node.sendmany("", {pending_fee: 1, mined_fee: 1, share_fee: 1})
        node.syncwithvalidationinterfacequeue()
        self.bump_mocktime(10 * 60 + 1)
        funding_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert funding_txid in node.getblock(funding_block)["tx"]

        pending_operator, mined_operator = [node.bls("generate")["public"] for _ in range(2)]
        transactions = []
        for operator, fee in ((pending_operator, pending_fee), (mined_operator, mined_fee)):
            prepared = node.protx("shared_update_registrar_prepare", protx_hash, operator, "", fee)
            sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
            transactions.append(node.protx("shared_combine", prepared["tx"], sigs))

        self.connect_nodes(0, 1)
        self.sync_all()
        self.disconnect_nodes(0, 1)
        pending_txid = node.sendrawtransaction(transactions[0])
        reward = node.getnewaddress()
        share_txid = node.protx("shared_update_share", protx_hash, 0, reward, share_fee)
        # Simulate another miner confirming a different rotation, using independent fee inputs.
        # The pending registrar update is signed by the immutable share owners and remains valid.
        other = self.nodes[1]
        mined_txid = other.sendrawtransaction(transactions[1])
        self.bump_mocktime(10 * 60 + 1)
        mined_block = self.generate(other, 1, sync_fun=self.no_op)[0]
        assert mined_txid in other.getblock(mined_block)["tx"]
        self.connect_nodes(0, 1)
        self.sync_blocks()
        assert_equal(node.protx("info", protx_hash)["state"]["pubKeyOperator"], mined_operator)
        assert_equal(set(node.getrawmempool()), {pending_txid, share_txid})

        self.bump_mocktime(10 * 60 + 1)
        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert {pending_txid, share_txid}.issubset(node.getblock(block_hash)["tx"])
        state = node.protx("info", protx_hash)["state"]
        assert_equal(state["pubKeyOperator"], pending_operator)
        assert_equal(state["shares"][0]["rewardAddress"], reward)

    def test_voting_payee_conflict_eviction(self, node, protx_hash):
        self.log.info("A confirmed voting-key or share reward update evicts the pending counterpart it invalidated")
        fee_addresses = [node.getnewaddress() for _ in range(4)]
        funding_txid = node.sendmany("", {address: 1 for address in fee_addresses})
        node.syncwithvalidationinterfacequeue()
        self.bump_mocktime(10 * 60 + 1)
        funding_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert funding_txid in node.getblock(funding_block)["tx"]

        for confirm_registrar in (True, False):
            voting = node.getnewaddress()
            registrar_fee, share_fee = fee_addresses.pop(), fee_addresses.pop()
            prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", voting, registrar_fee)
            registrar_sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
            registrar = node.protx("shared_combine", prepared["tx"], registrar_sigs)
            share_update = node.protx("shared_update_share", protx_hash, 0, voting, share_fee, False)
            pending, confirmed = (share_update, registrar) if confirm_registrar else (registrar, share_update)
            self.sync_all()
            self.disconnect_nodes(0, 1)
            pending_txid = node.sendrawtransaction(pending)
            # Simulate another miner that never saw the pending update confirming its counterpart.
            other = self.nodes[1]
            confirmed_txid = other.sendrawtransaction(confirmed)
            self.bump_mocktime(10 * 60 + 1)
            confirmed_block = self.generate(other, 1, sync_fun=self.no_op)[0]
            assert confirmed_txid in other.getblock(confirmed_block)["tx"]
            self.connect_nodes(0, 1)
            self.sync_blocks()
            state = node.protx("info", protx_hash)["state"]
            confirmed_address = state["votingAddress"] if confirm_registrar else state["shares"][0]["rewardAddress"]
            assert_equal(confirmed_address, voting)
            assert pending_txid not in node.getrawmempool()

    def test_service_update_before_key_change(self, node, protx_hash, port):
        self.log.info("A pending service update is mined before an operator key change of its masternode")

        def mine():
            self.bump_mocktime(10 * 60 + 1)
            return self.generate(node, 1, sync_fun=self.no_op)[0]

        def rotate(operator, fee):
            prepared = node.protx("shared_update_registrar_prepare", protx_hash, operator, "", fee)
            sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
            return node.protx("shared_combine", prepared["tx"], sigs, True)

        fees = [node.getnewaddress() for _ in range(3)]
        node.sendmany("", {address: 1 for address in fees})
        mine()
        old_key, new_key = node.bls("generate"), node.bls("generate")
        # Start from an operator key this test controls
        rotate(old_key["public"], fees.pop())
        mine()

        serv_txid = node.protx("update_service", protx_hash, [f"127.0.0.1:{p2p_port(port)}"], old_key["secret"], "",
                               fees.pop())
        rotation_txid = rotate(new_key["public"], fees.pop())
        # By fee alone the rotation would go first, and the service update after it would restore the
        # previous operator's service and revive the masternode the rotation bans
        node.prioritisetransaction(rotation_txid, COIN)
        block_txs = node.getblock(mine())["tx"]
        assert_greater_than(block_txs.index(rotation_txid), block_txs.index(serv_txid))
        state = node.protx("info", protx_hash)["state"]
        assert_equal(state["pubKeyOperator"], new_key["public"])
        assert_greater_than(state["PoSeBanHeight"], 0)

    def test_separate_participant_wallets(self):
        self.log.info("Eight separate wallets fund and authorize a shared masternode")
        node = self.nodes[0]
        miner = node.get_wallet_rpc(self.default_wallet_name)
        wallets, owner_keys, shares, funding_addresses, change_addresses = [], [], [], [], []
        common_reward = miner.getnewaddress()
        for i, amount in enumerate([100] * 7 + [300]):
            name = f"participant_{i}"
            node.createwallet(name, load_on_startup=True)
            wallet = node.get_wallet_rpc(name)
            wallets.append(wallet)
            refund = wallet.getnewaddress()
            if i == 7:
                keys = [get_generate_key() for _ in range(2)]
                if self.options.descriptors:
                    descriptor = descsum_create(f"sh(multi(2,{keys[0].privkey},{keys[1].privkey}))")
                    result, = wallet.importdescriptors([{"desc": descriptor, "timestamp": "now"}])
                    assert_equal(result["success"], True)
                    refund, = wallet.deriveaddresses(wallet.getdescriptorinfo(descriptor)["descriptor"])
                else:
                    for key in keys:
                        wallet.importprivkey(key.privkey)
                    refund = wallet.addmultisigaddress(2, [key.pubkey for key in keys])["address"]
            # The test keeps the owner key too, to sign dissolutions the wallet refuses to
            owner_key = ECKey()
            owner_key.generate()
            owner_wif = bytes_to_wif(owner_key.get_bytes())
            if self.options.descriptors:
                result, = wallet.importdescriptors([{"desc": descsum_create(f"pkh({owner_wif})"), "timestamp": "now"}])
                assert_equal(result["success"], True)
            else:
                wallet.importprivkey(owner_wif, "", False)
            owner_keys.append(owner_key)
            shares.append({"amount": amount * COIN, "refundAddress": refund,
                           "ownerAddress": key_to_p2pkh(owner_key.get_pubkey().get_bytes().hex()),
                           "rewardAddress": common_reward})
            funding_addresses.append(wallet.getnewaddress())
            change_addresses.append(wallet.getnewaddress())

        funding_txid = miner.sendmany("", {address: Decimal(share["amount"]) / COIN + 1
                                          for address, share in zip(funding_addresses, shares)})
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        inputs = []
        for wallet, address in zip(wallets, funding_addresses):
            coin, = wallet.listunspent(1, 9999999, [address])
            assert_equal(coin["txid"], funding_txid)
            inputs.append({"txid": coin["txid"], "vout": coin["vout"]})
        funding = miner.createrawtransaction(inputs, {address: Decimal("0.99995")
                                                      for address in change_addresses})
        operator = node.bls("generate")
        prepared = miner.protx("shared_register_prepare", funding, shares,
                               f"127.0.0.1:{p2p_port(6)}", operator["public"], miner.getnewaddress(),
                               "12.50", 100, 10 * COIN)
        signatures = []
        for i, wallet in enumerate(wallets):
            sig, = wallet.protx("shared_sign", prepared["tx"])["signatures"]
            assert_equal(sig["shareIndex"], i)
            signatures.append(sig)
        assert_raises_rpc_error(-5, "none of the share owner keys", miner.protx,
                                "shared_sign", prepared["tx"])
        assert_raises_rpc_error(-8, "requires a signature from every share", miner.protx,
                                "shared_combine", prepared["tx"], signatures[:-1])
        combined = miner.protx("shared_combine", prepared["tx"], signatures)
        for i, wallet in enumerate(wallets):
            signed = wallet.signrawtransactionwithwallet(combined)
            assert_equal(signed["complete"], i == len(wallets) - 1)
            combined = signed["hex"]
        self.log.info("Consent signatures prevent a coordinator from redirecting refunds or funding change")
        for target in ("refund", "change", "locktime", "sequence"):
            bad = tx_from_hex(combined)
            if target == "refund":
                original = bytes.fromhex(wallets[0].getaddressinfo(shares[0]["refundAddress"])["scriptPubKey"])
                replacement = bytes.fromhex(miner.getaddressinfo(miner.getnewaddress())["scriptPubKey"])
                assert_equal(bad.vExtraPayload.count(original), 1)
                bad.vExtraPayload = bad.vExtraPayload.replace(original, replacement)
            elif target == "change":
                bad.vout[0].scriptPubKey = CScript(bytes.fromhex(miner.getaddressinfo(miner.getnewaddress())["scriptPubKey"]))
            elif target == "locktime":
                bad.nLockTime += 1
            else:
                bad.vin[0].nSequence -= 1
            raw = bad.serialize().hex()
            for wallet in wallets:
                raw = wallet.signrawtransactionwithwallet(raw)["hex"]
            self.assert_rejected_transaction(node, tx_from_hex(raw), "bad-protx-shares-sig")
        protx_hash = node.sendrawtransaction(combined)
        self.bump_mocktime(10 * 60 + 1)
        registration_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        for wallet in wallets:
            assert protx_hash in wallet.protx("list", "wallet")
            assert_equal(wallet.protx("info", protx_hash)["wallet"]["hasOwnerKey"], True)

        self.log.info("All shared lifecycle transactions require registration in a prior block")
        update = wallets[0].protx("shared_update_share", protx_hash, 0, common_reward, change_addresses[0], False)
        prepared_registrar = wallets[0].protx("shared_update_registrar_prepare", protx_hash,
                                             "", "", change_addresses[0])
        sigs = [wallet.protx("shared_sign", prepared_registrar["tx"])["signatures"][0] for wallet in wallets]
        registrar = wallets[0].protx("shared_combine", prepared_registrar["tx"], sigs)
        shared_dissolve = wallets[0].protx("shared_dissolve", protx_hash, 0, DISSOLVE_FEE, False)
        node.invalidateblock(registration_block)
        for lifecycle, reason in ((update, "bad-proupshare-hash"),
                                  (registrar, "bad-proupsharedreg-hash"), (shared_dissolve, "bad-prodis-hash")):
            assert_raises_rpc_error(-25, reason, self.generateblock, node,
                                    miner.getnewaddress(), [combined, lifecycle], sync_fun=self.no_op)
        node.reconsiderblock(registration_block)

        self.log.info("Share and registrar updates reject unauthorized signers and malformed payloads")
        assert_equal(node.testmempoolaccept([update])[0]["allowed"], True)
        assert_equal(node.testmempoolaccept([registrar])[0]["allowed"], True)
        for index, reason in ((1, "bad-proupshare-sig"), (len(shares), "bad-proupshare-index")):
            bad = tx_from_hex(update)
            bad.vExtraPayload = bad.vExtraPayload[:34] + struct.pack("<H", index) + bad.vExtraPayload[36:]
            self.assert_rejected_transaction(node, bad, reason)
        bad = tx_from_hex(update)
        bad.vExtraPayload = bad.vExtraPayload[:-65] + b"\x00" * 65
        self.assert_rejected_transaction(node, bad, "bad-proupshare-sig")
        for count in (0, 1, len(shares) - 1):
            bad = tx_from_hex(registrar)
            prefix = bad.vExtraPayload[:-(1 + 65 * len(shares))]
            sigs_raw = bad.vExtraPayload[-65 * len(shares):]
            bad.vExtraPayload = prefix + bytes([count]) + sigs_raw[:65 * count]
            self.assert_rejected_transaction(node, bad, "bad-proupsharedreg-sig-count")
        bad = tx_from_hex(registrar)
        bad.vExtraPayload = bad.vExtraPayload[:-65 * len(shares)] + bad.vExtraPayload[-65:] * len(shares)
        self.assert_rejected_transaction(node, bad, "bad-proupsharedreg-sig")

        self.log.info("Duplicate share reward scripts coexist with a separate operator payout")
        operator_reward = miner.getnewaddress()
        fee_source = miner.getnewaddress()
        miner.sendtoaddress(fee_source, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        miner.protx("update_service", protx_hash, f"127.0.0.1:{p2p_port(6)}",
                    operator["secret"], operator_reward, fee_source)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        self.generate(node, 10, sync_fun=self.no_op)
        expected_payees = node.getblocktemplate()["masternode"]
        assert_equal(sum(payee["payee"] == common_reward for payee in expected_payees), 8)
        assert_equal(sum(payee["payee"] == operator_reward for payee in expected_payees), 1)
        owner_amounts = [payee["amount"] for payee in expected_payees if payee["payee"] == common_reward]
        owner_total = sum(owner_amounts)
        assert_equal(owner_amounts, [owner_total // 10] * 7 + [owner_total - 7 * (owner_total // 10)])
        operator_amount, = [payee["amount"] for payee in expected_payees if payee["payee"] == operator_reward]
        assert_equal(operator_amount, (owner_total + operator_amount) // 8)
        block = node.getblock(self.generate(node, 1, sync_fun=self.no_op)[0], 2)
        outputs = block["tx"][0]["vout"]
        for address in (common_reward, operator_reward):
            script = miner.getaddressinfo(address)["scriptPubKey"]
            assert_equal(sum(out["value"] for out in outputs if out["scriptPubKey"]["hex"] == script),
                         sum(Decimal(payee["amount"]) / COIN for payee in expected_payees if payee["payee"] == address))

        self.log.info("Unanimous registrar signing works across wallets; the funder finalizes inputs")
        voting = miner.getnewaddress()
        prepared_update = wallets[0].protx("shared_update_registrar_prepare", protx_hash,
                                           "", voting, change_addresses[0])
        signatures = [wallet.protx("shared_sign", prepared_update["tx"])["signatures"][0] for wallet in wallets]
        for submit in (True, False):
            assert_raises_rpc_error(-4, "must be combined by the wallet that funded it", wallets[1].protx,
                                    "shared_combine", prepared_update["tx"], signatures, submit)

        self.log.info("An approved registrar update is not signed if it moves the funder's coins elsewhere")

        def forge(script=None, value=None):
            forged = tx_from_hex(prepared_update["tx"])
            for output in forged.vout:
                output.scriptPubKey = script or output.scriptPubKey
                output.nValue = output.nValue if value is None else value
            return forged.serialize().hex()

        def script_of(wallet):
            return bytes.fromhex(wallet.getaddressinfo(wallet.getnewaddress())["scriptPubKey"])
        for owner in (miner, wallets[0]):
            assert_raises_rpc_error(-4, "must spend from and return change to a single address", wallets[0].protx,
                                    "shared_combine", forge(script=script_of(owner)), signatures, True)
        # The co-owner who receives the coins cannot combine it and leave the funder only an input to sign
        assert_raises_rpc_error(-4, "must be combined by the wallet that funded it", wallets[1].protx,
                                "shared_combine", forge(script=script_of(wallets[1])), signatures, False)
        assert_raises_rpc_error(-4, "exceeds this wallet's maximum fee", wallets[0].protx,
                                "shared_combine", forge(value=1000), signatures, False)
        # A locked coin, such as a masternode collateral, is never spent even back to its own address
        fee_inputs = [{"txid": f"{txin.prevout.hash:064x}", "vout": txin.prevout.n}
                      for txin in tx_from_hex(prepared_update["tx"]).vin]
        assert wallets[0].lockunspent(False, fee_inputs)
        assert_raises_rpc_error(-4, "must not spend a locked coin", wallets[0].protx,
                                "shared_combine", prepared_update["tx"], signatures, False)
        assert wallets[0].lockunspent(True, fee_inputs)

        self.log.info("A registrar update funded without change keeps only the zero-valued marker output")
        prepared_tx = tx_from_hex(prepared_update["tx"])
        prepared_fee = sum(node.gettxout(f"{txin.prevout.hash:064x}", txin.prevout.n)["value"] * COIN
                           for txin in prepared_tx.vin) - sum(out.nValue for out in prepared_tx.vout)
        tiny_fee_source = wallets[0].getnewaddress()
        # Enough for the same fee, leaving a remainder too small for a change output
        miner.sendtoaddress(tiny_fee_source, (prepared_fee + 100) / COIN)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        no_change = wallets[0].protx("shared_update_registrar_prepare", protx_hash, "", "", tiny_fee_source)
        assert_equal([(out.nValue, out.scriptPubKey) for out in tx_from_hex(no_change["tx"]).vout],
                     [(0, CScript(b"\x6a"))])
        no_change_sigs = [wallet.protx("shared_sign", no_change["tx"])["signatures"][0] for wallet in wallets]
        no_change_tx = wallets[0].protx("shared_combine", no_change["tx"], no_change_sigs)
        assert_equal(node.testmempoolaccept([no_change_tx])[0]["allowed"], True)

        self.log.info("Conflicting voting-key and share reward updates are consensus-invalid in both block orders")
        registrar = wallets[0].protx("shared_combine", prepared_update["tx"], signatures)
        reward_update = wallets[1].protx("shared_update_share", protx_hash, 1, voting, change_addresses[1], False)
        for transactions, reason in (([registrar, reward_update], "bad-proupshare-payee-reuse"),
                                     ([reward_update, registrar], "bad-proupsharedreg-payee-reuse")):
            assert_raises_rpc_error(-25, reason, self.generateblock, node,
                                    miner.getnewaddress(), transactions, sync_fun=self.no_op)
        wallets[0].protx("shared_combine", prepared_update["tx"], signatures, True)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(miner.protx("info", protx_hash)["state"]["votingAddress"], voting)

        self.test_invalid_dissolutions(node, miner, wallets, owner_keys, protx_hash, shares)

        self.log.info("A pre-signed unanimous dissolution survives reindex and pays spendable refunds")
        prepared_dissolve = miner.protx("shared_dissolve_prepare", protx_hash, 7, DISSOLVE_FEE)
        signatures = [wallet.protx("shared_sign", prepared_dissolve["tx"])["signatures"][0] for wallet in wallets]
        dissolution = miner.protx("shared_combine", prepared_dissolve["tx"], signatures)
        state = miner.protx("info", protx_hash)["state"]
        tip = node.getbestblockhash()
        self.restart_node(0, extra_args=node.extra_args + ["-reindex=1"])
        self.wait_until(lambda: node.getbestblockhash() == tip)
        miner = node.get_wallet_rpc(self.default_wallet_name)
        wallets = [node.get_wallet_rpc(f"participant_{i}") for i in range(8)]
        assert_equal(miner.protx("info", protx_hash)["state"], state)
        node.sendrawtransaction(dissolution)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        refund_spends = set()
        for i, (wallet, share) in enumerate(zip(wallets, shares)):
            expected = Decimal(share["amount"] - (DISSOLVE_FEE if i == 7 else 0)) / COIN
            assert_equal(wallet.getreceivedbyaddress(share["refundAddress"]), expected)
            coin, = wallet.listunspent(1, 9999999, [share["refundAddress"]])
            spend = wallet.createrawtransaction([{"txid": coin["txid"], "vout": coin["vout"]}],
                                               {miner.getnewaddress(): expected - Decimal("0.001")})
            signed = wallet.signrawtransactionwithwallet(spend)
            assert_equal(signed["complete"], True)
            refund_spends.add(node.sendrawtransaction(signed["hex"]))
        self.bump_mocktime(10 * 60 + 1)
        refund_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert refund_spends.issubset(node.getblock(refund_block)["tx"])
        assert_raises_rpc_error(None, None, miner.protx, "info", protx_hash)

    def run_test(self):
        node = self.nodes[0]
        # Keep the second miner isolated during the invalid-block and local-reorg checks.
        self.disconnect_nodes(0, 1)

        self.log.info("Shared masternode transactions are rejected before v24 activation")
        assert not softfork_active(node, "v24")
        pre_miner = node.getnewaddress()
        # The wallet path is closed while provider transaction version 3 is unavailable
        pre_shares = [
            {"amount": 600 * COIN, "refundAddress": node.getnewaddress(), "ownerAddress": node.getnewaddress()},
            {"amount": 400 * COIN, "refundAddress": node.getnewaddress(), "ownerAddress": node.getnewaddress()},
        ]
        pre_funding = node.createrawtransaction([], {node.getnewaddress(): 1})
        assert_raises_rpc_error(-8, "provider transaction version 3", node.protx, "shared_register_prepare",
                                pre_funding, pre_shares, "", node.bls("generate")["public"],
                                node.getnewaddress(), 0, EARLY_PERIOD_BLOCKS, EARLY_PENALTY)

        # Consensus gate: each lifecycle type carries a trivially-valid payload (version 1, zeroed
        # fields, 65-byte placeholder signatures, a valid operator key or P2PKH reward script where
        # one is required), so block connection gets past payload deserialization and fails at the
        # deployment check with the type's dedicated "too early" reason, not a payload or lookup error
        prodis_hex = self.build_lifecycle_tx(
            TRANSACTION_PROVIDER_DISSOLVE,
            struct.pack("<H", 1) + b"\x00" * 32 + struct.pack("<H", 0) + bytes([1]) + b"\x00" * 65)
        gate_reward_script = bytes.fromhex(node.getaddressinfo(node.getnewaddress())["scriptPubKey"])
        upshare_hex = self.build_lifecycle_tx(
            TRANSACTION_PROVIDER_UPDATE_SHARE,
            struct.pack("<H", 1) + b"\x00" * 32 + struct.pack("<H", 0) + bytes([len(gate_reward_script)]) +
            gate_reward_script + b"\x00" * 32 + bytes([65]) + b"\x00" * 65)
        # A ProUpShareTx must carry an explicit reward script; a zero-length script is rejected
        # statelessly, before the deployment gate
        upshare_empty_hex = self.build_lifecycle_tx(
            TRANSACTION_PROVIDER_UPDATE_SHARE,
            struct.pack("<H", 1) + b"\x00" * 32 + struct.pack("<H", 0) + b"\x00" + b"\x00" * 32 +
            bytes([65]) + b"\x00" * 65)
        assert_raises_rpc_error(-25, "bad-proupshare-payee", self.generateblock, node, pre_miner,
                                [upshare_empty_hex], sync_fun=self.no_op)
        upsharedreg_hex = self.build_lifecycle_tx(
            TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR,
            struct.pack("<H", 1) + b"\x00" * 32 + bytes.fromhex(node.bls("generate")["public"]) +
            b"\x01" * 20 + b"\x00" * 32 + bytes([1]) + b"\x00" * 65)
        for tx_hex, reason in ((prodis_hex, "bad-prodis-too-early"),
                               (upshare_hex, "bad-proupshare-too-early"),
                               (upsharedreg_hex, "bad-proupsharedreg-too-early")):
            assert_raises_rpc_error(-25, reason, self.generateblock, node, pre_miner, [tx_hex],
                                    sync_fun=self.no_op)

        self.log.info("A template output is consensus-valid to mine before activation")
        pre_tmpl_raw = node.createrawtransaction([], {node.getnewaddress(): 1})
        pre_tmpl_funded = node.fundrawtransaction(pre_tmpl_raw)["hex"]
        pre_tmpl_tx = tx_from_hex(pre_tmpl_funded)
        pre_tmpl_tx.vout[0].scriptPubKey = CScript(bytes.fromhex(SHARED_COLLATERAL_SCRIPT))
        pre_tmpl_value = pre_tmpl_tx.vout[0].nValue
        pre_tmpl_signed = node.signrawtransactionwithwallet(pre_tmpl_tx.serialize().hex())
        assert_equal(pre_tmpl_signed["complete"], True)
        pre_tmpl_txid = node.decoderawtransaction(pre_tmpl_signed["hex"])["txid"]
        self.generateblock(node, pre_miner, [pre_tmpl_signed["hex"]], sync_fun=self.no_op)

        self.activate("v24")

        self.log.info("A coinbase cannot create a shared collateral template output")
        assert_raises_rpc_error(-1, "bad-shared-collateral-create", self.generateblock, node,
                                f"raw({SHARED_COLLATERAL_SCRIPT})", [], sync_fun=self.no_op)

        # The same dissolution now clears the deployment gate and fails at the masternode lookup
        # instead, proving the pre-activation rejections above came from the gate itself
        assert_raises_rpc_error(-25, "bad-prodis-hash", self.generateblock, node, pre_miner,
                                [prodis_hex], sync_fun=self.no_op)

        self.log.info("A template output mined before activation is permanently unspendable after it")
        # No masternode owns this outpoint, so no ProDisTx can ever exist for it; the covenant
        # spend rule rejects every other spender, freezing the deliberately created output forever
        freeze_spend = CTransaction()
        freeze_spend.vin.append(CTxIn(COutPoint(int(pre_tmpl_txid, 16), 0)))
        freeze_spend.vout.append(CTxOut(pre_tmpl_value - 100000, CScript(b"\x51")))
        assert_raises_rpc_error(-25, "bad-shared-collateral-spend", self.generateblock, node, pre_miner,
                                [freeze_spend.serialize().hex()], sync_fun=self.no_op)

        self.log.info("A near-miss script is unrestricted by the covenant")
        # One tag byte off the template ('DSHD'): creation and an empty-scriptSig spend both
        # connect fine post-activation, proving the covenant matches the exact script only
        near_miss_script = CScript(bytes.fromhex("04445348447551"))
        near_raw = node.createrawtransaction([], {node.getnewaddress(): 1})
        near_funded = node.fundrawtransaction(near_raw)["hex"]
        near_tx = tx_from_hex(near_funded)
        near_tx.vout[0].scriptPubKey = near_miss_script
        near_value = near_tx.vout[0].nValue
        near_signed = node.signrawtransactionwithwallet(near_tx.serialize().hex())
        assert_equal(near_signed["complete"], True)
        near_txid = node.decoderawtransaction(near_signed["hex"])["txid"]
        self.generateblock(node, pre_miner, [near_signed["hex"]], sync_fun=self.no_op)
        near_spend = CTransaction()
        near_spend.vin.append(CTxIn(COutPoint(int(near_txid, 16), 0)))
        near_spend.vout.append(CTxOut(near_value - 100000, CScript(b"\x51")))
        near_block = self.generateblock(node, pre_miner, [near_spend.serialize().hex()], sync_fun=self.no_op)
        assert_equal(len(node.getblock(near_block["hash"])["tx"]), 2)

        self.log.info("Register a two-participant shared masternode")
        refund1, refund2 = node.getnewaddress(), node.getnewaddress()
        owner1, owner2 = node.getnewaddress(), node.getnewaddress()
        shares = [
            {"amount": 600 * COIN, "refundAddress": refund1, "ownerAddress": owner1},
            {"amount": 400 * COIN, "refundAddress": refund2, "ownerAddress": owner2},
        ]

        # shared_register_prepare preflights the consensus rules so consensus-invalid terms fail
        # before any participant signs: a share sum below the collateral, and a penalty that is
        # not strictly below the smallest share
        preflight_funding = self.build_funding_tx(node)
        preflight_args = [f"127.0.0.1:{p2p_port(1)}", node.bls("generate")["public"],
                          node.getnewaddress(), 0, EARLY_PERIOD_BLOCKS]
        bad_shares = [dict(shares[0], amount=shares[0]["amount"] - 1), shares[1]]
        assert_raises_rpc_error(-8, "invalid shared registration terms", node.protx,
                                "shared_register_prepare", preflight_funding, bad_shares,
                                *preflight_args, EARLY_PENALTY)
        assert_raises_rpc_error(-8, "invalid shared registration terms", node.protx,
                                "shared_register_prepare", preflight_funding, shares,
                                *preflight_args, 400 * COIN)
        # a penalty without an early period would only serve as a drain ceiling for a stolen key
        assert_raises_rpc_error(-8, "invalid shared registration terms", node.protx,
                                "shared_register_prepare", preflight_funding, shares,
                                *preflight_args[:-1], 0, EARLY_PENALTY)

        protx_hash, collateral_index = self.register_shared(node, shares, port_offset=1)

        raw = node.getrawtransaction(protx_hash, 1)
        assert_equal(raw["proRegTx"]["version"], 3)
        assert "ownerAddress" not in raw["proRegTx"]
        assert_equal([s["refundAddress"] for s in raw["proRegTx"]["shares"]], [refund1, refund2])
        assert_equal([s["amount"] for s in raw["proRegTx"]["shares"]], [600 * COIN, 400 * COIN])
        # empty reward script falls back to the refund script
        assert_equal([s["rewardAddress"] for s in raw["proRegTx"]["shares"]], [refund1, refund2])
        assert_equal(raw["proRegTx"]["earlyPeriodBlocks"], EARLY_PERIOD_BLOCKS)
        assert_equal(raw["proRegTx"]["earlyPenalty"], EARLY_PENALTY)
        assert_equal(raw["vout"][collateral_index]["scriptPubKey"]["hex"], SHARED_COLLATERAL_SCRIPT)

        info = node.protx("info", protx_hash)
        assert_equal(info["state"]["version"], 3)
        assert "ownerAddress" not in info["state"]
        assert_equal([s["ownerAddress"] for s in info["state"]["shares"]], [owner1, owner2])
        assert "payoutAddress" not in info["state"]
        assert "payouts" not in info["state"]

        self.log.info("Shared masternodes are visible in payee displays and wallet filters")
        # the payee list mode joins every share's effective reward address (here the refund
        # fallbacks, as no reward scripts are set yet)
        assert_equal(list(node.masternodelist("payee").values()), [f"{refund1}, {refund2}"])
        owners = f"{owner1}, {owner2}"
        assert_equal(list(node.masternodelist("owneraddress").values()), [owners])
        for mode in ("json", "recent"):
            entries = node.masternodelist(mode)
            assert_equal([entry["owneraddress"] for entry in entries.values()], [owners])
            for owner in (owner1, owner2):
                assert_equal(node.masternodelist(mode, owner), entries)
        # the wallet holds the share owner keys and reward scripts, so the masternode is
        # attributed to it despite the null registrar owner key
        assert_equal(info["wallet"]["hasOwnerKey"], True)
        assert_equal(info["wallet"]["ownsPayeeScript"], True)
        assert protx_hash in node.protx("list", "wallet")

        self.log.info("A registration with a tampered or misplaced consent signature is rejected by consensus")
        refund_a, refund_b = node.getnewaddress(), node.getnewaddress()
        owner_a, owner_b = node.getnewaddress(), node.getnewaddress()
        shares_sigtest = [
            {"amount": 600 * COIN, "refundAddress": refund_a, "ownerAddress": owner_a},
            {"amount": 400 * COIN, "refundAddress": refund_b, "ownerAddress": owner_b},
        ]
        sigtest_funding = self.build_funding_tx(node)
        sigtest_prepared = node.protx(
            "shared_register_prepare", sigtest_funding, shares_sigtest, f"127.0.0.1:{p2p_port(4)}",
            node.bls("generate")["public"], node.getnewaddress(), 0, EARLY_PERIOD_BLOCKS, EARLY_PENALTY)
        # The consent digest commits to the lock fields, so shared_sign refuses a time-locked
        # registration unless the signer explicitly opts in (mirroring the dissolution guard)
        # (the wallet-funded inputs already carry the fee-sniping sequence 0xfffffffe, which
        # is not a lock on its own and must not trip the guard; an unsatisfied nLockTime or a
        # BIP68 relative lock must)
        locked_reg = tx_from_hex(sigtest_prepared["tx"])
        assert_equal({vin.nSequence for vin in locked_reg.vin}, {0xfffffffe})
        locked_reg.nLockTime = node.getblockcount() + 100
        assert_raises_rpc_error(-8, "registration carries a lock time", node.protx, "shared_sign",
                                locked_reg.serialize().hex())
        locked_reg.nLockTime = 0
        locked_reg.vin[0].nSequence = 10
        assert_raises_rpc_error(-8, "registration carries a lock time", node.protx, "shared_sign",
                                locked_reg.serialize().hex())
        assert_equal(len(node.protx("shared_sign", locked_reg.serialize().hex(), True)["signatures"]), 2)
        sigtest_sigs = node.protx("shared_sign", sigtest_prepared["tx"])["signatures"]
        assert_equal(sorted(s["shareIndex"] for s in sigtest_sigs), [0, 1])

        def submit_with_sigs(sigs):
            combined = node.protx("shared_combine", sigtest_prepared["tx"], sigs)
            signed = node.signrawtransactionwithwallet(combined)
            assert_equal(signed["complete"], True)
            return node.testmempoolaccept([signed["hex"]])[0], signed["hex"]

        # A single flipped bit in one consent signature must invalidate the registration
        tampered = [dict(s) for s in sigtest_sigs]
        raw_sig = bytearray(base64.b64decode(tampered[0]["signature"]))
        raw_sig[10] ^= 0x01
        tampered[0]["signature"] = base64.b64encode(bytes(raw_sig)).decode()
        res, _ = submit_with_sigs(tampered)
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-protx-shares-sig")

        # The high-S malleation of a valid consent signature recovers the same key but is
        # non-canonical, so it must be rejected too (otherwise the registration txid is malleable)
        high_s = [dict(s) for s in sigtest_sigs]
        high_s[0]["signature"] = self.high_s(high_s[0]["signature"])
        res, _ = submit_with_sigs(high_s)
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-protx-shares-sig")

        # Two valid consent signatures in each other's slots must also be rejected: each
        # signature is verified against its own share's owner key, in share order
        swapped = [
            {"shareIndex": 0, "signature": sigtest_sigs[1]["signature"]},
            {"shareIndex": 1, "signature": sigtest_sigs[0]["signature"]},
        ]
        res, _ = submit_with_sigs(swapped)
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-protx-shares-sig")

        # The untampered signatures in the right slots are accepted, proving the two
        # rejections above were caused by the signatures alone
        res, good_hex = submit_with_sigs(sigtest_sigs)
        assert_equal(res["allowed"], True)

        self.log.info("A template output that is not the declared collateral does not relay")
        # The standardness carve-out only excuses the template script at the payload's declared
        # collateral index; a second template output would become permanently unspendable, so
        # policy must refuse to relay it even inside an otherwise valid shared registration
        extra_out_tx = tx_from_hex(good_hex)
        extra_out_tx.vout.append(CTxOut(1000, CScript(bytes.fromhex(SHARED_COLLATERAL_SCRIPT))))
        res = node.testmempoolaccept([extra_out_tx.serialize().hex()])[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "scriptpubkey")

        self.log.info("Owner reward is split by share amounts, remainder to the last entry")
        payees = self.owner_gbt_payees(node)
        assert_equal([p["payee"] for p in payees], [refund1, refund2])
        owner_total = sum(p["amount"] for p in payees)
        assert_equal(payees[0]["amount"], owner_total * (600 * COIN) // COLLATERAL)
        assert_equal(payees[1]["amount"], owner_total - payees[0]["amount"])
        self.generate(node, 1, sync_fun=self.no_op)

        miner_addr = node.getnewaddress()

        self.log.info("A normal transaction cannot spend the shared collateral (consensus, not just policy)")
        # The template input is anyone-can-spend at the script layer, so an empty scriptSig is a
        # complete spend. Drive it through block connection (generateblock runs consensus, not
        # policy) to prove the covenant rule rejects it, and specifically for that reason.
        steal_raw = node.createrawtransaction(
            [{"txid": protx_hash, "vout": collateral_index}], {node.getnewaddress(): 999.99})
        assert_raises_rpc_error(-25, "bad-shared-collateral-spend", self.generateblock, node, miner_addr,
                                [steal_raw], sync_fun=self.no_op)
        # It is also nonstandard for relay
        assert_equal(node.testmempoolaccept([steal_raw])[0]["allowed"], False)

        self.log.info("A normal transaction cannot create a template output (consensus, not just policy)")
        create_raw = node.createrawtransaction([], {node.getnewaddress(): 1})
        create_funded = node.fundrawtransaction(create_raw)["hex"]
        create_tx = tx_from_hex(create_funded)
        create_tx.vout[0].scriptPubKey = CScript(bytes.fromhex(SHARED_COLLATERAL_SCRIPT))
        # Sign the funding input so the only thing left to reject is the template output itself
        create_signed = node.signrawtransactionwithwallet(create_tx.serialize().hex())
        assert_equal(create_signed["complete"], True)
        assert_raises_rpc_error(-25, "bad-shared-collateral-create", self.generateblock, node, miner_addr,
                                [create_signed["hex"]], sync_fun=self.no_op)

        self.log.info("A share owner can update their reward script")
        # fee source must hold mature wallet funds (masternode rewards are still immature coinbase)
        fee_addr = node.getnewaddress()
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        # an empty reward address is not a reset; the refund address must be passed explicitly
        assert_raises_rpc_error(-5, "invalid reward address", node.protx, "shared_update_share", protx_hash, 0, "", fee_addr)
        reward1 = node.getnewaddress()
        # A high-S malleation of the share owner's payload signature is rejected: the signature is
        # the last 65 bytes of the payload behind a one-byte length prefix
        update_hex = node.protx("shared_update_share", protx_hash, 0, reward1, fee_addr, False)
        update_tx = tx_from_hex(update_hex)
        payload_sig = base64.b64encode(update_tx.vExtraPayload[-65:]).decode()
        assert_equal(update_tx.vExtraPayload[-66], 65)
        update_tx.vExtraPayload = update_tx.vExtraPayload[:-65] + base64.b64decode(self.high_s(payload_sig))
        res = node.testmempoolaccept([update_tx.serialize().hex()])[0]
        assert_equal(res["allowed"], False)
        assert_equal(res["reject-reason"], "bad-proupshare-sig")
        node.sendrawtransaction(update_hex)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info = node.protx("info", protx_hash)
        assert_equal(info["state"]["shares"][0]["rewardAddress"], reward1)
        payees = self.owner_gbt_payees(node)
        assert_equal([p["payee"] for p in payees], [reward1, refund2])
        # the payee display follows the reward-script update
        assert_equal(list(node.masternodelist("payee").values()), [f"{reward1}, {refund2}"])

        self.log.info("The coinbase actually pays the share reward script while the MN is active")
        # reward1 only ever receives coinbase owner rewards (dissolution refunds go to refund1/2),
        # so a mined coinbase paying it proves shared rewards are really paid, not just templated.
        reward1_script = node.getaddressinfo(reward1)["scriptPubKey"]
        paid_reward1 = False
        for _ in range(6):
            self.bump_mocktime(10 * 60 + 1)
            block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
            coinbase = node.getblock(block_hash, 2)["tx"][0]
            if any(o["scriptPubKey"]["hex"] == reward1_script for o in coinbase["vout"]):
                paid_reward1 = True
                break
        assert paid_reward1, "shared masternode reward was never paid to the share reward script"

        self.log.info("A plain ProUpRegTx cannot update a shared masternode")
        assert_raises_rpc_error(-8, "masternode is shared", node.protx,
                                "update_registrar", protx_hash, "", "", fee_addr)
        # Bypass the wallet guard with a well-formed ordinary registrar payload. Consensus
        # rejects the shared target before checking its dummy funding input or signature.
        payout_script = bytes.fromhex(node.getaddressinfo(fee_addr)["scriptPubKey"])
        ordinary_registrar = self.build_lifecycle_tx(
            3, struct.pack("<H", 3) + bytes.fromhex(protx_hash)[::-1] + struct.pack("<H", 0) +
            bytes.fromhex(info["state"]["pubKeyOperator"]) + b"\x01" * 20 +
            bytes([1, len(payout_script)]) + payout_script + struct.pack("<H", 10000) +
            b"\x00" * 32 + b"\x00")
        assert_raises_rpc_error(-25, "bad-protx-shared-mn", self.generateblock, node, miner_addr,
                                [ordinary_registrar], sync_fun=self.no_op)

        self.log.info("A unanimous registrar update changes the voting key")
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        new_voting = node.getnewaddress()
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", new_voting, fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        assert_equal(len(sigs), 2)
        node.protx("shared_combine", prepared["tx"], sigs, True)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info = node.protx("info", protx_hash)
        assert_equal(info["state"]["votingAddress"], new_voting)
        # the share table is untouched
        assert_equal([s["ownerAddress"] for s in info["state"]["shares"]], [owner1, owner2])

        self.log.info("A registrar update with swapped share signatures is rejected by consensus")
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", node.getnewaddress(), fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        assert_equal(sorted(s["shareIndex"] for s in sigs), [0, 1])
        swapped = [
            {"shareIndex": 0, "signature": sigs[1]["signature"]},
            {"shareIndex": 1, "signature": sigs[0]["signature"]},
        ]
        assert_raises_rpc_error(None, "bad-proupsharedreg-sig", node.protx,
                                "shared_combine", prepared["tx"], swapped, True)
        # ... as is the high-S malleation of an otherwise valid share signature
        high_s = [dict(s) for s in sigs]
        high_s[1]["signature"] = self.high_s(high_s[1]["signature"])
        assert_raises_rpc_error(None, "bad-proupsharedreg-sig", node.protx,
                                "shared_combine", prepared["tx"], high_s, True)

        self.log.info("A registrar update cannot move the voting key onto a share's payee script")
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", reward1, fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        assert_raises_rpc_error(None, "bad-proupsharedreg-payee-reuse", node.protx,
                                "shared_combine", prepared["tx"], sigs, True)

        self.log.info("A pending registrar update keeps a conflicting share update out of the mempool")
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        pair_addr = node.getnewaddress()
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", pair_addr, fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        node.protx("shared_combine", prepared["tx"], sigs, True)
        # both pass tip-level checks individually, so only the mempool pair guard keeps an honest
        # miner from assembling a block that consensus would then reject
        assert_raises_rpc_error(None, "protx-dup", node.protx, "shared_update_share", protx_hash, 0, pair_addr, fee_addr)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.protx("info", protx_hash)["state"]["votingAddress"], pair_addr)

        # the reverse direction: a pending share update blocks a registrar update onto its script
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        rev_addr = node.getnewaddress()
        node.protx("shared_update_share", protx_hash, 0, rev_addr, fee_addr)
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, "", rev_addr, fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        assert_raises_rpc_error(None, "protx-dup", node.protx,
                                "shared_combine", prepared["tx"], sigs, True)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.protx("info", protx_hash)["state"]["shares"][0]["rewardAddress"], rev_addr)
        # restore share 0's reward script for the sections below
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        node.protx("shared_update_share", protx_hash, 0, reward1, fee_addr)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.protx("info", protx_hash)["state"]["shares"][0]["rewardAddress"], reward1)

        self.log.info("An operator-key change PoSe-bans the shared masternode until a ProUpServTx revives it")
        node.sendtoaddress(fee_addr, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        new_operator = node.bls("generate")
        prepared = node.protx("shared_update_registrar_prepare", protx_hash, new_operator["public"], "", fee_addr)
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        node.protx("shared_combine", prepared["tx"], sigs, True)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info = node.protx("info", protx_hash)
        assert_equal(info["state"]["pubKeyOperator"], new_operator["public"])
        # operator-key change semantics match ProUpRegTx: operator fields reset, masternode banned
        assert_greater_than(info["state"]["PoSeBanHeight"], 0)
        assert_equal(self.owner_gbt_payees(node), [])

        # With no feeSourceAddress and no operator payout script, update_service falls back to
        # the first owner reward script like it does for ordinary masternodes; for a shared
        # masternode that is share 0's effective reward script (reward1 here, funded by the
        # coinbase rewards checked above)
        self.log.info("update_service falls back to the first share's reward script as fee source")
        node.sendtoaddress(reward1, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        fallback_txid = node.protx("update_service", protx_hash, [f"127.0.0.1:{p2p_port(1)}"], new_operator["secret"])
        fallback_tx = node.getrawtransaction(fallback_txid, 1)
        fallback_inputs = [node.getrawtransaction(vin["txid"], 1)["vout"][vin["vout"]] for vin in fallback_tx["vin"]]
        assert all(inp["scriptPubKey"]["address"] == reward1 for inp in fallback_inputs)
        # The revive gate requires all keys to be set, and a shared masternode has a null
        # keyIDOwner: this exercises the shared-specific carve-out, without which a banned shared
        # masternode could never be revived
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info = node.protx("info", protx_hash)
        assert_equal(info["state"]["PoSeBanHeight"], -1)
        # revived: the reward split resumes from the unchanged share table
        payees = self.owner_gbt_payees(node)
        assert_equal([p["payee"] for p in payees], [reward1, refund2])

        self.log.info("The dissolution fee is capped")
        assert_raises_rpc_error(-8, "fee exceeds the consensus ceiling", node.protx,
                                "shared_dissolve", protx_hash, 1, 1000001, False)

        self.log.info("A zero-penalty unilateral dissolution is invalid during the early period")
        assert_raises_rpc_error(-8, "must pay the penalty", node.protx,
                                "shared_dissolve", protx_hash, 1, DISSOLVE_FEE, True, False)
        standby_hex = node.protx("shared_dissolve", protx_hash, 1, DISSOLVE_FEE, False, False)
        standby_res = node.testmempoolaccept([standby_hex])[0]
        assert_equal(standby_res["allowed"], False)
        assert_equal(standby_res["reject-reason"], "bad-prodis-penalty-floor")

        self.log.info("A penalty-paying unilateral dissolution succeeds during the early period")
        registered_height = node.protx("info", protx_hash)["state"]["registeredHeight"]
        assert_greater_than(registered_height + EARLY_PERIOD_BLOCKS, node.getblockcount() + 1)
        dissolve_txid = node.protx("shared_dissolve", protx_hash, 1, DISSOLVE_FEE)
        dissolve_tx = node.getrawtransaction(dissolve_txid, 1)
        assert_equal(dissolve_tx["proDisTx"]["proTxHash"], protx_hash)
        assert_equal(dissolve_tx["proDisTx"]["actorIndex"], 1)
        assert_equal(dissolve_tx["proDisTx"]["sigCount"], 1)
        # non-actor share is refunded principal plus the entire penalty, the actor absorbs it
        assert_equal(dissolve_tx["vout"][0]["scriptPubKey"]["address"], refund1)
        assert_equal(int(dissolve_tx["vout"][0]["value"] * COIN), 600 * COIN + EARLY_PENALTY)
        assert_equal(dissolve_tx["vout"][1]["scriptPubKey"]["address"], refund2)
        assert_equal(int(dissolve_tx["vout"][1]["value"] * COIN), 400 * COIN - EARLY_PENALTY - DISSOLVE_FEE)
        # Fund a source for the ordinary registration below before the dissolution confirms, so
        # the registration's inputs do not depend on the dissolution block
        reuse_funds = node.getnewaddress()
        node.sendtoaddress(reuse_funds, 1001)
        self.bump_mocktime(10 * 60 + 1)
        dissolve_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert dissolve_txid in node.getblock(dissolve_block)["tx"]
        assert_raises_rpc_error(None, None, node.protx, "info", protx_hash)
        assert_equal(node.masternodelist(), {})

        self.log.info("A share owner key freed by a dissolution is reusable only from the next block")
        # An ordinary registration whose owner key is share 1's owner key. Share owner keys share
        # the keyIDOwner uniqueness namespace, and a dissolved masternode is removed in the
        # collateral-spend phase after the block's provider transactions are applied, so the key
        # is still taken in the dissolution's own block and free in the block after it. The wallet
        # preflights against the tip, so build the registration once the key is free, then
        # disconnect the dissolution block to try both in one block.
        reuse_hex = node.protx("register_fund", node.getnewaddress(), f"127.0.0.1:{p2p_port(7)}", owner2,
                               node.bls("generate")["public"], node.getnewaddress(), 0, node.getnewaddress(),
                               reuse_funds, False)
        node.invalidateblock(dissolve_block)
        assert dissolve_txid in node.getrawmempool()
        assert_equal(node.testmempoolaccept([reuse_hex])[0]["reject-reason"], "bad-protx-dup-key")
        assert_raises_rpc_error(-25, "bad-protx-dup-key", self.generateblock, node, miner_addr,
                                [dissolve_txid, reuse_hex], sync_fun=self.no_op)
        node.reconsiderblock(dissolve_block)
        assert_equal(node.getbestblockhash(), dissolve_block)
        reuse_txid = node.sendrawtransaction(reuse_hex)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        reuse_info = node.protx("info", reuse_txid)
        assert_equal(reuse_info["state"]["ownerAddress"], owner2)
        assert_equal(len(node.masternodelist()), 1)
        # Spend the ordinary masternode's collateral so the rest of the test sees the same
        # masternode list it did before this check
        reuse_spend = node.createrawtransaction([{"txid": reuse_txid, "vout": reuse_info["collateralIndex"]}],
                                                {node.getnewaddress(): 999.999})
        reuse_spend_signed = node.signrawtransactionwithwallet(reuse_spend)
        assert_equal(reuse_spend_signed["complete"], True)
        node.sendrawtransaction(reuse_spend_signed["hex"])
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.masternodelist(), {})

        self.log.info("A standby dissolution signed inside the early period is valid after it ends")
        refund3, refund4 = node.getnewaddress(), node.getnewaddress()
        owner3, owner4 = node.getnewaddress(), node.getnewaddress()
        shares2 = [
            {"amount": 500 * COIN, "refundAddress": refund3, "ownerAddress": owner3},
            {"amount": 500 * COIN, "refundAddress": refund4, "rewardAddress": node.getnewaddress(),
             "ownerAddress": owner4},
        ]
        protx_hash2, _ = self.register_shared(node, shares2, port_offset=2)
        standby_hex = node.protx("shared_dissolve", protx_hash2, 0, DISSOLVE_FEE, False, False)
        # a signed unilateral dissolution must not be re-signed via the multi-party flow:
        # shared_sign signs the unanimous digest, which can never verify on a one-signature
        # transaction, so it fails fast instead of producing unusable signatures
        assert_raises_rpc_error(-8, "needs no shared_sign step", node.protx, "shared_sign", standby_hex)
        assert_equal(node.testmempoolaccept([standby_hex])[0]["allowed"], False)
        # mine past the early-period boundary; the same signed hex becomes and stays valid
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, EARLY_PERIOD_BLOCKS + 1, sync_fun=self.no_op)
        assert_equal(node.testmempoolaccept([standby_hex])[0]["allowed"], True)

        self.log.info("A unanimous dissolution is penalty-free")
        prepared = node.protx("shared_dissolve_prepare", protx_hash2, 0, DISSOLVE_FEE)
        # The digest commits lock fields, so shared_sign refuses a time-locked dissolution unless
        # the signer explicitly opts in
        locked_tx = tx_from_hex(prepared["tx"])
        locked_tx.nLockTime = node.getblockcount() + 100
        locked_tx.vin[0].nSequence = 0xfffffffe
        assert_raises_rpc_error(-8, "pass allowTimeLocks=true", node.protx, "shared_sign",
                                locked_tx.serialize().hex())
        assert_equal(len(node.protx("shared_sign", locked_tx.serialize().hex(), True)["signatures"]), 2)
        # a relative (BIP68) lock on the collateral input is refused the same way
        relative_tx = tx_from_hex(prepared["tx"])
        relative_tx.vin[0].nSequence = 10
        assert_raises_rpc_error(-8, "pass allowTimeLocks=true", node.protx, "shared_sign",
                                relative_tx.serialize().hex())
        sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        assert_equal(len(sigs), 2)
        # shared_sign signatures cover the unanimous digest, so combining only the actor's
        # signature could never produce a valid one-signature transaction: it is rejected
        # rather than returned as an unusable "standby"
        assert_raises_rpc_error(-8, "requires a signature from every share", node.protx,
                                "shared_combine", prepared["tx"], [sigs[0]])
        dissolve_txid2 = node.protx("shared_combine", prepared["tx"], sigs, True)
        dissolve_tx2 = node.getrawtransaction(dissolve_txid2, 1)
        assert_equal(dissolve_tx2["proDisTx"]["sigCount"], 2)
        assert_equal(int(dissolve_tx2["vout"][0]["value"] * COIN), 500 * COIN)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(node.masternodelist(), {})

        self.log.info("A pending dissolution and a same-masternode update can be mined together")
        # A ProDisTx removes the MN in the collateral-spend phase, after all other provider txs in
        # the block are applied, so a same-MN update and the dissolution can share a block in
        # either order. Regression guard: with the removal done mid-provider-loop instead, a block
        # ordering the ProDisTx before the update would fail BuildNewListFromBlock and abort mining.
        refund5, refund6 = node.getnewaddress(), node.getnewaddress()
        owner5, owner6 = node.getnewaddress(), node.getnewaddress()
        shares3 = [
            {"amount": 700 * COIN, "refundAddress": refund5, "ownerAddress": owner5},
            {"amount": 300 * COIN, "refundAddress": refund6, "ownerAddress": owner6},
        ]
        protx_hash3, _ = self.register_shared(node, shares3, port_offset=3)
        fee_addr2 = node.getnewaddress()
        node.sendtoaddress(fee_addr2, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        # Both transactions coexist in the mempool (no false provider conflict).
        dissolve_txid = node.protx("shared_dissolve", protx_hash3, 0, 500000)
        update_txid = node.protx("shared_update_share", protx_hash3, 1, node.getnewaddress(), fee_addr2)
        mempool = node.getrawmempool()
        assert dissolve_txid in mempool
        assert update_txid in mempool
        dissolve_tx = tx_from_hex(node.getrawtransaction(dissolve_txid))
        update_tx = tx_from_hex(node.getrawtransaction(update_txid))
        self.bump_mocktime(10 * 60 + 1)
        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert {dissolve_txid, update_txid}.issubset(node.getblock(block_hash)["tx"])
        block = from_hex(CBlock(), node.getblock(block_hash, 0))
        # Keep quorum commitments and any fee-funding ancestors ahead of the lifecycle pair.
        other_txs = [tx for tx in block.vtx
                     if tx.nType not in (TRANSACTION_PROVIDER_DISSOLVE, TRANSACTION_PROVIDER_UPDATE_SHARE)]
        assert_equal(len(block.vtx) - len(other_txs), 2)
        node.invalidateblock(block_hash)
        for txs in ([dissolve_tx, update_tx], [update_tx, dissolve_tx]):
            block.vtx = other_txs + txs
            block.nTime += 1
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()
            assert_equal(node.submitblock(block.serialize().hex()), None)
            assert_equal(node.getbestblockhash(), block.hash)
            assert_raises_rpc_error(None, None, node.protx, "info", protx_hash3)
            if txs[0] == dissolve_tx:
                node.invalidateblock(block.hash)
                assert_equal(node.protx("info", protx_hash3)["state"]["shares"][1]["rewardAddress"], refund6)
        assert_equal(node.getrawmempool(), [])
        assert_raises_rpc_error(None, None, node.protx, "info", protx_hash3)

        self.log.info("A reorg across a share update reverts the share table")
        refund7, refund8 = node.getnewaddress(), node.getnewaddress()
        owner7, owner8 = node.getnewaddress(), node.getnewaddress()
        shares4 = [
            {"amount": 800 * COIN, "refundAddress": refund7, "ownerAddress": owner7},
            {"amount": 200 * COIN, "refundAddress": refund8, "ownerAddress": owner8},
        ]
        protx_hash4, _ = self.register_shared(node, shares4, port_offset=5)
        registered_height4 = node.protx("info", protx_hash4)["state"]["registeredHeight"]

        fee_addr3 = node.getnewaddress()
        node.sendtoaddress(fee_addr3, 1)
        self.generate(node, 1, sync_fun=self.no_op)
        reward7 = node.getnewaddress()
        node.protx("shared_update_share", protx_hash4, 0, reward7, fee_addr3)
        self.bump_mocktime(10 * 60 + 1)
        update_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert_equal(node.protx("info", protx_hash4)["state"]["shares"][0]["rewardAddress"], reward7)
        # disconnecting the block must roll the deterministic list state back to the refund
        # fallback, and reconnecting must replay the update
        node.invalidateblock(update_block)
        assert_equal(node.protx("info", protx_hash4)["state"]["shares"][0]["rewardAddress"], refund7)
        node.reconsiderblock(update_block)
        assert_equal(node.protx("info", protx_hash4)["state"]["shares"][0]["rewardAddress"], reward7)

        self.test_pending_registrar_update(node, protx_hash4)
        self.test_voting_payee_conflict_eviction(node, protx_hash4)
        self.test_service_update_before_key_change(node, protx_hash4, port=5)

        self.log.info("Shared state survives a node restart")
        state_before_restart = node.protx("info", protx_hash4)["state"]
        self.restart_node(0)
        assert_equal(node.protx("info", protx_hash4)["state"], state_before_restart)

        self.log.info("Shared state survives a restart that reloads the list from a disk snapshot")
        # The deterministic list is snapshotted every DISK_SNAPSHOT_PERIOD blocks; a restart with
        # the tip exactly on a snapshot boundary reloads the shared state from that snapshot
        # rather than replaying diffs. Do this once with the current share table, then change a
        # share reward script and do it again at the next boundary.
        snapshot_period = 576
        for round_index in range(2):
            if round_index == 1:
                fee_addr4 = node.getnewaddress()
                node.sendtoaddress(fee_addr4, 1)
                self.generate(node, 1, sync_fun=self.no_op)
                reward7b = node.getnewaddress()
                node.protx("shared_update_share", protx_hash4, 0, reward7b, fee_addr4)
                self.bump_mocktime(10 * 60 + 1)
                self.generate(node, 1, sync_fun=self.no_op)
                assert_equal(node.protx("info", protx_hash4)["state"]["shares"][0]["rewardAddress"], reward7b)
            height = node.getblockcount()
            to_boundary = snapshot_period - (height % snapshot_period)
            with node.assert_debug_log(["Wrote snapshot. nHeight=%d" % (height + to_boundary)]):
                self.bump_mocktime(to_boundary)
                self.generate(node, to_boundary, sync_fun=self.no_op)
            assert_equal(node.getblockcount() % snapshot_period, 0)
            state_at_snapshot = node.protx("info", protx_hash4)["state"]
            self.restart_node(0)
            assert_equal(node.protx("info", protx_hash4)["state"], state_at_snapshot)
            assert_equal(node.protx("info", protx_hash4, node.getbestblockhash())["state"], state_at_snapshot)

        self.log.info("A reorg across a dissolution restores the masternode and its shares")
        registrar_fee, share_fee = node.getnewaddress(), node.getnewaddress()
        node.sendmany("", {registrar_fee: 1, share_fee: 1})
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info_before_dissolve = node.protx("info", protx_hash4)
        dissolve_txid4 = node.protx("shared_dissolve", protx_hash4, 0, DISSOLVE_FEE)
        self.bump_mocktime(10 * 60 + 1)
        dissolve_block = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert_raises_rpc_error(None, None, node.protx, "info", protx_hash4)
        node.invalidateblock(dissolve_block)
        info_restored = node.protx("info", protx_hash4)
        assert_equal(info_restored["state"]["shares"], info_before_dissolve["state"]["shares"])
        assert_equal(info_restored["state"]["registeredHeight"], registered_height4)
        # the disconnected dissolution returns to the mempool: the restored masternode makes it
        # valid again, so it is not lost by the reorg
        assert dissolve_txid4 in node.getrawmempool()
        # the restored masternode can be updated again, but the dissolution block was mined without
        # these updates: once it is reconnected they can never be mined and must not stay behind
        prepared = node.protx("shared_update_registrar_prepare", protx_hash4, "", node.getnewaddress(), registrar_fee)
        registrar_sigs = node.protx("shared_sign", prepared["tx"])["signatures"]
        registrar = node.protx("shared_combine", prepared["tx"], registrar_sigs)
        pending_txids = {node.sendrawtransaction(registrar),
                         node.protx("shared_update_share", protx_hash4, 0, node.getnewaddress(), share_fee)}
        assert pending_txids.issubset(node.getrawmempool())
        node.reconsiderblock(dissolve_block)
        assert_raises_rpc_error(None, None, node.protx, "info", protx_hash4)
        assert dissolve_txid4 not in node.getrawmempool()
        assert pending_txids.isdisjoint(node.getrawmempool())
        assert_equal(node.masternodelist(), {})

        self.log.info("Every participant's principal was refunded by the dissolutions")
        # refund1/refund2 were refunded by the first MN's unilateral dissolution, refund3/refund4
        # by the second MN's unanimous dissolution (reward receipts are checked separately above)
        for addr in (refund1, refund2, refund3, refund4, refund5, refund6, refund7, refund8):
            assert_greater_than(node.getreceivedbyaddress(addr, 1), Decimal(0))

        self.test_separate_participant_wallets()
        self.test_evonode_shared_registration()

    def test_evonode_shared_registration(self):
        self.log.info("An EvoNode cannot be prepared for shared collateral before evo_shares activation")
        node = self.nodes[0]
        wallet = node.get_wallet_rpc(self.default_wallet_name)
        assert not softfork_active(node, "evo_shares")
        port = p2p_port(9)
        platform_node_id = "%040x" % port
        shares = [
            {"amount": 3000 * COIN, "refundAddress": wallet.getnewaddress(), "ownerAddress": wallet.getnewaddress()},
            {"amount": 1000 * COIN, "refundAddress": wallet.getnewaddress(), "ownerAddress": wallet.getnewaddress()},
        ]
        # Funded before activation so that its inputs are mature below the activation height too
        funding_hex = self.build_funding_tx(wallet, EVO_COLLATERAL)
        operator = node.bls("generate")
        args = [funding_hex, shares, f"127.0.0.1:{port}", operator["public"], wallet.getnewaddress(), 0,
                EARLY_PERIOD_BLOCKS, EARLY_PENALTY, platform_node_id, [f"127.0.0.1:{port + 1000}"],
                [f"127.0.0.1:{port + 2000}"]]
        assert_raises_rpc_error(-8, "requires the evo_shares deployment to be active", wallet.protx,
                                "shared_register_prepare_evo", *args)

        self.activate("evo_shares")
        prepared = wallet.protx("shared_register_prepare_evo", *args)
        assert_equal(prepared["terms"]["platformNodeID"], platform_node_id)
        signatures = wallet.protx("shared_sign", prepared["tx"])["signatures"]
        signed = wallet.signrawtransactionwithwallet(wallet.protx("shared_combine", prepared["tx"], signatures))
        assert_equal(signed["complete"], True)

        self.log.info("A shared EvoNode registration is invalid in a block before evo_shares activation")
        last_inactive = node.getblockhash(get_bip9_details(node, "evo_shares")["since"] - 1)
        node.invalidateblock(last_inactive)
        assert not softfork_active(node, "evo_shares")
        self.assert_rejected_transaction(node, tx_from_hex(signed["hex"]), "bad-protx-shares-evo")
        node.reconsiderblock(last_inactive)

        self.log.info("An EvoNode registers with shared collateral once evo_shares is active")
        protx_hash = node.sendrawtransaction(signed["hex"])
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        info = node.protx("info", protx_hash)
        assert_equal(info["type"], "Evo")
        assert_equal(info["state"]["platformNodeID"], platform_node_id)
        assert_equal([share["amount"] for share in info["state"]["shares"]], [share["amount"] for share in shares])

        self.log.info("A shared EvoNode's owner reward is split by share amounts")
        payees = self.owner_gbt_payees(node)
        assert_equal([p["payee"] for p in payees], [share["refundAddress"] for share in shares])
        owner_total = sum(p["amount"] for p in payees)
        assert_equal(payees[0]["amount"], owner_total * shares[0]["amount"] // EVO_COLLATERAL)
        assert_equal(payees[1]["amount"], owner_total - payees[0]["amount"])
        self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("A shared EvoNode updates its Platform service, a share reward and its voting key")
        fee_addresses = [wallet.getnewaddress() for _ in range(3)]
        wallet.sendmany("", {address: 1 for address in fee_addresses})
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        new_platform_node_id = "%040x" % (port + 1)
        wallet.protx("update_service_evo", protx_hash, [f"127.0.0.1:{port}"], operator["secret"], new_platform_node_id,
                     [f"127.0.0.1:{port + 3000}"], [f"127.0.0.1:{port + 4000}"], "", fee_addresses[0])
        reward = wallet.getnewaddress()
        wallet.protx("shared_update_share", protx_hash, 0, reward, fee_addresses[1])
        voting = wallet.getnewaddress()
        prepared = wallet.protx("shared_update_registrar_prepare", protx_hash, "", voting, fee_addresses[2])
        wallet.protx("shared_combine", prepared["tx"], wallet.protx("shared_sign", prepared["tx"])["signatures"], True)
        self.bump_mocktime(10 * 60 + 1)
        self.generate(node, 1, sync_fun=self.no_op)
        state = node.protx("info", protx_hash)["state"]
        assert_equal(state["platformNodeID"], new_platform_node_id)
        assert_equal(state["shares"][0]["rewardAddress"], reward)
        assert_equal(state["votingAddress"], voting)

        self.log.info("A shared EvoNode dissolves and refunds its collateral to every share")
        prepared = wallet.protx("shared_dissolve_prepare", protx_hash, 1, DISSOLVE_FEE)
        dissolution = wallet.protx("shared_combine", prepared["tx"], wallet.protx("shared_sign", prepared["tx"])["signatures"])
        refunds = {out["scriptPubKey"]["address"]: out["value"] for out in node.decoderawtransaction(dissolution)["vout"]}
        assert_equal(refunds, {shares[0]["refundAddress"]: Decimal(shares[0]["amount"]) / COIN,
                               shares[1]["refundAddress"]: Decimal(shares[1]["amount"] - DISSOLVE_FEE) / COIN})
        dissolve_txid = node.sendrawtransaction(dissolution)
        self.bump_mocktime(10 * 60 + 1)
        block_hash = self.generate(node, 1, sync_fun=self.no_op)[0]
        assert dissolve_txid in node.getblock(block_hash)["tx"]
        assert_equal(node.masternodelist(), {})


if __name__ == '__main__':
    MasternodeSharesTest().main()
