#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test vaults implemented with OP_VAULT.

See the `VaultSpec` class and nearby functions for a general sense of the
transactional structure of vaults.
"""

import copy
import typing as t
from collections import defaultdict, Counter

from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import P2PInterface
from test_framework.wallet import MiniWallet, MiniWalletMode
from test_framework.script import CScript, OP_VAULT, OP_UNVAULT
from test_framework.messages import CTransaction, COutPoint, CTxOut, CTxIn, COIN
from test_framework.util import assert_equal, assert_raises_rpc_error

from test_framework import script, key, messages, script_util


class RecoveryAuthorization:
    """
    Handles generating sPKs and corresponding spend scripts for the optional
    recovery authorization.
    """
    spk: t.Union[CScript, bytes] = b''

    def get_spend_wit_stack(self, tx_to: CTransaction, vin_idx: int, amount: int,
                            outpoint: t.List[CTxOut]):
        return []


class VaultsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            [
                # Use only one script thread to get the exact reject reason for testing
                "-par=1",
                # 0-value outputs, probably among other things in OP_VAULT, are
                # non-standard until transaction v3 policy gets merged.
                "-acceptnonstdtxn=1",
                # TODO: remove when package relay submission becomes a thing.
                "-minrelaytxfee=0",
                # "-dustrelayfee=0",
            ]
        ]
        self.setup_clean_chain = True

    def run_test(self):
        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_OP_TRUE)
        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())

        # Generate some matured UTXOs to spend into vaults.
        self.generate(wallet, 200)

        for recovery_auth in (NoRecoveryAuth(),
                              P2WPKHRecoveryAuth(),
                              TaprootKeyRecoveryAuth()):
            def title(msg: str) -> None:
                self.log.info("[%s] %s", recovery_auth.__class__.__name__, msg)

            title("testing normal vault spend")
            self.single_vault_test(node, wallet, recovery_auth)

            title("testing recovery from the vault")
            self.single_vault_test(
                node, wallet, recovery_auth, recover_from_vault=True)

            title("testing recovery from the unvault trigger")
            self.single_vault_test(
                node, wallet, recovery_auth, recover_from_unvault=True)

            title("testing a batch recovery operation across multiple vaults")
            self.test_batch_recovery(node, wallet, recovery_auth)

            title("testing a batch unvault")
            self.test_batch_unvault(node, wallet, recovery_auth)

            title("testing revaults")
            self.test_revault(node, wallet, recovery_auth)

            title("testing that recoveries must be replaceable")
            self.test_nonreplaceable_recovery(node, wallet, recovery_auth)

            if recovery_auth.__class__ != NoRecoveryAuth:
                title("testing that recovery authorization is checked")
                self.test_authed_recovery_witness_check(node, wallet, recovery_auth)

        self.log.info("testing recursive recovery attack")
        self.test_recursive_recovery_witness(node, wallet)

        self.log.info("testing unauth'd recovery")
        self.test_unauthed_recovery(node, wallet)

        self.log.info("testing bad spend delay")
        self.test_bad_spend_delay(node, wallet)

        self.log.info("testing bad vout idx")
        self.test_bad_vout_idx(node, wallet)

    def single_vault_test(
        self,
        node,
        wallet,
        recovery_auth: RecoveryAuthorization,
        recover_from_vault: bool = False,
        recover_from_unvault: bool = False,
    ):
        """
        Test the creation and spend of a single vault, optionally recovering at various
        stages of the vault lifecycle.
        """
        assert_equal(node.getmempoolinfo()["size"], 0)

        vault = VaultSpec(recovery_auth=recovery_auth)
        vault_init_tx = vault.get_initialize_vault_tx(wallet)

        self.assert_broadcast_tx(vault_init_tx, mine_all=True)

        tx_mutator = TxMutator(vault)

        if recover_from_vault:
            assert vault.vault_outpoint
            recovery_tx = get_recovery_tx({vault: vault.vault_outpoint})

            if type(recovery_auth) != NoRecoveryAuth:
                for mutation, err in {tx_mutator.recovery_with_bad_witness: 'script-verify-flag'}.items():
                    self.assert_tx_mutation_fails(mutation, recovery_tx, err=err)

            self.assert_broadcast_tx(recovery_tx, mine_all=True)
            return

        assert vault.total_amount_sats and vault.total_amount_sats > 0
        target_amounts = split_vault_value(vault.total_amount_sats)
        target_keys = [
            key.ECKey(secret=(4 + i).to_bytes(32, "big"))
            for i in range(len(target_amounts))
        ]

        # The final withdrawal destination for the vaulted funds.
        final_target_vout = [
            CTxOut(nValue=amt, scriptPubKey=make_segwit0_spk(key))
            for amt, key in zip(target_amounts, target_keys)
        ]

        target_out_hash = target_outputs_hash(final_target_vout)
        unvault_spec = UnvaultSpec([vault], target_out_hash)
        start_unvault_tx = get_trigger_unvault_tx([unvault_spec])

        mutation_to_err = {
            tx_mutator.unvault_with_bad_recovery_spk: 'OP_UNVAULT outputs not compatible',
            tx_mutator.unvault_with_bad_spend_delay: 'OP_UNVAULT outputs not compatible',
            tx_mutator.unvault_with_low_amount: 'OP_UNVAULT outputs not compatible',
            tx_mutator.unvault_with_high_amount: 'bad-txns-in-belowout',
            tx_mutator.unvault_with_bad_opcode: 'OP_UNVAULT outputs not compatible',
        }
        for mutation, err in mutation_to_err.items():
            self.assert_tx_mutation_fails(mutation, start_unvault_tx, err=err)

        start_unvault_txid = self.assert_broadcast_tx(start_unvault_tx)

        unvault_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(start_unvault_txid), byteorder="big"), n=0
        )

        if recover_from_unvault:
            recovery_tx = get_recovery_tx(
                {vault: unvault_outpoint},
                start_unvault_tx,
                presig_tx_mutator=tx_mutator.unvault_with_low_amount)
            # TODO make this more specific
            self.assert_broadcast_tx(recovery_tx, err_msg='script-verify-flag')

            if type(recovery_auth) != NoRecoveryAuth:
                for mutation, err in {tx_mutator.recovery_with_bad_witness: 'script-verify-flag'}.items():
                    self.assert_tx_mutation_fails(mutation, recovery_tx, err=err)

            recovery_tx = get_recovery_tx({vault: unvault_outpoint}, start_unvault_tx)

            self.assert_broadcast_tx(recovery_tx, mine_all=True)
            return

        final_tx = get_final_withdrawal_tx(
            unvault_outpoint, final_target_vout, unvault_spec)

        # Broadcasting before start_unvault is confirmed fails.
        self.assert_broadcast_tx(final_tx, err_msg="non-BIP68-final")

        XXX_mempool_fee_hack_for_no_pkg_relay(node, start_unvault_txid)

        # Generate almost past the timelock, but not quite.
        self.generate(wallet, vault.spend_delay - 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        self.assert_broadcast_tx(final_tx, err_msg="non-BIP68-final")

        self.generate(wallet, 1)

        # Generate bad nSequence values.
        final_bad = copy.deepcopy(final_tx)
        final_bad.vin[0].nSequence = final_tx.vin[0].nSequence - 1
        self.assert_broadcast_tx(final_bad, err_msg="timelock has not matured")

        # Generate bad amounts.
        final_bad = copy.deepcopy(final_tx)
        final_bad.vout[0].nValue = final_tx.vout[0].nValue - 1
        self.assert_broadcast_tx(final_bad, err_msg="target hash mismatch")

        # Finally the unvault completes.
        self.assert_broadcast_tx(final_tx, mine_all=True)

    def test_batch_recovery(
        self,
        node,
        wallet,
        recovery_auth: RecoveryAuthorization,
    ):
        """
        Test generating multiple vaults and recovering them to the recovery path in batch.

        One of the vaults has been triggered for withdrawal while the other two
        are still vaulted.
        """
        # Create some vaults with the same unvaulting key, recovery key, but different
        # spend delays.
        vaults = [
            VaultSpec(spend_delay=i, recovery_auth=recovery_auth) for i in [10, 11, 12]
        ]

        # Ensure that vaults with different recovery keys can be swept in the same
        # transaction, using the same fee management.
        vault_diff_recovery = VaultSpec(
            recovery_secret=(DEFAULT_RECOVERY_SECRET + 10),
            recovery_auth=recovery_auth,
        )

        for v in vaults + [vault_diff_recovery]:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        assert_equal(node.getmempoolinfo()["size"], len(vaults + [vault_diff_recovery]))
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Start unvaulting the first vault.
        first_unvault_spec = UnvaultSpec([vaults[0]], b'\x00' * 32)
        unvault1_tx = get_trigger_unvault_tx([first_unvault_spec])
        unvault1_txid = unvault1_tx.rehash()
        assert_equal(
            node.sendrawtransaction(unvault1_tx.serialize().hex()), unvault1_txid,
        )

        XXX_mempool_fee_hack_for_no_pkg_relay(node, unvault1_txid)
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # So now, 1 vault is in the process of unvaulting and the other two are still
        # vaulted. Recover them all with one transaction.
        unvault1_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(unvault1_txid), byteorder="big"), n=0
        )

        assert vaults[1].vault_outpoint
        assert vaults[2].vault_outpoint
        assert vault_diff_recovery.vault_outpoint

        outpoint_map = {
            vaults[0]: unvault1_outpoint,
            vaults[1]: vaults[1].vault_outpoint,
            vaults[2]: vaults[2].vault_outpoint,
            vault_diff_recovery: vault_diff_recovery.vault_outpoint,
        }

        # The no-recovery-auth case is different from an authorized recovery.
        if recovery_auth.__class__ == NoRecoveryAuth:
            bad_tx = get_recovery_tx(outpoint_map, unvault1_tx)

            # Will fail due to extra recovery-incompatible vault input.
            self.assert_broadcast_tx(bad_tx, err_msg='recovery transaction has bad output structure')

            outpoint_map.pop(vault_diff_recovery)

            bad_tx = get_recovery_tx(outpoint_map, unvault1_tx, fee_wallet=wallet)

            # Will fail due to extra non-vault fee input.
            self.assert_broadcast_tx(bad_tx, err_msg='recovery transaction has bad output structure')

            mutator = vout_amount_mutator(0, -1)
            bad_tx = get_recovery_tx(
                outpoint_map, unvault1_tx, presig_tx_mutator=mutator)
            self.assert_broadcast_tx(bad_tx, err_msg='vault-insufficient-recovery-value')

            # Finally okay.
            recovery_tx = get_recovery_tx(outpoint_map, unvault1_tx)
            assert_equal(len(recovery_tx.vout), 2)
            self.assert_broadcast_tx(recovery_tx, mine_all=True)
        else:
            # Test authorized recoveries, which are much more flexible.
            idx_error = {
                1: "vault-insufficient-recovery-value",
                2: "recovery output nValue too low",
            }

            # TODO clean this up; first vout is the fee bump. too implicit right now.
            for i, mutator in enumerate([vout_amount_mutator(vout_idx, -1) for vout_idx in [1, 2]]):
                self.log.info(f"  bad recovery tx: {idx_error[i + 1]}")
                bad_tx = get_recovery_tx(
                    outpoint_map, unvault1_tx,
                    fee_wallet=wallet,
                    presig_tx_mutator=mutator,
                )
                self.assert_broadcast_tx(bad_tx, err_msg=idx_error[i + 1])

            recovery_tx = get_recovery_tx(
                outpoint_map, unvault1_tx, fee_wallet=wallet)

            idxs = list(range(len(recovery_tx.vout)))
            assert_equal(len(idxs), 4)
            self.assert_broadcast_tx(recovery_tx, mine_all=True)

    def test_batch_unvault(
        self,
        node,
        wallet,
        recovery_auth: RecoveryAuthorization,
    ):
        """
        Test generating multiple vaults and ensure those with compatible parameters
        (spend delay and recovery path) can be unvaulted in batch.
        """
        common_spend_delay = 10
        # Create some vaults with the same spend delay and recovery key, so that they
        # can be unvaulted together, but different unvault keys.
        vaults = [
            VaultSpec(spend_delay=common_spend_delay,
                      unvault_trigger_secret=i) for i in [10, 11, 12]
        ]
        # Create a vault with a different recovery path than the vaults above; cannot
        # be batch unvaulted.
        vault_diff_recovery = VaultSpec(
            spend_delay=10, recovery_secret=(DEFAULT_RECOVERY_SECRET + 1))

        # Create a vault with the same recovery path but a different spend delay;
        # cannot be batch unvaulted.
        vault_diff_delay = VaultSpec(spend_delay=(common_spend_delay - 1))

        for v in vaults + [vault_diff_recovery, vault_diff_delay]:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        assert_equal(node.getmempoolinfo()["size"], len(vaults) + 2)
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Construct the final unvault target.
        unvault_total_sats = sum(v.total_amount_sats for v in vaults)  # type: ignore
        assert unvault_total_sats > 0
        target_key = key.ECKey(secret=(1).to_bytes(32, "big"))

        # The final withdrawal destination for the vaulted funds.
        final_target_vout = [
            CTxOut(nValue=unvault_total_sats,
                   scriptPubKey=make_segwit0_spk(target_key))
        ]
        target_out_hash = target_outputs_hash(final_target_vout)

        # Ensure that we can't batch with incompatible OP_VAULTs.
        for incompat_vaults in [[vault_diff_recovery],
                                [vault_diff_delay],
                                [vault_diff_recovery, vault_diff_delay]]:
            spec = UnvaultSpec(vaults + incompat_vaults, target_out_hash)
            failed_unvault = get_trigger_unvault_tx([spec])
            self.assert_broadcast_tx(
                failed_unvault, err_msg="OP_UNVAULT outputs not compatible")

        unvault_spec = UnvaultSpec(vaults, target_out_hash)
        good_batch_unvault = get_trigger_unvault_tx([unvault_spec])
        good_txid = self.assert_broadcast_tx(good_batch_unvault, mine_all=True)

        unvault_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(good_txid), byteorder="big"), n=0
        )
        final_tx = get_final_withdrawal_tx(
            unvault_outpoint, final_target_vout, unvault_spec)

        self.assert_broadcast_tx(final_tx, err_msg="non-BIP68-final")

        self.generate(node, common_spend_delay)

        # Finalize the batch withdrawal.
        self.assert_broadcast_tx(final_tx, mine_all=True)

    def test_revault(
        self,
        node,
        wallet,
        recovery_auth: RecoveryAuthorization,
    ):
        """
        Test that some amount can be "revaulted" during the unvault process. That
        amount is immediately deposited back into the same vault that is currently
        undergoing unvaulting, so that the remaining balance within the vault can
        be managed independent of the timelock that the first unvault process has
        induced.
        """
        common_spend_delay = 10
        # To support revaults when dealing with a batch, each vault must share the
        # same vault sPK.
        vaults = [
            VaultSpec(spend_delay=common_spend_delay, unvault_trigger_secret=10)
            for _ in range(3)
        ]

        for v in vaults:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        assert_equal(node.getmempoolinfo()["size"], len(vaults))
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        revault_amount = COIN
        # Construct the final unvault target; save a coin to peel off in the revault.
        unvault_total_sats = sum(
            v.total_amount_sats for v in vaults) - revault_amount  # type: ignore
        assert unvault_total_sats > 0
        target_key = key.ECKey(secret=(1).to_bytes(32, "big"))

        # The final withdrawal destination for the vaulted funds.
        final_target_vout = [
            CTxOut(nValue=unvault_total_sats,
                   scriptPubKey=make_segwit0_spk(target_key))
        ]
        target_out_hash = target_outputs_hash(final_target_vout)

        unvault_spec = UnvaultSpec(vaults, target_out_hash, revault_amount_sats=revault_amount)
        unvault_tx = get_trigger_unvault_tx([unvault_spec])

        tx_mutator = TxMutator(vaults[0])
        mutation_to_err = {
            tx_mutator.revault_with_high_amount: 'bad-txns-in-belowout',
            tx_mutator.revault_with_low_amount: 'vault-insufficient-trigger-value',
        }
        for mutation, err in mutation_to_err.items():
            self.log.info(f"  bad tx: {mutation.__name__}")
            spec = copy.deepcopy(unvault_spec)
            unvault_tx = get_trigger_unvault_tx([spec], presig_tx_mutator=mutation)
            txid = self.assert_broadcast_tx(unvault_tx, err_msg=err)

        unvault_tx = get_trigger_unvault_tx([unvault_spec])
        txid = self.assert_broadcast_tx(unvault_tx, mine_all=True)

        unvault_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(txid), byteorder="big"), n=0)

        final_tx = get_final_withdrawal_tx(
            unvault_outpoint, final_target_vout, unvault_spec)

        self.assert_broadcast_tx(final_tx, err_msg="non-BIP68-final")

        revault_spec = copy.deepcopy(vaults[0])
        revault_spec.total_amount_sats = revault_amount
        revault_spec.vault_output = unvault_tx.vout[1]
        revault_spec.vault_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(txid), byteorder="big"), n=1)

        revault_unvault_spec = UnvaultSpec([revault_spec], b'\x00' * 32)
        revault_unvault_tx = get_trigger_unvault_tx([revault_unvault_spec])
        # The revault output should be immediately spendable into an OP_UNVAULT
        # output.
        self.assert_broadcast_tx(revault_unvault_tx, mine_all=True)

        self.generate(node, common_spend_delay)

        # Finalize the batch withdrawal.
        self.assert_broadcast_tx(final_tx, mine_all=True)

    def test_recursive_recovery_witness(self, node, wallet):
        """
        Test a pathological script that would induce infinite recursion in the script
        interpreter if recursive EvalScript() calls were not specifically limited.
        """
        assert_equal(node.getmempoolinfo()["size"], 0)

        recursive_script = CScript(
            [script.OP_3, script.OP_PICK, script.OP_3, script.OP_PICK, script.OP_3,
             script.OP_PICK, script.OP_3, script.OP_PICK, script.OP_VAULT])

        exploit_auth = TaprootScriptRecoveryAuth(recursive_script)

        vault = VaultSpec(recovery_auth=exploit_auth)
        vault_init_tx = vault.get_initialize_vault_tx(wallet)
        assert vault.vault_outpoint

        exploit_auth.script_witness_stack = [
            recursive_script,
            b'',  # <recovery-vout-idx> = 0
            vault.recovery_params,
            CScript([vault.spend_delay]),
            b'\x00' * 32,
        ]

        self.assert_broadcast_tx(vault_init_tx, mine_all=True)
        recovery_tx = get_recovery_tx({vault: vault.vault_outpoint})
        self.assert_broadcast_tx(recovery_tx, err_msg="Too many recursive calls")

    def test_unauthed_recovery(self, node, wallet):
        """
        Test rules specific to unauthenticated recoveries.
        """
        # Create some vaults with the same unvaulting key, recovery key, but different
        # spend delays.
        recovery_auth = NoRecoveryAuth()
        vaults = [
            VaultSpec(spend_delay=i, recovery_auth=recovery_auth) for i in [10, 11, 200]
        ]

        # Ensure that vaults with different recovery keys can be swept in the same
        # transaction, using the same fee management.
        vault_diff_recovery = VaultSpec(
            recovery_secret=(DEFAULT_RECOVERY_SECRET + 10),
            recovery_auth=recovery_auth,
        )

        for v in vaults + [vault_diff_recovery]:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        assert vaults[0].vault_outpoint
        assert vaults[1].vault_outpoint
        assert vaults[2].vault_outpoint
        assert vault_diff_recovery.vault_outpoint

        outpoint_map = {
            vaults[0]: vaults[0].vault_outpoint,
            vaults[1]: vaults[1].vault_outpoint,
            vaults[2]: vaults[2].vault_outpoint,
            vault_diff_recovery: vault_diff_recovery.vault_outpoint,
        }

        bad1 = get_recovery_tx(outpoint_map)

        # Will fail due to extra recovery-incompatible vault input.
        self.assert_broadcast_tx(bad1, err_msg='recovery transaction has bad output structure')

        outpoint_map.pop(vault_diff_recovery)

        bad2 = get_recovery_tx(outpoint_map, fee_wallet=wallet)

        # Will fail due to extra non-vault fee input.
        self.assert_broadcast_tx(bad2, err_msg='recovery transaction has bad output structure')

        mutator = vout_amount_mutator(0, -1)
        bad3 = get_recovery_tx(outpoint_map, presig_tx_mutator=mutator)
        self.assert_broadcast_tx(bad3, err_msg='vault-insufficient-recovery-value')

        # Finally okay - and note that we can include an unrelated input that contributes
        # solely to fees.
        bad4 = get_recovery_tx(
            outpoint_map, fee_wallet=wallet, fee_coin_amount_sats=10_000)
        bad4.vout = bad4.vout[0:2]  # Include fee-change output and recovery output

        assert_equal(len(bad4.vout), 2)
        assert_equal(len(bad4.vin), 4)
        self.assert_broadcast_tx(bad4, err_msg='recovery transaction has bad output structure')

        # Finally okay - and note that we can include an unrelated input that contributes
        # solely to fees.
        good_tx = get_recovery_tx(
            outpoint_map, fee_wallet=wallet, fee_just_input=True, fee_coin_amount_sats=10_000)

        assert_equal(len(good_tx.vout), 2)
        assert_equal(len(good_tx.vin), 4)
        self.assert_broadcast_tx(good_tx, mine_all=True)

    def test_authed_recovery_witness_check(self, node, wallet, recovery_auth: RecoveryAuthorization):
        """
        Ensure that authorized recoveries actually check signatures.
        """
        vaults = [
            VaultSpec(spend_delay=i, recovery_auth=recovery_auth) for i in [10, 11, 200]
        ]

        # Ensure that vaults with different recovery keys can be swept in the same
        # transaction, using the same fee management.
        vault_diff_recovery = VaultSpec(
            recovery_secret=(DEFAULT_RECOVERY_SECRET + 10),
            recovery_auth=recovery_auth,
        )

        for v in vaults + [vault_diff_recovery]:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Start unvaulting the first vault.
        first_unvault_spec = UnvaultSpec([vaults[0]], b'\x00' * 32)
        unvault1_tx = get_trigger_unvault_tx([first_unvault_spec])
        unvault1_txid = unvault1_tx.rehash()
        assert_equal(
            node.sendrawtransaction(unvault1_tx.serialize().hex()), unvault1_txid,
        )

        XXX_mempool_fee_hack_for_no_pkg_relay(node, unvault1_txid)
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # So now, 1 vault is in the process of unvaulting and the other two are still
        # vaulted. Recover them all with one transaction.
        unvault1_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(unvault1_txid), byteorder="big"), n=0
        )
        assert vaults[1].vault_outpoint
        assert vaults[2].vault_outpoint
        assert vault_diff_recovery.vault_outpoint

        outpoint_map = {
            vaults[0]: unvault1_outpoint,
            vaults[1]: vaults[1].vault_outpoint,
            vaults[2]: vaults[2].vault_outpoint,
            vault_diff_recovery: vault_diff_recovery.vault_outpoint,
        }

        recov_tx = get_recovery_tx(outpoint_map, unvault1_tx, fee_wallet=wallet)

        # Perturb the input nSequence, disrupting the signature, but skip the fee input (last).
        for inp_index in range(len(recov_tx.vin) - 1):
            recov_tx.vin[inp_index].nSequence += 1

            err = 'Signature must be zero for failed CHECK'
            if recovery_auth.__class__ == TaprootKeyRecoveryAuth:
                err = 'Invalid Schnorr signature'

            self.assert_broadcast_tx(recov_tx, err_msg=err)

            # Reset all inputs to RBFable.
            for reset_index in range(len(recov_tx.vin)):
                recov_tx.vin[reset_index].nSequence = 0

    def test_nonreplaceable_recovery(
        self,
        node,
        wallet,
        recovery_auth: RecoveryAuthorization,
    ):
        """
        Ensure that recovery transactions (of any kind) are always replaceable.
        """
        # Create some vaults with the same unvaulting key, recovery key, but different
        # spend delays.
        vaults = [
            VaultSpec(spend_delay=i, recovery_auth=recovery_auth) for i in [10, 11, 200]
        ]

        for v in vaults:
            init_tx = v.get_initialize_vault_tx(wallet)
            self.assert_broadcast_tx(init_tx)

        assert_equal(node.getmempoolinfo()["size"], len(vaults))
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Start unvaulting the first vault.
        first_unvault_spec = UnvaultSpec([vaults[0]], b'\x00' * 32)
        unvault1_tx = get_trigger_unvault_tx([first_unvault_spec])
        unvault1_txid = unvault1_tx.rehash()
        assert_equal(
            node.sendrawtransaction(unvault1_tx.serialize().hex()), unvault1_txid,
        )

        XXX_mempool_fee_hack_for_no_pkg_relay(node, unvault1_txid)
        self.generate(wallet, 1)
        assert_equal(node.getmempoolinfo()["size"], 0)

        # So now, 1 vault is in the process of unvaulting and the other two are still
        # vaulted. Recover them all with one transaction.
        unvault1_outpoint = COutPoint(
            hash=int.from_bytes(bytes.fromhex(unvault1_txid), byteorder="big"), n=0
        )

        assert vaults[1].vault_outpoint
        assert vaults[2].vault_outpoint

        outpoint_map = {
            vaults[0]: unvault1_outpoint,
            vaults[1]: vaults[1].vault_outpoint,
            vaults[2]: vaults[2].vault_outpoint,
        }

        def make_final(vin_index: int):
            def mutator(tx):
                tx.vin[vin_index].nSequence = messages.MAX_BIP125_RBF_SEQUENCE + 1
            return mutator

        for inp_index in range(len(outpoint_map)):
            bad_tx = get_recovery_tx(
                outpoint_map, unvault1_tx, presig_tx_mutator=make_final(inp_index))

            self.assert_broadcast_tx(
                bad_tx, err_msg='Vault recovery inputs must be replaceable')

        # Finally okay.
        recov_tx = get_recovery_tx(outpoint_map, unvault1_tx)
        self.assert_broadcast_tx(recov_tx, mine_all=True)

    def test_bad_spend_delay(self, node, wallet):
        """
        Test that a negative spend delay fails.
        """
        # Create some vaults with the same unvaulting key, recovery key, but different
        # spend delays.
        recovery_auth = NoRecoveryAuth()
        vault = VaultSpec(spend_delay=-50, recovery_auth=recovery_auth)

        init_tx = vault.get_initialize_vault_tx(wallet)
        self.assert_broadcast_tx(init_tx, mine_all=True)

        recovery_tx = get_recovery_tx({vault: vault.vault_outpoint})
        self.assert_broadcast_tx(recovery_tx, err_msg='invalid spend delay')

        unvault_spec = UnvaultSpec([vault], b'\x00' * 32)
        start_unvault_tx = get_trigger_unvault_tx([unvault_spec])
        self.assert_broadcast_tx(start_unvault_tx, err_msg='invalid spend delay')

    def test_bad_vout_idx(self, node, wallet):
        # TODO
        pass

    def assert_broadcast_tx(
        self,
        tx: CTransaction,
        mine_all: bool = False,
        err_msg: t.Optional[str] = None
    ) -> str:
        """
        Broadcast a transaction and facilitate various assertions about how the
        broadcast went.
        """
        node = self.nodes[0]
        txhex = tx.serialize().hex()
        txid = tx.rehash()

        if not err_msg:
            assert_equal(node.sendrawtransaction(txhex), txid)
        else:
            assert_raises_rpc_error(
                -26, err_msg, node.sendrawtransaction, txhex,
            )

        if mine_all:
            XXX_mempool_fee_hack_for_no_pkg_relay(node, txid)
            self.generate(node, 1)
            assert_equal(node.getmempoolinfo()["size"], 0)

        return txid

    def assert_tx_mutation_fails(
        self,
        mutation_func: t.Callable[[CTransaction], None],
        tx_to_mutate: CTransaction,
        *,
        err: str,
        **kwargs,
    ):
        assert mutation_func.__doc__, "Mutation needs a description"
        desc = mutation_func.__doc__[:80]
        self.log.info("  bad tx: %s", desc)
        tx_copy = copy.deepcopy(tx_to_mutate)

        mutation_func(tx_copy)
        self.assert_broadcast_tx(tx_copy, err_msg=err)


def vout_amount_mutator(vout_idx, amount=1):
    def reducer(tx):
        # Have to copy the whole vout list, otherwise we're modifying CTxOut objects
        # and that modification persists into the VaultSpec object
        # (vs. just the transaction).
        tx.vout = copy.deepcopy(tx.vout)
        tx.vout[vout_idx].nValue += amount
    return reducer


def XXX_mempool_fee_hack_for_no_pkg_relay(node, txid: str):
    """
    XXX manually prioritizing the 0-fee trigger transaction is necessary because
    we aren't doing package relay here, so the block assembler (rationally?)
    isn't including the 0-fee transaction.
    """
    node.prioritisetransaction(txid=txid, fee_delta=COIN)


DEFAULT_RECOVERY_SECRET = 2
DEFAULT_UNVAULT_SECRET = 3


class NoRecoveryAuth(RecoveryAuthorization):
    spk = b''

    def get_spend_wit_stack(self, *args, **kwargs) -> t.List[bytes]:
        return []


class P2WPKHRecoveryAuth(RecoveryAuthorization):

    def __init__(self) -> None:
        self.key = key.ECKey(secret=DEFAULT_RECOVERY_SECRET.to_bytes(32, 'big'))
        self.pubkey = self.key.get_pubkey().get_bytes()
        self.spk = script_util.key_to_p2wpkh_script(self.key.get_pubkey().get_bytes())

    def get_spend_wit_stack(
        self, tx_to: CTransaction, vin_idx: int, amount: int, *args, **kwargs
    ) -> t.List[bytes]:
        sigscript = script_util.keyhash_to_p2pkh_script(script.hash160(self.pubkey))
        sighash = script.SegwitV0SignatureHash(
            sigscript, tx_to, vin_idx, script.SIGHASH_ALL, amount)
        sig = self.key.sign_ecdsa(sighash) + bytes([script.SIGHASH_ALL])
        return [sig, self.pubkey]


class TaprootKeyRecoveryAuth(RecoveryAuthorization):
    """Recovery authorization via Taproot script spend."""

    def __init__(self) -> None:
        self.key = key.ECKey(secret=DEFAULT_RECOVERY_SECRET.to_bytes(32, 'big'))
        self.tr_info = taproot_from_privkey(self.key)
        self.tweaked_privkey = key.tweak_add_privkey(
            self.key.get_bytes(), self.tr_info.tweak
        )

        self.spk = self.tr_info.scriptPubKey

    def get_spend_wit_stack(
        self, tx_to: CTransaction, vin_idx: int, amount: int, outpoints: t.List[CTxOut],
    ) -> t.List[bytes]:
        sigmsg = script.TaprootSignatureHash(
            tx_to, outpoints, input_index=vin_idx, hash_type=0
        )
        assert isinstance(self.tweaked_privkey, bytes)
        sig = key.sign_schnorr(self.tweaked_privkey, sigmsg)

        return [sig]


class TaprootScriptRecoveryAuth(RecoveryAuthorization):
    """Recovery authorization via Taproot script spend."""

    def __init__(self, script: CScript) -> None:
        self.script = script
        self.key = key.ECKey(secret=DEFAULT_RECOVERY_SECRET.to_bytes(32, 'big'))
        self.tr_info = taproot_from_privkey(self.key, scripts=[("only-path", script)])

        self.spk = self.tr_info.scriptPubKey

        # To be set later for spend.
        self.script_witness_stack: t.List[bytes] = []

    def get_spend_wit_stack(self, *args, **kwargs) -> t.List[bytes]:
        return [
            *self.script_witness_stack,
            self.script,
            (bytes([0xC0 + self.tr_info.negflag]) + self.tr_info.internal_pubkey),
        ]


class VaultSpec:
    """
    A specification for a single vault UTXO that consolidates the context needed to
    unvault or recover.
    """
    def __init__(
        self,
        recovery_secret: t.Optional[int] = None,
        unvault_trigger_secret: t.Optional[int] = None,
        spend_delay: int = 10,
        recovery_auth: RecoveryAuthorization = NoRecoveryAuth(),
    ):
        self.recovery_key = key.ECKey(
            secret=(recovery_secret or DEFAULT_RECOVERY_SECRET).to_bytes(32, 'big'))
        # Taproot info for the recovery key.
        self.recovery_tr_info = taproot_from_privkey(self.recovery_key)

        # The sPK that needs to be satisfied in order to sweep to the recovery path.
        self.recovery_auth = recovery_auth
        self.recovery_params: bytes = (
            recovery_spk_tagged_hash(self.recovery_tr_info.scriptPubKey) +
            self.recovery_auth.spk)

        # Use a basic key-path TR spend to trigger an unvault attempt.
        self.unvault_key = key.ECKey(
            secret=(
                unvault_trigger_secret or DEFAULT_UNVAULT_SECRET).to_bytes(32, 'big'))
        self.unvault_tr_info = taproot_from_privkey(self.unvault_key)
        self.unvault_tweaked_privkey = key.tweak_add_privkey(
            self.unvault_key.get_bytes(), self.unvault_tr_info.tweak
        )
        assert isinstance(self.unvault_tweaked_privkey, bytes)

        self.spend_delay = spend_delay

        # The OP_VAULT scriptPubKey that is hidden in the initializing vault
        # outputs's script-path spend.
        self.vault_spk = CScript([
            # <recovery-params>
            self.recovery_params,
            # <spend-delay>
            self.spend_delay,
            # <unvault-spk-hash>
            unvault_spk_tagged_hash(self.unvault_tr_info.scriptPubKey),
            OP_VAULT,
        ])

        # The initializing taproot output is either spendable via OP_VAULT
        # (script-path spend) or directly by the recovery key (key-path spend).
        self.init_tr_info = taproot_from_privkey(
            self.recovery_key, scripts=[("opvault", self.vault_spk)],
        )

        # Set when the vault is initialized.
        self.total_amount_sats: t.Optional[int] = None

        # Outpoint for spending the initialized vault.
        self.vault_outpoint: t.Optional[messages.COutPoint] = None
        self.vault_output: t.Optional[CTxOut] = None

    def get_initialize_vault_tx(
        self,
        wallet: MiniWallet,
        fees: t.Optional[int] = None,
    ) -> CTransaction:
        """
        Pull a UTXO from the wallet instance and spend it into the vault.

        The vault is spendable by either the usual means (unvault or recovery) via
        script-path spend, or immediately spendable by the recovery key via
        key-path spend.
        """
        utxo, utxo_in = wallet.get_utxo_as_txin()
        fees = fees if fees is not None else 10_000
        self.total_amount_sats = int(utxo["value"] * COIN) - fees

        self.vault_output = CTxOut(
            nValue=self.total_amount_sats,
            scriptPubKey=self.init_tr_info.scriptPubKey
        )
        vault_init = CTransaction()
        vault_init.nVersion = 2
        vault_init.vin = [utxo_in]
        vault_init.vout = [self.vault_output]

        # Cache the vault outpoint for later use during spending.
        self.vault_outpoint = messages.COutPoint(
            hash=int.from_bytes(bytes.fromhex(vault_init.rehash()), byteorder="big"),
            n=0
        )
        return vault_init


# Passed for <recovery-vout-idx> to signal that this spending transaction is not
# a recovery. For use on the witness stack, not in script.
NOT_A_RECOVERY_FLAG = script.bn2vch(-1)


class TxMutator(t.NamedTuple):
    """
    A class that tweaks transactions in various ways to test that invalid vault
    operations fail.

    Each method takes in a transaction copy and modifies it in place.

    For use with `VaultsTest.assert_tx_mutation_fails()`.
    """
    vault: VaultSpec

    def unvault_with_bad_recovery_spk(self, tx: CTransaction):
        """Unvault with wrong recovery path."""
        bad_unvault_spk = CScript([
            # <recovery-params>
            bytes([0x01] * 32),
            # <spend-delay>
            self.vault.spend_delay,
            # <target-outputs-hash>
            b'\x00' * 32,
            OP_UNVAULT,
        ])

        bad_taproot = script.taproot_construct(
            UNVAULT_NUMS_INTERNAL_PUBKEY,
            scripts=[('only-path', bad_unvault_spk, 0xC0)])

        tx.vout[0].scriptPubKey = bad_taproot.scriptPubKey

    def unvault_with_bad_spend_delay(self, tx: CTransaction):
        """Unvault with wrong spend delay."""
        bad_unvault_spk = CScript([
            # <recovery-params>
            self.vault.recovery_params,
            # <spend-delay>
            self.vault.spend_delay - 1,
            # <target-outputs-hash>
            b'\x00' * 32,
            OP_UNVAULT,
        ])

        bad_taproot = script.taproot_construct(
            UNVAULT_NUMS_INTERNAL_PUBKEY,
            scripts=[('only-path', bad_unvault_spk, 0xC0)])

        tx.vout[0].scriptPubKey = bad_taproot.scriptPubKey

    def unvault_with_bad_opcode(self, tx: CTransaction):
        """Unvault with wrong opcode."""
        spk = tx.vout[0].scriptPubKey
        tx.vout[0].scriptPubKey = spk[:-1] + bytes([OP_VAULT])

    def unvault_with_low_amount(self, tx: CTransaction):
        """Unvault with wrong amount (too little)."""
        tx.vout[0].nValue -= 1

    def unvault_with_high_amount(self, tx: CTransaction):
        """Unvault with wrong amount (too much)."""
        tx.vout[0].nValue += 1

    def revault_with_high_amount(self, tx: CTransaction):
        """Revault with wrong amount (too much)."""
        tx.vout[1].nValue += 1

    def revault_with_low_amount(self, tx: CTransaction):
        """Revault with wrong amount (too little)."""
        tx.vout[1].nValue -= 1

    def recovery_with_bad_witness(self, tx: CTransaction):
        """Recovery with a bad witness fails."""
        stack = tx.wit.vtxinwit[0].scriptWitness.stack

        assert len(stack) >= 1
        stack[0] = stack[0][::-1]


class UnvaultTriggerTransaction(CTransaction):
    """
    Used to track metadata about an unvault trigger transaction to make generating
    the final withdrawal transaction easier.
    """
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        # These are set after the transaction has been fully generated.
        self.unvault_specs: t.List[UnvaultSpec] = []

    @property
    def max_spend_delay(self) -> int:
        assert self.unvault_specs
        return max(spec.spend_delay for spec in self.unvault_specs)


def get_recovery_tx(
    vaults_to_outpoints: t.Dict[VaultSpec, messages.COutPoint],
    unvault_trigger_tx: t.Optional[UnvaultTriggerTransaction] = None,
    fee_wallet: t.Optional[MiniWallet] = None,
    fee_coin_amount_sats: t.Optional[int] = None,
    fee_just_input: bool = False,
    presig_tx_mutator: t.Optional[t.Callable] = None,
    postsig_tx_mutator: t.Optional[t.Callable] = None,
) -> CTransaction:
    """
    Generate a transaction that recovers the vaults to their shared recovery path, either
    from an OP_VAULT output or an OP_UNVAULT output, from the pairing
    outpoints.

    If we're recovering from an OP_UNVAULT trigger output, we need the TaprootInfo which
    accompanies `unvault_trigger_tx`.
    """
    vaults = list(vaults_to_outpoints.keys())

    compat_vaults = defaultdict(list)
    vault_totals: t.Any = Counter()
    vault_to_taproot  = {}
    vault_to_recov_output_index = {}

    for v in vaults:
        recov_tr = v.recovery_tr_info
        compat_vaults[recov_tr].append(v)
        assert v.total_amount_sats
        vault_totals[recov_tr] += v.total_amount_sats
        vault_to_taproot[v] = recov_tr

    recovery_tx = CTransaction()
    recovery_tx.nVersion = 2
    recovery_tx.vin = [
        CTxIn(vaults_to_outpoints[v]) for v in vaults
    ]

    recovery_tx.vout = []
    spent_outputs: t.List[CTxOut] = []
    vault_to_spent_output = {}

    for v in vaults:
        # If this vault uses the optional recovery authorization, push the necessary
        # witness data into the stack.
        spent_output = v.vault_output
        vault_outpoint = vaults_to_outpoints[v]
        recovering_from_vaulted = vault_outpoint == v.vault_outpoint
        if not recovering_from_vaulted:
            assert unvault_trigger_tx
            spent_output = unvault_trigger_tx.vout[0]
        assert isinstance(spent_output, CTxOut)
        vault_to_spent_output[v] = spent_output
        spent_outputs.append(spent_output)

    for i, (recov_tr_info, amount) in enumerate(vault_totals.items()):
        recovery_tx.vout.append(CTxOut(
            nValue=amount, scriptPubKey=recov_tr_info.scriptPubKey))

        for v, tr in vault_to_taproot.items():
            if tr == recov_tr_info:
                vault_to_recov_output_index[v] = i


    if fee_wallet:
        if fee_coin_amount_sats:
            tx = fee_wallet.send_self_transfer_multi(
                from_node=fee_wallet._test_node, amount_per_output=fee_coin_amount_sats)
            fee_utxo = fee_wallet.get_utxo_as_txin(txid=tx['txid'])
        else:
            fee_utxo = fee_wallet.get_utxo_as_txin()
        utxo = fee_utxo[0]
        FEE_IN_SATS = 10_000
        recovery_tx.vin.append(fee_utxo[1])
        val = int(utxo['value'] * COIN)

        if not fee_just_input:
            recovery_tx.vout.insert(0, CTxOut(
                nValue=(val - FEE_IN_SATS),
                scriptPubKey=CScript([script.OP_TRUE]),
            ))

            # Bump all vout indexes since we've inserted one before.
            for v in vault_to_recov_output_index:
                vault_to_recov_output_index[v] += 1

        spent_outputs.append(CTxOut(
            nValue=val, scriptPubKey=fee_wallet._scriptPubKey))

    # Attach an ephemeral anchor output.
    recovery_tx.vout.append(CTxOut(nValue=0, scriptPubKey=CScript([script.OP_2])))

    if presig_tx_mutator:
        presig_tx_mutator(recovery_tx)


    for i, vault in enumerate(vaults):
        recovery_tx.wit.vtxinwit += [messages.CTxInWitness()]
        scriptwit = recovery_tx.wit.vtxinwit[i].scriptWitness
        scriptwit.stack = []
        vault_outpoint = vaults_to_outpoints[vault]
        recovering_from_vaulted = vault_outpoint == vault.vault_outpoint
        scriptwit.stack.extend(vault.recovery_auth.get_spend_wit_stack(
            recovery_tx, i, vault_to_spent_output[vault].nValue, spent_outputs))

        # If the outpoint we're spending isn't an OP_UNVAULT (i.e. if the unvault
        # process hasn't yet been triggered), then we need to reveal the vault via
        # script-path TR witness.
        if recovering_from_vaulted:
            scriptwit.stack.extend([
                script.bn2vch(vault_to_recov_output_index[vault]),
                vault.vault_spk,
                (bytes([0xC0 + vault.init_tr_info.negflag])
                 + vault.init_tr_info.internal_pubkey),
            ])
        # Otherwise, we're recovering away from an OP_UNVAULT trigger transaction.
        elif unvault_trigger_tx:
            spend_wit_stack = None
            for spec in unvault_trigger_tx.unvault_specs:
                if vault in spec.compat_vaults:
                    # Have to make a copy to avoid modifying the spec for future
                    # usages.
                    spend_wit_stack = copy.deepcopy(spec.spend_witness_stack)

            assert spend_wit_stack is not None
            scriptwit.stack.extend([
                # Insert the recovery output index first
                script.bn2vch(vault_to_recov_output_index[vault]),
                *spend_wit_stack,
            ])
        else:
            raise ValueError(
                "must pass `unvault_trigger_tx` if spending from unvault")

    if postsig_tx_mutator:
        postsig_tx_mutator(recovery_tx)

    return recovery_tx


# Used to make the OP_UNVAULT output only script-path spendable.
# See `VAULT_NUMS_INTERNAL_PUBKEY` in script/interpreter.cpp.
UNVAULT_NUMS_INTERNAL_PUBKEY = bytes.fromhex(
    "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0")


class UnvaultSpec:
    """
    Gathers parameters for an unvault operation across potentially
    many vaults with compatible unvault parameters (i.e. shared recovery and delay).
    """
    def __init__(
        self,
        compat_vaults: t.List[VaultSpec],
        target_hash: bytes,
        revault_amount_sats: t.Optional[int] = None,
    ) -> None:
        self.compat_vaults = compat_vaults
        self.target_hash = target_hash
        self.revault_amount_sats = revault_amount_sats
        self.spend_delay = self.compat_vaults[0].spend_delay

        self.unvault_spk = CScript([
            # <recovery-params>
            self.compat_vaults[0].recovery_params,
            # <spend-delay>
            self.spend_delay,
            # <target-outputs-hash>
            self.target_hash,
            OP_UNVAULT,
        ])

        self.nums_tr = script.taproot_construct(
            UNVAULT_NUMS_INTERNAL_PUBKEY,
            scripts=[('only-path', self.unvault_spk, 0xC0)])

        self.trigger_spk = self.nums_tr.scriptPubKey

        self.amount = sum(v.total_amount_sats for v in self.compat_vaults)  # type: ignore

        if self.revault_amount_sats:
            self.amount -= self.revault_amount_sats

        self.trigger_txout = CTxOut(nValue=self.amount, scriptPubKey=self.trigger_spk)

        self.revault_txout = None
        if self.revault_amount_sats:
            vault_output = self.compat_vaults[0].vault_output
            assert vault_output

            self.revault_txout = CTxOut(
                nValue=self.revault_amount_sats,
                scriptPubKey=vault_output.scriptPubKey)

        # Cache the taproot info for later use when constructing a spend.
        self.spend_witness_stack = [
            self.unvault_spk,
            (bytes([0xC0 + self.nums_tr.negflag]) + self.nums_tr.internal_pubkey),
        ]

        self.vault_outpoints = [v.vault_outpoint for v in self.compat_vaults]
        self.vault_outputs = [v.vault_output for v in self.compat_vaults]


def get_trigger_unvault_tx(
    unvault_trigger_specs: t.List[UnvaultSpec],
    fee_wallet: t.Optional[MiniWallet] = None,
    presig_tx_mutator: t.Optional[t.Callable] = None,
    postsig_tx_mutator: t.Optional[t.Callable] = None,
) -> UnvaultTriggerTransaction:
    """
    Return a transaction that triggers the withdrawal process to some arbitrary
    target output set.

    Args:
        revault_amount_sats (int): if given, "peel off" this amount from the
            total vault value to be revaulted.
    """
    trigger_tx = UnvaultTriggerTransaction()
    trigger_tx.unvault_specs = unvault_trigger_specs
    trigger_tx.nVersion = 2
    trigger_tx.vin = []
    trigger_tx.vout = []
    vault_outputs = []
    vaults_to_spec = {}
    vout_idx = 0
    spec_to_vout_idx = {}

    for spec in unvault_trigger_specs:
        for vault in spec.compat_vaults:
            vaults_to_spec[vault] = spec

        trigger_tx.vin.extend(
            CTxIn(outpoint=outpoint) for outpoint in spec.vault_outpoints)
        trigger_tx.vout.append(spec.trigger_txout)

        spec_to_vout_idx[spec] = vout_idx
        vout_idx += 1

        if spec.revault_txout:
            trigger_tx.vout.append(spec.revault_txout)
            vout_idx += 1

        vault_outputs.extend(spec.vault_outputs)

    if fee_wallet:
        FEE_IN_SATS = 10_000
        fee_utxo = fee_wallet.get_utxo_as_txin()
        trigger_tx.vin.append(fee_utxo[1])
        trigger_tx.vout.insert(0, CTxOut(
            nValue=(int(fee_utxo[0]['value'] * COIN) - FEE_IN_SATS),
            scriptPubKey=CScript([script.OP_TRUE]),
        ))

        for k in spec_to_vout_idx:
            spec_to_vout_idx[k] += 1

    if presig_tx_mutator:
        presig_tx_mutator(trigger_tx)

    # Sign the input for each vault and attach a fitting witness.
    for i, (vault, spec) in enumerate(vaults_to_spec.items()):
        unvault_sigmsg = script.TaprootSignatureHash(
            trigger_tx, vault_outputs, input_index=i, hash_type=0
        )

        assert isinstance(vault.unvault_tweaked_privkey, bytes)
        unvault_signature = key.sign_schnorr(
            vault.unvault_tweaked_privkey, unvault_sigmsg)

        vout_idx = spec_to_vout_idx[spec]

        trigger_tx.wit.vtxinwit += [messages.CTxInWitness()]
        trigger_tx.wit.vtxinwit[i].scriptWitness.stack = [
            unvault_signature,
            vault.unvault_tr_info.scriptPubKey,
            spec.target_hash,
            CScript([vout_idx]) if vout_idx != 0 else b'',
            NOT_A_RECOVERY_FLAG,
            vault.vault_spk,
            (bytes([0xC0 + vault.init_tr_info.negflag])
             + vault.init_tr_info.internal_pubkey),
        ]

    if postsig_tx_mutator:
        postsig_tx_mutator(trigger_tx)

    return trigger_tx


def get_final_withdrawal_tx(
    unvault_outpoint: COutPoint,
    final_target_vout: t.List[CTxOut],
    unvault_spec: UnvaultSpec,
) -> CTransaction:
    """
    Return the final transaction, withdrawing a balance to the specified target.
    """
    final_tx = CTransaction()
    final_tx.nVersion = 2
    final_tx.vin = [
        CTxIn(outpoint=unvault_outpoint, nSequence=unvault_spec.spend_delay)
    ]
    final_tx.vout = final_target_vout
    final_tx.wit.vtxinwit += [messages.CTxInWitness()]
    final_tx.wit.vtxinwit[0].scriptWitness.stack = [
        NOT_A_RECOVERY_FLAG,
        *unvault_spec.spend_witness_stack
    ]

    return final_tx


def recovery_spk_tagged_hash(script: CScript) -> bytes:
    ser = messages.ser_string(script)
    return key.TaggedHash("VaultRecoverySPK", ser)


def unvault_spk_tagged_hash(script: CScript) -> bytes:
    ser = messages.ser_string(script)
    return key.TaggedHash("VaultTriggerSPK", ser)


def target_outputs_hash(vout: t.List[CTxOut]) -> bytes:
    """Equivalent to `PrecomputedTransactionData.hashOutputs`."""
    return messages.hash256(b"".join(v.serialize() for v in vout))


def taproot_from_privkey(pk: key.ECKey, scripts=None) -> script.TaprootInfo:
    x_only, _ = key.compute_xonly_pubkey(pk.get_bytes())
    return script.taproot_construct(x_only, scripts=scripts)


def split_vault_value(total_val: int, num: int = 3) -> t.List[int]:
    """
    Return kind-of evenly split amounts that preserve the total value of the vault.
    """
    val_segment = total_val // num
    amts = []
    for _ in range(num - 1):
        amts.append(val_segment)
        total_val -= val_segment
    amts.append(total_val)
    return amts


def make_segwit0_spk(privkey: key.ECKey) -> CScript:
    return CScript([script.OP_0, script.hash160(privkey.get_pubkey().get_bytes())])


if __name__ == "__main__":
    VaultsTest().main()
