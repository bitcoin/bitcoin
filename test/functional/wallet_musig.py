#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import re

from test_framework.descriptors import descsum_create
from test_framework.key import H_POINT
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)

PRIVKEY_RE = re.compile(r"^tr\((.+?)/.+\)#.{8}$")
PUBKEY_RE = re.compile(r"^tr\((\[.+?\].+?)/.+\)#.{8}$")
ORIGIN_PATH_RE = re.compile(r"^\[\w{8}(/.*)\].*$")
MULTIPATH_TWO_RE = re.compile(r"<(\d+);(\d+)>")
MUSIG_RE = re.compile(r"musig\((.*?)\)")
PLACEHOLDER_RE = re.compile(r"\$\d")

class WalletMuSigTest(BitcoinTestFramework):
    WALLET_NUM = 0
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def do_test(self, comment, pattern, sighash_type=None, scriptpath=False, nosign_wallets=None, only_one_musig_wallet=False):
        self.log.info(f"Testing {comment}")
        has_internal = MULTIPATH_TWO_RE.search(pattern) is not None

        wallets = []
        keys = []

        pat = pattern.replace("$H", H_POINT)

        # Figure out how many wallets are needed and create them
        expected_pubnonces = 0
        expected_partial_sigs = 0
        for musig in MUSIG_RE.findall(pat):
            musig_partial_sigs = 0
            for placeholder in PLACEHOLDER_RE.findall(musig):
                wallet_index = int(placeholder[1:])
                if nosign_wallets is None or wallet_index not in nosign_wallets:
                    expected_pubnonces += 1
                else:
                    musig_partial_sigs = None
                if musig_partial_sigs is not None:
                    musig_partial_sigs += 1
                if wallet_index < len(wallets):
                    continue
                wallet_name = f"musig_{self.WALLET_NUM}"
                self.WALLET_NUM += 1
                self.nodes[0].createwallet(wallet_name)
                wallet = self.nodes[0].get_wallet_rpc(wallet_name)
                wallets.append(wallet)

                for priv_desc in wallet.listdescriptors(True)["descriptors"]:
                    desc = priv_desc["desc"]
                    if not desc.startswith("tr("):
                        continue
                    privkey = PRIVKEY_RE.search(desc).group(1)
                    break
                for pub_desc in wallet.listdescriptors()["descriptors"]:
                    desc = pub_desc["desc"]
                    if not desc.startswith("tr("):
                        continue
                    pubkey = PUBKEY_RE.search(desc).group(1)
                    # Since the pubkey is derived from the private key that we have, we need
                    # to extract and insert the origin path from the pubkey as well.
                    privkey += ORIGIN_PATH_RE.search(pubkey).group(1)
                    break
                keys.append((privkey, pubkey))
            if musig_partial_sigs is not None:
                expected_partial_sigs += musig_partial_sigs

        # Construct and import each wallet's musig descriptor that
        # contains the private key from that wallet and pubkeys of the others
        for i, wallet in enumerate(wallets):
            if only_one_musig_wallet and i > 0:
                continue
            desc = pat
            import_descs = []
            for j, (priv, pub) in enumerate(keys):
                if j == i:
                    desc = desc.replace(f"${i}", priv)
                else:
                    desc = desc.replace(f"${j}", pub)

            import_descs.append({
                "desc": descsum_create(desc),
                "active": True,
                "timestamp": "now",
            })

            res = wallet.importdescriptors(import_descs)
            for r in res:
                assert_equal(r["success"], True)

        # Check that the wallets agree on the same musig address
        addr = None
        change_addr = None
        for i, wallet in enumerate(wallets):
            if only_one_musig_wallet and i > 0:
                continue
            if addr is None:
                addr = wallet.getnewaddress(address_type="bech32m")
            else:
                assert_equal(addr, wallet.getnewaddress(address_type="bech32m"))
            if has_internal:
                if change_addr is None:
                    change_addr = wallet.getrawchangeaddress(address_type="bech32m")
                else:
                    assert_equal(change_addr, wallet.getrawchangeaddress(address_type="bech32m"))

        # Fund that address
        self.def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        # Spend that UTXO
        utxo = None
        for i, wallet in enumerate(wallets):
            if only_one_musig_wallet and i > 0:
                continue
            if utxo is None:
                utxo = wallet.listunspent()[0]
            else:
                assert_equal(utxo, wallet.listunspent()[0])
        psbt = wallets[0].walletcreatefundedpsbt(outputs=[{self.def_wallet.getnewaddress(): 5}], inputs=[utxo], change_type="bech32m", changePosition=1)["psbt"]

        dec_psbt = self.nodes[0].decodepsbt(psbt)
        assert_equal(len(dec_psbt["inputs"]), 1)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_participant_pubkeys"]), pattern.count("musig("))
        if has_internal:
            assert_equal(len(dec_psbt["outputs"][1]["musig2_participant_pubkeys"]), pattern.count("musig("))

        # Check all participant pubkeys in the input and change output
        psbt_maps = [dec_psbt["inputs"][0]]
        if has_internal:
            psbt_maps.append(dec_psbt["outputs"][1])
        for psbt_map in psbt_maps:
            part_pks = set()
            for agg in psbt_map["musig2_participant_pubkeys"]:
                for part_pub in agg["participant_pubkeys"]:
                    part_pks.add(part_pub[2:])
            # Check that there are as many participants as we expected
            assert_equal(len(part_pks), len(keys))
            # Check that each participant has a derivation path
            for deriv_path in psbt_map["taproot_bip32_derivs"]:
                if deriv_path["pubkey"] in part_pks:
                    part_pks.remove(deriv_path["pubkey"])
            assert_equal(len(part_pks), 0)

        # Add pubnonces
        nonce_psbts = []
        for i, wallet in enumerate(wallets):
            if nosign_wallets and i in nosign_wallets:
                continue
            proc = wallet.walletprocesspsbt(psbt=psbt, sighashtype=sighash_type)
            assert_equal(proc["complete"], False)
            nonce_psbts.append(proc["psbt"])

        comb_nonce_psbt = self.nodes[0].combinepsbt(nonce_psbts)

        dec_psbt = self.nodes[0].decodepsbt(comb_nonce_psbt)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_pubnonces"]), expected_pubnonces)
        for pn in dec_psbt["inputs"][0]["musig2_pubnonces"]:
            pubkey = pn["aggregate_pubkey"][2:]
            if pubkey in dec_psbt["inputs"][0]["witness_utxo"]["scriptPubKey"]["hex"]:
                continue
            elif "taproot_scripts" in dec_psbt["inputs"][0]:
                for leaf_scripts in dec_psbt["inputs"][0]["taproot_scripts"]:
                    if pubkey in leaf_scripts["script"]:
                        break
                else:
                    assert False, "Aggregate pubkey for pubnonce not seen as output key, or in any scripts"
            else:
                assert False, "Aggregate pubkey for pubnonce not seen as output key or internal key"

        # Add partial sigs
        psig_psbts = []
        for i, wallet in enumerate(wallets):
            if nosign_wallets and i in nosign_wallets:
                continue
            proc = wallet.walletprocesspsbt(psbt=comb_nonce_psbt, sighashtype=sighash_type)
            assert_equal(proc["complete"], False)
            psig_psbts.append(proc["psbt"])

        comb_psig_psbt = self.nodes[0].combinepsbt(psig_psbts)

        dec_psbt = self.nodes[0].decodepsbt(comb_psig_psbt)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_partial_sigs"]), expected_partial_sigs)
        for ps in dec_psbt["inputs"][0]["musig2_partial_sigs"]:
            pubkey = ps["aggregate_pubkey"][2:]
            if pubkey in dec_psbt["inputs"][0]["witness_utxo"]["scriptPubKey"]["hex"]:
                continue
            elif "taproot_scripts" in dec_psbt["inputs"][0]:
                for leaf_scripts in dec_psbt["inputs"][0]["taproot_scripts"]:
                    if pubkey in leaf_scripts["script"]:
                        break
                else:
                    assert False, "Aggregate pubkey for partial sig not seen as output key or in any scripts"
            else:
                assert False, "Aggregate pubkey for partial sig not seen as output key"

        # Non-participant aggregates partial sigs and send
        finalized = self.nodes[0].finalizepsbt(psbt=comb_psig_psbt, extract=False)
        assert_equal(finalized["complete"], True)
        witness = self.nodes[0].decodepsbt(finalized["psbt"])["inputs"][0]["final_scriptwitness"]
        if scriptpath:
            assert_greater_than(len(witness), 1)
        else:
            assert_equal(len(witness), 1)
        finalized = self.nodes[0].finalizepsbt(comb_psig_psbt)
        assert "hex" in finalized
        self.nodes[0].sendrawtransaction(finalized["hex"])

    def run_test(self):
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.do_test("rawtr(musig(keys/*))", "rawtr(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*))")
        self.do_test("rawtr(musig(keys/*)) with ALL|ANYONECANPAY", "rawtr(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*))", "ALL|ANYONECANPAY")
        self.do_test("tr(musig(keys/*)) no multipath", "tr(musig($0/0/*,$1/1/*,$2/2/*))")
        self.do_test("tr(musig(keys/*)) 2 index multipath", "tr(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*))")
        self.do_test("tr(musig(keys/*)) 3 index multipath", "tr(musig($0/<0;1;2>/*,$1/<1;2;3>/*,$2/<2;3;4>/*))")
        self.do_test("rawtr(musig/*)", "rawtr(musig($0,$1,$2)/<0;1>/*)")
        self.do_test("tr(musig/*)", "tr(musig($0,$1,$2)/<0;1>/*)")
        self.do_test("rawtr(musig(keys/*)) without all wallets importing", "rawtr(musig($0/<0;1>/*,$1/<0;1>/*,$2/<0;1>/*))", only_one_musig_wallet=True)
        self.do_test("tr(musig(keys/*)) without all wallets importing", "tr(musig($0/<0;1>/*,$1/<0;1>/*,$2/<0;1>/*))", only_one_musig_wallet=True)
        self.do_test("tr(H, pk(musig(keys/*)))", "tr($H,pk(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*)))", scriptpath=True)
        self.do_test("tr(H,pk(musig/*))", "tr($H,pk(musig($0,$1,$2)/<0;1>/*))", scriptpath=True)
        self.do_test("tr(H,{pk(musig/*), pk(musig/*)})", "tr($H,{pk(musig($0,$1,$2)/<0;1>/*),pk(musig($3,$4,$5)/0/*)})", scriptpath=True)
        self.do_test("tr(H,{pk(musig/*), pk(same keys different musig/*)})", "tr($H,{pk(musig($0,$1,$2)/<0;1>/*),pk(musig($1,$2)/0/*)})", scriptpath=True)
        self.do_test("tr(musig/*,{pk(partial keys diff musig-1/*),pk(partial keys diff musig-2/*)})}", "tr(musig($0,$1,$2)/<3;4>/*,{pk(musig($0,$1)/<5;6>/*),pk(musig($1,$2)/7/*)})")
        self.do_test("tr(musig/*,{pk(partial keys diff musig-1/*),pk(partial keys diff musig-2/*)})} script-path", "tr(musig($0,$1,$2)/<3;4>/*,{pk(musig($0,$1)/<5;6>/*),pk(musig($1,$2)/7/*)})", scriptpath=True, nosign_wallets=[0])


if __name__ == '__main__':
    WalletMuSigTest(__file__).main()
