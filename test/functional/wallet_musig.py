#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import re

from test_framework.descriptors import descsum_create
from test_framework.key import H_POINT
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

PRIVKEY_RE = re.compile(r"^tr\((.+?)/.+\)#.{8}$")
PUBKEY_RE = re.compile(r"^tr\((\[.+?\].+?)/.+\)#.{8}$")
ORIGIN_PATH_RE = re.compile(r"^\[\w{8}(/.*)\].*$")
MULTIPATH_RE = re.compile(r"(.*?)<(\d+);(\d+)>")


class WalletMuSigTest(BitcoinTestFramework):
    WALLET_NUM = 0
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def do_test(self, comment, pattern):
        self.log.info(f"Testing {comment}")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        has_int = "<" in pattern and ">" in pattern

        wallets = []
        keys = []

        pat = pattern.replace("$H", H_POINT)

        # Figure out how many wallets are needed and create them
        exp_key_leaf = 0
        for i in range(10):
            if f"${i}" in pat:
                exp_key_leaf += pat.count(f"${i}")
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

        # Construct and import each wallet's musig descriptor
        for i, wallet in enumerate(wallets):
            desc = pat
            import_descs = []
            for j, (priv, pub) in enumerate(keys):
                if j == i:
                    desc = desc.replace(f"${i}", priv)
                else:
                    desc = desc.replace(f"${j}", pub)

            # Deal with the multipath descriptor syntax for external and internal
            if has_int:
                ext_desc = ""
                int_desc = ""
                for m in MULTIPATH_RE.finditer(desc):
                    ext_desc += m.group(1) + m.group(2)
                    int_desc += m.group(1) + m.group(3)
                postfix = desc.split(">")[-1]
                ext_desc += postfix
                int_desc += postfix

                import_descs.append({
                    "desc": descsum_create(int_desc),
                    "active": True,
                    "internal": True,
                    "timestamp": "now",
                })
            else:
                ext_desc = desc

            import_descs.append({
                "desc": descsum_create(ext_desc),
                "active": True,
                "internal": False,
                "timestamp": "now",
            })

            res = wallet.importdescriptors(import_descs)
            for r in res:
                assert_equal(r["success"], True)

        # Check that the wallets agree on the same musig address
        addr = None
        change_addr = None
        for wallet in wallets:
            if addr is None:
                addr = wallet.getnewaddress(address_type="bech32m")
            else:
                assert_equal(addr, wallet.getnewaddress(address_type="bech32m"))
            if has_int:
                if change_addr is None:
                    change_addr = wallet.getrawchangeaddress(address_type="bech32m")
                else:
                    assert_equal(change_addr, wallet.getrawchangeaddress(address_type="bech32m"))

        # Fund that address
        def_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        # Spend that UTXO
        utxo = wallets[0].listunspent()[0]
        psbt = wallets[0].send(outputs=[{def_wallet.getnewaddress(): 5}], inputs=[utxo], change_type="bech32m")["psbt"]

        dec_psbt = self.nodes[0].decodepsbt(psbt)
        assert_equal(len(dec_psbt["inputs"]), 1)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_participant_pubkeys"]), pattern.count("musig("))

        # Retrieve all participant pubkeys
        part_pks = set()
        for agg in dec_psbt["inputs"][0]["musig2_participant_pubkeys"]:
            for part_pub in agg["participant_pubkeys"]:
                part_pks.add(part_pub[2:])
        # Check that there are as many participants as we expected
        assert_equal(len(part_pks), len(keys))
        # Check that each participant has a derivation path
        for deriv_path in dec_psbt["inputs"][0]["taproot_bip32_derivs"]:
            if deriv_path["pubkey"] in part_pks:
                part_pks.remove(deriv_path["pubkey"])
        assert_equal(len(part_pks), 0)

        # Add pubnonces
        nonce_psbts = []
        for wallet in wallets:
            proc = wallet.walletprocesspsbt(psbt)
            assert_equal(proc["complete"], False)
            nonce_psbts.append(proc["psbt"])

        comb_nonce_psbt = self.nodes[0].combinepsbt(nonce_psbts)

        # Add partial sigs
        dec_psbt = self.nodes[0].decodepsbt(comb_nonce_psbt)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_pubnonces"]), exp_key_leaf)

        psig_psbts = []
        for wallet in wallets:
            proc = wallet.walletprocesspsbt(comb_nonce_psbt)
            assert_equal(proc["complete"], False)
            psig_psbts.append(proc["psbt"])

        comb_psig_psbt = self.nodes[0].combinepsbt(psig_psbts)

        dec_psbt = self.nodes[0].decodepsbt(comb_psig_psbt)
        assert_equal(len(dec_psbt["inputs"][0]["musig2_pubnonces"]), exp_key_leaf)

        # Non-participant aggregates partial sigs and send
        finalized = self.nodes[0].finalizepsbt(comb_psig_psbt)
        assert_equal(finalized["complete"], True)
        assert "hex" in finalized
        self.nodes[0].sendrawtransaction(finalized["hex"])

    def run_test(self):
        self.do_test("rawtr(musig(keys/*))", "rawtr(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*))")
        self.do_test("tr(musig(keys/*))", "tr(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*))")
        self.do_test("rawtr(musig/*)", "rawtr(musig($0,$1,$2)/<0;1>/*)")
        self.do_test("tr(musig/*)", "tr(musig($0,$1,$2)/<0;1>/*)")
        self.do_test("tr(H, pk(musig(keys/*)))", "tr($H,pk(musig($0/<0;1>/*,$1/<1;2>/*,$2/<2;3>/*)))")
        self.do_test("tr(H,pk(musig/*))", "tr($H,pk(musig($0,$1,$2)/<0;1>/*))")
        self.do_test("tr(H,{pk(musig/*), pk(musig/*)})", "tr($H,{pk(musig($0,$1,$2)/<0;1>/*),pk(musig($3,$4,$5)/0/*)})")
        self.do_test("tr(H,{pk(musig/*), pk(same keys different musig/*)})", "tr($H,{pk(musig($0,$1,$2)/<0;1>/*),pk(musig($1,$2)/0/*)})")


if __name__ == '__main__':
    WalletMuSigTest(__file__).main()
