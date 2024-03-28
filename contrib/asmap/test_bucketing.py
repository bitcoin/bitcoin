#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# Test the addrman bucketing logic with asmap
#
# It reads the specified asmap file (--asmap) and starts a node with it.
# Then, it will get N (--num_asns) unique ASNs from the asmap file and try
# to add, for 1/3 of them, 1000 addresses per ASN in the "new" table and
# one address per ASN for the 2/3. For the successfully added addresses, it
# will check if the node mapped the ASN correctly. It will also restart the
# node to check whether the addrman is successfully loaded.

import os
import ipaddress
import random
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../seeds'))

from asmap import ASMap, prefix_to_net # noqa: E402

sys.path.append(os.path.join(os.path.dirname(__file__), '../../test/functional'))

from test_framework.test_framework import BitcoinTestFramework  # noqa: E402
from test_framework.util import assert_equal  # noqa: E402


class AsmapBucketingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.filename = os.path.join(self.options.asmap_file)
        self.args = ["-checkaddrman=1", "-debug=addrman", f"-asmap={self.filename}"]
        self.extra_args = [self.args]  # Do addrman checks on all operations.

    def add_options(self, parser):
        parser.add_argument(
            "--num_asns", type=int, action='store', dest="num_asns",
            help="Number of unique ASNs from file to use in the test. Set -1 to use all.",
            default=-1)
        parser.add_argument(
            "--asmap", action='store', dest="asmap_file",
            help="Path to asmap file",
            default=False, required=True)

    def generate_addrs(self, subnet, num_addrs=1):
        ip_addresses = []
        for _ in range(num_addrs):
            random_ip = ipaddress.ip_address(random.randint(int(subnet.network_address), int(subnet.broadcast_address)))
            ip_addresses.append(random_ip)
        return ip_addresses

    def read_asmap_file(self):
        with open(self.options.asmap_file, 'rb') as f:
            from_bin = ASMap.from_binary(f.read())
            entries_flat = from_bin._to_entries_flat()
            new_entries = []
            for entries in entries_flat:
                new_entries.append((prefix_to_net(entries[0]), entries[1]))
            return new_entries

    def run_test(self):
        node = self.nodes[0]
        self.log.info('Test addrman bucketing with asmap')
        entries = self.read_asmap_file()
        random.shuffle(entries)
        asns = []
        NUM_ASNS = self.options.num_asns if self.options.num_asns <= len(entries) and self.options.num_asns > 0 else len(entries)
        num_added_addrs = 0
        for i in range(NUM_ASNS):
            asn = entries[i][1]
            if asn not in asns:
                num_addrs = 1000 if i < int(NUM_ASNS / 3) else 1
                addrs = self.generate_addrs(subnet=entries[i][0], num_addrs=num_addrs)
                if len(addrs) == 0 and not addrs[0]:
                    continue
                for addr in addrs:
                    if node.addpeeraddress(address=str(addr), tried=False, port=8333)["success"]:
                        self.log.info(f"added {addr} (ASN {asn}) to the new table")
                        num_added_addrs += 1
                        if asn not in asns:
                            asns.append(asn)

        raw_addrman = node.getrawaddrman()
        self.log.info("Check addrman is successfully loaded after restarting")
        with node.assert_debug_log([f"ASMap Health Check: {num_added_addrs} clearnet peers are mapped to {len(asns)} ASNs with 0 peers being unmapped"]):
            self.restart_node(0, extra_args=self.args)
        assert_equal(raw_addrman, node.getrawaddrman())


if __name__ == '__main__':
    AsmapBucketingTest().main()
