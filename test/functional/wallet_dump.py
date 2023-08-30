#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the dumpwallet RPC."""
import datetime
import os
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


def read_dump(file_name, addrs, script_addrs, hd_master_addr_old):
    """
    Read the given dump, count the addrs that match, count change and reserve.
    Also check that the old hd_master is inactive
    """
    with open(file_name, encoding='utf8') as inputfile:
        found_comments = []
        found_addr = 0
        found_script_addr = 0
        found_addr_chg = 0
        found_addr_rsv = 0
        hd_master_addr_ret = None
        for line in inputfile:
            line = line.strip()
            if not line:
                continue
            if line[0] == '#':
                found_comments.append(line)
            else:
                # split out some data
                key_date_label, comment = line.split("#")
                key_date_label = key_date_label.split(" ")
                # key = key_date_label[0]
                date = key_date_label[1]
                keytype = key_date_label[2]

                imported_key = date == '1970-01-01T00:00:01Z'
                if imported_key:
                    # Imported keys have multiple addresses, no label (keypath) and timestamp
                    # Skip them
                    continue

                addr_keypath = comment.split(" addr=")[1]
                addr = addr_keypath.split(" ")[0]
                keypath = None
                if keytype == "inactivehdseed=1":
                    # ensure the old master is still available
                    assert hd_master_addr_old == addr
                elif keytype == "hdseed=1":
                    # ensure we have generated a new hd master key
                    assert hd_master_addr_old != addr
                    hd_master_addr_ret = addr
                elif keytype == "script=1":
                    # scripts don't have keypaths
                    keypath = None
                else:
                    keypath = addr_keypath.rstrip().split("hdkeypath=")[1]

                # count key types
                for addrObj in addrs:
                    if addrObj['address'] == addr and addrObj['hdkeypath'] == keypath and keytype == "label=":
                        found_addr += 1
                        break
                    elif keytype == "change=1":
                        found_addr_chg += 1
                        break
                    elif keytype == "reserve=1":
                        found_addr_rsv += 1
                        break

                # count scripts
                for script_addr in script_addrs:
                    if script_addr == addr.rstrip() and keytype == "script=1":
                        found_script_addr += 1
                        break

        return found_comments, found_addr, found_script_addr, found_addr_chg, found_addr_rsv, hd_master_addr_ret


class WalletDumpTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-keypool=90", "-usehd=1"]]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.disable_mocktime()
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()

    def run_test(self):
        self.nodes[0].createwallet("dump")

        wallet_unenc_dump = os.path.join(self.nodes[0].datadir, "wallet.unencrypted.dump")
        wallet_enc_dump = os.path.join(self.nodes[0].datadir, "wallet.encrypted.dump")

        # generate 20 addresses to compare against the dump
        test_addr_count = 20
        addrs = []
        for i in range(0,test_addr_count):
            addr = self.nodes[0].getnewaddress()
            vaddr= self.nodes[0].getaddressinfo(addr) #required to get hd keypath
            addrs.append(vaddr)
        # Should be a no-op:
        self.nodes[0].keypoolrefill()

        # Test scripts dump by adding a 1-of-1 multisig address
        multisig_addr = self.nodes[0].addmultisigaddress(1, [addrs[1]["address"]])["address"]
        script_addrs = [multisig_addr]

        self.log.info('Mine a block one second before the wallet is dumped')
        dump_time = int(time.time())
        self.nodes[0].setmocktime(dump_time - 1)
        self.nodes[0].generate(1)
        self.nodes[0].setmocktime(dump_time)
        dump_time_str = '# * Created on {}Z'.format(
            datetime.datetime.fromtimestamp(
                dump_time,
                tz=datetime.timezone.utc,
            ).replace(tzinfo=None).isoformat())
        dump_best_block_1 = '# * Best block at time of backup was {} ({}),'.format(
            self.nodes[0].getblockcount(),
            self.nodes[0].getbestblockhash(),
        )
        dump_best_block_2 = '#   mined on {}Z'.format(
            datetime.datetime.fromtimestamp(
                dump_time - 1,
                tz=datetime.timezone.utc,
            ).replace(tzinfo=None).isoformat())

        self.log.info('Dump unencrypted wallet')
        result = self.nodes[0].dumpwallet(wallet_unenc_dump)
        assert_equal(result['filename'], wallet_unenc_dump)

        found_comments, found_addr, found_script_addr, found_addr_chg, found_addr_rsv, hd_master_addr_unenc = \
            read_dump(wallet_unenc_dump, addrs, script_addrs, None)
        assert '# End of dump' in found_comments  # Check that file is not corrupt
        assert_equal(dump_time_str, next(c for c in found_comments if c.startswith('# * Created on')))
        assert_equal(dump_best_block_1, next(c for c in found_comments if c.startswith('# * Best block')))
        assert_equal(dump_best_block_2, next(c for c in found_comments if c.startswith('#   mined on')))
        assert_equal(found_addr, test_addr_count)  # all keys must be in the dump
        # This is 1, not 2 because we aren't testing for witness scripts
        assert_equal(found_script_addr, 1)  # all scripts must be in the dump
        assert_equal(found_addr_chg, 0)  # 0 blocks where mined
        assert_equal(found_addr_rsv, 180)  # keypool size (external+internal)

        #encrypt wallet, restart, unlock and dump
        self.nodes[0].encryptwallet('test')
        self.nodes[0].walletpassphrase('test', 300)
        # Should be a no-op:
        self.nodes[0].keypoolrefill()
        self.nodes[0].dumpwallet(wallet_enc_dump)

        found_comments, found_addr, found_script_addr, found_addr_chg, found_addr_rsv, _ = \
            read_dump(wallet_enc_dump, addrs, script_addrs, hd_master_addr_unenc)
        assert '# End of dump' in found_comments  # Check that file is not corrupt
        assert_equal(dump_time_str, next(c for c in found_comments if c.startswith('# * Created on')))
        assert_equal(dump_best_block_1, next(c for c in found_comments if c.startswith('# * Best block')))
        assert_equal(dump_best_block_2, next(c for c in found_comments if c.startswith('#   mined on')))
        assert_equal(found_addr, test_addr_count)
        # This is 1, not 2 because we aren't testing for witness scripts
        assert_equal(found_script_addr, 1)
        # TODO clarify if we want the behavior that is tested below in Dash (only when HD seed was generated and not user-provided)
        # assert_equal(found_addr_chg, 180 + 50)  # old reserve keys are marked as change now
        assert_equal(found_addr_rsv, 180)  # keypool size

        # Overwriting should fail
        assert_raises_rpc_error(-8, "already exists", lambda: self.nodes[0].dumpwallet(wallet_enc_dump))

        # Restart node with new wallet, and test importwallet
        self.restart_node(0)
        self.nodes[0].createwallet("w2")

        # Make sure the address is not IsMine before import
        result = self.nodes[0].getaddressinfo(multisig_addr)
        assert result['ismine'] == False

        self.nodes[0].importwallet(wallet_unenc_dump)

        # Now check IsMine is true
        result = self.nodes[0].getaddressinfo(multisig_addr)
        assert result['ismine'] == True

        if self.is_bdb_compiled():
            self.log.info('Check that wallet is flushed')
            with self.nodes[0].assert_debug_log(['Flushing wallet.dat'], timeout=20):
                self.nodes[0].getnewaddress()


if __name__ == '__main__':
    WalletDumpTest().main()
