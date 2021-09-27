#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test addrman functionality"""

import os
import re
import struct

from test_framework.messages import ser_uint256, hash256
from test_framework.p2p import MAGIC_BYTES
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal


def serialize_addrman(
    *,
    format=1,
    lowest_compatible=3,
    net_magic="regtest",
    bucket_key=1,
    len_new=None,
    len_tried=None,
    mock_checksum=None,
):
    new = []
    tried = []
    INCOMPATIBILITY_BASE = 32
    r = MAGIC_BYTES[net_magic]
    r += struct.pack("B", format)
    r += struct.pack("B", INCOMPATIBILITY_BASE + lowest_compatible)
    r += ser_uint256(bucket_key)
    r += struct.pack("i", len_new or len(new))
    r += struct.pack("i", len_tried or len(tried))
    ADDRMAN_NEW_BUCKET_COUNT = 1 << 10
    r += struct.pack("i", ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30))
    for _ in range(ADDRMAN_NEW_BUCKET_COUNT):
        r += struct.pack("i", 0)
    checksum = hash256(r)
    r += mock_checksum or checksum
    return r


def write_addrman(peers_dat, **kwargs):
    with open(peers_dat, "wb") as f:
        f.write(serialize_addrman(**kwargs))


class AddrmanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        peers_dat = os.path.join(self.nodes[0].datadir, self.chain, "peers.dat")
        init_error = lambda reason: (
            f"Error: Invalid or corrupt peers.dat \\({reason}\\). If you believe this "
            f"is a bug, please report it to {self.config['environment']['PACKAGE_BUGREPORT']}. "
            f'As a workaround, you can move the file \\("{re.escape(peers_dat)}"\\) out of the way \\(rename, '
            "move, or delete\\) to have a new one created on the next start."
        )

        self.log.info("Check that mocked addrman is valid")
        self.stop_node(0)
        write_addrman(peers_dat)
        with self.nodes[0].assert_debug_log(["Loaded 0 addresses from peers.dat"]):
            self.start_node(0, extra_args=["-checkaddrman=1"])
        assert_equal(self.nodes[0].getnodeaddresses(), [])

        self.log.info("Check that addrman from future cannot be read")
        self.stop_node(0)
        write_addrman(peers_dat, lowest_compatible=111)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error(
                "Unsupported format of addrman database: 1. It is compatible with "
                "formats >=111, but the maximum supported by this version of "
                f"{self.config['environment']['PACKAGE_NAME']} is 3.: (.+)"
            ),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (EOF)")
        self.stop_node(0)
        with open(peers_dat, "wb") as f:
            f.write(serialize_addrman()[:-1])
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("CAutoFile::read: end of file.*"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (magic)")
        self.stop_node(0)
        write_addrman(peers_dat, net_magic="signet")
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("Invalid network magic number"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (checksum)")
        self.stop_node(0)
        write_addrman(peers_dat, mock_checksum=b"ab" * 32)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("Checksum mismatch, data corrupted"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (len_tried)")
        self.stop_node(0)
        write_addrman(peers_dat, len_tried=-1)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("Corrupt CAddrMan serialization: nTried=-1, should be in \\[0, 16384\\]:.*"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (len_new)")
        self.stop_node(0)
        write_addrman(peers_dat, len_new=-1)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("Corrupt CAddrMan serialization: nNew=-1, should be in \\[0, 65536\\]:.*"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that corrupt addrman cannot be read (failed check)")
        self.stop_node(0)
        write_addrman(peers_dat, bucket_key=0)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg=init_error("Corrupt data. Consistency check failed with code -16: .*"),
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Check that missing addrman is recreated")
        self.stop_node(0)
        os.remove(peers_dat)
        with self.nodes[0].assert_debug_log([
                f'Creating peers.dat because the file was not found ("{peers_dat}")',
        ]):
            self.start_node(0)
        assert_equal(self.nodes[0].getnodeaddresses(), [])


if __name__ == "__main__":
    AddrmanTest().main()
