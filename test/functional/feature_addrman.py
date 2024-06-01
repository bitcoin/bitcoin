#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test addrman functionality"""

import os
import struct

from test_framework.messages import ser_uint256, hash256
from test_framework.p2p import MAGIC_BYTES
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def serialize_addrman(*, format=1, lowest_compatible=3):
    new = []
    tried = []
    INCOMPATIBILITY_BASE = 32
    r = MAGIC_BYTES["regtest"]
    r += struct.pack("B", format)
    r += struct.pack("B", INCOMPATIBILITY_BASE + lowest_compatible)
    r += ser_uint256(1)
    r += struct.pack("i", len(new))
    r += struct.pack("i", len(tried))
    ADDRMAN_NEW_BUCKET_COUNT = 1 << 10
    r += struct.pack("i", ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30))
    for _ in range(ADDRMAN_NEW_BUCKET_COUNT):
        r += struct.pack("i", 0)
    checksum = hash256(r)
    r += checksum
    return r


def write_addrman(peers_dat, **kwargs):
    with open(peers_dat, "wb") as f:
        f.write(serialize_addrman(**kwargs))


class AddrmanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        peers_dat = os.path.join(self.nodes[0].datadir, self.chain, "peers.dat")

        self.log.info("Check that mocked addrman is valid")
        self.stop_node(0)
        write_addrman(peers_dat)
        with self.nodes[0].assert_debug_log(["Loaded 0 addresses from peers.dat"]):
            self.start_node(0, extra_args=["-checkaddrman=1"])
        assert_equal(self.nodes[0].getnodeaddresses(), [])

        self.log.info("Check that addrman from future cannot be read")
        self.stop_node(0)
        write_addrman(peers_dat, lowest_compatible=111)
        with self.nodes[0].assert_debug_log([
                f'ERROR: DeserializeDB: Deserialize or I/O error - Unsupported format of addrman database: 1. It is compatible with formats >=111, but the maximum supported by this version of {self.config["environment"]["PACKAGE_NAME"]} is 3.',
                "Recreating peers.dat",
        ]):
            self.start_node(0)
        assert_equal(self.nodes[0].getnodeaddresses(), [])

        self.log.info("Check that corrupt addrman cannot be read")
        self.stop_node(0)
        with open(peers_dat, "wb") as f:
            f.write(serialize_addrman()[:-1])
        with self.nodes[0].assert_debug_log([
                "ERROR: DeserializeDB: Deserialize or I/O error - CAutoFile::read: end of file",
                "Recreating peers.dat",
        ]):
            self.start_node(0)
        assert_equal(self.nodes[0].getnodeaddresses(), [])

        self.log.info("Check that missing addrman is recreated")
        self.stop_node(0)
        os.remove(peers_dat)
        with self.nodes[0].assert_debug_log([
                f"Missing or invalid file {peers_dat}",
                "Recreating peers.dat",
        ]):
            self.start_node(0)
        assert_equal(self.nodes[0].getnodeaddresses(), [])


if __name__ == "__main__":
    AddrmanTest().main()
